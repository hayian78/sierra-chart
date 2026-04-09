# Daily AVWAP - User Guide

## Overview
> Computes an anchored Volume Weighted Average Price (VWAP) that resets daily at a configurable time. This prevents overnight gaps from skewing averages and provides a statistical envelope around the day's volume-weighted mean price using up to five pairs of standard deviation bands.

---

## Configuration (Study Inputs)
| Input Name | Type | Recommended Value | Description |
| :--- | :--- | :--- | :--- |
| **Daily Reset Time** | Time | `17:00:00` | The time of day that the VWAP resets and begins a new calculation period (typically 17:00 ET for CME futures). |
| **Alert Sound Number** | Integer | `0` | Sierra Chart alert sound number to play when price crosses any visible VWAP/band line. Set to 0 to disable. |
| **Alert Message** | String | `Daily AVWAP Crossover` | The message displayed when the alert is triggered. |
| **StdDev Band X Multiplier** | Float | `0.5`, `1.0`, etc. | Multipliers for the standard deviation bands. |
| **Enable Band X (+/-)** | Yes/No | `No` | Master toggle for each band pair. Setting this to Yes will immediately draw both the positive and negative bands for that multiplier. |

## Operational Use
* **Visual Cues:** The chart plots a central VWAP line and up to five standard deviation bands around it. When price is above VWAP, buyers are typically in control; when below, sellers are in control.
* **Hotkeys:** None natively defined.
* **Limitations:** Requires accurate tick-level volume from your data feed to properly compute the variance and volume-weighted mean.

## Visual Configuration: Price Scale Labels

For execution-focused charts (Footprint, DOM), it is often beneficial to move the Standard Deviation levels to the right-hand price axis (Y-axis) to reduce visual clutter while maintaining volatility context.

### Setup Instructions
1.  **Open Study Settings (F6)** and select the `Daily AVWAP` study.
2.  Navigate to the **Subgraphs** tab.
3.  For each enabled STD band (e.g., STD +1, STD -1):
    *   Set **Draw Style** to `Subgraph Name and Value Labels Only` (this removes the line but keeps the text markers).
    *   Enable **Value Label** (shows the price).
    *   Enable **Name Label** (shows "STD +1").
4.  **Note:** Using this specific Draw Style is the most reliable way to display labels on the price scale without horizontal "clutter" lines.

### Trader's Strategy Note
Moving levels to the axis is recommended for **ES/MES Scalping** to:
*   **Clear Visual Real Estate:** Prevents horizontal lines from obscuring footprint delta or order flow imbalances.
*   **Precision Targeting:** Provides the exact statistical "stretch points" (SD1/SD2) as fixed coordinates on the price scale for rapid limit-order placement.
*   **Reduce Visual Fatigue:** Minimizes "visual fatigue" (spiderwebbing) on the chart during high-volatility sessions (RTH Open, FOMC).

## Developer Notes (ACSIL)
* **DLL Name:** `DailyAVWAP`
* **Build Requirements:** MSVC 2022+ / x64.
* **Performance:** Defaults to `sc.AutoLoop = 0` (Manual Looping). Employs double-precision accumulators for variance/VWAP, ensuring integer/float cast truncation does not dilute tick-level accuracy. Built-in crossover alerts feature per-bar deduplication.
