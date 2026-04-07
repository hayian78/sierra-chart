#include "sierrachart.h"

SCDLLName("DOM Balance Zone")

// ============================================================================
// DOM Balance Zone (Lite version for Trading DOM)
// 
// Projects balance zone edges purely for the DOM ladder.
// Dormant execution (AutoLoop = 0) - only recalculates when High/Low inputs change.
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
        sc.GraphName   = "DOM Balance Zone";
        sc.GraphRegion = 0;
        sc.AutoLoop    = 0; // Dormant execution
        
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

    double currentUpper   = InUpperLimit.GetFloat();
    double currentLower   = InLowerLimit.GetFloat();
    int currentUpColor    = InUpColor.GetColor();
    int currentDownColor  = InDownColor.GetColor();
    int currentBaseColor  = InBaseColor.GetColor();
    int currentEnableUp   = InEnableUp.GetYesNo();
    int currentEnableDown = InEnableDown.GetYesNo();

    bool changed = false;
    if (P_LastUpper != currentUpper || P_LastLower != currentLower ||
        P_LastUpColor != currentUpColor || P_LastDownColor != currentDownColor ||
        P_LastBaseColor != currentBaseColor ||
        P_LastEnableUp != currentEnableUp || P_LastEnableDown != currentEnableDown)
    {
        changed = true;
        P_LastUpper   = currentUpper;
        P_LastLower   = currentLower;
        P_LastUpColor = currentUpColor;
        P_LastDownColor = currentDownColor;
        P_LastBaseColor = currentBaseColor;
        P_LastEnableUp  = currentEnableUp;
        P_LastEnableDown = currentEnableDown;
    }

    const int BASE_LINE_ID = 88000;

    auto DeleteAllLines = [&]() {
        for (int i = 0; i < 15; i++) {
            sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, BASE_LINE_ID + i);
        }
    };

    // Cleanup when study is removed or chart is closed
    if (sc.LastCallToFunction)
    {
        DeleteAllLines();
        return;
    }

    // Dormant execution: only redraw if inputs changed or full recalc forced
    if (!changed && !sc.IsFullRecalculation)
        return; 

    // If limits are 0 or invalid, remove everything and stay dormant
    if (currentUpper <= 0.0 || currentLower <= 0.0 || currentUpper <= currentLower)
    {
        DeleteAllLines();
        return;
    }

    double range = currentUpper - currentLower;

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

    // Fades the color intensity for higher multipliers
    auto FadeColor = [&](COLORREF baseColor, int step, int maxSteps) -> COLORREF {
        int r = GetRValue(baseColor);
        int g = GetGValue(baseColor);
        int b = GetBValue(baseColor);
        
        // Decrease intensity by scaling RGB values down towards 0 (black/dark)
        // Step 0 = 100% intensity. Max step = ~30% intensity.
        float factor = 1.0f - ((float)step / (float)maxSteps) * 0.70f; 
        
        r = (int)(r * factor);
        g = (int)(g * factor);
        b = (int)(b * factor);
        
        return RGB(r, g, b);
    };

    // 1. Draw Cost Basis (1x)
    DrawLine(0, currentUpper, currentBaseColor, "CB High");
    DrawLine(1, currentLower, currentBaseColor, "CB Low");

    // Exact multipliers requested
    int multipliers[] = {2, 4, 8, 16};
    int numMultipliers = 4;

    // 2. Draw Up Zones (Premium)
    if (currentEnableUp) {
        for (int i = 0; i < numMultipliers; i++) {
            double price = currentUpper + (range * multipliers[i]);
            COLORREF c = FadeColor(currentUpColor, i, numMultipliers);
            
            SCString text;
            text.Format("+%dx", multipliers[i]);
            DrawLine(2 + i, price, c, text);
        }
    } else {
        for (int i = 0; i < numMultipliers; i++) {
            sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, BASE_LINE_ID + 2 + i);
        }
    }

    // 3. Draw Down Zones (Discount)
    if (currentEnableDown) {
        for (int i = 0; i < numMultipliers; i++) {
            double price = currentLower - (range * multipliers[i]);
            COLORREF c = FadeColor(currentDownColor, i, numMultipliers);
            
            SCString text;
            text.Format("-%dx", multipliers[i]);
            DrawLine(6 + i, price, c, text);
        }
    } else {
        for (int i = 0; i < numMultipliers; i++) {
            sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, BASE_LINE_ID + 6 + i);
        }
    }
}
