#include "sierrachart.h"

SCDLLName("DOM Balance Zone")

// ============================================================================
// DOM Balance Zone (Lite version for Trading DOM)
//
// Projects balance zone edges purely for the DOM ladder.
// Dormant execution (AutoLoop = 0) - only recalculates when inputs change.
//
// PERFORMANCE: the early-return at the "dormant gate" (after change detection)
// is load-bearing. The Trading DOM redraws under heavy quote pressure, so any
// new work added above that gate runs on every tick. Keep heavy work below it.
//
// Each line sits at the OUTER boundary of an engine tier from BalanceZoneEngine.cpp
// (MapXToMaxMultiplier): tier "2x" -> m=2, tier "4x" -> m=6, tier "8x" -> m=14,
// tier "16x" -> m=30. The label is the tier's "+Nx" user-input name (2/4/8/16); the
// position is that tier's max m times the anchor height h. h equals the full
// cost-basis range so the DOM lines land where the engine draws its tier boundaries.
// ============================================================================

SCSFExport scsf_DOMBalanceZone(SCStudyInterfaceRef sc)
{
    // Inputs
    SCInputRef InUpperLimit = sc.Input[0];
    SCInputRef InLowerLimit = sc.Input[1];
    SCInputRef InUpColor    = sc.Input[2];
    SCInputRef InDownColor  = sc.Input[3];
    SCInputRef InBaseColor  = sc.Input[4];
    SCInputRef InEnableUp   = sc.Input[7];
    SCInputRef InEnableDown = sc.Input[8];

    if (sc.SetDefaults)
    {
        sc.GraphName    = "DOM Balance Zone";
        sc.GraphRegion  = 0;
        sc.AutoLoop     = 0; // Dormant execution
        sc.UpdateAlways = 0; // Explicit: never run on every tick
        
        InUpperLimit.Name = "Cost Basis Upper Limit (0 to disable)";
        InUpperLimit.SetFloat(0.0f);
        
        InLowerLimit.Name = "Cost Basis Lower Limit (0 to disable)";
        InLowerLimit.SetFloat(0.0f);
        
        InBaseColor.Name = "Cost Basis Color (1x)";
        InBaseColor.SetColor(RGB(128, 128, 128)); // Neutral grey default
        
        InUpColor.Name = "Up Color Base (Premium)";
        InUpColor.SetColor(RGB(0, 255, 0)); // Green default
        
        InDownColor.Name = "Down Color Base (Discount)";
        InDownColor.SetColor(RGB(255, 0, 0)); // Red default
        
        InEnableUp.Name = "Enable Up Zones";
        InEnableUp.SetYesNo(1);
        
        InEnableDown.Name = "Enable Down Zones";
        InEnableDown.SetYesNo(1);
        
        return;
    }

    // Persistent variables to track state changes
    double& P_LastUpper   = sc.GetPersistentDouble(0);
    double& P_LastLower   = sc.GetPersistentDouble(1);
    int& P_LastUpColor    = sc.GetPersistentInt(2);
    int& P_LastDownColor  = sc.GetPersistentInt(3);
    int& P_LastBaseColor  = sc.GetPersistentInt(4);
    int& P_LastEnableUp   = sc.GetPersistentInt(7);
    int& P_LastEnableDown = sc.GetPersistentInt(8);

    const double currentUpper   = InUpperLimit.GetFloat();
    const double currentLower   = InLowerLimit.GetFloat();
    const int    currentUpColor    = InUpColor.GetColor();
    const int    currentDownColor  = InDownColor.GetColor();
    const int    currentBaseColor  = InBaseColor.GetColor();
    const int    currentEnableUp   = InEnableUp.GetYesNo();
    const int    currentEnableDown = InEnableDown.GetYesNo();

    // Capture previous enable state before the change-detection block updates
    // the persistents — used below to delete only on enabled->disabled transitions.
    const int prevEnableUp   = P_LastEnableUp;
    const int prevEnableDown = P_LastEnableDown;

    bool changed = false;
    if (P_LastUpper != currentUpper || P_LastLower != currentLower ||
        P_LastUpColor != currentUpColor || P_LastDownColor != currentDownColor ||
        P_LastBaseColor != currentBaseColor ||
        P_LastEnableUp != currentEnableUp || P_LastEnableDown != currentEnableDown)
    {
        changed = true;
        P_LastUpper      = currentUpper;
        P_LastLower      = currentLower;
        P_LastUpColor    = currentUpColor;
        P_LastDownColor  = currentDownColor;
        P_LastBaseColor  = currentBaseColor;
        P_LastEnableUp   = currentEnableUp;
        P_LastEnableDown = currentEnableDown;
    }

    const int BASE_LINE_ID = 88000;
    const int TOTAL_LINE_COUNT = 10; // 2 cost-basis + 4 up zones + 4 down zones

    // Cleanup when study is removed or chart is closed (must run regardless of dormant gate).
    if (sc.LastCallToFunction)
    {
        for (int i = 0; i < TOTAL_LINE_COUNT; i++)
            sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, BASE_LINE_ID + i);
        return;
    }

    // Dormant execution: only redraw if inputs changed or full recalc forced.
    // Everything below this gate is skipped on the no-change path.
    if (!changed && !sc.IsFullRecalculation)
        return;

    auto DeleteAllLines = [&]() {
        for (int i = 0; i < TOTAL_LINE_COUNT; i++)
            sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, BASE_LINE_ID + i);
    };

    // If limits are 0 or invalid, remove everything and stay dormant
    if (currentUpper <= 0.0 || currentLower <= 0.0 || currentUpper <= currentLower)
    {
        DeleteAllLines();
        return;
    }

    // Anchor unit-height = full cost-basis range, matching BalanceZoneEngine's
    // snappedTop - snappedBot. Each tier line is then at h * (tier's max m).
    const double h = currentUpper - currentLower;

    auto DrawLine = [&](int idOffset, double price, COLORREF color, const char* text) {
        s_UseTool t;
        t.Clear();
        t.ChartNumber   = sc.ChartNumber;
        t.DrawingType   = DRAWING_HORIZONTALLINE;
        t.LineNumber    = BASE_LINE_ID + idOffset;
        t.BeginValue    = price;
        t.EndValue      = price;
        t.Color         = color;
        t.LineWidth     = 0; // Hides the line
        t.AddMethod     = UTAM_ADD_OR_ADJUST;
        t.Text          = text;
        t.ShowPrice     = 1;
        t.TransparentLabelBackground = 1;
        // DT_RIGHT | DT_VCENTER aligns the text to the right and vertically centers it in the price box
        t.TextAlignment = DT_RIGHT | DT_VCENTER;
        sc.UseTool(t);
    };

    // Fades the color intensity for higher multipliers.
    // Step 0 = 100% intensity. Max step = ~30% intensity.
    auto FadeColor = [](COLORREF baseColor, int step, int maxSteps) -> COLORREF {
        const float factor = 1.0f - ((float)step / (float)maxSteps) * 0.70f;
        const int r = (int)(GetRValue(baseColor) * factor);
        const int g = (int)(GetGValue(baseColor) * factor);
        const int b = (int)(GetBValue(baseColor) * factor);
        return RGB(r, g, b);
    };

    // Outer m-boundary of each engine tier (G1 max=2, G2 max=6, G3 max=14, G4 max=30).
    // See BalanceZoneEngine.cpp:222 (MapXToMaxMultiplier). Labels are the tier's
    // "+Nx" user-input names (2/4/8/16), not the m-value itself.
    static const int multipliers[]        = {2, 6, 14, 30};
    static const int numMultipliers       = 4;
    static const char* const upLabels[]   = {"+2x", "+4x", "+8x", "+16x"};
    static const char* const downLabels[] = {"-2x", "-4x", "-8x", "-16x"};

    // Precompute faded colors once per redraw instead of per zone.
    COLORREF upColors[numMultipliers];
    COLORREF downColors[numMultipliers];
    for (int i = 0; i < numMultipliers; i++) {
        upColors[i]   = FadeColor(currentUpColor,   i, numMultipliers);
        downColors[i] = FadeColor(currentDownColor, i, numMultipliers);
    }

    // 1. Draw Cost Basis (1x)
    DrawLine(0, currentUpper, currentBaseColor, "CB High");
    DrawLine(1, currentLower, currentBaseColor, "CB Low");

    // 2. Up Zones (Premium)
    if (currentEnableUp) {
        for (int i = 0; i < numMultipliers; i++) {
            const double price = currentUpper + (h * multipliers[i]);
            DrawLine(2 + i, price, upColors[i], upLabels[i]);
        }
    } else if (prevEnableUp || sc.IsFullRecalculation) {
        // Only delete on enabled->disabled transition, or on full recalc (chart reload safety).
        for (int i = 0; i < numMultipliers; i++) {
            sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, BASE_LINE_ID + 2 + i);
        }
    }

    // 3. Down Zones (Discount)
    if (currentEnableDown) {
        for (int i = 0; i < numMultipliers; i++) {
            const double price = currentLower - (h * multipliers[i]);
            DrawLine(6 + i, price, downColors[i], downLabels[i]);
        }
    } else if (prevEnableDown || sc.IsFullRecalculation) {
        for (int i = 0; i < numMultipliers; i++) {
            sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, BASE_LINE_ID + 6 + i);
        }
    }
}
