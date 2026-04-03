# Level Aggregator Study Guide

## Overview

The **Level Aggregator** is a high-performance Sierra Chart study designed to consolidate price levels from across multiple charts and timeframes into a single, actionable display. It scans for user-drawn text objects (like Horizontal Lines, Rays, and Rectangles) and aggregates them into a real-time table or draws them directly on your active chart.

This study is optimized for ES/MES scalpers who need real-time structural awareness without sacrificing Sierra Chart performance during high-volatility events.

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
-   **Table Toggle Button #**: The ID of the button (1-150) on your Sierra Chart Control Bar that will toggle the **HUD Table** visibility. Set to `0` to disable button control.
-   **Table Button Text**: The label shown on the Control Bar button (e.g., "Levels").
-   **Line Toggle Button #**: The ID of the button (1-150) that toggles the **Chart Lines** visibility. Set to `0` to disable button control.
-   **Line Button Text**: The label shown on the line toggle button (e.g., "Lines").
-   **Line Type**: Choose between **Short Line** (right-aligned) or **Full Width** horizontal lines.
-   **Display Mode (Fallback)**: If both button numbers are set to `0`, the study uses this dropdown to determine what is shown (Always-On mode).
-   **Target Labels**: A comma-separated list of labels to scan for. 
    -   Use `|All` suffix (e.g., `BZ|All`) to find every instance of that label across all charts. 
    -   Without `|All`, it only finds the *nearest* instance above or below the current price for that specific label.
-   **Charts to Scan**: A comma-separated list of Chart IDs and their friendly names (Format: `ChartNum|Name`).
-   **Auto-Scan Heartbeat**: Configures the background scanning interval (Default: 1 second). This ensures cross-chart updates are tactical and responsive while protecting the UI thread from constant API calls.

### 2. Table Settings
-   **X/Y Position**: Pixel offsets for the HUD table.
-   **Max Table Levels**: Limits the number of rows shown above and below the current price (0 = Show All).
-   **Colors & Font**: Full control over text, background, and highlight colors.
-   **Highlighting**: The study automatically highlights the nearest level immediately above and below the current price.

### 3. Chart Line Settings
-   **Line Style/Width/Color**: Customize the visual appearance of the aggregated lines.
-   **Show Labels on Lines**: Toggles the `Description (Chart)` labels on the chart lines.
-   **Short Line Length (Bars)**: When using "Short Line" mode, this dictates how many bars back from the right edge the line starts.

### 4. Sort & Export Settings
-   **Multi-tier Sorting**: Sort by Price (Asc/Desc), Chart Priority, Label, or Description.
-   **Export on Scan**: Automatically export aggregated levels to the Clipboard or a local text file.
-   **Template Support**: Use custom `.txt` templates with tags like `{{PRICE}}`, `{{LABEL}}`, `{{DESC}}`, and `{{CHART}}` for seamless integration with external tools (like Obsidian).

---

## 🚀 Control & Fallback Logic

-   **Manual Control**: Toggle HUD elements independently via Control Bar buttons.
-   **Always-On**: Set button numbers to `0` to follow the **Display Mode** dropdown by default.

---

## 🚀 Performance Notes ("Zero-Lag" Engine)

The Level Aggregator is designed for "Institutional Grade" efficiency:
-   **1-Second Heartbeat**: Background scans are throttled to a 1-second interval to prevent UI lag during high-tick activity.
-   **Boundary-Based Redraws**: The study only calls `sc.UseTool` (the most expensive UI operation) when price crosses a level boundary or a new bar is added.
-   **String Caching**: The HUD table text is cached in memory and only rebuilt when levels change or highlights shift.
-   **Manual Looping**: Uses `sc.AutoLoop = 0` to minimize per-tick CPU overhead.
