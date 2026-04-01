# Daily AVWAP — User Guide

**Version:** 1.4.0  
**Platform:** Sierra Chart (ACSIL Custom Study)  
**DLL Name:** `DailyAVWAP`  
**Study Name:** Daily AVWAP  

---

## Overview

Daily AVWAP calculates a **Volume Weighted Average Price (VWAP)** that anchors and resets at a configurable time each day. This is designed for instruments with an overnight session (e.g. CME futures) where the trading day starts at a specific time (typically 17:00 ET for US futures).

The study plots the VWAP line along with up to five pairs of standard deviation bands, giving you a statistical envelope around the day's volume-weighted mean price.

### Key Features (v1.4.0)

- **One-Click Toggles** — Quickly enable/disable both the positive and negative bands for each standard deviation level using the "Enable Band X" inputs.
- **Improved Defaults** — All bands now default to `DRAWSTYLE_LINE` for immediate visibility once enabled.
- **Five Band Pairs** — Supports up to 5 standard deviation levels (defaulting to increments of 0.5).

---

## Installation

1. Place `DailyAVWAP.cpp` into your Sierra Chart **Data** folder (or your custom studies source folder).
2. In Sierra Chart, go to **Analysis → Build Custom Studies DLL** to compile.
3. Add the study to your chart via **Analysis → Studies → Add Custom Study → Daily AVWAP**.

> **Note:** If upgrading from a previous version, you may need to **remove and re-add** the study for new settings to appear.

---

## Settings

### Daily Reset Time
| | |
|---|---|
| **Default** | `17:00:00` |
| **Description** | The time of day that the VWAP resets and begins a new calculation period. |

### Enable Band X (+/-)
| | |
|---|---|
| **Default** | `No` |
| **Description** | A "Master Toggle" for each band pair. Setting this to `Yes` will immediately draw both the + and - bands for that multiplier. |

### StdDev Band Multipliers (1-5)
| | |
|---|---|
| **Defaults** | `0.5`, `1.0`, `1.5`, `2.0`, `2.5` |
| **Description** | Multipliers for the standard deviation bands. These defaults match the standard Sierra Chart increment pattern. |

### Alert Sound Number
| | |
|---|---|
| **Default** | `0` (Off) |
| **Description** | Sierra Chart alert sound number to play when price crosses any visible VWAP/band line. Set to `0` to disable alerts. |

---

## Enabling the Standard Deviation Bands

In v1.4.0, managing bands is significantly easier:

1. **Quick Toggle:** Use the **Inputs** tab to set `Enable Band X (+/-)` to **Yes**. This will turn on both lines for that multiplier.
2. **Granular Control:** If you only want the *positive* band but not the negative one, leave the input enabled and go to the **Subgraphs** tab to set the specific negative band to `DRAWSTYLE_IGNORE`.
3. **Alerts:** Alerts will only trigger for bands that are **Enabled** in the inputs AND have a visible **Draw Style** in the subgraphs.

---

## Usage Tips

### Interpreting VWAP

- **Price above VWAP** → Buyers are in control; longs are in profit on average.
- **Price below VWAP** → Sellers are in control; shorts are in profit on average.

### Interpreting the Bands

- **±1.0 StdDev** → Approximately 68% of the volume-weighted action falls within this range.
- **±2.0 StdDev** → Approximately 95% of the action. Touches here often represent potential mean-reversion setups.

---

## Changelog

### v1.4.0 — 2026-04-02
- Added `Enable Band X (+/-)` master toggles for all 5 band pairs.
- Changed default Subgraph DrawStyle to `LINE` (hidden by toggle by default).
- Updated default multipliers to match SC standard: `0.5, 1.0, 1.5, 2.0, 2.5`.
- Improved alert logic to respect the new enable toggles.

### v1.0.0 — 2026-02-18
- Initial public release
- VWAP with daily anchor reset
- Crossover alerts with deduplication
