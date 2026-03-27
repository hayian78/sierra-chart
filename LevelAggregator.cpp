#include "sierrachart.h"
#include <vector>
#include <algorithm>
#include <string>
#include <cfloat>

/*============================================================================
    Level Aggregator - Sierra Chart Study
    
    Purpose: Scans user-drawn chart objects across multiple charts and 
             aggregates matching price levels into a consolidated display.
    
    Drawing Label Format: LABEL|Description (e.g., "BZ|Top of Balance")
      - LABEL is matched against Target Labels filter
      - Description is displayed in the table
    
    Chart Settings Format: ChartNum|CustomName (e.g., "1|TPO,9|Weekly")
      - ChartNum is the chart to scan
      - CustomName is shown in the display
    
    Display: Table shows Price, Chart, Description with real-time
             highlighting of nearest levels above/below current price.
    
    Trigger: Control Bar button toggles table visibility and rescans.
============================================================================*/

SCDLLName("Level Aggregator")

// Structure to hold a level for sorting
struct LevelEntry
{
    SCString Label;
    SCString Description;
    double Price;
    int SourceChart;
    SCString ChartName;
    int ChartPriority;
};

// Structure to hold chart config
struct ChartConfig
{
    int ChartNumber;
    SCString CustomName;
    int Priority;
};

// Persistent state
struct GlobalState
{
    std::vector<LevelEntry> Levels;
    bool HasScanned = false;
    SCDateTime LastScanTime;
    bool IsTableVisible = false;
    int LastHighIdx = -1;
    int LastLowIdx = -1;
    double LastPrice = 0;
    bool ForceRedraw = false;
    int LastButtonState = 0;  // Track button state for edge detection
    int LastFirstVisibleBar = -1;  // Track scroll position
};

// Calculate highlight indices for nearest levels above/below price
void CalculateHighlights(const std::vector<LevelEntry>& levels, double currentPrice, int& idxHigh, int& idxLow)
{
    idxHigh = -1;
    idxLow = -1;
    double minDistHigh = DBL_MAX;
    double minDistLow = DBL_MAX;
    
    for (size_t i = 0; i < levels.size(); ++i)
    {
        double diff = levels[i].Price - currentPrice;
        if (diff >= 0 && diff < minDistHigh)
        {
            minDistHigh = diff;
            idxHigh = (int)i;
        }
        else if (diff < 0 && -diff < minDistLow)
        {
            minDistLow = -diff;
            idxLow = (int)i;
        }
    }
}

