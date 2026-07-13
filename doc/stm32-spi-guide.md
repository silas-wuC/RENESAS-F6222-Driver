# F6222 Driver 在 STM32 上怎麼透過 SPI 送出去 —— 從零開始搞懂 STM32 SPI 結構

這份文件假設你對 STM32 HAL 的 SPI 相關結構「一無所知」，從最底層的 struct 開始解釋，
一路講到 F6222 driver 怎麼把資料送出 SPI bus。範例程式取自
`examples/stm32/f6222-spi-demo/`（STM32H743，SPI1，CS 接 PD14）。

---

## 1. 整體架構：四層洋蔥

```
main.c (你的應用邏輯)
   │  呼叫 f6222_local_reg_write() / f6222_init() ...
   ▼
f6222.h / f6222_spi.c   ← 平台無關的 F6222 driver（不知道 STM32 是什麼）
   │  透過一個 function pointer：dev->spi_xfer(ctx, tx, rx, len)
   ▼
f6222_spi_stm32.c       ← 你要寫的「膠水層」，把 F6222 的抽象呼叫轉成 STM32 HAL 呼叫
   │  呼叫 HAL_GPIO_WritePin() / HAL_SPI_Transmit() / HAL_SPI_TransmitReceive()
   ▼
STM32 HAL SPI Driver    ← ST 官方函式庫，操作真正的 SPI1 硬體暫存器
   │
   ▼
SPI1 硬體周邊 → MOSI/MISO/SCK 腳位 → F6222 晶片
```

F6222 driver 本身完全不 include 任何 STM32 標頭檔（`f6222.h` 開頭寫得很清楚：
「Platform-agnostic: no OS, no stdlib, no SPI implementation」）。它只要求你填一個
callback，剩下的事情它自己做。所以要搞懂「怎麼送出 SPI」，重點其實是搞懂
**STM32 HAL 那些 struct**，以及**膠水層怎麼接起來**。

---

## 1.5 先搞懂「抽象」是什麼——不然後面的設計看起來會很奇怪

上面圖裡寫「F6222 的抽象呼叫」，這個「抽象（abstraction）」是整份文件、
甚至整個 driver 設計的核心概念，一定要先搞懂，不然會不理解「為什麼要
這樣拐彎抹角地設計」。

**抽象＝只講「要做什麼」，不講「怎麼做到」。**

拿生活例子講：你按電燈開關「開燈」，你不需要知道電燈是白熾燈、LED燈、還是
省電燈泡，也不需要知道電線怎麼接、電壓幾伏特。「開關」就是一層抽象——它
定義了一個固定的介面（按下去＝通電），底下實際怎麼發光，是燈泡自己的事，
跟你按開關的手完全無關。你甚至可以把白熾燈換成 LED 燈，開關的用法一個字
都不用改。

放回程式設計：**F6222 driver 想講的話只有「把這幾個 byte 送到 SPI bus 上，
如果有 rx 就把回應收回來」，它完全不想知道「這台機器是 STM32 還是 ESP32、
SPI 硬體暫存器長什麼樣、CS 腳是哪一根」**。這件事「怎麼送出去」，driver
選擇不自己管，而是丟給你（呼叫端）決定。

### 沒有抽象會長怎樣（反面對照）

如果 F6222 driver **沒有**做這層抽象，`f6222_local_reg_write()` 內部可能會
直接寫死：

```c
/* 反例：driver 直接綁死 STM32，完全沒有抽象 */
f6222_status_t f6222_local_reg_write(uint8_t chip_addr, uint8_t reg, uint16_t val) {
    uint8_t tx[5] = { ... };
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);   // 寫死用 STM32 HAL
    HAL_SPI_Transmit(&hspi1, tx, sizeof(tx), 100);            // 寫死用 hspi1
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
    return F6222_OK;
}
```

這樣寫的問題：這個 driver 只能在「剛好也用 STM32、剛好 SPI 接 SPI1、
剛好 CS 接 PD14」的板子上動。換一顆 ESP32、或同一顆 STM32 但 CS 改接別根腳，
就得整個 `f6222_spi.c` 改掉重編。晶片邏輯（datasheet 上那些 bit 排列、
命令格式）跟硬體接線（哪個 MCU、哪根腳）被**綁死攪在一起**，沒辦法分開換。

