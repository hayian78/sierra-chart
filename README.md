# Sierra Chart Quantitative Studies - Master Index

## Overview
This repository contains a suite of high-performance, custom ACSIL (Advanced Custom Study Interface and Language) studies for Sierra Chart. Designed for active futures traders, these tools focus on **operational clarity, quantitative level generation, volume-weighted metrics, and strict risk mitigation** during live execution.

---

## Core Trading Systems

The following modules represent the primary operational tools in this suite. Each module is optimized for zero-lag execution and includes dedicated documentation.

### 1. [Daily AVWAP](DailyAVWAP_README.md)
**Utility:** Computes an anchored Volume Weighted Average Price (VWAP) that resets dynamically at a configurable daily time (e.g., 17:00 ET).
* **Risk/Safety:** Ensures overnight session volume is accurately handled without skewing regular trading hours (RTH) calculations.
* **Key Features:** High-precision standard deviation bands, gap handling for zero-volume bars, and tick-level accuracy using double-precision accumulators.

### 2. [Balance Zone Engine](BalanceZone_TieredLifecycle_Plan.md)
**Utility:** Projects dynamic balance and premium/discount zones derived from user-drawn anchor rectangles directly on the chart.
* **Operational Edge:** Tiered lifecycle management automatically ghost-archives older zones to reduce chart clutter while maintaining historical context.
* **Key Features:** **Tiered Lifecycle (Focus/Context/Archive)**, Multi-Anchor Labeling, On-the-fly configuration via text labels (e.g., `BZ +6x,-2x`), and O(1) alert boundary detection.

### 3. [Level Aggregator](LevelAggregator_README.md)
**Utility:** Consolidates essential structural levels scattered across multiple charts and timeframes into a single on-chart HUD.
* **Operational Edge:** Maintains focus by pulling liquidity/structure levels directly to the execution chart.
* **Key Features:** Dynamic visualization modes, auto-syncing labels, and strict naming convention scanning (`LABEL|Description`).

### 4. [DOM Account Visualizer](DOMAccountVisualizer_README.md)
**Utility:** Provides a high-visibility, 360-degree safety overlay for Trading DOMs to instantly distinguish between SIM and LIVE execution states.
* **Risk/Safety:** **CRITICAL** for preventing catastrophic order execution errors on the wrong account.
* **Key Features:** Zero-lag gating (0% CPU impact during active scalping), customizable detection keywords, and peripheral awareness pillars.

### 5. [Time Block Highlighter](TimeBlockHighlighter_README.md)
**Utility:** Visually segments the chart based on time-based sessions (e.g., London, NY Open) to aid in macroeconomic timing.
* **Operational Edge:** Keeps the trader oriented within specific liquidity windows.
* **Key Features:** Configurable transparency, multiple session highlights, and secondary timezone labels without axis clutter.

---

## Secondary Utilities

* **Drawing Cleaner (`DrawingCleaner.cpp`):** Selective chart cleanup utility to instantly clear specific drawing types without affecting labeled structural zones.
* **Chart Snapshot Engine (`ChartSnapshotEngine.cpp`):** Automated image capture system with Telegram integration for journaling and external review.
* **Trade Entry Drills (`TradeEntryDrills.cpp`):** Execution training tool simulating slippage and multi-contract entries for practice.
* **Obsidian Exporter (`ObsidianExporter.cpp`):** Bridges live trading analysis with personal knowledge management (PKM) via markdown exports.

---

## Global Installation

1. **Locate Data Folder:** Navigate to your Sierra Chart `/Data` directory.
2. **Transfer Files:** Copy the desired `.cpp` files into this directory.
3. **Compile:** Inside Sierra Chart, open **Analysis >> Build Custom Studies DLL**.
4. **Deploy:** Select the study file, click **Build and Remote Join**.
5. **Apply:** Add the compiled study to your chart or DOM via **Analysis >> Studies**.

---

## Technical & Engineering Standards

* **Memory Management:** State persistence relies strictly on `sc.GetPersistentPointer` and `sc.AllocateMemory`. Static variables are strictly prohibited to ensure stability across chart reloads.
* **Execution Efficiency:** Studies default to `sc.AutoLoop = 0` (Manual Looping) to securely handle historical accumulation. UI checks and drawing scans are restricted to fresh ticks or specific user actions. Administrative tools (like the Balance Zone Manager) utilize `sc.IsFullRecalculation` gating to ensure **zero-lag** performance by remaining dormant during active trading.
* **Calculation Precedence:** Critical execution studies are prioritized, while maintenance utilities are assigned `LOW_PREC_LEVEL` to ensure they do not impact trade management.
* **Calculation Precision:** Volume-weighted metrics use `double` precision prior to standard deviation calculations to prevent truncation errors.
