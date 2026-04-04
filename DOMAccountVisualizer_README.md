# DOM Account Visualizer Study Guide

## Overview

The **DOM Account Visualizer** is a high-visibility safety overlay designed specifically for Sierra Chart Trading DOMs. Its primary purpose is to prevent costly execution errors by providing immediate, peripheral feedback on whether the active trading account is in **LIVE** or **SIM** mode.

This study is critical for traders running multiple Sierra Chart instances or managing a mix of funded and paper accounts.

---

## 🚀 Performance Architecture ("Zero-Lag")

Execution speed is paramount on the Trading DOM. This study is engineered for institutional-grade efficiency:

-   **State-Change Gating**: The study utilizes a persistent memory cache to monitor the active account name and chart scroll position.
-   **Fast Exit**: If no state change is detected, the function returns immediately at the top of the execution cycle. Redraw logic consumes **0% CPU** for 99% of market ticks.
-   **Inside-the-Gate Detection**: Keyword searching and string parsing only occur when a user physically switches accounts, protecting your frame rate during high-volatility events.
-   **Memory Safety**: Implements explicit cleanup to prevent memory leaks during study removal.

---

## 🎨 Visual Indicators

The study provides three layers of visual confirmation:

1.  **HUD Text Overlay**: A bold text label anchored at the top of the DOM (e.g., `[LIVE - RISK ON]`). It features a solid background for maximum contrast against volume profiles.
2.  **Dual Edge Pillars**: Vertical strips on both the far-left and far-right edges of the DOM window. These provide constant peripheral awareness without obscuring order flow data.
3.  **Perimeter Border**: A solid frame that wraps the entire window in the active mode's color (e.g., Red for Live, Cyan for Sim).

---

## ⚙️ Configuration Settings

### 1. Detection Keywords
-   **Live Account Keywords (CSV)**: Comma-separated list of strings that trigger LIVE mode (e.g., `Live,Funded,Real`).
-   **Sim Account Keywords (CSV)**: Comma-separated list of strings that trigger SIM mode (e.g., `Sim,Paper,SC`).
-   **Detection Fallback**: Determines the default mode if the active account name matches neither keyword list.

### 2. Labels & Colors
-   **Mode Labels**: Fully customizable text for both LIVE and SIM modes.
-   **Mode Colors**: Independent color selection for each mode. Defaults are high-contrast **Red (Live)** and **Electric Cyan (Sim)**.

### 3. Dimensions & Style
-   **Edge Pillar Width**: Adjust the thickness of the vertical side indicators (Bars-based scaling).
-   **Perimeter Border Width**: Thickness of the window frame.
-   **HUD Font Size**: Size of the primary text overlay.
-   **Transparency**: Opacity level (0-100) for all visual elements.

---

## 📥 Installation

1.  Place `DOMAccountVisualizer.cpp` in your Sierra Chart `/ACS_Source` directory.
2.  In Sierra Chart, go to **Analysis >> Build Custom Studies DLL**.
3.  Select `DOMAccountVisualizer.cpp` and click **Build and Remote Join**.
4.  Add the study to your Trading DOM via **Analysis >> Studies**.
