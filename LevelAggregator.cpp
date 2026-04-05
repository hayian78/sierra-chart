#include "sierrachart.h"
#include <vector>
#include <algorithm>
#include <string>
#include <cfloat>
#include <cctype>

/*============================================================================
    Level Aggregator - Sierra Chart Study
    
    Purpose: Scans user-drawn chart objects across multiple charts and 
             aggregates matching price levels into a consolidated display.
    
    Optimized for ES/MES Scalping: 
    - 1-second heartbeat (Throttled background scanning).
    - Persistent HUD visibility (always see levels above/below).
    - Boundary-based redraws to minimize UI thread load.
============================================================================*/

SCDLLName("Level Aggregator")

struct LevelEntry {
    SCString Label;
    SCString Description;
    double Price;
    int SourceChart;
    SCString ChartName;
    int ChartPriority;
    int StaggerOffset = 0;
};

struct ChartConfig {
    int ChartNumber;
    SCString CustomName;
    int Priority;
};

struct GlobalState {
    std::vector<LevelEntry> Levels;
    bool HasScanned = false;
    SCDateTime LastScanTime;
    SCDateTime LastAutoScanTime;
    bool IsTableVisible = false;
    bool AreLinesVisible = false;
    bool HasInitialized = false;
    int LastHighIdx = -1;
    int LastLowIdx = -1;
    double LastPrice = 0;
    bool ForceRedraw = false;
    int LastFirstVisibleBar = -1;
    bool LastShowLines = false;
    int LastArraySize = 0;
    SCString CachedTableNormal;
    SCString CachedTableHighlight;
};

SCString TrimSCString(const SCString& input) {
    if (input.GetLength() == 0) return input;
    const char* start = input.GetChars();
    const char* end = start + input.GetLength() - 1;
    while (start <= end && isspace((unsigned char)*start)) start++;
    while (end >= start && isspace((unsigned char)*end)) end--;
    if (start > end) return "";
    SCString result;
    result.Format("%.*s", (int)(end - start + 1), start);
    return result;
}

void CalculateHighlights(const std::vector<LevelEntry>& levels, double currentPrice, int& idxHigh, int& idxLow) {
    idxHigh = -1; idxLow = -1;
    double minDistHigh = DBL_MAX; double minDistLow = DBL_MAX;
    for (size_t i = 0; i < levels.size(); ++i) {
        double diff = levels[i].Price - currentPrice;
        if (diff >= 0 && diff < minDistHigh) { minDistHigh = diff; idxHigh = (int)i; }
        else if (diff < 0 && -diff < minDistLow) { minDistLow = -diff; idxLow = (int)i; }
    }
}

void DrawHUD(SCStudyInterfaceRef sc, bool active, const SCString& prefix, int xPos, int yPos, int fontSize, COLORREF color) {
    const int hudLineID = 98765431;
    if (!active) {
        sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, hudLineID);
        return;
    }
    s_UseTool Tool;
    Tool.Clear();
    Tool.ChartNumber = sc.ChartNumber;
    Tool.LineNumber = hudLineID;
    Tool.DrawingType = DRAWING_TEXT;
    Tool.UseRelativeVerticalValues = 1;
    int xOffsetBars = xPos / 12;
    int startBar = sc.IndexOfFirstVisibleBar + (xOffsetBars < 0 ? 0 : xOffsetBars);
    if (startBar >= sc.ArraySize) startBar = sc.ArraySize - 1;
    Tool.BeginDateTime = sc.BaseDateTimeIn[startBar > 0 ? startBar : 0];
    double yPercent = (double)(yPos - 20) / 10.0; 
    Tool.BeginValue = 100.0 - yPercent;
    if (Tool.BeginValue > 99) Tool.BeginValue = 99;
    Tool.Text.Format("%s ON", prefix.GetChars());
    Tool.Color = color; Tool.FontSize = fontSize; Tool.FontBold = 1;
    Tool.TransparentLabelBackground = 1;
    Tool.AddMethod = UTAM_ADD_OR_ADJUST;
    sc.UseTool(Tool);
}

