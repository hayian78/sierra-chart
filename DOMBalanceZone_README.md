# DOM Balance Zone - User Guide

## Overview
> A lite version of the Balance Zone Engine optimized specifically for the Trading DOM. It projects key premium and discount balance zones (2x, 4x, 8x, 16x) with a zero-footprint visual design, rendering only floating text labels aligned cleanly within the DOM's price cells.

---

## Configuration (Study Inputs)

| Input Name | Type | Recommended Value | Description |
| :--- | :--- | :--- | :--- |
| **Cost Basis Upper Limit** | Float | `0.0` | The top edge of the primary cost basis. Set to 0 to instantly hide and disable all drawings. |
| **Cost Basis Lower Limit** | Float | `0.0` | The bottom edge of the primary cost basis. Set to 0 to instantly hide and disable all drawings. |
| **Cost Basis Color (1x)** | Color | Neutral Grey | Base color for the exact 1x upper/lower bounds. |
| **Up Color Base (Premium)** | Color | Green | Base color for upside (+2x, +4x, +8x, +16x) multiples. Fades in intensity as distance increases. |
| **Down Color Base (Discount)**| Color | Red | Base color for downside (-2x, -4x, -8x, -16x) multiples. Fades in intensity as distance increases. |
| **Enable Up Zones** | Yes/No | `Yes` | Toggles the generation of upward multiples on or off. |
| **Enable Down Zones** | Yes/No | `Yes` | Toggles the generation of downward multiples on or off. |

## Operational Use
* **Visual Cues:** You will see text labels like `+2x`, `+4x`, `-8x` floating on the far right of the Trading DOM's price column. The line width itself is `0`, meaning only the floating text appears. Colors automatically fade (decrease in RGB intensity) as the multipliers expand further from the cost basis.
* **Hotkeys:** No hotkeys native to this study. Adjust limits through the Study Settings window.
* **Limitations:** Designed strictly for visual use on the DOM. Requires valid manual upper and lower limits to display any zones.

## Developer Notes (ACSIL)
* **DLL Name:** `DOM Balance Zone`
* **Build Requirements:** MSVC 2022+ / x64.
* **Performance:** Dormant execution model (`sc.AutoLoop = 0`). The study consumes virtually zero CPU while idle. Recalculations and redraws strictly occur only when the High/Low limit inputs or color settings are modified by the user. Setting limits to `0` cleanly triggers an active deletion loop for all associated `sc.UseTool` objects to prevent artifacting.