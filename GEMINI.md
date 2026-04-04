# Sierra Chart Quantitative Studies

## Project Overview

This repository contains custom Sierra Chart ACSIL (Advanced Custom Study Interface and Language) studies focused on quantitative level generation, volume-weighted metrics, balance projection, and cross-chart data aggregation. The primary goal is to provide clear, actionable insights by extending Sierra Chart's native drawing and calculation capabilities. 

## Core Modules & Intent

### 1. Daily AVWAP (`DailyAVWAP.cpp`)
- **Intent**: Computes an anchored Volume Weighted Average Price (VWAP) that resets daily at a configurable time (e.g., 17:00 ET for CME futures) to ensure accurate overnight session handling.
- **Key Features**: 
  - Tracks running volume-weighted variance to precisely generate multiple standard deviation bands.
  - Efficient manual loop calculations with double-precision accumulators.
  - Zero-volume bar gap handling (carries forward previous values without skewing averages).
  - Built-in crossover alerts with per-bar deduplication.

### 2. Balance Zone Engine (`BalanceZoneEngine.cpp`)
- **Intent**: Projects balance and premium/discount zones derived from user-defined anchor rectangles.
- **Key Features**: 
  - Dynamic zone configuration via chart drawing text labels (e.g., `BZ +6x,-2x` overrides up/down multipliers).
  - High-performance design: explicitly filters and processes only on-screen anchors to optimize rendering.
  - Multi-tier formatting ("Nice" vs. "Basic" modes) including midlines, price labels, and distinct color bands based on zone distance.
  
### 3. Level Aggregator (`LevelAggregator.cpp`)
- **Intent**: Consolidates essential structural levels scattered across multiple charts/timeframes into a single on-chart heads-up display.
- **Key Features**:
  - Scans user shapes across specified charts using a strict naming convention (`LABEL|Description`).
  - Sorts and maps levels based on price proximity, automatically highlighting the nearest liquidity/structure above and below the current executing price tick.
  - Cross-chart data retrieval using `sc.GetUserDrawnChartDrawing`.

## Engineering Guidelines & Constraints

When modifying or expanding these studies, adhere to the following principles:

1. **Memory & State Persistence**: 
   - Rely on `sc.GetPersistentPointer` combined with structures for retaining state across updates instead of abusing static variables. Remember to handle proper memory allocation (`sc.AllocateMemory`) and reallocation safely during chart reloads.
2. **Calculation Precision**: 
   - Always map aggregations (such as Price * Volume) into `double` variables before standard deviation square roots. Ensure integer/float cast truncation does not dilute tick-level accuracy.
3. **Execution Efficiency**: 
   - Default to `sc.AutoLoop = 0` (Manual Looping) to securely handle historical accumulation, caching inputs appropriately before the main data loop, and restrict costly UI checks or string manipulations to fresh ticks only or specifically isolated user-actions (like button clicks).
4. **Drawing and Tool Cleanup**: 
   - Always provide correct line numbers and cleanup paths for `sc.UseTool` objects (like text/tables) to avoid memory leaks or visual artifact ghosting.
5. **Code Modification Mandate (CRITICAL ANTI-CORRUPTION RULE)**:
   - **Never rewrite large files (e.g., >500 lines) from memory using `write_file`.** This inevitably leads to LLM output truncation, resulting in deleted parameter lists (`sc.SetDefaults`), missing logic, and broken UI configurations.
   - **Always use `replace`** for targeted, surgical updates.
   - Before modifying a file, read the relevant sections to ensure you are anchoring your `replace` blocks to the exact current codebase, preserving all surrounding logic and inputs.

## Previous Context & Known Areas for Focus
- Ensure adaptive logic aligns dynamically with the "Point in Time" capabilities when testing execution paths.
- Stop-before-target execution and conservative stop bounds are actively factored into momentum/reversal signaling.
- Maintain accurate alignment visually with internal Sierra Chart equivalents (like built-in VWAP) for continuous UX cohesion.

## 🚀 Deployment Protocol (Mandate)

1.  **Granular Local Commits**: Perform a local `git commit` after every logical piece of work or successful modification. This ensures a clean history for rollbacks.
2.  **Push Seldom**: Do NOT `git push` after every request. Pushing to the remote should only happen in batches.
3.  **Explicit Push Trigger**: Only initiate the `git push` workflow when the user explicitly references `@SHIP.md` or issues a direct command to "Ship it" or "Deploy".
4.  **Validation**: Ensure all remote builds pass (if build output was provided) before any commit or push.
