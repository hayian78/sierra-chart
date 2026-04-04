#include "sierrachart.h"
#include <string>
#include <vector>
#include <sstream>

/*============================================================================
    DOM Account Visualizer - Sierra Chart Study
    
    Purpose: Provides clear, high-visibility feedback on the active trading 
             account (SIM vs LIVE) directly on the Trading DOM.
    
    Optimized for ES/MES Scalping:
    - Zero-Lag Engine: REDRAW logic is gated by account/window state changes.
    - Screen-Space Layout: UI elements are pinned to window edges (0% drift).
    - Tactical Awareness: Dual pillars and perimeter border for peripheral vision.
============================================================================*/

SCDLLName("DOM Account Visualizer")

struct GlobalState {
    SCString LastAccount;
    int LastWidth = 0;
    int LastHeight = 0;
    bool LastIsSim = true;
    bool Initialized = false;
};

// Helper: Case-insensitive search for keywords in CSV string
bool ContainsKeyword(const SCString& account, const SCString& keywords) {
    if (account.IsEmpty() || keywords.IsEmpty()) return false;
    
    std::string acctStr = account.GetChars();
    for (auto & c: acctStr) c = (char)tolower(c);

    std::stringstream ss(keywords.GetChars());
    std::string segment;
    while (std::getline(ss, segment, ',')) {
        // Trim whitespace from segment
        segment.erase(0, segment.find_first_not_of(" "));
        segment.erase(segment.find_last_not_of(" ") + 1);
        if (segment.empty()) continue;

        for (auto & c: segment) c = (char)tolower(c);
        if (acctStr.find(segment) != std::string::npos) return true;
    }
    return false;
}

