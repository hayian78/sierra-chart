# Tiered Anchor Lifecycle (Auto-Calculated)

This plan implements the tiered timeline approach to unify the historical "NICE" and "LOCK" mechanics into a cohesive chronological lifecycle. The system dictates anchor prominence based strictly on anchor order, while providing `T1`, `T2`, `T3` text overrides for explicit manual control.

## How the Engine Evaluates Chronology
The engine uses a highly reliable sorting algorithm (previously used locally by the "Nice" feature). It scans every anchor on the chart and sorts them by determining their "Latest" status using their **Right Edge Index** first, and their **Begin Index** second. This ensures chronological ranking is always accurate even if anchors are drawn out of order or moved on the chart.

## Proposed Tier Mechanics

The traditional `NICE` and `LOCK` settings will be replaced by a pure Tier system. Anchors will automatically cascade through the tiers based on two study inputs:
- **Tier 1 Anchors (Latest N)**: Defines the number of anchors that receive the Tier 1 experience.
- **Tier 2 Anchors (Next N)**: Defines the number of anchors that remain in Tier 2 before hitting Tier 3.

### Tier 1 (Focused / Full Experience)
- **Calculated**: The absolute newest `N` anchors (e.g., the last 1 anchor).
- **Visuals**: Full details, midlines, top/bottom text, and solid borders. (Revises the old "NICE" mode).

### Tier 2 (Standard / Clean)
- **Calculated**: The subsequent `M` older anchors (e.g., the next 3 anchors).
- **Visuals**: Clean minimal rectangle fills. Removes the noisy text and midlines, but retains standard fill opacity.

### Tier 3 (Muted / Archived)
- **Calculated**: All remaining historical anchors that age out of Tier 2.
- **Visuals**: Dotted borders, highly transparent/ghosted background fills, and dimmed zone labels. (Automates the old "LOCK" mechanics).

## Explicit Text Overrides

If you ever want an anchor to defy the automatic timeline (for example, keeping an old critical anchor at full detail, or immediately archiving a new anchor), you simply append the tier label to its text block:
- **`BZ T1`**: Permanently locks the anchor into Tier 1 (Full Detail), overriding chronology.
- **`BZ T2`**: Permanently locks the anchor into Tier 2 (Standard).
- **`BZ T3`**: Permanently locks the anchor into Tier 3 (Muted).

If you just type traditional `BZ`, it enters the automatic chronological pipeline. Legacy keywords like `LOCK` and `*` can be mapped to these tiers beneath the hood to ensure none of your existing saved chartbooks break. Specifically, appending `LOCK` (e.g., `BZ LOCK`) forces the anchor into Tier 3 (Archive). Because **Auto-Extend now strictly applies only to Tier 1 (Focus) anchors**, using `LOCK` is the recommended method to temporarily prevent a live anchor from auto-extending and taking over the screen while you are actively resizing it.

## Implementation Final Status (2026-04-06)

The Tiered Lifecycle system is fully implemented in `BalanceZoneEngine.cpp` (v1.0.3).

### Key Technical Achievements
- **Chronological Sorting**: Uses a robust O(N log N) sort by Right Edge, then Begin Index, to assign tiers (Focus/Context/Archive) deterministically.
- **Visual Hierarchy**: 
    - **Tier 1 (Focus)**: Full detail (labels, midlines, solid borders).
    - **Tier 2 (Context)**: Clean fills (no text/midlines, standard opacity).
    - **Tier 3 (Archive)**: "Ghosted" (60% transparency boost, dotted borders).
- **Keyword Mapping**: Supports `T1`, `T2`, `T3`, `FOCUS`, `CONTEXT`, `ARCHIVE`. Legacy `*` maps to Tier 1; `LOCK`/`FIXED` maps to Tier 3.
- **Zone Name Labels**: Refactored to support multiple concurrent labels for Tier 1 and Tier 1 & 2 modes. Labels automatically update position during chart scrolling.
- **Performance**: Optimized with O(1) alert path detection and O(N) assignment passes. Visible-only processing remains fully functional.
- **Backward Compatibility**: Existing study settings for "Nice N" have been repurposed for Tier 1 counts.

### 4. Lifecycle Maintenance: Cleanup & Shrink-Wrap
As anchors age and move into the Archive (Tier 3), they often project zones into price levels that the market never reached. To optimize visual clarity and performance, the **Balance Zone Manager** tool provides a "Shrink-Wrap" utility.

- **Trigger**: Manual button on the control bar ("Shrink-Wrap BZ").
- **Action**: 
    1. Scans all `BZ` anchors on the chart.
    2. Identifies the actual High/Low reached by price from the anchor's start to the current bar.
    3. Calculates the exact number of multipliers needed to cover that range.
    4. Updates the anchor label (e.g., `BZ` -> `BZ +4,-2`) to "shrink-wrap" the projections to realized price action.
- **Safety**: By default, the manager will **exclude** any anchor that already has explicit multipliers defined in its label (e.g., `BZ +6x,-2x`), assuming these are intentional user overrides.
- **Goal**: Minimize "ghost" levels in areas where candles do not go, ensuring the Archive remains a clean record of historical structure.