void DrawTable(SCStudyInterfaceRef sc, GlobalState* p_State, bool showTable, int xPos, int yPos, 
               int fontSize, int fontColorInt, int bgColorInt, int highlightColorInt,
               int maxLevelsAround, bool forceRedraw)
{
    // If not showing, always delete drawings unconditionally
    if (!showTable)
    {
        sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, 98765432);
        sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, 98765433);
        // Reset tracking
        p_State->LastHighIdx = -1;
        p_State->LastLowIdx = -1;
        p_State->LastPrice = 0;
        return;
    }
    
    // If no data, nothing to show
    if (!p_State->HasScanned || p_State->Levels.empty())
    {
        sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, 98765432);
        sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, 98765433);
        return;
    }
    
    // Get current price and calculate highlights
    double currentPrice = sc.Close[sc.ArraySize - 1];
    int idxHigh, idxLow;
    CalculateHighlights(p_State->Levels, currentPrice, idxHigh, idxLow);
    
    // Only skip redraw if highlights AND price haven't changed (and not forced)
    bool highlightsChanged = (idxHigh != p_State->LastHighIdx || idxLow != p_State->LastLowIdx);
    bool priceChanged = (currentPrice != p_State->LastPrice);
    
    if (!forceRedraw && !highlightsChanged && !priceChanged && p_State->LastPrice != 0)
    {
        return;
    }
    
    p_State->LastHighIdx = idxHigh;
    p_State->LastLowIdx = idxLow;
    p_State->LastPrice = currentPrice;

    // Determine which slice of levels to display around the current price.
    // This affects ONLY the on-chart table, not exports.
    int visibleStart = 0;
    int visibleEnd = (int)p_State->Levels.size() - 1;
    int window = maxLevelsAround;
    if (window < 0) window = 0;

    if (window > 0 && visibleEnd >= 0)
    {
        if (idxHigh == -1 && idxLow == -1)
        {
            // No reference levels; show all.
        }
        else if (idxHigh == -1)
        {
            int center = idxLow;
            visibleStart = center - window;
            if (visibleStart < 0) visibleStart = 0;
            visibleEnd = center + window;
            if (visibleEnd > (int)p_State->Levels.size() - 1)
                visibleEnd = (int)p_State->Levels.size() - 1;
        }
        else if (idxLow == -1)
        {
            int center = idxHigh;
            visibleStart = center - window;
            if (visibleStart < 0) visibleStart = 0;
            visibleEnd = center + window;
            if (visibleEnd > (int)p_State->Levels.size() - 1)
                visibleEnd = (int)p_State->Levels.size() - 1;
        }
        else
        {
            // Both sides exist: show N rows above the high highlight and
            // N rows below the low highlight, plus everything in between.
            visibleStart = idxHigh - window;
            if (visibleStart < 0) visibleStart = 0;
            visibleEnd = idxLow + window;
            if (visibleEnd > (int)p_State->Levels.size() - 1)
                visibleEnd = (int)p_State->Levels.size() - 1;
            if (visibleEnd < visibleStart)
            {
                int center = (idxHigh + idxLow) / 2;
                visibleStart = center - window;
                if (visibleStart < 0) visibleStart = 0;
                visibleEnd = center + window;
                if (visibleEnd > (int)p_State->Levels.size() - 1)
                    visibleEnd = (int)p_State->Levels.size() - 1;
            }
        }
    }

    // Build table text in a compact "label / value" style, similar to
    // Sierra Chart's built‑in data window: left column is a combined
    // chart + description label, right column is the price.
    SCString tableTextNormal;
    SCString tableTextHighlight;
    
    const int wLabel = 30; // left column width
    const int wPrice = 11; // right column width
    
    // Data rows (no header/separator for a cleaner, more compact panel)
    for (size_t i = 0; i < p_State->Levels.size(); ++i)
    {
        int iIndex = (int)i;
        if (iIndex < visibleStart || iIndex > visibleEnd)
            continue;
        const auto& level = p_State->Levels[i];
        
        SCString priceStr = sc.FormatGraphValue(level.Price, sc.BaseGraphValueFormat);
        
        // Build a compact label: "Description (Chart)" or "Label (Chart)"
        SCString baseName;
        if (level.Description.GetLength() > 0)
            baseName = level.Description;
        else
            baseName = level.Label;
        
        SCString labelStr;
        labelStr.Format("%s (%s)", baseName.GetChars(), level.ChartName.GetChars());
        
        if (labelStr.GetLength() > wLabel)
            labelStr = labelStr.Left(wLabel - 1);
        
        SCString row;
        row.Format("%-*s %*s\n",
            wLabel, labelStr.GetChars(),
            wPrice, priceStr.GetChars());
        
        bool isHighlight = ((int)i == idxHigh || (int)i == idxLow);
        
        tableTextNormal += row;
        
        if (isHighlight)
            tableTextHighlight += row;
        else
            tableTextHighlight += "\n";
    }
    
    // Setup drawing tool - chart-relative with frequent updates
    s_UseTool Tool;
    Tool.Clear();
    Tool.ChartNumber = sc.ChartNumber;
    Tool.DrawingType = DRAWING_TEXT;
    Tool.UseRelativeVerticalValues = 1;
    
    // Position relative to first visible bar (stays on screen when scrolling)
    int xOffsetBars = xPos / 12;
    if (xOffsetBars < 0) xOffsetBars = 0;
    int startBar = sc.IndexOfFirstVisibleBar + xOffsetBars;
    if (startBar >= sc.ArraySize) startBar = sc.ArraySize - 1;
    if (startBar < 0) startBar = 0;
    Tool.BeginDateTime = sc.BaseDateTimeIn[startBar];
    
    // Y position as percentage
    double yPercent = (double)yPos / 10.0;
    Tool.BeginValue = 100.0 - yPercent;
    if (Tool.BeginValue > 98) Tool.BeginValue = 98;
    if (Tool.BeginValue < 2) Tool.BeginValue = 2;
    
    Tool.FontSize = fontSize;
    Tool.FontFace = "Courier New";
    Tool.FontBold = 1;
    Tool.AddMethod = UTAM_ADD_OR_ADJUST;
    
    // Draw base layer
    Tool.LineNumber = 98765432;
    Tool.Text = tableTextNormal;
    Tool.Color = fontColorInt;
    Tool.FontBackColor = bgColorInt;
    Tool.TransparentLabelBackground = 0;
    sc.UseTool(Tool);
    
    // Draw highlight overlay
    Tool.LineNumber = 98765433;
    Tool.Text = tableTextHighlight;
    Tool.Color = highlightColorInt;
    Tool.TransparentLabelBackground = 1;
    sc.UseTool(Tool);
}

