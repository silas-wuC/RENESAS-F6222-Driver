# RENESAS F6222 Driver

[Renesas F6222](https://www.renesas.com/) 十六通道雙波束接收主動波束成形 RF IC 之驅動庫。適用 Ka 頻段 SATCOM 平面相控陣天線。

> 規格依據：R35DS0065EU0100 Rev.1.00（2025-01-27）

## 概述

F6222 為雙波束接收主動波束成形 IC，工作於 **17.7 GHz – 21.2 GHz**。具八 RF 輸入、二 RF 輸出、十六路（每波束八路）相位/幅度控制通道。

八輸入可接八單極化元件，或四雙極化天線元件。各路具 6-bit 數位相位與增益控制，覆蓋 360° 與 28.4 dB，典型 RMS 相位誤差 2°、增益誤差 0.18 dB。

典型功耗（標稱偏置）：
- 雙波束：556 mW（35 mW/ch）
- 單波束：377 mW（47 mW/ch）

典型電子增益 30 dB、噪聲係數 1.3 dB、通道間隔離 35 dB。

## 應用

- 電子掃描相控陣天線（ESA）
- 航空、海事、動中通（SOTM）
- Ka 頻段 SATCOM 終端

## 主要特性

| 項目 | 規格 |
|------|------|
| 頻率範圍 | 17.7 – 21.2 GHz |
| 電子增益 | 1.6 – 30 dB（步進 0.45 dB，RMS 誤差 0.18 dB） |
| 相位控制 | 360°（步進 5.6°，RMS 誤差 2°） |
| 輸入 P1dB | -52.5 dBm |
| 噪聲係數 | 1.3 dB |
| 通道間隔離 | 35 dB |
| 封裝 | 7.60 × 7.60 × 0.90 mm，165-FCCSP（λ/2 網格平面整合） |
| 工作溫度 | -40°C – +85°C |

### 功耗模式

| 模式 | 典型功耗 |
|------|----------|
| 單波束 | 47 mW/ch |
| 雙波束 | 35 mW/ch |
| 低偏置（CH_MODE=0） | 較標稱降約 18% |
| 待機（core） | 7 mW |

### 供電

| 電源 | 電壓 |
|------|------|
| Core（VDD） | 2.1 – 2.5 V（典型 2.3 V） |
| LNA（VDDB） | 1.1 – 1.3 V（典型 1.2 V） |
| 內部 LDO（數位） | 1.8 V |

## 硬體介面

### RF 埠

| 埠 | 類型 | 說明 |
|----|------|------|
| RF1 – RF8 | 輸入 | 八 RF 輸入；每埠驅動兩通道 |
| RFC1 | 輸出 | 波束 1 合路輸出（奇數通道） |
| RFC2 | 輸出 | 波束 2 合路輸出（偶數通道） |

RF 埠對應：

| RF 埠 | 通道 |
|-------|------|
| RF1 | CH1, CH2 |
| RF2 | CH3, CH4 |
| RF3 | CH5, CH6 |
| RF4 | CH7, CH8 |
| RF5 | CH9, CH10 |
| RF6 | CH11, CH12 |
| RF7 | CH13, CH14 |
| RF8 | CH15, CH16 |

### 數位介面（4-wire SPI）

| 信號 | 說明 |
|------|------|
| SCLK | SPI 時鐘（最高 50 MHz，上升沿採樣） |
| SDI | 資料輸入 |
| SDO | 資料輸出（listen 時高阻） |
| CSB | 片選（低有效） |
| ADD[1:5] | 5-bit 位址，每 SPI 匯流排最多 32 顆 |
| RST | 非同步復位（低有效，恢復出廠預設） |
| MODE | 單/雙波束選擇（低=SB，高=DB） |

### 其他控制腳

| 腳位 | 說明 |
|------|------|
| LNA1_EN – LNA4_EN | LNA 使能；未用時接 VDDB |
| REXT | 可選外部偏置電阻 |
| CREG | 1.8 V LDO 輸出，需 10 µF 電容至 GND |
| AOUT | 類比參數讀出（保留，建議浮接或接地） |

## 功能概要

### 雙波束操作

- **MODE 腳高**：雙波束（預設）
- **MODE 腳低**：單波束；亦可經 SPI 將偶數通道 `CH_PWD=1` 關閉
- 單波束系統僅用 RFC1，RFC2 應端接或開路

### 快速波束指向（FBS）

內建 **128 態 LUT**，每態含 16 通道相位、增益、使能設定。相位/增益 settling 約 **50 ns**。支援：

- Local / Global Register Write & Read
- Local / Global LUT Write & Read
- Local / Global Fast Beam Steering（含 Toggle Enable 連續掃描）

### 可編程偏置

| 功能 | 控制欄位 | 效果 |
|------|----------|------|
| 低功耗偏置 | `CH_MODE` | IDD 降約 18%，增益降約 6 dB |
| 線性度提升 | `LNA_SW` | 旁路 DA，IP1 升約 5 dB，增益降約 6 dB |
| 溫補 | `MB_PT2_EN` | PTAT/PTAT2 自動偏置溫補 |

### 溫度感測

片內 ADC 讀取溫度（-40°C – 125°C）。寫 `ADC_CTRL.TEMP` 觸發量測，結果存於 `TEMP_DATA[9:0]`。單點校準公式：

```
T = (C - C0) / 1.3 + T0
```

其中 C0 為待機態已知環溫 T0 下讀取之基準碼。

## SPI 協議摘要

| M[2:0] | 模式 | 範圍 | 位元長 |
|--------|------|------|--------|
| 001 | Local Register Write | Local | 40 |
| 000 | Local Register Read | Local | 24 |
| 110 | Local LUT Write | Local | 40 |
| 111 | Local LUT Read | Local | 24 |
| 011 | Global Register Write | Global | 32 |
| 010 | Global LUT Write | Global | 32 |
| 101 | Local Fast Beam Steering | Local | 24 |
| 100 | Global Fast Beam Steering | Global | 16 + 8×N |

`RF Load = 01` 可立即將 `CHn_SET` 相位/增益鎖存至 RF 通道；支援匯流排上多晶片同步更新。

## 暫存器地圖（摘要）

| 位址 | 名稱 | 說明 |
|------|------|------|
| 0x00 | CTRL_CFG | 全域關斷、子陣列索引 |
| 0x01 | PTAT_BIAS_CFG | 主偏置、PTAT/PTAT2 |
| 0x02 | SCRATCH | 測試用 |
| 0x03 | MO_MEM_ACT | 當前模式與 LUT 態 |
| 0x05 | CLK_CTRL | ADC 時鐘 |
| 0x06 | ADC_CTRL | 振盪器與 ADC 觸發 |
| 0x0B | TEMP_DATA | 溫度 ADC 資料 |
| 0x20–0x5C | CHn_BIAS | 各通道偏置（步進 +4） |
| 0x21–0x5D | CHn_CTRL | 各通道控制（步進 +4） |
| 0x22–0x5E | CHn_SET | 各通道相位/增益/使能（步進 +4） |
| 0x69–0x6C | LNAIREFn | LNA IDAC 偏置 |

各通道 `CHn_SET` 欄位：

| 欄位 | 位元 | 說明 |
|------|------|------|
| PS_SET | [15:10] | 6-bit 相位（0–63 → 0°–360°） |
| VGA_SET | [9:4] | 6-bit 增益（0–31 低段，32–63 高段） |
| LNA_SW | [3] | DA 旁路（0=高線性，1=標稱） |
| CH_PWD | [0] | 通道關閉（1=關） |

## 子陣列拓撲

典型應用：每顆 F6222 驅動四雙極化 patch，設定接收極化與波束指向。同一 4-wire SPI 上最多 **32 顆** F6222 為一子陣列，各晶片以 `ADD[1:5]` 編址。多子陣列合併為完整相控陣。

天線單元間距建議 **λ/2**（最高工作頻率），以抑制柵瓣。

## 訂購資訊

| 料號 | 說明 |
|------|------|
| RRF62220-B00 | 165-FCCSP，Tray |
| RRF62220-K00 | 165-FCCSP，Reel |
| RTKRF62220-000 | 評估套件 |

## 本庫規劃

本 repo 將提供 F6222 之 SPI 驅動、暫存器定義、波束/LUT 配置介面。詳盡電氣規格、時序、評估板 BOM 請參原廠 datasheet **R35DS0065EU0100**。

## 參考

- Datasheet：R35DS0065EU0100 Rev.1.00（2025-01-27）
- 評估系統：F62xx Evaluation System User Guide
