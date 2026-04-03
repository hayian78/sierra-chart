# Level Aggregator Study Guide

## Overview

The **Level Aggregator** is a high-performance Sierra Chart study designed to consolidate price levels from across multiple charts and timeframes into a single, actionable display. It scans for user-drawn text objects (like Horizontal Lines, Rays, and Rectangles) and aggregates them into a real-time table or draws them directly on your active chart.

This study is ideal for traders who manage multiple timeframes (e.g., Weekly, Daily, 4HR, 30MIN) and need a unified "Heads-Up Display" (HUD) of key structural levels like Balance Zones, Support/Resistance, or Highs/Lows.

---

## 🛠 Drawing Format (The "Protocol")

To make a drawing discoverable by the Level Aggregator, you must follow a simple naming convention in the **Text** field of the Sierra Chart drawing tool:

**Format:** `LABEL|Description`

-   **LABEL**: A short tag used for filtering (e.g., `BZ`, `PDC`, `>>`).
-   **Description**: A human-readable description shown in the table and on line labels (e.g., `Top of Balance`, `Previous Day Close`).
-   **Example**: `BZ|Weekly Balance High`

---

## ⚙️ Configuration Settings

### 1. Core Settings
-   **Control Bar Button #**: The ID of the button (1-150) on your Sierra Chart Control Bar that will toggle the display on/off and trigger a fresh scan.
-   **Button Text**: The label shown on the Control Bar button (e.g., "Levels").
-   **Display Mode**: Determines how levels are shown:
    -   **Table Only**: Shows the consolidated HUD table.
    -   **Table + Short Lines**: Shows the table plus short lines on the right edge of the chart.
    -   **Table + Full Lines**: Shows the table plus full-width horizontal lines.
    -   **Short Lines Only**: No table, just short lines on the right edge.
    -   **Full Lines Only**: No table, just full-width horizontal lines.
-   **Target Labels**: A comma-separated list of labels to scan for. 
    -   Use `|All` suffix (e.g., `BZ|All`) to find every instance of that label across all charts. 
    -   Without `|All`, it only finds the *nearest* instance above or below the current price for that specific label.
-   **Charts to Scan**: A comma-separated list of Chart IDs and their friendly names.
    -   Format: `ChartNum|Name`
    -   Example: `1|TPO, 9|1 MIN, 5|Weekly`

### 2. Table Settings
-   **X/Y Position**: Pixel offsets from the top-left of the chart.
-   **Max Table Levels**: Limits the number of rows shown above and below the current price (0 = Show All).
-   **Colors & Font**: Full control over text, background, and highlight colors.
-   **Highlighting**: The study automatically highlights the nearest level immediately above and below the current price.

### 3. Chart Line Settings
-   **Line Style/Width/Color**: Customize the visual appearance of the aggregated lines.
-   **Show Labels on Lines**: Toggles the `Description (Chart)` labels on the chart lines.
-   **Short Line Length (Bars)**: When using "Short Lines" modes, this dictates how many bars back from the right edge the line starts.

### 4. Sort Settings
-   Multi-tier sorting support. You can sort by Price (Asc/Desc), Chart Priority, Label, or Description.

---

## 📤 Export Features (Optional)

The Level Aggregator can export its results to the **Clipboard** or a **File** every time a scan is triggered.

-   **Use Template**: You can provide a template file with markers like `{{PRICE}}`, `{{DESCRIPTION}}`, and `{{CHART_NAME}}` to generate custom-formatted lists (e.g., for import into other tools or for session notes).

---

## 🚀 Performance Notes

-   **Manual Looping**: The study uses `sc.AutoLoop = 0` and only recalculates on new ticks or user actions.
-   **Scan Efficiency**: It only scans the charts you explicitly define.
-   **Drawing Management**: All on-chart drawings created by the study use a reserved ID range (`98765432` to `98765599`) and are automatically cleaned up when hidden or the study is removed.
