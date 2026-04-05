# Time Block Highlighter - User Guide

## Overview
> Renders professional-grade session highlights (banners) at the top or bottom of your chart to immediately identify key market periods (e.g., RTH or Globex). It dynamically projects upcoming sessions and time markers to visualize the trading day's timeline.

---

## Configuration (Study Inputs)
| Input Name | Type | Recommended Value | Description |
| :--- | :--- | :--- | :--- |
| **Enable Session A/B** | Yes/No | `Yes` | Toggles the respective session banner on/off. |
| **Session Start/End Time** | Time | `09:30:00 / 16:00:00` | The time window for the highlight. Supports overnight sessions. |
| **Tick Interval Minutes** | Integer | `0` | TZ Tick Label Interval (0 = auto-scale dynamic tick labels based on zoom). |
| **Time Markers List** | String | `09:30\|Open\|Orange` | Comma-separated list for vertical time markers (`HH:MM\|Label\|Color`). |
| **Extend Active Block** | Yes/No | `Yes` | Projects the current session banner forward into blank space. |

## Operational Use
* **Visual Cues:** Filled colored banners indicating trading sessions. Time ticks projecting local time intervals. **New Update:** TZ tick labels dynamically scale their interval based on your chart's zoom level, preventing overlap.
* **Hotkeys:** None natively defined.
* **Limitations:** None.

## Developer Notes (ACSIL)
* **DLL Name:** `Time Block Highlighter`
* **Build Requirements:** MSVC 2022+ / x64.
* **Performance:** `sc.AutoLoop = 0`, `sc.UpdateAlways = 1`. Employs O(Sessions) Jump Logic for instantaneous historical scanning. Features "Lazy Sticky Edge" labels with 30-bar quantized anchoring to eliminate micro-stuttering during chart scrolling.