void DrawLines(SCStudyInterfaceRef sc, GlobalState* p_State, bool showLines, 
               int lineStyle, int lineWidth, COLORREF lineColor, bool showLabels,
               int drawingMode, int shortLineBars, bool forceRedraw) {
    const int baseLineID = 98765500;
    const int maxLevels = 100;
    if (!showLines || !p_State->HasScanned || p_State->Levels.empty()) {
        if (p_State->LastShowLines || forceRedraw) {
            for (int i = 0; i < maxLevels * 2; ++i)
                sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, baseLineID + i);
        }
        p_State->LastShowLines = false; return;
    }
    bool barCountChanged = (sc.ArraySize != p_State->LastArraySize);
    if (!forceRedraw && !barCountChanged && p_State->LastShowLines) return;
    p_State->LastShowLines = true;
    int levelsToDraw = (int)p_State->Levels.size();
    if (levelsToDraw > maxLevels) levelsToDraw = maxLevels;
    for (int i = 0; i < maxLevels; ++i) {
        int lineID = baseLineID + (i * 2); int labelID = baseLineID + (i * 2) + 1;
        if (i >= levelsToDraw) {
            sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, lineID);
            sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, labelID);
            continue;
        }
        const auto& level = p_State->Levels[i];
        s_UseTool Tool; Tool.Clear();
        Tool.ChartNumber = sc.ChartNumber; Tool.LineNumber = lineID;
        Tool.BeginValue = level.Price; Tool.EndValue = level.Price;
        Tool.Color = lineColor; Tool.LineWidth = lineWidth;
        Tool.LineStyle = (SubgraphLineStyles)lineStyle;
        Tool.AddMethod = UTAM_ADD_OR_ADJUST;
        if (drawingMode == 0) Tool.DrawingType = DRAWING_HORIZONTALLINE;
        else {
            Tool.DrawingType = DRAWING_LINE;
            int endIdx = sc.ArraySize - 1;
            int startIdx = (endIdx - shortLineBars < 0) ? 0 : endIdx - shortLineBars;
            Tool.BeginIndex = startIdx; Tool.EndIndex = endIdx;
            Tool.ExtendRight = 1;
        }
        sc.UseTool(Tool);
        if (showLabels) {
            s_UseTool Text; Text.Clear();
            Text.ChartNumber = sc.ChartNumber; Text.LineNumber = labelID;
            Text.DrawingType = DRAWING_TEXT;
            Text.Text.Format("%s (%s)", (level.Description.GetLength() > 0 ? level.Description.GetChars() : level.Label.GetChars()), level.ChartName.GetChars());
            Text.BeginValue = level.Price; Text.Color = lineColor;
            Text.FontSize = 10; Text.TransparentLabelBackground = 1;
            if (drawingMode == 0) {
                Text.BeginIndex = sc.ArraySize - 1; Text.TextAlignment = DT_RIGHT | DT_BOTTOM;
            } else {
                int endIdx = sc.ArraySize - 1;
                int startIdx = (endIdx - shortLineBars < 0) ? 0 : endIdx - shortLineBars;
                Text.BeginIndex = startIdx; Text.TextAlignment = DT_LEFT | DT_BOTTOM;
            }
            Text.BeginIndex -= (level.StaggerOffset * 10);
            Text.AddMethod = UTAM_ADD_OR_ADJUST;
            sc.UseTool(Text);
        } else sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, labelID);
    }
}

void DrawTable(SCStudyInterfaceRef sc, GlobalState* p_State, bool showTable, int xPos, int yPos, 
               int fontSize, int fontColorInt, int bgColorInt, int highlightColorInt,
               int maxLevelsAround, bool forceRedraw) {
    if (!showTable) {
        sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, 98765432);
        sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, 98765433);
        p_State->LastHighIdx = -1; p_State->LastLowIdx = -1; p_State->LastPrice = 0;
        return;
    }
    if (!p_State->HasScanned || p_State->Levels.empty()) return;
    double currentPrice = sc.Close[sc.ArraySize - 1];
    int idxHigh, idxLow;
    CalculateHighlights(p_State->Levels, currentPrice, idxHigh, idxLow);
    bool highlightsChanged = (idxHigh != p_State->LastHighIdx || idxLow != p_State->LastLowIdx);
    bool scrollChanged = (sc.IndexOfFirstVisibleBar != p_State->LastFirstVisibleBar);
    if (!forceRedraw && !highlightsChanged && !scrollChanged) return;
    p_State->LastHighIdx = idxHigh; p_State->LastLowIdx = idxLow;
    p_State->LastPrice = currentPrice; p_State->LastFirstVisibleBar = sc.IndexOfFirstVisibleBar;
    if (forceRedraw || highlightsChanged) {
        int visibleStart = 0, visibleEnd = (int)p_State->Levels.size() - 1;
        int window = maxLevelsAround;
        if (window > 0 && visibleEnd >= 0) {
            int center = (idxHigh != -1) ? idxHigh : (idxLow != -1) ? idxLow : -1;
            if (center != -1) {
                visibleStart = (center - window < 0) ? 0 : center - window;
                visibleEnd = (center + window > (int)p_State->Levels.size() - 1) ? (int)p_State->Levels.size() - 1 : center + window;
            }
        }
        p_State->CachedTableNormal = ""; p_State->CachedTableHighlight = "";
        const int wLabel = 28, wPrice = 10;
        for (size_t i = 0; i < p_State->Levels.size(); ++i) {
            if ((int)i < visibleStart || (int)i > visibleEnd) continue;
            const auto& level = p_State->Levels[i];
            SCString labelStr;
            labelStr.Format("%s (%s)", (level.Description.GetLength() > 0 ? level.Description.GetChars() : level.Label.GetChars()), level.ChartName.GetChars());
            if (labelStr.GetLength() > wLabel) labelStr = labelStr.Left(wLabel - 1);
            SCString row;
            row.Format("%-*s %*s\n", wLabel, labelStr.GetChars(), wPrice, sc.FormatGraphValue(level.Price, sc.BaseGraphValueFormat).GetChars());
            p_State->CachedTableNormal += row;
            if ((int)i == idxHigh || (int)i == idxLow) p_State->CachedTableHighlight += row;
            else p_State->CachedTableHighlight += "\n";
        }
    }
    s_UseTool Tool; Tool.Clear();
    Tool.ChartNumber = sc.ChartNumber; Tool.DrawingType = DRAWING_TEXT;
    Tool.UseRelativeVerticalValues = 1;
    int xOffsetBars = xPos / 12;
    int startBar = sc.IndexOfFirstVisibleBar + (xOffsetBars < 0 ? 0 : xOffsetBars);
    if (startBar >= sc.ArraySize) startBar = sc.ArraySize - 1;
    Tool.BeginDateTime = sc.BaseDateTimeIn[startBar > 0 ? startBar : 0];
    Tool.BeginValue = 100.0 - ((double)yPos / 10.0);
    Tool.FontSize = fontSize; Tool.FontFace = "Courier New"; Tool.FontBold = 1;
    Tool.AddMethod = UTAM_ADD_OR_ADJUST;
    Tool.LineNumber = 98765432; Tool.Text = p_State->CachedTableNormal;
    Tool.Color = fontColorInt; Tool.FontBackColor = bgColorInt; Tool.TransparentLabelBackground = 0;
    sc.UseTool(Tool);
    Tool.LineNumber = 98765433; Tool.Text = p_State->CachedTableHighlight;
    Tool.Color = highlightColorInt; Tool.TransparentLabelBackground = 1;
    sc.UseTool(Tool);
}