SCSFExport scsf_DOMAccountVisualizer(SCStudyInterfaceRef sc) {
    enum InputIdx {
        IN_LIVE_KEYWORDS = 0,
        IN_SIM_KEYWORDS,
        IN_LIVE_LABEL,
        IN_SIM_LABEL,
        IN_LIVE_COLOR,
        IN_SIM_COLOR,
        IN_PILLAR_WIDTH,
        IN_BORDER_WIDTH,
        IN_FONT_SIZE,
        IN_TRANSPARENCY,
        IN_FALLBACK
    };

    if (sc.SetDefaults) {
        sc.GraphName = "DOM Account Visualizer";
        sc.AutoLoop = 0;
        sc.UpdateAlways = 1;
        sc.GraphRegion = 0;

        sc.Input[IN_LIVE_KEYWORDS].Name = "Live Account Keywords (CSV)";
        sc.Input[IN_LIVE_KEYWORDS].SetString("Live,Real,Prop");

        sc.Input[IN_SIM_KEYWORDS].Name = "Sim Account Keywords (CSV)";
        sc.Input[IN_SIM_KEYWORDS].SetString("Sim,Paper,SC");

        sc.Input[IN_LIVE_LABEL].Name = "Live HUD Label";
        sc.Input[IN_LIVE_LABEL].SetString("[LIVE - RISK ON]");

        sc.Input[IN_SIM_LABEL].Name = "Sim HUD Label";
        sc.Input[IN_SIM_LABEL].SetString("[SIM - PAPER]");

        sc.Input[IN_LIVE_COLOR].Name = "Live Mode Color";
        sc.Input[IN_LIVE_COLOR].SetColor(RGB(255, 0, 0));

        sc.Input[IN_SIM_COLOR].Name = "Sim Mode Color";
        sc.Input[IN_SIM_COLOR].SetColor(RGB(0, 255, 255));

        sc.Input[IN_PILLAR_WIDTH].Name = "Edge Pillar Width (Pixels)";
        sc.Input[IN_PILLAR_WIDTH].SetInt(6);

        sc.Input[IN_BORDER_WIDTH].Name = "Perimeter Border Width (Pixels)";
        sc.Input[IN_BORDER_WIDTH].SetInt(2);

        sc.Input[IN_FONT_SIZE].Name = "HUD Font Size";
        sc.Input[IN_FONT_SIZE].SetInt(12);

        sc.Input[IN_TRANSPARENCY].Name = "HUD/Pillar Transparency (0-100)";
        sc.Input[IN_TRANSPARENCY].SetInt(50);

        sc.Input[IN_FALLBACK].Name = "Detection Fallback";
        sc.Input[IN_FALLBACK].SetCustomInputStrings("Default to SIM;Default to LIVE");
        sc.Input[IN_FALLBACK].SetCustomInputIndex(0);

        return;
    }

    GlobalState* p_State = (GlobalState*)sc.GetPersistentPointer(1);
    if (!p_State) {
        p_State = new GlobalState;
        sc.SetPersistentPointer(1, p_State);
    }

    // --- DETECTION LOGIC ---
    SCString CurrentAccount = sc.SelectedTradeAccount;
    bool IsSim = (sc.Input[IN_FALLBACK].GetIndex() == 0); // Default based on fallback

    if (ContainsKeyword(CurrentAccount, sc.Input[IN_SIM_KEYWORDS].GetString())) {
        IsSim = true;
    } else if (ContainsKeyword(CurrentAccount, sc.Input[IN_LIVE_KEYWORDS].GetString())) {
        IsSim = false;
    }

    // --- GATING ENGINE (Zero-Lag) ---
    bool accountChanged = (CurrentAccount != p_State->LastAccount);
    bool windowResized = (sc.ChartWidth != p_State->LastWidth || sc.ChartHeight != p_State->LastHeight);
    bool modeChanged = (IsSim != p_State->LastIsSim);

    if (!accountChanged && !windowResized && !modeChanged && p_State->Initialized && !sc.IsFullRecalculation) {
        return; // Fast exit - CPU stays at 0%
    }

    // Update state cache
    p_State->LastAccount = CurrentAccount;
    p_State->LastWidth = sc.ChartWidth;
    p_State->LastHeight = sc.ChartHeight;
    p_State->LastIsSim = IsSim;
    p_State->Initialized = true;

    // --- VISUALS ---
    COLORREF color = IsSim ? sc.Input[IN_SIM_COLOR].GetColor() : sc.Input[IN_LIVE_COLOR].GetColor();
    SCString label = IsSim ? sc.Input[IN_SIM_LABEL].GetString() : sc.Input[IN_LIVE_LABEL].GetString();
    int pillarWidth = sc.Input[IN_PILLAR_WIDTH].GetInt();
    int borderWidth = sc.Input[IN_BORDER_WIDTH].GetInt();
    int transparency = sc.Input[IN_TRANSPARENCY].GetInt();

    s_UseTool Tool;
    Tool.Clear();
    Tool.ChartNumber = sc.ChartNumber;
    Tool.AddMethod = UTAM_ADD_OR_ADJUST;
    Tool.UseScreenCoordinates = 1;
    Tool.FillStyle = FILL_SOLID;
    Tool.TransparencyLevel = transparency;

    // 1. Perimeter Border (Using DRAWING_RECTANGLEHIGHLIGHT for better overlay behavior)
    Tool.LineNumber = 10001;
    Tool.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
    Tool.BeginLinePixelX = 0;
    Tool.BeginLinePixelY = 0;
    Tool.EndLinePixelX = sc.ChartWidth;
    Tool.EndLinePixelY = sc.ChartHeight;
    Tool.Color = color;
    Tool.SecondaryColor = color;
    Tool.LineWidth = borderWidth;
    sc.UseTool(Tool);

    // 2. Left Edge Pillar
    Tool.LineNumber = 10002;
    Tool.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
    Tool.BeginLinePixelX = 0;
    Tool.BeginLinePixelY = 0;
    Tool.EndLinePixelX = pillarWidth;
    Tool.EndLinePixelY = sc.ChartHeight;
    Tool.Color = color;
    Tool.SecondaryColor = color;
    Tool.LineWidth = 1; 
    sc.UseTool(Tool);

    // 3. Right Edge Pillar
    Tool.LineNumber = 10003;
    Tool.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
    Tool.BeginLinePixelX = sc.ChartWidth - pillarWidth;
    Tool.BeginLinePixelY = 0;
    Tool.EndLinePixelX = sc.ChartWidth;
    Tool.EndLinePixelY = sc.ChartHeight;
    Tool.Color = color;
    Tool.SecondaryColor = color;
    Tool.LineWidth = 1;
    sc.UseTool(Tool);

    // 4. HUD Text Label
    Tool.LineNumber = 10004;
    Tool.DrawingType = DRAWING_TEXT;
    Tool.BeginLinePixelX = pillarWidth + 10;
    Tool.BeginLinePixelY = 10;
    Tool.Text = label;
    Tool.FontSize = sc.Input[IN_FONT_SIZE].GetInt();
    Tool.FontBold = 1;
    Tool.Color = color;
    Tool.BackgroundFill = 1;
    Tool.TextBackgroundColor = RGB(0, 0, 0); // Black background for text contrast
    Tool.TransparencyLevel = transparency;
    sc.UseTool(Tool);
}
