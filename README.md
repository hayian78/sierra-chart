# Sierra Chart Quantitative Studies

A collection of high-performance, custom ACSIL (Advanced Custom Study Interface and Language) studies for Sierra Chart, focusing on quantitative level generation, volume-weighted metrics, balance projection, and automated chart management.

## 🚀 Core Studies

### 1. Daily AVWAP (`DailyAVWAP.cpp`)
- **Intent**: Computes an anchored Volume Weighted Average Price (VWAP) that resets daily at a configurable time (e.g., 17:00 ET).
- **Features**: 
  - Precision standard deviation bands (±0.5, ±1.0, ±1.5, ±2.0, ±3.0).
  - High-performance manual loop calculation using double-precision accumulators.
  - Zero-volume bar gap handling (carries forward previous values without skewing averages).
  - Built-in crossover alerts with per-bar deduplication.

### 2. Balance Zone Engine (`BalanceZoneEngine.cpp`)
- **Intent**: Projects dynamic balance and premium/discount zones derived from user-drawn anchor rectangles.
- **Features**: 
  - On-the-fly configuration via chart drawing text labels (e.g., `BZ +6x,-2x`).
  - **Zero-Lag Architecture**: Highly optimized AnchorID tracking and targeted redraws eliminate UI flickering and scrolling lag.
  - **O(1) Gap Detection**: Constant-time mathematical boundary checking for high-precision ES/MES scalping alerts without the overhead of iterative looping.
  - Multi-tier formatting: "Nice" vs. "Basic" modes with midlines, price labels, and distinct color bands.

### 3. Level Aggregator (`LevelAggregator.cpp`)
- **Intent**: Consolidates essential structural levels from multiple charts and timeframes into a single on-chart HUD.
- **Features**:
  - Scans across specified charts using naming conventions (`LABEL|Description`).
  - **Dynamic Visualization**: Multiple display modes including on-chart HUD table, short edge lines, and full horizontal lines.
  - **Auto-Sync Labels**: Line labels automatically include source chart and description (e.g., `Description (Chart)`).
  - **Control Bar Integration**: Single-button toggle for all visual elements.
- **Detailed Guide**: [Level Aggregator Guide](LevelAggregator_README.md)

### 4. Drawing Cleaner (`DrawingCleaner.cpp`)
- **Intent**: A utility bound to a toolbar button for selective chart cleanup.
- **Features**:
  - **Customizable**: Choose exactly which types of drawings to delete (Rays, Extended Lines, Horizontal Segments, etc.).
  - **Exclusions**: Comma-separated, case-insensitive wildcard support to protect specific labeled drawings (e.g., exclude "Pivot" or "Zone").
  - **Zero Overhead**: Logic only executes upon button click; early-exit guards ensure no performance impact during normal trading.

### 5. Chart Snapshot Engine (`ChartSnapshotEngine.cpp`)
- **Intent**: Automated and manual chart image capture system.
- **Features**:
  - Configurable snapshot intervals and image dimensions.
  - **Telegram Integration**: Works with `TelegramWatcher.ps1` to export snapshots to messaging platforms.
  - Control bar toggle buttons for easy activation/deactivation.

### 6. Time Block Highlighter (`TimeBlockHighlighter.cpp`)
- **Intent**: Visually segments the chart based on time-based sessions.
- **Features**:
  - Highlights multiple sessions (e.g., London, NY Open) with configurable transparency and colors.
  - **Secondary Timezone Labels**: Displays offset timezone ticks (e.g., UTC/GMT) along the axis without cluttering the main price scale.

### 7. Trade Entry Drills (`TradeEntryDrills.cpp`)
- **Intent**: Training tool for execution and entry practice.
- **Features**:
  - Simulates multi-contract entries with randomized slippage/timing to mimic real-world conditions.
  - Session tracking and performance metrics for drill sets.

### 8. Obsidian Exporter (`ObsidianExporter.cpp`)
- **Intent**: Bridges trading analysis with personal knowledge management.
- **Features**:
  - Exports chart data, levels, and notes directly into Obsidian-compatible markdown formats.

### 9. DOM Account Visualizer (`DOMAccountVisualizer.cpp`)
- **Intent**: High-visibility safety overlay for Trading DOMs to prevent SIM/LIVE execution errors.
- **Features**:
  - **Zero-Lag Gating**: REDRAW logic only executes on account or window size changes, ensuring 0% CPU impact during active scalping.
  - **Peripheral Awareness**: Dual 6px pillars and a perimeter border provide 360-degree awareness of the active mode.
  - **Dynamic Detection**: Customizable keywords for SIM/LIVE detection with independent color and label configurations.
- **Detailed Guide**: [DOM Account Visualizer Guide](DOMAccountVisualizer_README.md)

---

## 🛠 Engineering Guidelines

When modifying or expanding these studies, the following principles are strictly enforced:

1.  **Memory & State Persistence**: Use `sc.GetPersistentPointer` and `sc.AllocateMemory` to retain state across updates. Avoid static variables.
2.  **Manual Looping**: Studies default to `sc.AutoLoop = 0` to handle historical accumulation patterns and caching inputs efficiently.
3.  **Calculation Precision**: Map aggregations (Price * Volume) to `double` variables before standard deviation square roots to ensure tick-level accuracy.
4.  **UI Efficiency**: Restrict costly UI checks, string manipulations, or drawing scans to fresh ticks or specific user actions (button clicks).

## 📥 Installation

1.  Place `.cpp` files in your Sierra Chart `/Data` directory.
2.  In Sierra Chart, go to **Analysis >> Build Custom Studies DLL**.
3.  Select the desired study and click **Build and Remote Join**.
4.  Once compiled, add the study to your chart via **Analysis >> Studies**.

## 📜 License

This project is intended for personal and quantitative research use. All rights reserved.