### 有抽象是長怎樣（這個 driver 實際的做法）

```c
/* f6222.h ── driver 只定義「介面」長怎樣，不管「實作」是誰 */
typedef struct {
    int (*spi_xfer)(void* ctx, const uint8_t* tx, uint8_t* rx, size_t len);
    void* ctx;
} f6222_dev_t;

/* f6222_spi.c ── driver 內部只呼叫這個介面，完全不管背後是誰在實作 */
dev->spi_xfer(dev->ctx, tx, rx, len);
```

`spi_xfer` 這個 function pointer 就是抽象出來的「介面」——driver 只規定
「這個函式要有這樣的形狀（參數、回傳值），呼叫它就等於把資料送出去」，
至於背後真正執行的是 STM32 HAL、ESP32 的 SPI driver、還是你在電腦上寫的
假測試函式，driver 完全不知道，**也不需要知道**。

- 對 STM32：介面背後接的是 `f6222_spi_stm32_xfer()`（第 4 節）。
- 換成別顆 MCU：只要照第 7 節寫一個新的 `spi_xfer` 函式接上去，
  `f6222.h` / `src/f6222_*.c` 一行都不用改。
- 甚至寫單元測試時：可以塞一個假的 `spi_xfer`，把 `tx` 內容記下來斷言，
  完全不需要真的硬體。

這就是「抽象」帶來的好處：**上層（F6222 的命令邏輯）跟下層（怎麼把 byte
真正送出去）互相不依賴彼此的細節，只透過一個講好的介面溝通**。整份文件
後面看到的 `f6222_dev_t`、`spi_xfer`、`f6222_spi_stm32_t` 這些設計，
全部都是圍繞著「抽象」這個概念展開的，看不懂某個設計為什麼要多繞一層，
回頭想「這樣做是不是為了不要把某兩件事綁死」通常就會豁然開朗。

---

## 2. STM32 SPI 結構全解剖（由淺入深）

STM32 HAL 對每個週邊（GPIO、SPI、UART…）都用同一套設計哲學：

- 一個 `XXX_TypeDef`：對應**硬體暫存器**本身（記憶體位址），你幾乎不會直接動它。
- 一個 `XXX_InitTypeDef`：你要**填的設定值**（純資料，沒有函式），像一張「設定單」。
- 一個 `XXX_HandleTypeDef`：把「硬體位址」+「設定單」+「執行期狀態」包成一包，
  之後所有 HAL 函式都靠這個 handle 操作。

下面照這個順序，從 GPIO 講到 SPI。

### 2.1 `GPIO_TypeDef` —— 硬體暫存器本身

```c
GPIOA, GPIOB, GPIOD   // 這些其實是 macro，展開後是 (GPIO_TypeDef*)0x580200xx
```

`GPIOA`/`GPIOB`/`GPIOD` 不是變數，是「某個記憶體位址」被強制轉型成
`GPIO_TypeDef*` 指標。`GPIO_TypeDef` 這個 struct 裡的每個成員（`MODER`、`ODR`、
`BSRR`…）都對應晶片手冊上的一個暫存器位址偏移量。HAL 函式內部就是直接讀寫
這些暫存器 bit，你平常完全不需要碰它，只需要知道「`GPIOD` 代表 D 埠這一整組
腳位」。

### 2.2 `GPIO_InitTypeDef` —— 你要填的 GPIO 設定單

範例專案 `MX_GPIO_Init()`（CubeMX 自動產生）：

```c
GPIO_InitTypeDef GPIO_InitStruct = {0};

GPIO_InitStruct.Pin   = GPIO_PIN_14;          // 要設定哪一根腳（PD14）
GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;  // Push-Pull 輸出（一般數位輸出腳都用這個）
GPIO_InitStruct.Pull  = GPIO_NOPULL;          // 不接內部上/下拉電阻
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;  // 腳位切換速度（越高速越耗電/雜訊越大）
HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);       // 把設定單套用到 GPIOD
```

填完丟給 `HAL_GPIO_Init(port, &struct)`，HAL 就會把這張設定單翻譯成暫存器 bit
寫進 `GPIO_TypeDef`。之後要拉高/拉低這根腳，用：