SCSFExport scsf_LevelAggregator(SCStudyInterfaceRef sc) {
    enum InputIdx {
        IN_HDR_CORE = 0, IN_TABLE_BUTTON_NUM, IN_TABLE_BUTTON_TEXT, IN_LINE_BUTTON_NUM, IN_LINE_BUTTON_TEXT,
        IN_LINE_TYPE, IN_DISPLAY_MODE, IN_HUD_PREFIX, IN_LABEL_FILTERS, IN_CHART_CONFIG, IN_AUTO_SCAN_INTERVAL,
        IN_CLUSTER_THRESHOLD, IN_CLUSTER_HANDLING, IN_HDR_TABLE, IN_TABLE_X, IN_TABLE_Y, IN_TABLE_RANGE_LEVELS, IN_FONT_SIZE, IN_FONT_COLOR, IN_BG_COLOR, IN_HIGHLIGHT_COLOR,
        IN_HDR_LINE_SETTINGS, IN_LINE_STYLE, IN_LINE_WIDTH, IN_LINE_COLOR, IN_SHOW_LINE_LABELS, IN_SHORT_LINE_BARS,
        IN_HDR_SORT, IN_SORT_BY_1, IN_SORT_BY_2,
        IN_HDR_EXPORT, IN_EXPORT_ON_SCAN, IN_OUTPUT_DEST, IN_FILE_PATH, IN_USE_TEMPLATE, IN_TEMPLATE_PATH, IN_INCLUDE_CHART_NAME, IN_INCLUDE_DESCRIPTION
    };
    if (sc.SetDefaults) {
        sc.GraphName = "Level Aggregator";
        sc.StudyDescription = 
            "Aggregates price levels from user-drawn chart objects across multiple charts.\n\n"
            "DRAWING FORMAT: Label|Description\n"
            "  Example: BZ|Top of Balance Zone\n\n"
            "CHART CONFIG: ChartNum|Name (comma-separated)\n"
            "  Example: 1|TPO,9|Weekly\n\n"
            "LABEL MODIFIER: |All suffix finds all instances\n"
            "  Example: PDC,Support|All\n\n"
            "DISPLAY: Dual Control Bar buttons toggle table and line visibility.\n"
            "Highlighted rows show nearest levels above/below current price.";
        sc.AutoLoop = 0;
        sc.GraphRegion = 0;
        sc.UpdateAlways = 1;
        
        int order = 1;
        
        // ===== CORE SETTINGS =====
        sc.Input[IN_HDR_CORE].Name = "===== CORE SETTINGS =====";
        sc.Input[IN_HDR_CORE].SetInt(0);
        sc.Input[IN_HDR_CORE].SetIntLimits(0, 0);
        sc.Input[IN_HDR_CORE].DisplayOrder = order++;
        
        sc.Input[IN_TABLE_BUTTON_NUM].Name = "Table Toggle Button # (0 = Disable)";
        sc.Input[IN_TABLE_BUTTON_NUM].SetInt(1);
        sc.Input[IN_TABLE_BUTTON_NUM].SetIntLimits(0, 150);
        sc.Input[IN_TABLE_BUTTON_NUM].DisplayOrder = order++;
        
        sc.Input[IN_TABLE_BUTTON_TEXT].Name = "Table Button Text";
        sc.Input[IN_TABLE_BUTTON_TEXT].SetString("Levels");
        sc.Input[IN_TABLE_BUTTON_TEXT].DisplayOrder = order++;

        sc.Input[IN_LINE_BUTTON_NUM].Name = "Line Toggle Button # (0 = Disable)";
        sc.Input[IN_LINE_BUTTON_NUM].SetInt(2);
        sc.Input[IN_LINE_BUTTON_NUM].SetIntLimits(0, 150);
        sc.Input[IN_LINE_BUTTON_NUM].DisplayOrder = order++;
        
        sc.Input[IN_LINE_BUTTON_TEXT].Name = "Line Button Text";
        sc.Input[IN_LINE_BUTTON_TEXT].SetString("Lines");
        sc.Input[IN_LINE_BUTTON_TEXT].DisplayOrder = order++;

        sc.Input[IN_LINE_TYPE].Name = "Line Type (For Buttons/Fallback)";
        sc.Input[IN_LINE_TYPE].SetCustomInputStrings("Short Line;Full Width");
        sc.Input[IN_LINE_TYPE].SetCustomInputIndex(0);
        sc.Input[IN_LINE_TYPE].DisplayOrder = order++;

        sc.Input[IN_DISPLAY_MODE].Name = "Display Mode (Fallback if buttons = 0)";
        sc.Input[IN_DISPLAY_MODE].SetCustomInputStrings("Table Only;Table + Short Lines;Table + Full Lines;Short Lines Only;Full Lines Only");
        sc.Input[IN_DISPLAY_MODE].SetCustomInputIndex(1);
        sc.Input[IN_DISPLAY_MODE].DisplayOrder = order++;

        sc.Input[IN_HUD_PREFIX].Name = "HUD Status Prefix";
        sc.Input[IN_HUD_PREFIX].SetString("Level Aggregator:");
        sc.Input[IN_HUD_PREFIX].DisplayOrder = order++;
        
        sc.Input[IN_LABEL_FILTERS].Name = "Target Labels (comma-separated, |All for all instances)";
        sc.Input[IN_LABEL_FILTERS].SetString(">>|All");
        sc.Input[IN_LABEL_FILTERS].DisplayOrder = order++;
        
        sc.Input[IN_CHART_CONFIG].Name = "Charts: ChartNum|Name (e.g., 1|TPO,9|Weekly)";
        sc.Input[IN_CHART_CONFIG].SetString("1|TPO,9|1 MIN,3|Daily TPO,5|Weekly,13|30 MIN,8|4 HR");
        sc.Input[IN_CHART_CONFIG].DisplayOrder = order++;

        sc.Input[IN_AUTO_SCAN_INTERVAL].Name = "Auto-Scan Heartbeat (Seconds, 0=Manual)";
        sc.Input[IN_AUTO_SCAN_INTERVAL].SetInt(1);
        sc.Input[IN_AUTO_SCAN_INTERVAL].SetIntLimits(0, 3600);
        sc.Input[IN_AUTO_SCAN_INTERVAL].DisplayOrder = order++;

        sc.Input[IN_CLUSTER_THRESHOLD].Name = "Cluster Threshold (Ticks)";
        sc.Input[IN_CLUSTER_THRESHOLD].SetInt(5);
        sc.Input[IN_CLUSTER_THRESHOLD].DisplayOrder = order++;

        sc.Input[IN_CLUSTER_HANDLING].Name = "Clustered Text Handling";
        sc.Input[IN_CLUSTER_HANDLING].SetCustomInputStrings("Merge & Concatenate;Stagger Horizontally");
        sc.Input[IN_CLUSTER_HANDLING].SetCustomInputIndex(0);
        sc.Input[IN_CLUSTER_HANDLING].DisplayOrder = order++;
        
        // ===== TABLE SETTINGS =====
        sc.Input[IN_HDR_TABLE].Name = "===== TABLE SETTINGS =====";
        sc.Input[IN_HDR_TABLE].SetInt(0);
        sc.Input[IN_HDR_TABLE].SetIntLimits(0, 0);
        sc.Input[IN_HDR_TABLE].DisplayOrder = order++;

        sc.Input[IN_TABLE_X].Name = "Table X Position (Pixels)";
        sc.Input[IN_TABLE_X].SetInt(20);
        sc.Input[IN_TABLE_X].DisplayOrder = order++;

        sc.Input[IN_TABLE_Y].Name = "Table Y Position (Pixels)";
        sc.Input[IN_TABLE_Y].SetInt(80);
        sc.Input[IN_TABLE_Y].DisplayOrder = order++;

        sc.Input[IN_TABLE_RANGE_LEVELS].Name = "Max Table Levels Above/Below Current (0 = All)";
        sc.Input[IN_TABLE_RANGE_LEVELS].SetInt(0);
        sc.Input[IN_TABLE_RANGE_LEVELS].SetIntLimits(0, 1000);
        sc.Input[IN_TABLE_RANGE_LEVELS].DisplayOrder = order++;
        
        sc.Input[IN_FONT_SIZE].Name = "Font Size";
        sc.Input[IN_FONT_SIZE].SetInt(10);
        sc.Input[IN_FONT_SIZE].DisplayOrder = order++;

        sc.Input[IN_FONT_COLOR].Name = "Text Color";
        sc.Input[IN_FONT_COLOR].SetColor(RGB(220, 220, 220));
        sc.Input[IN_FONT_COLOR].DisplayOrder = order++;

        sc.Input[IN_BG_COLOR].Name = "Background Color";
        sc.Input[IN_BG_COLOR].SetColor(RGB(0, 0, 0));
        sc.Input[IN_BG_COLOR].DisplayOrder = order++;
        
        sc.Input[IN_HIGHLIGHT_COLOR].Name = "Highlight Color (Next High/Low)";
        sc.Input[IN_HIGHLIGHT_COLOR].SetColor(RGB(255, 255, 0));
        sc.Input[IN_HIGHLIGHT_COLOR].DisplayOrder = order++;

        // ===== CHART LINE SETTINGS =====
        sc.Input[IN_HDR_LINE_SETTINGS].Name = "===== CHART LINE SETTINGS =====";
        sc.Input[IN_HDR_LINE_SETTINGS].SetInt(0);
        sc.Input[IN_HDR_LINE_SETTINGS].SetIntLimits(0, 0);
        sc.Input[IN_HDR_LINE_SETTINGS].DisplayOrder = order++;

        sc.Input[IN_LINE_STYLE].Name = "Line Style";
        sc.Input[IN_LINE_STYLE].SetCustomInputStrings("Solid;Dash;Dot;DashDot;DashDotDot");
        sc.Input[IN_LINE_STYLE].SetCustomInputIndex(0);
        sc.Input[IN_LINE_STYLE].DisplayOrder = order++;

        sc.Input[IN_LINE_WIDTH].Name = "Line Width";
        sc.Input[IN_LINE_WIDTH].SetInt(1);
        sc.Input[IN_LINE_WIDTH].SetIntLimits(1, 10);
        sc.Input[IN_LINE_WIDTH].DisplayOrder = order++;

        sc.Input[IN_LINE_COLOR].Name = "Line Color";
        sc.Input[IN_LINE_COLOR].SetColor(RGB(255, 255, 255));
        sc.Input[IN_LINE_COLOR].DisplayOrder = order++;

        sc.Input[IN_SHOW_LINE_LABELS].Name = "Show Labels on Lines";
        sc.Input[IN_SHOW_LINE_LABELS].SetYesNo(1);
        sc.Input[IN_SHOW_LINE_LABELS].DisplayOrder = order++;

        sc.Input[IN_SHORT_LINE_BARS].Name = "Short Line Length (Bars)";
        sc.Input[IN_SHORT_LINE_BARS].SetInt(20);
        sc.Input[IN_SHORT_LINE_BARS].SetIntLimits(1, 500);
        sc.Input[IN_SHORT_LINE_BARS].DisplayOrder = order++;
        
        // ===== SORT SETTINGS =====
        sc.Input[IN_HDR_SORT].Name = "===== SORT SETTINGS =====";
        sc.Input[IN_HDR_SORT].SetInt(0);
        sc.Input[IN_HDR_SORT].SetIntLimits(0, 0);
        sc.Input[IN_HDR_SORT].DisplayOrder = order++;
        
        sc.Input[IN_SORT_BY_1].Name = "First Sort By";
        sc.Input[IN_SORT_BY_1].SetCustomInputStrings("Price Descending;Price Ascending;Chart Priority;Label;Description;Chart Name");
        sc.Input[IN_SORT_BY_1].SetCustomInputIndex(0);
        sc.Input[IN_SORT_BY_1].DisplayOrder = order++;
        
        sc.Input[IN_SORT_BY_2].Name = "Second Sort By";
        sc.Input[IN_SORT_BY_2].SetCustomInputStrings("Price Descending;Price Ascending;Chart Priority;Label;Description;Chart Name");
        sc.Input[IN_SORT_BY_2].SetCustomInputIndex(2);
        sc.Input[IN_SORT_BY_2].DisplayOrder = order++;
        
        // ===== EXPORT SETTINGS =====
        sc.Input[IN_HDR_EXPORT].Name = "===== EXPORT SETTINGS (Optional) =====";
        sc.Input[IN_HDR_EXPORT].SetInt(0);
        sc.Input[IN_HDR_EXPORT].SetIntLimits(0, 0);
        sc.Input[IN_HDR_EXPORT].DisplayOrder = order++;
        
        sc.Input[IN_EXPORT_ON_SCAN].Name = "Export on Scan";
        sc.Input[IN_EXPORT_ON_SCAN].SetYesNo(0);
        sc.Input[IN_EXPORT_ON_SCAN].DisplayOrder = order++;
        
        sc.Input[IN_OUTPUT_DEST].Name = "Export Destination";
        sc.Input[IN_OUTPUT_DEST].SetCustomInputStrings("Clipboard;File;Clipboard+File");
        sc.Input[IN_OUTPUT_DEST].SetCustomInputIndex(0);
        sc.Input[IN_OUTPUT_DEST].DisplayOrder = order++;
        
        sc.Input[IN_FILE_PATH].Name = "Export File Path";
        sc.Input[IN_FILE_PATH].SetPathAndFileName("C:\\temp\\levels.txt");
        sc.Input[IN_FILE_PATH].DisplayOrder = order++;
        
        sc.Input[IN_USE_TEMPLATE].Name = "Use Template File";
        sc.Input[IN_USE_TEMPLATE].SetYesNo(0);
        sc.Input[IN_USE_TEMPLATE].DisplayOrder = order++;
        
        sc.Input[IN_TEMPLATE_PATH].Name = "Template File Path";
        sc.Input[IN_TEMPLATE_PATH].SetPathAndFileName("C:\\temp\\level_template.txt");
        sc.Input[IN_TEMPLATE_PATH].DisplayOrder = order++;
        
        sc.Input[IN_INCLUDE_CHART_NAME].Name = "Include Chart Name in Export";
        sc.Input[IN_INCLUDE_CHART_NAME].SetYesNo(1);
        sc.Input[IN_INCLUDE_CHART_NAME].DisplayOrder = order++;
        
        sc.Input[IN_INCLUDE_DESCRIPTION].Name = "Include Description in Export";
        sc.Input[IN_INCLUDE_DESCRIPTION].SetYesNo(1);
        sc.Input[IN_INCLUDE_DESCRIPTION].DisplayOrder = order++;
        
        return;
    }
    GlobalState* p_State = (GlobalState*)sc.GetPersistentPointer(1);
    if (!p_State) { p_State = new GlobalState; sc.SetPersistentPointer(1, p_State); }
    if (!p_State->HasInitialized) {
        if (sc.Input[IN_TABLE_BUTTON_NUM].GetInt() == 0 && sc.Input[IN_LINE_BUTTON_NUM].GetInt() == 0) {
            int m = sc.Input[IN_DISPLAY_MODE].GetIndex();
            p_State->IsTableVisible = (m <= 2); p_State->AreLinesVisible = (m >= 1);
        } else { p_State->IsTableVisible = p_State->HasScanned; p_State->AreLinesVisible = p_State->HasScanned; }
        p_State->HasInitialized = true;
    }
    SCDateTime now = sc.CurrentSystemDateTime; bool runScan = false;
    static SCDateTime lastTblTog = 0, lastLinTog = 0;
    auto HandleBtn = [&](int btnIdx, bool& state, SCDateTime& lastToggle) {
        int btnNum = sc.Input[btnIdx].GetInt(); if (btnNum <= 0) return;
        sc.SetCustomStudyControlBarButtonText(btnNum, sc.Input[btnIdx+1].GetString());
        int cur = sc.GetCustomStudyControlBarButtonEnableState(btnNum);
        if (cur != (state ? 1 : 0)) {
            if ((now.GetAsDouble() - lastToggle.GetAsDouble()) * 86400.0 > 0.5) {
                state = (cur == 1); lastToggle = now; p_State->ForceRedraw = true; if (state) runScan = true;
            } else sc.SetCustomStudyControlBarButtonEnable(btnNum, state ? 1 : 0);
        }
    };
    HandleBtn(IN_TABLE_BUTTON_NUM, p_State->IsTableVisible, lastTblTog);
    HandleBtn(IN_LINE_BUTTON_NUM, p_State->AreLinesVisible, lastLinTog);
    int interval = sc.Input[IN_AUTO_SCAN_INTERVAL].GetInt();
    if (interval > 0 && (now.GetAsDouble() - p_State->LastAutoScanTime.GetAsDouble()) * 86400.0 > (double)interval) {
        runScan = true; p_State->LastAutoScanTime = now;
    }
    int modeLineType = (sc.Input[IN_LINE_TYPE].GetIndex() == 1) ? 0 : 1; 
    if (sc.Input[IN_TABLE_BUTTON_NUM].GetInt() == 0 && sc.Input[IN_LINE_BUTTON_NUM].GetInt() == 0) {
        int m = sc.Input[IN_DISPLAY_MODE].GetIndex();
        if (m == 2 || m == 4) modeLineType = 0; else if (m == 1 || m == 3) modeLineType = 1;
    }
    if (p_State->IsTableVisible && sc.IndexOfFirstVisibleBar != p_State->LastFirstVisibleBar) {
        p_State->LastFirstVisibleBar = sc.IndexOfFirstVisibleBar; p_State->ForceRedraw = true;
    }
    DrawTable(sc, p_State, p_State->IsTableVisible, sc.Input[IN_TABLE_X].GetInt(), sc.Input[IN_TABLE_Y].GetInt(),
              sc.Input[IN_FONT_SIZE].GetInt(), sc.Input[IN_FONT_COLOR].GetColor(), sc.Input[IN_BG_COLOR].GetColor(), 
              sc.Input[IN_HIGHLIGHT_COLOR].GetColor(), sc.Input[IN_TABLE_RANGE_LEVELS].GetInt(), p_State->ForceRedraw);
    DrawLines(sc, p_State, p_State->AreLinesVisible, sc.Input[IN_LINE_STYLE].GetIndex(), sc.Input[IN_LINE_WIDTH].GetInt(),
              sc.Input[IN_LINE_COLOR].GetColor(), sc.Input[IN_SHOW_LINE_LABELS].GetYesNo(), modeLineType, sc.Input[IN_SHORT_LINE_BARS].GetInt(), p_State->ForceRedraw);
    DrawHUD(sc, p_State->AreLinesVisible, sc.Input[IN_HUD_PREFIX].GetString(), sc.Input[IN_TABLE_X].GetInt(), sc.Input[IN_TABLE_Y].GetInt(), sc.Input[IN_FONT_SIZE].GetInt(), sc.Input[IN_FONT_COLOR].GetColor());
    p_State->LastArraySize = sc.ArraySize;
    if (!runScan && !p_State->ForceRedraw) return;
    if (!runScan && p_State->ForceRedraw) { p_State->ForceRedraw = false; return; }

    std::vector<SCString> targetLabels; std::vector<bool> labelFindAll;
    SCString filters = sc.Input[IN_LABEL_FILTERS].GetString();
    if (!filters.IsEmpty()) {
        char fBuf[1024]; strncpy(fBuf, filters.GetChars(), 1023); char* fToken = strtok(fBuf, ",");
        while (fToken) {
            while (*fToken == ' ') fToken++; bool findAll = false; char* pipe = strstr(fToken, "|All");
            if (pipe) { *pipe = '\0'; findAll = true; }
            if (strlen(fToken) > 0) { targetLabels.push_back(fToken); labelFindAll.push_back(findAll); }
            fToken = strtok(NULL, ",");
        }
    }
    if (targetLabels.empty()) return;
    std::vector<ChartConfig> charts; SCString cfgStr = sc.Input[IN_CHART_CONFIG].GetString();
    if (!cfgStr.IsEmpty()) {
        char fBuf[1024]; strncpy(fBuf, cfgStr.GetChars(), 1023); char* fToken = strtok(fBuf, ","); int priority = 0;
        while (fToken) {
            while (*fToken == ' ') fToken++; char* pipe = strchr(fToken, '|');
            ChartConfig c; c.Priority = priority++;
            if (pipe) { *pipe = '\0'; c.ChartNumber = atoi(fToken); c.CustomName = pipe + 1; }
            else { c.ChartNumber = atoi(fToken); c.CustomName = sc.GetChartSymbol(c.ChartNumber); }
            if (c.ChartNumber > 0) charts.push_back(c); fToken = strtok(NULL, ",");
        }
    } else { charts.push_back({sc.ChartNumber, sc.GetChartSymbol(sc.ChartNumber), 0}); }
    std::vector<LevelEntry> allLevels; std::vector<bool> foundGlobally(targetLabels.size(), false);
    for (auto& c : charts) {
        int dIdx = 0; s_UseTool d;
        while (sc.GetUserDrawnChartDrawing(c.ChartNumber, 0, d, dIdx++) != 0) {
            if (d.Text.GetLength() == 0) continue;
            const char* fullText = d.Text.GetChars(); const char* pipe = strchr(fullText, '|');
            for (size_t li = 0; li < targetLabels.size(); li++) {
                if (foundGlobally[li] && !labelFindAll[li]) continue;
                if (strstr(fullText, targetLabels[li].GetChars())) {
                    LevelEntry e; e.Label = targetLabels[li];
                    e.Description = pipe ? TrimSCString(pipe + 1) : TrimSCString(fullText);
                    e.SourceChart = c.ChartNumber; e.ChartName = c.CustomName;
                    e.Price = d.BeginValue; e.ChartPriority = c.Priority;
                    allLevels.push_back(e); foundGlobally[li] = true; break;
                }
            }
            if (dIdx > 5000) break;
        }
    }
    auto Compare = [](const LevelEntry& a, const LevelEntry& b, int type) -> int {
        switch (type) {
            case 0: return (a.Price > b.Price) ? 1 : (a.Price < b.Price) ? -1 : 0;
            case 1: return (a.Price < b.Price) ? 1 : (a.Price > b.Price) ? -1 : 0;
            case 2: return (a.ChartPriority < b.ChartPriority) ? 1 : -1;
            case 3: return a.Label.CompareNoCase(b.Label);
            case 4: return a.Description.CompareNoCase(b.Description);
            case 5: return a.ChartName.CompareNoCase(b.ChartName);
        } return 0;
    };
    int s1 = sc.Input[IN_SORT_BY_1].GetIndex(), s2 = sc.Input[IN_SORT_BY_2].GetIndex();
    std::sort(allLevels.begin(), allLevels.end(), [&](const LevelEntry& a, const LevelEntry& b) {
        int r = Compare(a, b, s1); if (r != 0) return r == 1; return Compare(a, b, s2) == 1;
    });

    if (runScan && !allLevels.empty()) {
        double thresholdPrice = sc.Input[IN_CLUSTER_THRESHOLD].GetInt() * sc.TickSize;
        int clusterMode = sc.Input[IN_CLUSTER_HANDLING].GetIndex();
        std::vector<LevelEntry> processedLevels;
        processedLevels.push_back(allLevels[0]);

        for (size_t i = 1; i < allLevels.size(); ++i) {
            LevelEntry& current = processedLevels.back();
            LevelEntry next = allLevels[i];
            
            double diff = current.Price - next.Price;
            if (diff < 0) diff = -diff;

            if (diff <= thresholdPrice) {
                if (clusterMode == 0) { // Merge
                    current.Price = (current.Price + next.Price) / 2.0;
                    SCString labelA = current.Label;
                    SCString labelB = next.Label;
                    current.Label.Format("%s / %s", labelA.GetChars(), labelB.GetChars());
                    SCString descA = current.Description.GetLength() > 0 ? current.Description : labelA;
                    SCString descB = next.Description.GetLength() > 0 ? next.Description : labelB;
                    current.Description.Format("%s / %s", descA.GetChars(), descB.GetChars());
                } else { // Stagger
                    next.StaggerOffset = current.StaggerOffset + 1;
                    processedLevels.push_back(next);
                }
            } else {
                processedLevels.push_back(next);
            }
        }
        allLevels = processedLevels;
    }

    p_State->Levels = allLevels; p_State->HasScanned = true; p_State->LastScanTime = now;
    p_State->LastHighIdx = -1; p_State->ForceRedraw = true;
    sc.AddMessageToLog(SCString().Format("Level Aggregator: Scanned %d charts, %d levels found.", (int)charts.size(), (int)allLevels.size()), 0);

    if (!sc.Input[IN_EXPORT_ON_SCAN].GetYesNo()) return;
    SCString output;
    if (sc.Input[IN_USE_TEMPLATE].GetYesNo()) {
        SCString templatePath = sc.Input[IN_TEMPLATE_PATH].GetPathAndFileName();
        HANDLE hFile = CreateFile(templatePath.GetChars(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD fileSize = GetFileSize(hFile, NULL);
            if (fileSize > 0 && fileSize < 65536) {
                char* buffer = new char[fileSize + 1]; DWORD bytesRead; ReadFile(hFile, buffer, fileSize, &bytesRead, NULL);
                buffer[bytesRead] = '\0'; CloseHandle(hFile); SCString content(buffer); delete[] buffer;
                for (const auto& level : allLevels) {
                    SCString result; const char* p = content.GetChars();
                    while (*p) {
                        if (strncmp(p, "{{LABEL}}", 9) == 0) { result += level.Label; p += 9; }
                        else if (strncmp(p, "{{DESC}}", 8) == 0) { result += level.Description; p += 8; }
                        else if (strncmp(p, "{{PRICE}}", 9) == 0) { result += sc.FormatGraphValue(level.Price, sc.BaseGraphValueFormat); p += 9; }
                        else if (strncmp(p, "{{CHART}}", 9) == 0) { result += level.ChartName; p += 9; }
                        else { result += *p; p++; }
                    }
                    output += result + "\n";
                }
            } else CloseHandle(hFile);
        }
    } else {
        output += "---\n# Levels Export\n\n";
        for (const auto& level : allLevels) {
            SCString line; SCString name = (sc.Input[IN_INCLUDE_DESCRIPTION].GetYesNo() && level.Description.GetLength() > 0) ? level.Description : level.Label;
            if (sc.Input[IN_INCLUDE_CHART_NAME].GetYesNo()) line.Format("%s (%s):: %.2f\n", name.GetChars(), level.ChartName.GetChars(), level.Price);
            else line.Format("%s:: %.2f\n", name.GetChars(), level.Price);
            output += line;
        }
        output += "---\n";
    }
    int dest = sc.Input[IN_OUTPUT_DEST].GetIndex();
    if (dest == 0 || dest == 2) {
        if (OpenClipboard(NULL)) {
            EmptyClipboard(); HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, output.GetLength() + 1);
            if (hMem) {
                char* pMem = (char*)GlobalLock(hMem);
                if (pMem) { strcpy(pMem, output.GetChars()); GlobalUnlock(hMem); SetClipboardData(CF_TEXT, hMem); }
                else GlobalFree(hMem);
            } CloseClipboard();
        }
    }
    if (dest == 1 || dest == 2) {
        SCString path = sc.Input[IN_FILE_PATH].GetPathAndFileName();
        HANDLE hFile = CreateFile(path.GetChars(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD bw; WriteFile(hFile, output.GetChars(), output.GetLength(), &bw, NULL); CloseHandle(hFile);
        }
    }
}