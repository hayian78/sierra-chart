# Time Block Highlighter — User Guide

**Version:** 2.1.0  
**Platform:** Sierra Chart (ACSIL Custom Study)  
**DLL Name:** `TimeBlockHighlighter`  
**Study Name:** Time Block Highlighter  

---

## Overview

The **Time Block Highlighter** is a high-performance visual aid for Sierra Chart that provides clear, professional-grade session highlights (banners) at the top or bottom of your chart. It is designed to help traders immediately identify key market periods (like RTH or Globex) and visualize upcoming sessions before they begin.

### Key Features (v2.1.0)

- **Professional Banners** — Renders clean, labeled rectangles that act as headers for your trading sessions.
- **Forward Projection** — Active sessions dynamically extend into the future blank space, ending precisely at their scheduled close.
- **Upcoming Previews** — Pre-draws the next session (e.g., "See RTH coming up in 60 minutes") into the right-side blank space.
- **Lazy Sticky Edge Labels** — **NEW:** Optimized label logic ensures session titles stay visible while you scroll, using a 30-bar quantized snap to eliminate micro-stuttering.
- **Future Time Ticks** — Projects your local time intervals into the future banner space for a complete timeline view.
- **Time Markers** — Add vertical lines with custom labels at specific times (e.g., "9:30|Open").

---

## Installation

1. Place `TimeBlockHighlighter.cpp` into your Sierra Chart **ACS_Source** folder.
2. In Sierra Chart, go to **Analysis → Build Custom Studies DLL** to compile.
3. Add the study to your chart via **Analysis → Studies → Add Custom Study → Time Block Highlighter**.

---

## Settings

### Session Configuration (A & B)
| Input | Default | Description |
|---|---|---|
| **Enable Session** | `Yes` | Toggle the session on/off. |
| **Start/End Time** | Varies | The time window for the highlight. Supports overnight sessions (e.g., 18:00 to 09:30). |
| **Label Text** | `RTH` / `Globex` | The title displayed in the banner. |

### Projections
| Input | Default | Description |
|---|---|---|
| **Extend Active Block** | `Yes` | Projects the current session banner into the future blank space. |
| **Preview Upcoming Block** | `Yes` | Draws the *next* session on the chart before it officially starts. |
| **Preview Minutes** | `60` | How many minutes before a session starts that the preview should appear. |

### Styling
| Input | Default | Description |
|---|---|---|
| **Location** | `From Top` | Position the banners at the top, bottom, or a custom vertical offset. |
| **Height (%)** | `4.0` | Vertical thickness of the banner as a percentage of the chart region. |
| **Transparency** | `60` | Opacity of the fill (0 = opaque, 100 = invisible). |
| **Text Alignment** | `Center` | Horizontal placement of the session title (Left, Center, Right). |

---

## Institutional Performance ("Zero-Lag" Engine)

This study is engineered specifically for ES/MES scalpers who require zero interference with their charting frame rate:

-   **O(Sessions) Jump Logic**: Replaced linear bar-by-bar scanning with a time-based jump strategy. The study "jumps" directly to session boundaries, making historical processing instantaneous even on charts with 500+ days of data.
-   **State-Change Gating**: A high-performance hash check ensures the study exits immediately at the top of the function if no settings or scroll positions have changed. Redraws consume **0% CPU** during idle market periods.
-   **Lazy Redraws**: By snap-anchoring labels every 30 bars, the study avoids redundant UI calls on every single pixel of scroll, preserving your GPU resources for order flow data.
-   **Daily Caching**: Day-of-week and date calculations are performed once per day instead of once per bar, minimizing expensive `SCDateTime` method calls.

---

## Changelog

### v2.1.0 — 2026-04-04
- **Performance Overhaul**: Implemented $O(Sessions)$ Jump Logic for instantaneous historical scanning.
- **Micro-Stutter Fix**: Introduced "Lazy Sticky Edge" labels with 30-bar quantized anchoring.
- **Zero-Lag Gating**: Added hash-based state gating to eliminate CPU load when idle.
- **Daily Caching**: Optimized `SCDateTime` handling within the core execution loops.

### v2.0.0 — 2026-04-02
- **Major UX Update**: Added Forward Projections for active sessions.
- **Upcoming Preview**: Added the ability to see the next session block before it starts.
- **Floating Labels**: Labels now stay visible on screen instead of scrolling off.
- **Future Ticks**: Time ticks now project into the future blank space.
- **Time Markers**: Added support for vertical time-based lines with custom labels.

### v1.0.0 — 2026-02-15
- Initial release with dual session highlighting and banner styling.