```c
HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);    // 拉高
HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);  // 拉低
```

**PD14 就是 F6222 的 CS（片選）腳**，這點下面第 4 節會再講。

### 2.3 `SPI_TypeDef` —— SPI 硬體暫存器本身

跟 `GPIO_TypeDef` 同一套邏輯，`SPI1` 這個 macro 展開後是
`(SPI_TypeDef*)SPI1_BASE`，指向 SPI1 週邊在記憶體中的暫存器群組
（控制暫存器、狀態暫存器、資料暫存器…）。你一樣不用直接碰它。

### 2.4 `SPI_InitTypeDef` —— SPI 的設定單（重點！）

這是整份文件最重要的 struct，決定「SPI 怎麼講話」。範例的
`MX_SPI1_Init()` 設定如下（STM32H7 系列欄位比 F1/F4 系列多很多，因為 H7
的 SPI 週邊功能更強）：

```c
hspi1.Instance = SPI1;                              // 這個 handle 綁定哪一組 SPI 硬體

hspi1.Init.Mode              = SPI_MODE_MASTER;     // 我方當 Master（主機），F6222 是 Slave
hspi1.Init.Direction         = SPI_DIRECTION_2LINES;// 全雙工：MOSI + MISO 各一條線
hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;   // 每次傳輸單位是 8-bit（一個 byte）
hspi1.Init.CLKPolarity       = SPI_POLARITY_LOW;    // CPOL=0：SCK 閒置時是低電位
hspi1.Init.CLKPhase          = SPI_PHASE_1EDGE;     // CPHA=0：第一個邊緣就取樣資料
hspi1.Init.NSS               = SPI_NSS_SOFT;        // CS 腳「不」交給 SPI 硬體自動控制，改成軟體手動控制
hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128; // SCK 頻率 = APB 時脈 / 128
hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;    // 每個 byte 先送最高位元（bit7 先出）
hspi1.Init.TIMode            = SPI_TIMODE_DISABLE;  // 不使用 TI 協定，用標準 SPI
hspi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE; // 不用硬體 CRC 校驗

/* 以下是 H7 系列特有、其他系列沒有的欄位 —— 全部保持晶片預設值即可 */
hspi1.Init.CRCPolynomial               = 0x0;
hspi1.Init.NSSPMode                    = SPI_NSS_PULSE_ENABLE;
hspi1.Init.NSSPolarity                 = SPI_NSS_POLARITY_LOW;
hspi1.Init.FifoThreshold                = SPI_FIFO_THRESHOLD_01DATA;
hspi1.Init.TxCRCInitializationPattern   = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
hspi1.Init.RxCRCInitializationPattern   = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
hspi1.Init.MasterSSIdleness             = SPI_MASTER_SS_IDLENESS_00CYCLE;
hspi1.Init.MasterInterDataIdleness      = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
hspi1.Init.MasterReceiverAutoSusp       = SPI_MASTER_RX_AUTOSUSP_DISABLE;
hspi1.Init.MasterKeepIOState            = SPI_MASTER_KEEP_IO_STATE_DISABLE;
hspi1.Init.IOSwap                       = SPI_IO_SWAP_DISABLE;

if (HAL_SPI_Init(&hspi1) != HAL_OK) {
    Error_Handler();
}
```

逐一解釋每個欄位（前面 10 個是所有 STM32 系列都有的核心欄位，一定要懂；
後面那串 H7 專屬的欄位保持預設值就好，先知道它們存在即可）：

| 欄位 | 意思 | 這個專案的選擇 | 為什麼 |
|---|---|---|---|
| `Mode` | Master／Slave | `MASTER` | STM32 主動發起傳輸，F6222 被動回應 |
| `Direction` | 幾條資料線 | `2LINES`（全雙工） | 讀暫存器時要邊送位址邊收資料 |
| `DataSize` | 一次傳輸幾個 bit | `8BIT` | F6222 的命令格式是以 byte 為單位組成的 |
| `CLKPolarity` + `CLKPhase` | 就是俗稱的 **SPI Mode 0/1/2/3** | `LOW` + `1EDGE` = **Mode 0** | F6222 datasheet 規定 CPOL=0, CPHA=0（`f6222.h` 註解也直接寫了） |
| `NSS` | CS 腳由誰控制 | `SOFT`（軟體） | 見下方「為什麼 CS 要軟體控制」 |
| `BaudRatePrescaler` | SCK 速度分頻 | `/128` | F6222 最高支援 50 MHz，但這裡刻意用低速求穩，除錯期先求對再求快 |
| `FirstBit` | MSB 還是 LSB 先送 | `MSB` | F6222 命令格式是「高位元在前」 |
| `TIMode` | 用不用 TI 的變形協定 | `DISABLE` | F6222 是標準 SPI，不是 TI 協定 |
| `CRCCalculation` | 硬體自動加 CRC 校驗碼 | `DISABLE` | F6222 協定沒有 CRC 欄位 |

