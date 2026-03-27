# Daily AVWAP — User Guide

**Version:** 1.0.0  
**Platform:** Sierra Chart (ACSIL Custom Study)  
**DLL Name:** `DailyAVWAP`  
**Study Name:** Daily AVWAP  

---

## Overview

Daily AVWAP calculates a **Volume Weighted Average Price (VWAP)** that anchors and resets at a configurable time each day. This is designed for instruments with an overnight session (e.g. CME futures) where the trading day starts at a specific time (typically 17:00 ET for US futures).

The study plots the VWAP line along with two pairs of optional standard deviation bands, giving you a statistical envelope around the day's volume-weighted mean price.

### What It Shows

- **VWAP Line** — The running volume-weighted average price since the daily anchor time.
- **±1 StdDev Bands** — The first standard deviation envelope (disabled by default).
- **±2 StdDev Bands** — The second standard deviation envelope (disabled by default).

### How It Calculates

- **Price Basis:** Last/Close price — matches Sierra Chart's built-in VWAP default.
- **Standard Deviation:** Calculated using the volume-weighted variance method (`sqrt(Σ(P²·V)/ΣV − VWAP²)`), using VWAP Variance (matching the built-in default).
- **Reset:** The VWAP resets its running totals when the configured anchor time is crossed. Each new "trading day" starts fresh.

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

Set this to match the session boundary of your instrument:
- **CME Futures (ES, NQ, CL, etc.):** `17:00:00` (5:00 PM ET)
- **Forex:** `17:00:00` (typical)
- **Equities / ETFs:** `09:30:00` (market open) or `04:00:00` (pre-market)

The study correctly handles overnight sessions — if the reset time is 17:00, bars from 17:00 onwards belong to the current anchor, and bars before 17:00 belong to the previous day's anchor.

### StdDev Band 1 Multiplier
| | |
|---|---|
| **Default** | `1.0` |
| **Description** | Multiplier for the first pair of standard deviation bands (±1 StdDev). |

Adjust this to change the width of the inner bands. Common values: `0.5`, `1.0`, `1.5`.

### StdDev Band 2 Multiplier
| | |
|---|---|
| **Default** | `2.0` |
| **Description** | Multiplier for the second pair of standard deviation bands (±2 StdDev). |

Adjust this to change the width of the outer bands. Common values: `1.5`, `2.0`, `2.5`, `3.0`.

### Alert Sound Number
| | |
|---|---|
| **Default** | `0` (Off) |
| **Description** | Sierra Chart alert sound number to play when price crosses any visible VWAP/band line. Set to `0` to disable alerts. |

Alerts trigger once per bar when price crosses above or below a visible line. Bands set to **"Ignore"** draw style will not trigger alerts.

### Alert Message
| | |
|---|---|
| **Default** | `Daily AVWAP Crossover` |
| **Description** | Text message displayed in Sierra Chart's alert log when a crossover is detected. |

---

## Enabling the Standard Deviation Bands

The StdDev bands are **hidden by default** (draw style set to "Ignore"). To enable them:

1. Open the study settings (**Analysis → Studies**, select Daily AVWAP, click **Settings**).
2. Go to the **Subgraphs** tab.
3. For each band you want to show (e.g. `+1.0 StdDev`, `-1.0 StdDev`), change the **Draw Style** from `Ignore` to `Line` (or `Dash`, etc.).
4. Customise the colour, width, and style to your preference.

---

## Usage Tips

### Interpreting VWAP

- **Price above VWAP** → Buyers are in control for the session; longs are in profit on average.
- **Price below VWAP** → Sellers are in control; shorts are in profit on average.
- **VWAP acts as a magnet** → Price tends to return to VWAP, especially in range-bound markets.
- **VWAP as support/resistance** → Institutional traders often benchmark execution against VWAP, making it a natural level for reactions.

### Interpreting the Bands

- **±1 StdDev** → Approximately 68% of the volume-weighted price action falls within this range. Moves beyond ±1 StdDev suggest directional conviction.
- **±2 StdDev** → Approximately 95% of the action. Touches here often represent overextension and potential mean-reversion setups.
- **Band width expanding** → Increasing volatility / trending.
- **Band width contracting** → Decreasing volatility / consolidating.

### Combining with Other Tools

Daily AVWAP works well alongside:
- **Balance Zone Projector** — Use VWAP bands as confluence with value area levels.
- **Volume Profile** — Compare VWAP with the Point of Control (POC) for convergence/divergence.
- **Time Block Highlighter** — Visualise session boundaries that align with the VWAP reset time.

---

## FAQ

**Q: Why does the VWAP line flatline during some periods?**  
A: This happens on bars with zero volume (e.g. low-liquidity periods, data gaps). The study carries forward the previous VWAP value until volume resumes. This is normal and correct behaviour.

**Q: Do I need to change the reset time for different instruments?**  
A: Yes. The default of 17:00 is correct for CME futures. For equities, you may want 09:30 (RTH open) or 04:00 (pre-market). Choose the time that represents the natural "start of day" for your instrument.

**Q: Why don't I see the StdDev bands?**  
A: They are hidden by default. See the [Enabling the Standard Deviation Bands](#enabling-the-standard-deviation-bands) section above.

**Q: Will alerts fire multiple times on the same bar?**  
A: No. The study deduplicates alerts so each crossover only fires once per bar, regardless of how many times the study recalculates.

**Q: Do disabled (hidden) bands trigger alerts?**  
A: No. Only bands with a visible draw style (anything other than "Ignore") are checked for crossovers.

**Q: Can I use this on tick charts or Renko charts?**  
A: Yes, it works on any chart type that has volume data. The calculation uses per-bar volume regardless of bar type.

---

## Changelog

### v1.0.0 — 2026-02-18
- Initial public release
- VWAP with HLC/3 Typical Price, daily anchor reset
- Two configurable standard deviation band pairs
- Crossover alerts with deduplication
- Alerts respect band visibility (hidden bands don't alert)
