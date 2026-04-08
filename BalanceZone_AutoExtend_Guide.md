# Balance Zone Engine - User Guide

## Overview
> Projects balance and premium/discount zones derived from user-defined anchor rectangles. It allows on-the-fly dynamic zone configuration via chart drawing text labels to adjust up/down multipliers.

---

## Configuration (Study Inputs)
| Input Name | Type | Recommended Value | Description |
| :--- | :--- | :--- | :--- |
| **Anchor Rectangle Label Text(s)** | String | `BZ` | Comma-separated base labels to match from user-drawn rectangles. |
| **Auto-Extend Latest Anchor Mode** | Dropdown | `Control Bar Button` | Determines if the most recent anchor automatically extends its zones to the right edge of the chart. |
| **Auto-Extend Button ID** | Integer | `1` | Button ID for the Custom Study Control Bar to toggle the Auto-Extend feature. |
| **Enable Up / Down Projections** | Yes/No | `Yes` | Master switch to enable or disable projections in the respective directions. |
| **Max Up / Down Zone Groups** | Integer | `4` | Limits the number of generated zone groups (0 to 4). |

## Operational Use
* **Visual Cues:** Draws detailed balance zones above and below an anchor rectangle based on the anchor's text (e.g., `BZ +2,-4`). Extended zones automatically project to the right edge if auto-extend is enabled (note: this auto-extend behavior exclusively applies to Tier 1 / Focus anchors).
* **Hotkeys:** Use the specified **Auto-Extend Button ID** on the Sierra Chart Custom Study Control Bar to quickly toggle the extension of the latest anchor.
* **Limitations:** Requires Sierra Chart v2813+. Overriding multipliers via text requires exact syntax (e.g., `BZ +6x,-2x`). To prevent a live anchor from auto-extending while you are actively resizing it, append `LOCK` to its text label (e.g., `BZ LOCK`). This manually pushes the anchor to the Archive tier (Tier 3), effectively disabling auto-extension for that specific anchor.

## Developer Notes (ACSIL)
* **DLL Name:** `Balance Zone Engine`
* **Build Requirements:** MSVC 2022+ / x64.
* **Performance:** `sc.AutoLoop = 0` and `sc.UpdateAlways = 1`. High-performance design explicitly filters and processes only on-screen anchors to optimize rendering. State persistence relies on `sc.GetPersistentPointer`.