/*============================================================================
    Main study function
----------------------------------------------------------------------------*/
SCSFExport scsf_LevelAggregator(SCStudyInterfaceRef sc)
{
    // Input indices (sequential for proper display in settings)
    enum InputIdx
    {
        // Core Settings
        IN_HDR_CORE = 0,
        IN_BUTTON_NUM,
        IN_BUTTON_TEXT,
        IN_LABEL_FILTERS,
        IN_CHART_CONFIG,
        
        // Display Settings  
        IN_HDR_DISPLAY,
        IN_SHOW_TABLE,
        IN_TABLE_X,
        IN_TABLE_Y,
        IN_TABLE_RANGE_LEVELS,
        IN_FONT_SIZE,
        IN_FONT_COLOR,
        IN_BG_COLOR,
        IN_HIGHLIGHT_COLOR,
        
        // Sort Settings
        IN_HDR_SORT,
        IN_SORT_BY_1,
        IN_SORT_BY_2,
        
        // Export Settings
        IN_HDR_EXPORT,
        IN_EXPORT_ON_SCAN,
        IN_OUTPUT_DEST,
        IN_FILE_PATH,
        IN_USE_TEMPLATE,
        IN_TEMPLATE_PATH,
        IN_INCLUDE_CHART_NAME,
        IN_INCLUDE_DESCRIPTION
    };
    
    if (sc.SetDefaults)
    {
        sc.GraphName = "Level Aggregator";
        sc.StudyDescription = 
            "Aggregates price levels from user-drawn chart objects across multiple charts.\n\n"
            "DRAWING FORMAT: Label|Description\n"
            "  Example: BZ|Top of Balance Zone\n\n"
            "CHART CONFIG: ChartNum|Name (comma-separated)\n"
            "  Example: 1|TPO,9|Weekly\n\n"
            "LABEL MODIFIER: |All suffix finds all instances\n"
            "  Example: PDC,Support|All\n\n"
            "DISPLAY: Button toggles table visibility.\n"
            "Highlighted rows show nearest levels above/below current price.";
        sc.AutoLoop = 0;
        sc.GraphRegion = 0;
        // Update on every chart redraw (scroll/zoom) so the table
        // tracks the viewport smoothly. The study keeps CPU usage low
        // by only redrawing when something actually changes.
        sc.UpdateAlways = 1;
        
        int order = 1;
        
        // ===== CORE SETTINGS =====
        sc.Input[IN_HDR_CORE].Name = "===== CORE SETTINGS =====";
        sc.Input[IN_HDR_CORE].SetInt(0);
        sc.Input[IN_HDR_CORE].SetIntLimits(0, 0);
        sc.Input[IN_HDR_CORE].DisplayOrder = order++;
        
        sc.Input[IN_BUTTON_NUM].Name = "Control Bar Button # (1-150)";
        sc.Input[IN_BUTTON_NUM].SetInt(1);
        sc.Input[IN_BUTTON_NUM].SetIntLimits(1, 150);
        sc.Input[IN_BUTTON_NUM].DisplayOrder = order++;
        
        sc.Input[IN_BUTTON_TEXT].Name = "Button Text";
        sc.Input[IN_BUTTON_TEXT].SetString("Levels");
        sc.Input[IN_BUTTON_TEXT].DisplayOrder = order++;
        
        sc.Input[IN_LABEL_FILTERS].Name = "Target Labels (comma-separated, |All for all instances)";
        sc.Input[IN_LABEL_FILTERS].SetString(">>|All");
        sc.Input[IN_LABEL_FILTERS].DisplayOrder = order++;
        
        sc.Input[IN_CHART_CONFIG].Name = "Charts: ChartNum|Name (e.g., 1|TPO,9|Weekly)";
        sc.Input[IN_CHART_CONFIG].SetString("1|TPO,9|1 MIN,3|Daily TPO,5|Weekly,13|30 MIN,8|4 HR");
        sc.Input[IN_CHART_CONFIG].DisplayOrder = order++;
        
        // ===== DISPLAY SETTINGS =====
        sc.Input[IN_HDR_DISPLAY].Name = "===== DISPLAY SETTINGS =====";
        sc.Input[IN_HDR_DISPLAY].SetInt(0);
        sc.Input[IN_HDR_DISPLAY].SetIntLimits(0, 0);
        sc.Input[IN_HDR_DISPLAY].DisplayOrder = order++;
        
        sc.Input[IN_SHOW_TABLE].Name = "Show Table on Chart";
        sc.Input[IN_SHOW_TABLE].SetYesNo(1);
        sc.Input[IN_SHOW_TABLE].DisplayOrder = order++;

        sc.Input[IN_TABLE_X].Name = "Table X Position (Pixels)";
        sc.Input[IN_TABLE_X].SetInt(20);
        sc.Input[IN_TABLE_X].DisplayOrder = order++;

        sc.Input[IN_TABLE_Y].Name = "Table Y Position (Pixels)";
        sc.Input[IN_TABLE_Y].SetInt(60);
        sc.Input[IN_TABLE_Y].DisplayOrder = order++;

        sc.Input[IN_TABLE_RANGE_LEVELS].Name = "Max Levels Above/Below Current (0 = All)";
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
    
    // Get input references using enum
    SCInputRef Input_ButtonNum = sc.Input[IN_BUTTON_NUM];
    SCInputRef Input_ButtonText = sc.Input[IN_BUTTON_TEXT];
    SCInputRef Input_LabelFilters = sc.Input[IN_LABEL_FILTERS];
    SCInputRef Input_ChartConfig = sc.Input[IN_CHART_CONFIG];
    SCInputRef Input_ShowTable = sc.Input[IN_SHOW_TABLE];
    SCInputRef Input_TableX = sc.Input[IN_TABLE_X];
    SCInputRef Input_TableY = sc.Input[IN_TABLE_Y];
    SCInputRef Input_TableRangeLevels = sc.Input[IN_TABLE_RANGE_LEVELS];
    SCInputRef Input_FontSize = sc.Input[IN_FONT_SIZE];
    SCInputRef Input_FontColor = sc.Input[IN_FONT_COLOR];
    SCInputRef Input_BgColor = sc.Input[IN_BG_COLOR];
    SCInputRef Input_HighlightColor = sc.Input[IN_HIGHLIGHT_COLOR];
    SCInputRef Input_SortBy1 = sc.Input[IN_SORT_BY_1];
    SCInputRef Input_SortBy2 = sc.Input[IN_SORT_BY_2];
    SCInputRef Input_ExportOnScan = sc.Input[IN_EXPORT_ON_SCAN];
    SCInputRef Input_OutputDest = sc.Input[IN_OUTPUT_DEST];
    SCInputRef Input_FilePath = sc.Input[IN_FILE_PATH];
    SCInputRef Input_UseTemplate = sc.Input[IN_USE_TEMPLATE];
    SCInputRef Input_TemplatePath = sc.Input[IN_TEMPLATE_PATH];
    SCInputRef Input_IncludeChartName = sc.Input[IN_INCLUDE_CHART_NAME];
    SCInputRef Input_IncludeDesc = sc.Input[IN_INCLUDE_DESCRIPTION];
    
    // Manage persistent state
    GlobalState* p_State = (GlobalState*)sc.GetPersistentPointer(1);
    if (p_State == NULL)
    {
        p_State = new GlobalState;
        sc.SetPersistentPointer(1, p_State);
    }
    
    // Button handling - TIME-BASED DEBOUNCE approach
    // Static variables survive study recalculations better than persistent pointers
    static SCDateTime s_LastToggleTime = 0;
    static int s_StudyID = -1;
    
    // Reset static state if study ID changed (different study instance)
    if (s_StudyID != sc.StudyGraphInstanceID)
    {
        s_StudyID = sc.StudyGraphInstanceID;
        s_LastToggleTime = 0;
    }
    
    int buttonNum = Input_ButtonNum.GetInt();
    sc.SetCustomStudyControlBarButtonText(buttonNum, Input_ButtonText.GetString());
    int buttonState = sc.GetCustomStudyControlBarButtonEnableState(buttonNum);
    
    bool runScan = false;
    
    // Time-based debounce: only allow toggle if 500ms has passed since last toggle
    SCDateTime now = sc.CurrentSystemDateTime;
    double elapsedSeconds = 999.0;
    if (now.IsDateSet() && s_LastToggleTime.IsDateSet())
    {
        double diffDays = now.GetAsDouble() - s_LastToggleTime.GetAsDouble();
        elapsedSeconds = diffDays * 86400.0;
    }
    
    if (buttonState == 1 && elapsedSeconds > 0.5)
    {
        // Toggle visibility
        p_State->IsTableVisible = !p_State->IsTableVisible;
        s_LastToggleTime = now;
        
        if (p_State->IsTableVisible)
        {
            runScan = true;
            p_State->ForceRedraw = true;
        }
        else
        {
            // Immediately hide the table
            sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, 98765432);
            sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, 98765433);
            p_State->LastHighIdx = -1;
            p_State->LastLowIdx = -1;
            p_State->LastPrice = 0;
        }
        
        // Reset button
        sc.SetCustomStudyControlBarButtonEnable(buttonNum, 0);
    }
    
    // Early exit if table is hidden and no scan needed - minimize CPU usage
    if (!p_State->IsTableVisible && !runScan)
        return;
    
    // Detect scroll/zoom changes so the table position keeps up with
    // the chart as you pan. We only force a redraw when the first
    // visible bar actually moves.
    int currentFirstVisible = sc.IndexOfFirstVisibleBar;
    if (p_State->IsTableVisible && currentFirstVisible != p_State->LastFirstVisibleBar)
    {
        p_State->LastFirstVisibleBar = currentFirstVisible;
        p_State->ForceRedraw = true;
    }
    
    // Draw table (respects change detection)
    bool showTable = Input_ShowTable.GetYesNo() && p_State->IsTableVisible;
    bool forceRedraw = p_State->ForceRedraw;
    p_State->ForceRedraw = false;
    
    DrawTable(sc, p_State, showTable, Input_TableX.GetInt(), Input_TableY.GetInt(),
              Input_FontSize.GetInt(), Input_FontColor.GetColor(), 
              Input_BgColor.GetColor(), Input_HighlightColor.GetColor(),
              Input_TableRangeLevels.GetInt(), forceRedraw);
    
    if (!runScan) 
        return;
    
    // --- SCAN LOGIC ---
    
    // Parse target labels
    std::vector<SCString> targetLabels;
    std::vector<bool> labelFindAll;
    SCString labelFiltersStr = Input_LabelFilters.GetString();
    
    if (!labelFiltersStr.IsEmpty())
    {
        char labelBuffer[1024];
        strncpy(labelBuffer, labelFiltersStr.GetChars(), sizeof(labelBuffer) - 1);
        labelBuffer[sizeof(labelBuffer) - 1] = '\0';
        
        char* token = strtok(labelBuffer, ",");
        while (token != NULL)
        {
            while (*token == ' ') token++;
            
            if (strlen(token) > 0)
            {
                char* end = token + strlen(token) - 1;
                while (end > token && *end == ' ') { *end = '\0'; end--; }
            }
            
            if (strlen(token) > 0)
            {
                bool findAll = false;
                char* pipePos = strstr(token, "|All");
                if (pipePos != NULL && strlen(pipePos) == 4)
                {
                    *pipePos = '\0';
                    findAll = true;
                }
                
                targetLabels.push_back(SCString(token));
                labelFindAll.push_back(findAll);
            }
            
            token = strtok(NULL, ",");
        }
    }
    
    if (targetLabels.empty())
    {
        sc.AddMessageToLog("Level Aggregator: No target labels defined!", 1);
        return;
    }
    
    // Parse chart configuration
    std::vector<ChartConfig> chartsToScan;
    SCString chartConfigStr = Input_ChartConfig.GetString();
    
    if (chartConfigStr.IsEmpty())
    {
        ChartConfig cfg;
        cfg.ChartNumber = sc.ChartNumber;
        cfg.CustomName = sc.GetChartSymbol(sc.ChartNumber);
        cfg.Priority = 0;
        chartsToScan.push_back(cfg);
    }
    else
    {
        char configBuffer[1024];
        strncpy(configBuffer, chartConfigStr.GetChars(), sizeof(configBuffer) - 1);
        configBuffer[sizeof(configBuffer) - 1] = '\0';
        
        char* token = strtok(configBuffer, ",");
        int entryCount = 0;
        
        while (token != NULL)
        {
            entryCount++;
            while (*token == ' ') token++;
            
            char* end = token + strlen(token) - 1;
            while (end > token && *end == ' ') { *end = '\0'; end--; }
            
            char* pipePos = strchr(token, '|');
            
            ChartConfig cfg;
            cfg.Priority = entryCount - 1;
            
            if (pipePos != NULL)
            {
                *pipePos = '\0';
                char* chartNumStr = token;
                char* customName = pipePos + 1;
                
                cfg.ChartNumber = atoi(chartNumStr);
                cfg.CustomName = customName;
            }
            else
            {
                cfg.ChartNumber = atoi(token);
                cfg.CustomName = sc.GetChartSymbol(cfg.ChartNumber);
            }
            
            if (cfg.ChartNumber > 0)
                chartsToScan.push_back(cfg);
            
            token = strtok(NULL, ",");
        }
    }
    
    // Scan all charts for matching levels
    std::vector<LevelEntry> allLevels;
    std::vector<bool> labelFoundGlobally(targetLabels.size(), false);
    
    for (size_t ci = 0; ci < chartsToScan.size(); ci++)
    {
        ChartConfig& chartCfg = chartsToScan[ci];
        int chartNum = chartCfg.ChartNumber;
        int drawingIndex = 0;
        s_UseTool Drawing;
        
        while (sc.GetUserDrawnChartDrawing(chartNum, 0, Drawing, drawingIndex) != 0)
        {
            if (Drawing.Text.GetLength() > 0)
            {
                char textBuffer[512];
                strncpy(textBuffer, Drawing.Text.GetChars(), sizeof(textBuffer) - 1);
                textBuffer[sizeof(textBuffer) - 1] = '\0';
                
                char* labelPart = textBuffer;
                char* descriptionPart = NULL;
                
                char* pipePos = strchr(textBuffer, '|');
                if (pipePos != NULL)
                {
                    *pipePos = '\0';
                    descriptionPart = pipePos + 1;
                }
                
                while (*labelPart == ' ') labelPart++;
                
                if (strlen(labelPart) > 0)
                {
                    char* end = labelPart + strlen(labelPart) - 1;
                    while (end > labelPart && *end == ' ') { *end = '\0'; end--; }
                }
                
                if (descriptionPart != NULL)
                {
                    while (*descriptionPart == ' ') descriptionPart++;
                    if (strlen(descriptionPart) > 0)
                    {
                        char* end = descriptionPart + strlen(descriptionPart) - 1;
                        while (end > descriptionPart && *end == ' ') { *end = '\0'; end--; }
                    }
                }
                
                for (size_t li = 0; li < targetLabels.size(); li++)
                {
                    if (labelFoundGlobally[li] && !labelFindAll[li])
                        continue;
                    
                    if (strstr(labelPart, targetLabels[li].GetChars()) != NULL)
                    {
                        LevelEntry entry;
                        entry.Label = targetLabels[li];
                        entry.Description = (descriptionPart != NULL && strlen(descriptionPart) > 0) 
                                          ? descriptionPart : labelPart;
                        entry.SourceChart = chartNum;
                        entry.ChartName = chartCfg.CustomName;
                        entry.Price = Drawing.BeginValue;
                        entry.ChartPriority = chartCfg.Priority;
                        
                        allLevels.push_back(entry);
                        labelFoundGlobally[li] = true;
                        break;
                    }
                }
            }
            
            drawingIndex++;
            if (drawingIndex > 10000) break;
        }
    }
    
    // Sort levels
    int sort1 = Input_SortBy1.GetIndex();
    int sort2 = Input_SortBy2.GetIndex();

    auto CompareLevels = [](const LevelEntry& a, const LevelEntry& b, int sortType) -> int {
        switch (sortType) {
            case 0: // Price Descending
                if (a.Price > b.Price) return 1;
                if (a.Price < b.Price) return -1;
                return 0;
            case 1: // Price Ascending
                if (a.Price < b.Price) return 1;
                if (a.Price > b.Price) return -1;
                return 0;
            case 2: // Chart Priority
                if (a.ChartPriority < b.ChartPriority) return 1;
                if (a.ChartPriority > b.ChartPriority) return -1;
                return 0;
            case 3: // Label
                if (a.Label.CompareNoCase(b.Label) < 0) return 1;
                if (a.Label.CompareNoCase(b.Label) > 0) return -1;
                return 0;
            case 4: // Description
                if (a.Description.CompareNoCase(b.Description) < 0) return 1;
                if (a.Description.CompareNoCase(b.Description) > 0) return -1;
                return 0;
            case 5: // Chart Name
                if (a.ChartName.CompareNoCase(b.ChartName) < 0) return 1;
                if (a.ChartName.CompareNoCase(b.ChartName) > 0) return -1;
                return 0;
        }
        return 0;
    };

    std::sort(allLevels.begin(), allLevels.end(), [sort1, sort2, CompareLevels](const LevelEntry& a, const LevelEntry& b) {
        int r = CompareLevels(a, b, sort1);
        if (r != 0) return r == 1;
        r = CompareLevels(a, b, sort2);
        if (r != 0) return r == 1;
        return false;
    });
    
    // Store results
    p_State->Levels = allLevels;
    p_State->HasScanned = true;
    p_State->LastScanTime = sc.CurrentSystemDateTime;
    p_State->LastHighIdx = -1;  // Force redraw
    p_State->LastLowIdx = -1;
    p_State->ForceRedraw = true;
    
    // Force immediate table draw
    showTable = Input_ShowTable.GetYesNo() && p_State->IsTableVisible;
    DrawTable(sc, p_State, showTable, Input_TableX.GetInt(), Input_TableY.GetInt(),
              Input_FontSize.GetInt(), Input_FontColor.GetColor(), 
              Input_BgColor.GetColor(), Input_HighlightColor.GetColor(),
              Input_TableRangeLevels.GetInt(), true);
    p_State->ForceRedraw = false;

    // Log summary
    sc.AddMessageToLog(SCString().Format("Level Aggregator: Scanned %d chart(s), found %d level(s)", 
                       (int)chartsToScan.size(), (int)allLevels.size()), 0);
    
    // --- EXPORT LOGIC (Only if enabled) ---
    if (!Input_ExportOnScan.GetYesNo())
        return;
    
    // Build output string
    SCString output;
    int includeChartName = Input_IncludeChartName.GetYesNo();
    int includeDescription = Input_IncludeDesc.GetYesNo();
    
    if (Input_UseTemplate.GetYesNo())
    {
        SCString templatePath = Input_TemplatePath.GetPathAndFileName();
        
        HANDLE hFile = CreateFile(templatePath.GetChars(), GENERIC_READ, FILE_SHARE_READ, 
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        
        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD fileSize = GetFileSize(hFile, NULL);
            if (fileSize > 0 && fileSize < 65536)
            {
                char* templateBuffer = new char[fileSize + 1];
                DWORD bytesRead;
                ReadFile(hFile, templateBuffer, fileSize, &bytesRead, NULL);
                templateBuffer[bytesRead] = '\0';
                CloseHandle(hFile);
                
                // Remove carriage returns
                char* writePtr = templateBuffer;
                char* readPtr = templateBuffer;
                while (*readPtr)
                {
                    if (*readPtr != '\r')
                    {
                        *writePtr = *readPtr;
                        writePtr++;
                    }
                    readPtr++;
                }
                *writePtr = '\0';
                bytesRead = (DWORD)(writePtr - templateBuffer);
                
                const char* templateData = templateBuffer;
                const char* levelLinePtr = NULL;
                
                if (strncmp(templateData, "{{LEVEL_LINE}}", 14) == 0)
                    levelLinePtr = templateData;
                else
                {
                    const char* searchStart = templateData;
                    while ((searchStart = strstr(searchStart, "\n{{LEVEL_LINE}}")) != NULL)
                    {
                        levelLinePtr = searchStart + 1;
                        break;
                    }
                }
                
                if (levelLinePtr != NULL)
                {
                    // Extract header
                    SCString header;
                    const char* lineStart = templateData;
                    while (lineStart < levelLinePtr)
                    {
                        const char* lineEnd = strchr(lineStart, '\n');
                        if (lineEnd == NULL || lineEnd > levelLinePtr)
                            lineEnd = levelLinePtr;
                        
                        if (lineStart < lineEnd && *lineStart != '#')
                        {
                            size_t lineLen = lineEnd - lineStart;
                            char lineBuf[1024];
                            if (lineLen >= sizeof(lineBuf)) lineLen = sizeof(lineBuf) - 1;
                            strncpy(lineBuf, lineStart, lineLen);
                            lineBuf[lineLen] = '\0';
                            
                            if (lineLen > 0)
                            {
                                header += lineBuf;
                                if (lineEnd < levelLinePtr) header += "\n";
                            }
                        }
                        
                        if (*lineEnd == '\n')
                            lineStart = lineEnd + 1;
                        else
                            break;
                    }
                    
                    // Extract level line template
                    const char* levelLineContent = levelLinePtr + 14;
                    const char* levelLineEnd = strchr(levelLineContent, '\n');
                    if (levelLineEnd == NULL) levelLineEnd = templateData + bytesRead;
                    
                    size_t levelTemplateLen = levelLineEnd - levelLineContent;
                    char levelTemplateBuf[1024];
                    if (levelTemplateLen >= sizeof(levelTemplateBuf)) levelTemplateLen = sizeof(levelTemplateBuf) - 1;
                    strncpy(levelTemplateBuf, levelLineContent, levelTemplateLen);
                    levelTemplateBuf[levelTemplateLen] = '\0';
                    
                    // Extract footer
                    SCString footer;
                    if (*levelLineEnd == '\n')
                    {
                        lineStart = levelLineEnd + 1;
                        const char* dataEnd = templateData + bytesRead;
                        
                        while (lineStart < dataEnd)
                        {
                            const char* lineEnd = strchr(lineStart, '\n');
                            if (lineEnd == NULL) lineEnd = dataEnd;
                            
                            if (lineStart < lineEnd && *lineStart != '#')
                            {
                                size_t lineLen = lineEnd - lineStart;
                                char lineBuf[1024];
                                if (lineLen >= sizeof(lineBuf)) lineLen = sizeof(lineBuf) - 1;
                                strncpy(lineBuf, lineStart, lineLen);
                                lineBuf[lineLen] = '\0';
                                
                                if (lineLen > 0)
                                {
                                    footer += lineBuf;
                                    footer += "\n";
                                }
                            }
                            
                            if (*lineEnd == '\n')
                                lineStart = lineEnd + 1;
                            else
                                break;
                        }
                    }
                    
                    delete[] templateBuffer;
                    
                    // Build output
                    output = header;
                    
                    for (size_t i = 0; i < allLevels.size(); i++)
                    {
                        SCString displayName;
                        if (includeDescription && allLevels[i].Description.GetLength() > 0)
                            displayName = allLevels[i].Description;
                        else
                            displayName = allLevels[i].Label;
                        
                        SCString priceStr;
                        priceStr.Format("%.2f", allLevels[i].Price);
                        
                        SCString chartNumStr;
                        chartNumStr.Format("%d", allLevels[i].SourceChart);
                        
                        SCString result;
                        const char* p = levelTemplateBuf;
                        while (*p)
                        {
                            if (strncmp(p, "{{LABEL}}", 9) == 0) {
                                result += allLevels[i].Label;
                                p += 9;
                            } else if (strncmp(p, "{{DESCRIPTION}}", 15) == 0) {
                                result += allLevels[i].Description;
                                p += 15;
                            } else if (strncmp(p, "{{DISPLAY_NAME}}", 16) == 0) {
                                result += displayName;
                                p += 16;
                            } else if (strncmp(p, "{{PRICE}}", 9) == 0) {
                                result += priceStr;
                                p += 9;
                            } else if (strncmp(p, "{{CHART_NAME}}", 14) == 0) {
                                result += allLevels[i].ChartName;
                                p += 14;
                            } else if (strncmp(p, "{{CHART_NUMBER}}", 16) == 0) {
                                result += chartNumStr;
                                p += 16;
                            } else {
                                result += *p;
                                p++;
                            }
                        }
                        
                        output += result;
                        output += "\n";
                    }
                    
                    output += footer;
                }
                else
                {
                    delete[] templateBuffer;
                    sc.AddMessageToLog("Template missing {{LEVEL_LINE}} marker - using built-in format.", 1);
                    goto use_builtin;
                }
            }
            else
            {
                CloseHandle(hFile);
                sc.AddMessageToLog("Template file issue - using built-in format.", 1);
                goto use_builtin;
            }
        }
        else
        {
            sc.AddMessageToLog("Template file not found - using built-in format.", 1);
            goto use_builtin;
        }
    }
    else
    {
use_builtin:
        output += "---\n# Levels Export\n\n";
        
        for (size_t i = 0; i < allLevels.size(); i++)
        {
            SCString line;
            
            SCString displayName;
            if (includeDescription && allLevels[i].Description.GetLength() > 0)
                displayName = allLevels[i].Description;
            else
                displayName = allLevels[i].Label;
            
            if (includeChartName)
                line.Format("%s (%s):: %.2f\n", displayName.GetChars(), allLevels[i].ChartName.GetChars(), allLevels[i].Price);
            else
                line.Format("%s:: %.2f\n", displayName.GetChars(), allLevels[i].Price);
            
            output += line;
        }
        
        output += "---\n";
    }
    
    // Output to destination
    int outputDest = Input_OutputDest.GetIndex();
    bool writeToClipboard = (outputDest == 0 || outputDest == 2);
    bool writeToFile = (outputDest == 1 || outputDest == 2);
    
    if (writeToClipboard)
    {
        if (OpenClipboard(NULL))
        {
            EmptyClipboard();
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, output.GetLength() + 1);
            if (hMem != NULL)
            {
                char* pMem = (char*)GlobalLock(hMem);
                if (pMem != NULL)
                {
                    strcpy(pMem, output.GetChars());
                    GlobalUnlock(hMem);
                    SetClipboardData(CF_TEXT, hMem);
                    sc.AddMessageToLog("Exported to clipboard.", 0);
                }
                else
                    GlobalFree(hMem);
            }
            CloseClipboard();
        }
    }
    
    if (writeToFile)
    {
        SCString filePath = Input_FilePath.GetPathAndFileName();
        
        HANDLE fileHandle = CreateFile(filePath.GetChars(), GENERIC_WRITE, 0, NULL, 
                                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        
        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            DWORD bytesWritten;
            WriteFile(fileHandle, output.GetChars(), output.GetLength(), &bytesWritten, NULL);
            CloseHandle(fileHandle);
            sc.AddMessageToLog(SCString().Format("Exported to: %s", filePath.GetChars()), 0);
        }
        else
        {
            sc.AddMessageToLog(SCString().Format("ERROR: Could not write to: %s", filePath.GetChars()), 1);
        }
    }
}
