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

## Developer Notes (ACSIL)
* **DLL Name:** `DailyAVWAP`
* **Build Requirements:** MSVC 2022+ / x64.
* **Performance:** Defaults to `sc.AutoLoop = 0` (Manual Looping). Employs double-precision accumulators for variance/VWAP, ensuring integer/float cast truncation does not dilute tick-level accuracy. Built-in crossover alerts feature per-bar deduplication.
