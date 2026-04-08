# Level Aggregator - User Guide

## Overview
> Consolidates essential structural levels scattered across multiple charts and timeframes into a single on-chart HUD or line display. It automatically maps and sorts nearest liquidity levels, ensuring immediate situational awareness above and below the current executing price.

---

## Configuration (Study Inputs)
| Input Name | Type | Recommended Value | Description |
| :--- | :--- | :--- | :--- |
| **Table Toggle Button #** | Integer | `1` | ID of the Control Bar button (1-150) that toggles the HUD Table visibility. Set to 0 to disable. |
| **Line Toggle Button #** | Integer | `2` | ID of the Control Bar button (1-150) that toggles the Chart Lines visibility. Set to 0 to disable. |
| **Target Labels** | String | `>>\|All` | Comma-separated list of labels to scan for. The `\|All` suffix finds every instance across charts. |
| **Charts to Scan** | String | `1\|TPO,9\|1 MIN` | Comma-separated list of Chart IDs and their friendly names (Format: `ChartNum\|Name`). |
| **Cluster Threshold** | Integer | `5` | Distance in ticks to consider levels as clustered. |
| **Clustered Text Handling** | Dropdown | `Stagger Horizontally` | Determines if overlapping levels should merge or stagger horizontally to maintain legibility. |
| **Show Price on Lines** | Yes/No | `Yes` | Appends the price in brackets to chart line labels: `Label (Chart) [Price]`. |
| **Label Margin** | Integer | `20` | Number of bars beyond the right axis to offset labels for improved legibility. |

## Operational Use
* **Visual Cues:** A HUD table displaying nearest levels, and/or horizontal lines on the chart.
* **Label Formatting:** Chart lines use the format: `Label (Source Chart) [Price]`. The price component can be toggled off for a more compact display.
* **Margin Handling:** Labels are projected into the right margin (20 bars by default) to prevent price action from obscuring the text.
* **Clustering:** Levels clustered closely together will automatically merge or stagger their text labels, ensuring readability on compressed charts.
* **Hotkeys:** Configurable Control Bar Buttons to toggle the HUD Table and Lines.
* **Limitations:** Dependent on user-drawn shapes adhering to a strict naming convention in the text field (`LABEL\|Description`, e.g., `BZ\|Top of Balance`).

## Developer Notes (ACSIL)
* **DLL Name:** `Level Aggregator`
* **Build Requirements:** MSVC 2022+ / x64.
* **Performance:** `sc.AutoLoop = 0`, `sc.UpdateAlways = 1`. Implements a throttled 1-second background scanning heartbeat to retrieve data via `sc.GetUserDrawnChartDrawing`, protecting the UI thread from lag during high-tick activity.
