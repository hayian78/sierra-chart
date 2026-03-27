# Balance Zone Projector: Auto-Extend Feature Guide

## Overview
The **Auto-Extend** feature allows the *most recent* Balance Zone anchor to automatically extend its projected zones (and the anchor zone itself) to the right edge of the chart (current bar + 5 bars). This eliminates the need to manually drag the right edge of your anchor rectangle as new bars form, ensuring your levels are always visible in real-time.

## Key Features

### 1. Automatic Extension
*   **Target:** Only the **latest** anchor (the one furthest to the right) is extended.
*   **Behavior:** The study detects the latest anchor and artificially projects its zones forward to the current time, regardless of where the anchor's actual rectangle ends.
*   **Visuals:** 
    *   The **Projected Zones** (x2, x4, etc.) extend to the far right.
    *   The **Anchor Zone** (Cost Basis) itself is visually extended to the far right.
    *   The **Midline** (if enabled on the anchor) is also extended to the far right.

### 2. Seamless Integration
*   **Native Midline Support:** If your anchor rectangle has the generic "Midline" option enabled in Sierra Chart (Study Settings -> "Draw Midline" or via the drawing tool config), the Auto-Extend feature detects this and draws a matching midline through the extended area.
*   **Updates:** As new bars arrive, the extension updates automatically without user intervention.

---

## Configuration & Usage

### Enabling the Feature
In the **Study Settings** for the Balance Zone Engine, look for the following inputs:

#### `Auto-Extend Mode`
1.  **Disabled:** Standard behavior. Zones stop exactly where you draw the anchor rectangle.
2.  **Always Enabled:** The latest anchor is *always* auto-extended.
3.  **Control Bar Button:** The feature is toggled on/off via a button on the Custom Study Control Bar.

#### `Auto-Extend Button ID`
*   **Description:** Specifies which button on the **Custom Study Control Bar** will toggle the feature.
*   **Value:** Enter a number (e.g., `1` for Button 1, `5` for Button 5).
*   **Note:** You must enable the Custom Study Control Bar in Sierra Chart (`Window` -> `Control Bars` -> `Custom Study Control Bar`) to see and use this button.

### Using the Control Bar Button
If you select **Control Bar Button** mode:
*   **Click the button** to toggle Auto-Extend ON or OFF.
*   **Button Text:** The button label will update dynamically to show the current state:
    *   `BZ Ext: ON`
    *   `BZ Ext: OFF`

---

## Frequently Asked Questions

**Q: Why isn't my anchor extending?**
*   **A:** Ensure it is the *latest* anchor on the chart (furthest to the right). Older anchors are not extended to keep the chart clean.

**Q: My anchor has a midline, but the extension doesn't.**
*   **A:** Ensure the "Draw Midline" option is enabled on the **Anchor Rectangle** itself (Right-click anchor -> Settings -> "Draw Midline" = Yes). The study mirrors the anchor's own settings.

**Q: I see "BZ Ext: Disabled" on the button.**
*   **A:** This means the `Auto-Extend Mode` input is set to "Disabled" or "Always Enabled". Change it to "Control Bar Button" to enable toggling.