**為什麼 `NSS = SPI_NSS_SOFT`（CS 要自己手動拉）？**
F6222 的每一筆命令（例如 Local Register Write 是 40-bit = 5 個 byte）必須整包
包在**同一次 CS 低電位**期間送完，命令跟命令之間 CS 要拉回高電位再重新拉低。
如果讓 SPI 硬體自動控制 NSS，硬體不知道「這幾個 byte 算一包」的邊界在哪裡，
容易拉錯時機。所以這個 driver 選擇用一根普通 GPIO（PD14）當 CS，
由膠水層程式碼手動控制「傳輸前拉低、傳輸後拉高」（第 4 節會看到實際程式碼）。

### 2.5 `SPI_HandleTypeDef` —— 打包成一個「操作手把」

```c
SPI_HandleTypeDef hspi1;   // 這是個全域變數，宣告在 main.c
```

這個 struct 大致長這樣（HAL 內部定義，你不用自己寫，CubeMX 幫你產生）：

```c
typedef struct {
    SPI_TypeDef       *Instance;  // 指向哪一組 SPI 硬體（SPI1/SPI2/...）
    SPI_InitTypeDef   Init;       // 上面第 2.4 節填的那張設定單
    uint8_t           *pTxBuffPtr; // 內部狀態：目前傳輸緩衝區指標
    uint16_t          TxXferSize;  // 內部狀態：這次要送幾個 byte
    ...                            // 其他 HAL 內部記帳用的欄位（鎖、狀態機、DMA handle...）
    HAL_LockTypeDef   Lock;        // 避免多執行緒同時搶用同一個 SPI
    HAL_SPI_StateTypeDef State;    // 目前是 READY / BUSY / ERROR
} SPI_HandleTypeDef;
```

重點記住：**`hspi1.Instance` 決定是哪一組硬體，`hspi1.Init` 是設定值，
其餘欄位是 HAL 自己在傳輸過程中用來記帳的狀態，你不用手動改**。之後呼叫
`HAL_SPI_Transmit(&hspi1, ...)` 就是把這個「操作手把」整包傳進去。

### 2.6 `HAL_StatusTypeDef` —— 每個 HAL 函式的回傳值

```c
typedef enum {
    HAL_OK      = 0x00,
    HAL_ERROR   = 0x01,
    HAL_BUSY    = 0x02,
    HAL_TIMEOUT = 0x03,
} HAL_StatusTypeDef;
```

所有 `HAL_SPI_Transmit()`、`HAL_SPI_TransmitReceive()`、`HAL_SPI_Init()`
都回傳這個 enum，永遠要檢查是不是 `HAL_OK`。

### 2.7 傳輸用的兩個 API

```c
HAL_StatusTypeDef HAL_SPI_Transmit(
    SPI_HandleTypeDef *hspi,   // 操作手把
    uint8_t *pData,            // 要送出去的資料（MOSI）
    uint16_t Size,             // 幾個 byte
    uint32_t Timeout);         // 逾時毫秒數，超過就回傳 HAL_TIMEOUT

HAL_StatusTypeDef HAL_SPI_TransmitReceive(
    SPI_HandleTypeDef *hspi,
    uint8_t *pTxData,          // MOSI 送出的資料
    uint8_t *pRxData,          // MISO 收到的資料存這裡
    uint16_t Size,
    uint32_t Timeout);
```

`Transmit` 只送不收（F6222 的 Write 命令用這個）；`TransmitReceive` 邊送邊收
（F6222 的 Read 命令要邊送位址邊收資料，用這個）。

