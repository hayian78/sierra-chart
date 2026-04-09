# Balance Zone Suite: Professional Operational Guide

## 🎯 Suite Overview
The **Balance Zone Suite** is a triad of high-performance Sierra Chart studies designed for Auction Market Theory (AMT) practitioners. It provides a unified framework for identifying, projecting, and maintaining balance-based structural levels across both charts and Trading DOMs.

The suite comprises:
1.  **Balance Zone Engine (BZE)**: The primary projection and lifecycle management engine.
2.  **DOM Balance Zone**: A "zero-footprint" visual overlay for execution-focused Trading DOMs.
3.  **Balance Zone Manager**: A maintenance utility for optimizing historical anchor distributions.

---

## 1. Balance Zone Engine (BZE)
**Purpose**: Projects standard deviation-style multiples (2x, 4x, 8x, 16x, up to 30x) derived from user-defined anchor rectangles.

### 🔄 The Tiered Lifecycle
BZE automatically manages chart clutter by transitioning anchors through three distinct phases based on their chronological age:

| Tier | Name | Visual Experience | Trigger |
| :--- | :--- | :--- | :--- |
| **Tier 1** | **Focus** | Full detail: Midlines, price labels, solid borders, auto-extend. | Newest $N$ anchors |
| **Tier 2** | **Context** | Standard fills: Clean blocks without labels or midlines. | Next $M$ anchors |
| **Tier 3** | **Archive** | Ghosted: 60% transparency boost, dotted borders, no labels. | All older anchors |

### ⌨️ On-Chart Configuration (Text Overrides)
Modify anchor behavior instantly by editing the rectangle's text label:

*   **Multipliers**: `BZ +4x,-2x` (Up 4 groups, Down 2 groups).
*   **Direct Counts**: `BZ +12,-6` (Up 12 units, Down 6 units).
*   **Tier Locking**: `BZ T1` (Force Focus), `BZ T2` (Force Context), `BZ T3` or `BZ LOCK` (Force Archive).
*   **Styling**: `BZ*` (Force "Nice" rendering regardless of Tier).
*   **Descriptions**: `BZ | 4H Balance` (Preserves text after the pipe).

---

## 2. DOM Balance Zone
**Purpose**: Lite, high-speed visualization for the Trading DOM.

### ⚡ Operational Edge
- **Zero-Footprint Design**: Renders only floating text labels (`+2x`, `-4x`, etc.) within the DOM's price column.
- **Visual Heatmap**: Colors automatically fade in intensity as zones move further from the cost basis, providing instant peripheral awareness of price extremes.
- **Performance**: Zero CPU impact during active scalping. Recalculates only on input modification.

---

## 3. Balance Zone Manager (New)
**Purpose**: Maintenance utility to "Shrink-Wrap" historical projections and minimize visual noise.

### 🛠️ The "Shrink-Wrap" Utility
As the market moves, archived anchors often project zones into areas price never visited. The Manager optimizes these automatically.

- **Trigger**: Study Input `Run Shrink-Wrap Now` -> Set to **Yes** (auto-resets to No).
- **Inputs**:
    - **Base Labels**: Comma-separated list (e.g., `BZ,BAL`) to identify anchors.
    - **Minimum Multiplier**: Ensures it doesn't shrink to 0 (default: 0).
    - **Force Update**: By default, the manager excludes anchors with existing explicit multipliers (e.g., `BZ +6x,-2x`) to avoid overwriting manual settings. Set to **Yes** to bypass this safety.
- **Logic**: 
    1. Scans all matching anchors.
    2. Identifies the actual High/Low reached by price *horizontally inside* each anchor's bounds.
    3. Calculates the exact multipliers needed to cover only the realized price action.
    4. Rewrites the label (e.g., `BZ` -> `BZ +5,-2`) while preserving all keywords, tiers, and descriptions.
- **Safety**: Clamps results to the BZE engine limit (30x).

---

## ⚙️ Technical Specifications & Deployment

### Performance Architecture
- **Manual Looping (`sc.AutoLoop = 0`)**: All tools utilize manual loops to ensure O(N) or better performance.
- **Zero-Lag Gating**: `sc.UpdateAlways = 0` ensures studies remain dormant until a user action or fresh bar arrival occurs.
- **Memory Safety**: Uses `sc.GetPersistentPointer` for cross-calculation state persistence.

### Installation
1.  Copy `.cpp` files to your Sierra Chart `/Data` folder.
2.  **Analysis >> Build Custom Studies DLL**.
3.  Add `Balance Zone Engine` to your execution chart.
4.  Add `DOM Balance Zone` to your Trading DOM (requires manual H/L inputs).
5.  Add `Tool: Balance Zone Manager` for periodic maintenance.

---
**Lead Quantitative Architect** | *Quantitative structural analysis for professional futures execution.*
