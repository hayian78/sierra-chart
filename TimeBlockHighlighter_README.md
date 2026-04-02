# Time Block Highlighter — User Guide

**Version:** 2.0.0  
**Platform:** Sierra Chart (ACSIL Custom Study)  
**DLL Name:** `TimeBlockHighlighter`  
**Study Name:** Time Block Highlighter  

---

## Overview

The **Time Block Highlighter** is a high-performance visual aid for Sierra Chart that provides clear, professional-grade session highlights (banners) at the top or bottom of your chart. It is designed to help traders immediately identify key market periods (like RTH or Globex) and visualize upcoming sessions before they begin.

### Key Features (v2.0.0)

- **Professional Banners** — Renders clean, labeled rectangles that act as headers for your trading sessions.
- **Forward Projection** — Active sessions dynamically extend into the future blank space, ending precisely at their scheduled close.
- **Upcoming Previews** — Pre-draws the next session (e.g., "See RTH coming up in 60 minutes") into the right-side blank space.
- **Floating Labels** — Smart label logic ensures session titles (e.g., "Globex") stay visible on your screen as you scroll or as new bars arrive.
- **Future Time Ticks** — Projects your local time intervals into the future banner space for a complete timeline view.
- **Time Markers** — Add vertical lines with custom labels at specific times (e.g., "9:30|Open").

---

## Installation

1. Place `TimeBlockHighlighter.cpp` into your Sierra Chart **Data** folder.
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

## Professional UX Features

### Floating Labels
In standard studies, session labels often disappear if the midpoint of the session is off-screen. This study uses **Floating Label Logic**: the title will "slide" along the banner to stay centered within your *visible* chart range, ensuring you always know which session you are looking at.

### Time Markers
You can define a list of specific times to mark with vertical lines and labels using the following format:  
`HH:MM|Label|Color` (e.g., `09:30|Open|Yellow, 10:00|IB High|Cyan`).

### Future Time Ticks
When enabled, the study will calculate and draw local time labels (e.g., "19:00", "20:00") directly inside the banner, even in the future projections where no bars yet exist.

---

## Performance Notes

- **Stable Rendering Mode** — Recommended for most users; ensures drawings stay perfectly aligned during fast price action.
- **Incremental Update Mode** — Optimized to process only new bars as they arrive, significantly reducing CPU load on high-volume tick charts.
- **Hash Caching** — The study only redraws when settings or the visible range change, preventing redundant GPU calls.

---

## Changelog

### v2.0.0 — 2026-04-02
- **Major UX Update**: Added Forward Projections for active sessions.
- **Upcoming Preview**: Added the ability to see the next session block before it starts.
- **Floating Labels**: Labels now stay visible on screen instead of scrolling off.
- **Future Ticks**: Time ticks now project into the future blank space.
- **Time Markers**: Added support for vertical time-based lines with custom labels.

### v1.0.0 — 2026-02-15
- Initial release with dual session highlighting and banner styling.