---

## 3. F6222 driver 那一側的兩個 struct

`include/f6222.h` 裡只有兩個跟 SPI 有關的東西，完全不依賴 STM32：

```c
typedef struct {
    int (*spi_xfer)(void* ctx, const uint8_t* tx, uint8_t* rx, size_t len);
    void* ctx;   // 原封不動傳給 spi_xfer 的第一個參數
} f6222_dev_t;

typedef enum {
    F6222_OK = 0,
    F6222_ERR_INVALID_ARG = -1,
    F6222_ERR_SPI = -2,
    ...
} f6222_status_t;
```

`f6222_dev_t` 就是整個 driver 唯一需要你「填」的東西：一個 function pointer
`spi_xfer`，加一個你自己定義的 `ctx`（driver 完全不管 `ctx` 裡面是什麼，
只是原封不動傳給你的 callback）。`f6222_spi.c` 裡所有函式（像
`f6222_local_reg_write()`）最後都是呼叫：

```c
dev->spi_xfer(dev->ctx, tx, rx, len);
```

driver 只知道「呼叫這個函式就能把 `tx` 陣列送出去、`rx` 陣列會收到回應」，
其餘完全不關心。

---

## 4. 膠水層：`f6222_spi_stm32.c` —— 把兩邊接起來

這是你（或 board support 工程師）要寫的部分，範例專案已經寫好了
`platform/stm32/src/f6222_spi_stm32.c`：

```c
/* platform/stm32/include/f6222_spi_stm32.h（結構定義） */
typedef struct {
    SPI_HandleTypeDef* hspi;        // 用哪個 SPI handle（&hspi1）
    void*              cs_gpio_port;// CS 腳在哪個 GPIO port（GPIOD）
    uint16_t           cs_pin;      // CS 是哪一根腳（GPIO_PIN_14）
    uint32_t           timeout_ms;  // HAL 傳輸逾時時間
} f6222_spi_stm32_t;
```

```c
/* platform/stm32/src/f6222_spi_stm32.c */

int f6222_spi_stm32_xfer(void* ctx, const uint8_t* tx, uint8_t* rx, size_t len) {
    if (ctx == NULL || tx == NULL || len == 0U) {
        return -1;
    }

    f6222_spi_stm32_t* hal = (f6222_spi_stm32_t*)ctx;
    GPIO_TypeDef* cs_port = (GPIO_TypeDef*)hal->cs_gpio_port;
    HAL_StatusTypeDef status;

    HAL_GPIO_WritePin(cs_port, hal->cs_pin, GPIO_PIN_RESET); // ① CS 拉低，開始一筆交易

    if (rx != NULL) {
        status = HAL_SPI_TransmitReceive(hal->hspi, (uint8_t*)tx, rx, len, hal->timeout_ms); // ② 邊送邊收
    } else {
        status = HAL_SPI_Transmit(hal->hspi, (uint8_t*)tx, len, hal->timeout_ms);             // ② 只送
    }

    HAL_GPIO_WritePin(cs_port, hal->cs_pin, GPIO_PIN_SET);   // ③ CS 拉高，交易結束

    return (status == HAL_OK) ? 0 : -1;                       // ④ 轉成 F6222 認得的 0/-1
}

void f6222_spi_stm32_bind(f6222_dev_t* dev, f6222_spi_stm32_t* hal) {
    if (dev == NULL || hal == NULL) return;
    dev->spi_xfer = f6222_spi_stm32_xfer;  // 把上面這個函式登記成 callback
    dev->ctx = hal;                        // ctx 指向這包 STM32 專屬設定
}
```

這個函式就是**唯一一個真正呼叫 STM32 HAL SPI API 的地方**。步驟固定四步：
CS 拉低 → 傳輸 → CS 拉高 → 把 `HAL_OK`/其他 轉成 F6222 認得的 `0`/`-1`。

---

## 5. main.c 怎麼把整條路接起來（實際初始化順序）

