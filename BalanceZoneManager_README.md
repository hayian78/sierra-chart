# Tool: Balance Zone Manager - User Guide

## Overview
> Maintenance utility for the Balance Zone Suite that "Shrink-Wraps" historical anchor projections. It optimizes archived anchors by adjusting their multipliers to cover only the realized price action, significantly reducing chart clutter.

---

## Configuration (Study Inputs)

| Input Name | Type | Recommended Value | Description |
| :--- | :--- | :--- | :--- |
| **Run Shrink-Wrap Now** | Yes/No | `No` | One-time trigger. Set to **Yes** to run the optimization; it will auto-reset to No. |
| **Base Labels** | String | `BZ` | Comma-separated list of labels to identify anchor rectangles (e.g., `BZ, BAL`). |
| **Minimum Multiplier** | Integer | `0` | The minimum number of zone groups to maintain, even if price never reached them. |
| **Force Update** | Yes/No | `No` | If **No**, skips anchors with explicit multipliers (e.g., `BZ +4,-2`). Set to **Yes** to overwrite manual settings. |
| **Enable Debug Logging** | Yes/No | `No` | If **Yes**, outputs diagnostic details to the Sierra Chart Message Log. |

## Operational Use
* **Shrink-Wrap Logic**: The tool scans all rectangles matching the **Base Labels**. It looks at the High/Low of all bars *between* the rectangle's start and end time. It then calculates the minimum multipliers needed to contain that price action and updates the label (e.g., `BZ` -> `BZ +3,-1`).
* **Triggering**: To optimize your chart, simply toggle **Run Shrink-Wrap Now** to **Yes**. The tool will process all anchors and immediately reset the trigger.
* **Preservation**: The manager is designed to be "non-destructive." It preserves your custom descriptions (text after the `|`), Tier overrides (`T1`, `T2`, `T3`), and "Nice" stars (`BZ*`).

## Technical Performance (Zero-Lag)
* **Zero Per-Tick Overhead**: This study utilizes `sc.IsFullRecalculation` gating. It remains completely dormant during active trading, performing **zero calculations** on new price ticks.
* **Low Priority**: Set to `sc.CalculationPrecedence = LOW_PREC_LEVEL` to ensure it never competes with execution-critical studies or order management logic.
* **Optimized Parsing**: Uses high-performance string parsing to handle complex labels and descriptions with minimal CPU impact.

## Developer Notes (ACSIL)
* **DLL Name:** `Balance Zone Manager`
* **Build Requirements:** MSVC 2022+ / x64.
* **Execution**: Manual Looping (`sc.AutoLoop = 0`).
* **Maintenance**: Recommended for use on archived charts or at the end of a session to clean up historical levels.

---
**Lead Quantitative Architect** | *Quantitative structural analysis for professional futures execution.*
