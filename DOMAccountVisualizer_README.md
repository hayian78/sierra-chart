# DOM Account Visualizer - User Guide

## Overview
> Prevents trading on the wrong account by providing high-visibility color overlays on the Trading DOM. It actively monitors the active account name and creates peripheral visual cues indicating whether you are in SIM or LIVE mode.

---

## Configuration (Study Inputs)
| Input Name | Type | Recommended Value | Description |
| :--- | :--- | :--- | :--- |
| **Live Account Keywords** | String | `Live,Real,Prop` | Comma-separated keywords that trigger LIVE mode. |
| **Sim Account Keywords** | String | `Sim,Paper,SC` | Comma-separated keywords that trigger SIM mode. |
| **Live HUD Label** | String | `[LIVE - RISK ON]` | Text displayed at the top of the DOM when in Live mode. |
| **Sim HUD Label** | String | `[SIM - PAPER]` | Text displayed at the top of the DOM when in Sim mode. |
| **Live/Sim Mode Color** | Color | `Red / Cyan` | Color for the perimeter border and edge pillars. |
| **Detection Fallback** | Dropdown | `Default to SIM` | The default mode if the active account name matches no keywords. |

## Operational Use
* **Visual Cues:** The DOM window features a HUD Text overlay at the top, vertical edge pillars on the left and right, and a perimeter border wrapping the window in the active mode's color.
* **Hotkeys:** None natively defined.
* **Limitations:** Operates strictly based on keyword matching against the active account string in Sierra Chart.

## Developer Notes (ACSIL)
* **DLL Name:** `DOM Account Visualizer`
* **Build Requirements:** MSVC 2022+ / x64.
* **Performance:** `sc.AutoLoop = 0`, `sc.UpdateAlways = 1`. Features a zero-lag gating engine; string parsing only occurs when a user physically switches accounts or scrolls, ensuring 0% CPU impact when idle.