```c
/* 全域變數（main.c） */
SPI_HandleTypeDef hspi1;              // §2.5 SPI 操作手把
f6222_dev_t       f6222_dev;          // §3  F6222 抽象裝置
f6222_spi_stm32_t f6222_spi = {       // §4  STM32 專屬設定
    .hspi         = &hspi1,
    .cs_gpio_port = GPIOD,
    .cs_pin       = GPIO_PIN_14,
    .timeout_ms   = 100,
};

int main(void) {
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();     // 設定 PD14 為輸出腳（CS），先預設拉「高」（不選中）
    MX_SPI1_Init();     // 把 §2.4 那張設定單套進 hspi1，初始化 SPI1 硬體
    MX_USART3_UART_Init();

    f6222_spi_stm32_bind(&f6222_dev, &f6222_spi); // 把 f6222_dev.spi_xfer 接到膠水層

    if (f6222_init(&f6222_dev, 0) != F6222_OK) {  // 之後 driver 呼叫都靠 f6222_dev 這個 handle
        printf("f6222_init fail\r\n");
    }
    ...
}
```

初始化順序記一句話：**先把硬體（GPIO、SPI1）準備好，再把 F6222 driver
「插」到膠水層上，最後才呼叫 F6222 的高階 API**。`f6222_init()`／
`f6222_local_reg_write()` 這些函式全程不知道自己是跑在 STM32 上，
它們只認得 `f6222_dev_t`。

---

## 6. 實際送出去的 bit 長什麼樣（跟 SPI 結構對起來）

以 `f6222_local_reg_write()`（`src/f6222_spi.c`）為例：

```c
uint8_t tx[5] = {0};
tx[0] = (F6222_SPI_M_LOCAL_REG_WRITE << 5) | ((rf_load & 0x03u) << 3); // Mode + RF_LOAD
tx[1] = chip_addr & F6222_CHIP_ADDR_MASK;                              // 晶片位址 ADD[4:0]
tx[2] = reg & F6222_SPI_REG_ADDR_MASK;                                 // 暫存器位址 RA[6:0]
tx[3] = (uint8_t)(val >> F6222_SPI_DATA_HIGH_SHIFT);                   // 資料高位元組
tx[4] = (uint8_t)(val & F6222_SPI_DATA_LOW_MASK);                      // 資料低位元組

dev->spi_xfer(dev->ctx, tx, NULL, sizeof(tx));  // 5 bytes，只送不收
```

對照 §2.4 的 `SPI_InitTypeDef`：

- `DataSize = SPI_DATASIZE_8BIT` → 這 5 個 byte 是一個一個送，不是拆成 bit 流亂序。
- `FirstBit = SPI_FIRSTBIT_MSB` → 每個 byte 內部 bit7 先出，所以 `tx[0]` 的
  `<< 5`／`<< 3` 這種位移排列才會對應到 datasheet 上「M[2:0] 在最高 3 bit」的圖。
- CS 一路保持低電位直到 5 個 byte 全部送完（膠水層一次呼叫
  `HAL_SPI_Transmit(..., 5, ...)`，不是呼叫 5 次），這樣 F6222 才知道
  這 5 個 byte 是同一筆命令。

`f6222_local_reg_read()` 則是 `rx` 不為 NULL，膠水層改用
`HAL_SPI_TransmitReceive()`：MOSI 送出「命令 + padding」，同時 MISO
收回「命令回音 + 16-bit 資料」，收到的 `rx[3]`/`rx[4]` 就是暫存器內容。

---

## 7. 移植到別的 MCU / 別片 SPI，要改哪裡？

`f6222.h` 開頭的註解已經寫了移植三步驟，對應到這份文件就是：

1. 照著第 4 節的樣子，寫一個你自己的 `spi_xfer()`：CS 拉低 → 呼叫你 MCU 的
   SPI 傳輸函式（CPOL=0, CPHA=0, MSB first, ≤50MHz）→ CS 拉高。
2. 把 `f6222_dev_t.spi_xfer` 指向你寫的函式，`ctx` 指向你自己的
   SPI handle 包裝 struct（不一定要叫 `f6222_spi_stm32_t`，換 MCU就換名字）。
3. F6222 driver 本體（`f6222.h` / `src/*.c`）完全不用改一行。

換句話說，STM32 那些 `SPI_HandleTypeDef`／`GPIO_InitTypeDef` 只活在
「膠水層」這一層，F6222 driver 核心永遠只看得到 `f6222_dev_t`。
