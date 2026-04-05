#include "sierrachart.h"
#include <cstring>
#include <cctype>

/*============================================================================
    DOM Account Visualizer - Sierra Chart Study
    
    Purpose: Provides clear, high-visibility feedback on the active trading 
             account (SIM vs LIVE) directly on the Trading DOM.
    
    Optimized for ES/MES Scalping:
    - Zero-Lag Engine: REDRAW logic is gated by account/scroll state changes.
    - Fast Exit: Function returns immediately if no state change detected.
    - Tactical Awareness: Dual pillars and perimeter border for peripheral vision.
============================================================================*/

SCDLLName("DOM Account Visualizer")

struct GlobalState {
    SCString LastAccount;
    int LastFirstBar;
    int LastLastBar;
    bool IsSim;
    bool Initialized;

    GlobalState() : LastFirstBar(-1), LastLastBar(-1), IsSim(true), Initialized(false) {}
};

// Helper: Case-insensitive search for keywords in CSV string
bool ContainsKeyword(const char* account, const char* keywords) {
    if (!account || !keywords || account[0] == '\0' || keywords[0] == '\0') return false;

    char lowerAccount[256];
    int i = 0;
    for (; account[i] != '\0' && i < 255; ++i) {
        lowerAccount[i] = (char)tolower(account[i]);
    }
    lowerAccount[i] = '\0';

    char lowerKeywords[512];
    int j = 0;
    for (; keywords[j] != '\0' && j < 511; ++j) {
        lowerKeywords[j] = (char)tolower(keywords[j]);
    }
    lowerKeywords[j] = '\0';

    char* start = lowerKeywords;
    while (*start) {
        char* end = strchr(start, ',');
        if (end) *end = '\0';
        while (*start == ' ') start++;
        int len = (int)strlen(start);
        if (len > 0) {
            char* t = start + len - 1;
            while (t >= start && *t == ' ') { *t = '\0'; t--; }
        }
        if (*start != '\0' && strstr(lowerAccount, start)) return true;
        if (end) start = end + 1;
        else break;
    }
    return false;
}

SCSFExport scsf_DOMAccountVisualizer(SCStudyInterfaceRef sc) {
    enum InputIdx {
        IN_LIVE_KEYWORDS = 0, IN_SIM_KEYWORDS, IN_LIVE_LABEL, IN_SIM_LABEL,
        IN_LIVE_COLOR, IN_SIM_COLOR, IN_PILLAR_WIDTH_BARS, IN_BORDER_WIDTH,
        IN_FONT_SIZE, IN_TRANSPARENCY, IN_FALLBACK
    };

    if (sc.SetDefaults) {
        sc.GraphName = "DOM Account Visualizer";
        sc.StudyDescription = 
            "High-visibility safety overlay for Trading DOMs to prevent SIM/LIVE execution errors.\n\n"
            "DETECTION: Automatically checks account name against keywords (CSV).\n"
            "PERFORMANCE: Zero-Lag gating engine ensures 0% CPU impact when idle.\n"
            "VISUALS: Perimeter border, dual edge pillars, and top HUD text.";
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

        sc.Input[IN_PILLAR_WIDTH_BARS].Name = "Edge Pillar Width (Bars)";
        sc.Input[IN_PILLAR_WIDTH_BARS].SetInt(1);

        sc.Input[IN_BORDER_WIDTH].Name = "Perimeter Border Width";
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

    if (sc.LastCallToFunction) {
        GlobalState* p_State = (GlobalState*)sc.GetPersistentPointer(1);
        if (p_State) { 
            delete p_State; 
            sc.SetPersistentPointer(1, NULL); 
        }
        return;
    }

    GlobalState* p_State = (GlobalState*)sc.GetPersistentPointer(1);
    if (!p_State) {
        p_State = new GlobalState;
        sc.SetPersistentPointer(1, p_State);
    }

    SCString CurrentAccount = sc.SelectedTradeAccount;
    int firstBar = sc.IndexOfFirstVisibleBar;
    int lastBar = sc.IndexOfLastVisibleBar;

    bool accountChanged = (CurrentAccount != p_State->LastAccount);
    bool scrollChanged = (firstBar != p_State->LastFirstBar || lastBar != p_State->LastLastBar);
    bool shouldRecalculate = (sc.IsFullRecalculation || !p_State->Initialized);

    if (!accountChanged && !scrollChanged && !shouldRecalculate) return;

    if (accountChanged || shouldRecalculate) {
        p_State->IsSim = (sc.Input[IN_FALLBACK].GetIndex() == 0);
        const char* accStr = CurrentAccount.GetChars();
        if (ContainsKeyword(accStr, sc.Input[IN_SIM_KEYWORDS].GetString())) p_State->IsSim = true;
        else if (ContainsKeyword(accStr, sc.Input[IN_LIVE_KEYWORDS].GetString())) p_State->IsSim = false;
        p_State->LastAccount = CurrentAccount;
    }

    p_State->LastFirstBar = firstBar;
    p_State->LastLastBar = lastBar;
    p_State->Initialized = true;

    COLORREF color = p_State->IsSim ? sc.Input[IN_SIM_COLOR].GetColor() : sc.Input[IN_LIVE_COLOR].GetColor();
    SCString label = p_State->IsSim ? sc.Input[IN_SIM_LABEL].GetString() : sc.Input[IN_LIVE_LABEL].GetString();

    s_UseTool Tool;
    Tool.Clear();
    Tool.ChartNumber = sc.ChartNumber;
    Tool.AddMethod = UTAM_ADD_OR_ADJUST;
    Tool.UseRelativeVerticalValues = 1;
    Tool.TransparencyLevel = sc.Input[IN_TRANSPARENCY].GetInt();
    Tool.Color = color;

    // Borders (Horizontal)
    Tool.LineNumber = 10001; Tool.DrawingType = DRAWING_HORIZONTALLINE; Tool.BeginValue = 100; Tool.LineWidth = sc.Input[IN_BORDER_WIDTH].GetInt(); sc.UseTool(Tool);
    Tool.LineNumber = 10002; Tool.BeginValue = 0; sc.UseTool(Tool);

    // Pillars (Vertical)
    Tool.DrawingType = DRAWING_VERTICALLINE;
    Tool.LineWidth = sc.Input[IN_PILLAR_WIDTH_BARS].GetInt() * 10; // Scaled for visibility on DOM
    Tool.LineNumber = 10003; Tool.BeginIndex = firstBar; sc.UseTool(Tool);
    Tool.LineNumber = 10004; Tool.BeginIndex = lastBar; sc.UseTool(Tool);

    // HUD Text
    Tool.LineNumber = 10005; Tool.DrawingType = DRAWING_TEXT; Tool.BeginIndex = firstBar; Tool.BeginValue = 95;
    Tool.Text.Format("  %s", label.GetChars()); // Added spaces to offset from left pillar
    Tool.FontSize = sc.Input[IN_FONT_SIZE].GetInt(); Tool.FontBold = 1; Tool.TransparentLabelBackground = 0; Tool.FontBackColor = RGB(0, 0, 0);
    Tool.TextAlignment = DT_LEFT | DT_TOP;
    sc.UseTool(Tool);
}
