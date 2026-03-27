#include "sierrachart.h"
#include <vector>
#include <algorithm>
#include <string>

/*============================================================================
    ObsidianLevelExporter - Sierra Chart Study
    
    Purpose: Scans user-drawn chart objects and exports levels with specific 
             labels to Obsidian Markdown format.
    
    Drawing Label Format: LABEL|Description (e.g., "BZ|Top of 4x Balance")
      - LABEL is searched for, Description is exported
    
    Chart Settings Format: ChartNum|CustomName (e.g., "1|1 MIN Chart,9|Weekly")
      - ChartNum is the chart to scan, CustomName is shown in export
    
    Output Format: Label:: Price - Description (Chart Name)
    Trigger: ACS Control Bar button press (runs once per click)
============================================================================*/

SCDLLName("Obsidian Level Exporter")

// Structure to hold a level for sorting
struct LevelEntry
{
    SCString Label;
    SCString Description;
    double Price;
    int SourceChart;
    SCString ChartName;
    int ChartPriority;  // Order as defined in chart config (0 = first chart listed)
};

// Structure to hold chart config
struct ChartConfig
{
    int ChartNumber;
    SCString CustomName;
    int Priority;  // Order as defined (0 = first)
};

// Persistent state to hold levels for display
struct GlobalState
{
    std::vector<LevelEntry> Levels;
    bool HasScanned = false;
    SCDateTime LastScanTime;
    bool IsTableVisible = false;
};

void DrawTable(SCStudyInterfaceRef sc, GlobalState* p_State, int showTable, int xPos, int yPos, int fontSize, int fontColorInt, int bgColorInt, int highlightColorInt)
{
    // If not showing, delete any existing drawings and return
    if (!showTable || !p_State->HasScanned || p_State->Levels.empty())
    {
        // Use DeleteACSChartDrawing for drawings made with UseTool
        sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, 98765432);
        sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, 98765433);
        return;
    }
    
    // Calculate Next High and Next Low indices based on current price
    int idxHigh = -1;
    int idxLow = -1;
    double minDistHigh = DBL_MAX;
    double minDistLow = DBL_MAX;
    double currentPrice = sc.Close[sc.ArraySize - 1];
    
    for (size_t i = 0; i < p_State->Levels.size(); ++i)
    {
        double diff = p_State->Levels[i].Price - currentPrice;
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

    // Build clean table text - no ASCII borders, just aligned columns
    SCString tableTextNormal;
    SCString tableTextHighlight;
    
    // Column widths for clean spacing
    const int wPrice = 11;
    const int wChart = 13;
    const int wDesc = 35;
    
    // Header
    SCString header;
    header.Format("%-*s %-*s %s\n",
        wPrice, "Price",
        wChart, "Chart",
        "Description");
    tableTextNormal += header;
    tableTextHighlight += "\n";
    
    // Separator line (simple dashes)
    SCString separator;
    separator.Format("%s\n", std::string(wPrice + wChart + wDesc + 2, '-').c_str());
    tableTextNormal += separator;
    tableTextHighlight += "\n";
    
    // Data rows
    for (size_t i = 0; i < p_State->Levels.size(); ++i)
    {
        const auto& level = p_State->Levels[i];
        
        SCString priceStr = sc.FormatGraphValue(level.Price, sc.BaseGraphValueFormat);
        
        SCString chartStr = level.ChartName;
        if (chartStr.GetLength() > wChart) chartStr = chartStr.Left(wChart - 1);
        
        SCString descStr = level.Description;
        if (descStr.GetLength() > wDesc) descStr = descStr.Left(wDesc - 1);
        
        SCString row;
        row.Format("%-*s %-*s %s\n", 
            wPrice, priceStr.GetChars(), 
            wChart, chartStr.GetChars(),
            descStr.GetChars());
        
        bool isHighlight = (i == idxHigh || i == idxLow);
        
        // Normal text draws all rows
        tableTextNormal += row;
        
        // Highlight text draws only highlighted rows
        if (isHighlight)
            tableTextHighlight += row;
        else
            tableTextHighlight += "\n";
    }
    
    // Setup positioning (shared for both tools)
    s_UseTool Tool;
    Tool.Clear();
    Tool.ChartNumber = sc.ChartNumber;
    Tool.DrawingType = DRAWING_TEXT;
    Tool.UseRelativeVerticalValues = 1;
    
    // X position: anchor to first visible bar
    int xOffsetBars = xPos / 12;
    if (xOffsetBars < 0) xOffsetBars = 0;
    int startBar = sc.IndexOfFirstVisibleBar + xOffsetBars;
    if (startBar >= sc.ArraySize) startBar = sc.ArraySize - 1;
    if (startBar < 0) startBar = 0;
    Tool.BeginDateTime = sc.BaseDateTimeIn[startBar];
    
    // Y position: convert pixels to percentage
    double yPercent = (double)yPos / 10.0;
    Tool.BeginValue = 100.0 - yPercent;
    if (Tool.BeginValue > 98) Tool.BeginValue = 98;
    if (Tool.BeginValue < 2) Tool.BeginValue = 2;
    
    // Common properties
    Tool.FontSize = fontSize;
    Tool.FontFace = "Courier New";
    Tool.FontBold = 1;
    Tool.AddMethod = UTAM_ADD_OR_ADJUST;
    
    // Draw normal text (base layer)
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
SCSFExport scsf_ObsidianLevelExporter(SCStudyInterfaceRef sc)
{
    // Input references
    SCInputRef Input_ACSButtonNumber = sc.Input[0];
    SCInputRef Input_LabelFilters = sc.Input[1];
    SCInputRef Input_ChartConfig = sc.Input[2];
    SCInputRef Input_OutputDestination = sc.Input[3];
    SCInputRef Input_FilePath = sc.Input[4];
    SCInputRef Input_IncludeChartName = sc.Input[5];
    SCInputRef Input_IncludeDescription = sc.Input[6];
    SCInputRef Input_ButtonText = sc.Input[7];
    SCInputRef Input_UseTemplate = sc.Input[8];
    SCInputRef Input_TemplatePath = sc.Input[9];
    SCInputRef Input_SortBy1 = sc.Input[10];
    SCInputRef Input_SortBy2 = sc.Input[11];
    
    // Table Display Inputs
    SCInputRef Input_ShowTable = sc.Input[12];
    SCInputRef Input_TableX = sc.Input[13];
    SCInputRef Input_TableY = sc.Input[14];
    SCInputRef Input_FontSize = sc.Input[15];
    SCInputRef Input_FontColor = sc.Input[16];
    SCInputRef Input_BackgroundColor = sc.Input[17];
    
    // Set configuration
    if (sc.SetDefaults)
    {
        sc.GraphName = "Obsidian Level Exporter";
        sc.StudyDescription = 
            "Scans user-drawn chart objects and exports levels to Obsidian format.\n\n"
            "DRAWING LABEL FORMAT: Label|Description\n"
            "  Example: BZ|Top of 4x Balance\n"
            "  - 'BZ' is matched against Target Labels\n"
            "  - 'Top of 4x Balance' is exported as the description\n\n"
            "CHART CONFIG FORMAT: ChartNum|CustomName (comma-separated)\n"
            "  Example: 1|1 MIN,9|Weekly,3|Daily TPO\n"
            "  - ChartNum must be a number (the chart number)\n"
            "  - CustomName is optional, uses symbol if omitted\n\n"
            "LABEL MODIFIER: Add |All to find all instances\n"
            "  Example: PDC,Export|All - PDC finds first, Export finds all";
        sc.AutoLoop = 0;
        sc.GraphRegion = 0;
        sc.UpdateAlways = 1;
        
        Input_ACSButtonNumber.Name = "ACS Control Bar Button # (1-150)";
        Input_ACSButtonNumber.SetInt(1);
        Input_ACSButtonNumber.SetIntLimits(1, 150);
        Input_ACSButtonNumber.DisplayOrder = 1;
        
        Input_LabelFilters.Name = "Target Labels (comma-separated, |All for all instances)";
        Input_LabelFilters.SetString(">>|All");
        Input_LabelFilters.DisplayOrder = 3;
        
        Input_ChartConfig.Name = "Charts: ChartNum|Name (e.g., 1|1MIN,9|Weekly)";
        Input_ChartConfig.SetString("1|TPO,9|1 MIN,3|Daily TPO,5|Weekly,13|30 MIN,8|4 HR");
        Input_ChartConfig.DisplayOrder = 4;
        
        Input_OutputDestination.Name = "Output Destination";
        Input_OutputDestination.SetCustomInputStrings("Log Only;Clipboard;File;Clipboard+File");
        Input_OutputDestination.SetCustomInputIndex(1);  // Default to Clipboard
        Input_OutputDestination.DisplayOrder = 5;
        
        Input_FilePath.Name = "Output File Path (if File selected)";
        Input_FilePath.SetPathAndFileName("C:\\temp\\obsidian_levels.txt");
        Input_FilePath.DisplayOrder = 6;
        
        Input_IncludeChartName.Name = "Include Chart Name in Output";
        Input_IncludeChartName.SetYesNo(1);
        Input_IncludeChartName.DisplayOrder = 9;
        
        Input_IncludeDescription.Name = "Include Description in Output";
        Input_IncludeDescription.SetYesNo(1);
        Input_IncludeDescription.DisplayOrder = 10;
        
        Input_ButtonText.Name = "Button Text";
        Input_ButtonText.SetString("Export");
        Input_ButtonText.DisplayOrder = 2;
        
        Input_UseTemplate.Name = "Use External Template File";
        Input_UseTemplate.SetYesNo(0);
        Input_UseTemplate.DisplayOrder = 11;
        
        Input_TemplatePath.Name = "Template File Path";
        Input_TemplatePath.SetPathAndFileName("C:\\temp\\obsidian_template.txt");
        Input_TemplatePath.DisplayOrder = 12;
        
        Input_SortBy1.Name = "First Sort By";
        Input_SortBy1.SetCustomInputStrings("Chart Priority;Price Descending;Price Ascending;Label;Description;Chart Name");
        Input_SortBy1.SetCustomInputIndex(0); // Default: Chart Priority
        Input_SortBy1.DisplayOrder = 7;
        
        Input_SortBy2.Name = "Second Sort By";
        Input_SortBy2.SetCustomInputStrings("Chart Priority;Price Descending;Price Ascending;Label;Description;Chart Name");
        Input_SortBy2.SetCustomInputIndex(1); // Default: Price Descending
        Input_SortBy2.DisplayOrder = 8;
        
        Input_ShowTable.Name = "Show Table on Chart";
        Input_ShowTable.SetYesNo(1);
        Input_ShowTable.DisplayOrder = 20;

        Input_TableX.Name = "Table X Position (Pixels)";
        Input_TableX.SetInt(20);
        Input_TableX.DisplayOrder = 21;

        Input_TableY.Name = "Table Y Position (Pixels)";
        Input_TableY.SetInt(60);
        Input_TableY.DisplayOrder = 22;
        
        Input_FontSize.Name = "Table Font Size";
        Input_FontSize.SetInt(10);
        Input_FontSize.DisplayOrder = 23;

        Input_FontColor.Name = "Text Color";
        Input_FontColor.SetColor(RGB(220, 220, 220));
        Input_FontColor.DisplayOrder = 24;

        Input_BackgroundColor.Name = "Background Color";
        Input_BackgroundColor.SetColor(RGB(0, 0, 0));
        Input_BackgroundColor.DisplayOrder = 25;
        
        SCInputRef Input_HighlightColor = sc.Input[18];
        Input_HighlightColor.Name = "Highlight Color (Next High/Low)";
        Input_HighlightColor.SetColor(RGB(255, 255, 0)); // Yellow default
        Input_HighlightColor.DisplayOrder = 26;
        
        return;
    }
    
    // Manage persistent state
    GlobalState* p_State = (GlobalState*)sc.GetPersistentPointer(1);
    if (p_State == NULL)
    {
        p_State = new GlobalState;
        sc.SetPersistentPointer(1, p_State);
    }
    
    // Set the button text
    int buttonNum = Input_ACSButtonNumber.GetInt();
    sc.SetCustomStudyControlBarButtonText(buttonNum, Input_ButtonText.GetString());
    int buttonState = sc.GetCustomStudyControlBarButtonEnableState(buttonNum);
    
    bool runScan = false;
    
    // Debounce/Toggle inputs
    int& hasRunThisPress = sc.GetPersistentInt(0);
    SCDateTime& lastRunTime = sc.GetPersistentSCDateTime(0);
    SCDateTime currentTime = sc.CurrentSystemDateTime;
    double timeDiff = currentTime.GetAsDouble() - lastRunTime.GetAsDouble();
    double secondsSinceLastRun = currentTime.IsUnset() ? 999 : (timeDiff * 86400.0);
    
    // Simple, reliable button toggle logic
    if (buttonState == 1 && hasRunThisPress == 0)
    {
        // Toggle visibility
        p_State->IsTableVisible = !p_State->IsTableVisible;
        hasRunThisPress = 1;
        
        // If turning ON, trigger scan
        if (p_State->IsTableVisible)
        {
            runScan = true;
        }
        
        // Reset button
        sc.SetCustomStudyControlBarButtonEnable(buttonNum, 0);
    }
    else if (buttonState == 0)
    {
        // Reset press flag when button is off
        hasRunThisPress = 0;
    }

    // --- DRAWING LOGIC ---
    // Pass IsTableVisible (combined with user Input master override)
    bool showTable = Input_ShowTable.GetYesNo() && p_State->IsTableVisible;
    
    // Always call DrawTable so it can handle clearing if disabled
    // And to update the highlighting in real-time based on current price
    DrawTable(sc, p_State, showTable, Input_TableX.GetInt(), Input_TableY.GetInt(),
              Input_FontSize.GetInt(), Input_FontColor.GetColor(), Input_BackgroundColor.GetColor(), sc.Input[18].GetColor());
    
    // --- SCAN & EXPORT LOGIC ---
    // If not triggered by button, we are done for this tick
    if (!runScan) 
        return;
    
    // --- BELOW IS THE EXISTING SCAN LOGIC, WHICH WILL NOW RUN ONLY WHEN runScan IS TRUE ---

    
    // DEBUG: Show all raw input values
    sc.AddMessageToLog("=== DEBUG: INPUT VALUES ===", 0);
    sc.AddMessageToLog(SCString().Format("Button #%d pressed (state=%d)", buttonNum, buttonState), 0);
    sc.AddMessageToLog(SCString().Format("Raw Labels: '%s'", Input_LabelFilters.GetString()), 0);
    sc.AddMessageToLog(SCString().Format("Raw Chart Config: '%s'", Input_ChartConfig.GetString()), 0);
    
    // Parse target labels
    // Format: "PDC,PDH,Export|All,BZone|All" - |All suffix means find all instances per chart
    std::vector<SCString> targetLabels;
    std::vector<bool> labelFindAll;  // Parallel vector: true = find all, false = first only
    SCString labelFiltersStr = Input_LabelFilters.GetString();
    
    sc.AddMessageToLog(SCString().Format("Label string length: %d", labelFiltersStr.GetLength()), 0);
    
    if (!labelFiltersStr.IsEmpty())
    {
        // Copy to buffer for parsing
        char labelBuffer[1024];
        strncpy(labelBuffer, labelFiltersStr.GetChars(), sizeof(labelBuffer) - 1);
        labelBuffer[sizeof(labelBuffer) - 1] = '\0';
        
        char* token = strtok(labelBuffer, ",");
        int labelIndex = 0;
        
        while (token != NULL)
        {
            // Trim leading whitespace
            while (*token == ' ') token++;
            
            // Trim trailing whitespace
            if (strlen(token) > 0)
            {
                char* end = token + strlen(token) - 1;
                while (end > token && *end == ' ') { *end = '\0'; end--; }
            }
            
            if (strlen(token) > 0)
            {
                // Check for |All suffix
                bool findAll = false;
                char* pipePos = strstr(token, "|All");
                if (pipePos != NULL && strlen(pipePos) == 4)  // "|All" at end
                {
                    *pipePos = '\0';  // Remove the |All suffix
                    findAll = true;
                }
                
                targetLabels.push_back(SCString(token));
                labelFindAll.push_back(findAll);
                
                sc.AddMessageToLog(SCString().Format("  Label[%d]: '%s' %s", 
                                   labelIndex, token, findAll ? "[FIND ALL]" : "[FIRST ONLY]"), 0);
                labelIndex++;
            }
            
            token = strtok(NULL, ",");
        }
    }
    
    if (targetLabels.empty())
    {
        sc.AddMessageToLog("Obsidian Level Exporter: No target labels defined!", 1);
        return;
    }
    
    // Parse chart configuration: "1|1 MIN,9|Weekly" (comma-separated)
    std::vector<ChartConfig> chartsToScan;
    SCString chartConfigStr = Input_ChartConfig.GetString();
    SCString validationErrors;
    
    sc.AddMessageToLog(SCString().Format("DEBUG: Raw chart config: '%s'", chartConfigStr.GetChars()), 0);
    
    if (chartConfigStr.IsEmpty())
    {
        // Default: current chart with auto name
        ChartConfig cfg;
        cfg.ChartNumber = sc.ChartNumber;
        cfg.CustomName = sc.GetChartSymbol(sc.ChartNumber);
        cfg.Priority = 0;
        chartsToScan.push_back(cfg);
        sc.AddMessageToLog(SCString().Format("DEBUG: Using current chart #%d", sc.ChartNumber), 0);
    }
    else
    {
        // Make a copy of the string since strtok modifies it
        char configBuffer[1024];
        strncpy(configBuffer, chartConfigStr.GetChars(), sizeof(configBuffer) - 1);
        configBuffer[sizeof(configBuffer) - 1] = '\0';
        
        // Use comma as chart separator
        char* token = strtok(configBuffer, ",");
        int entryCount = 0;
        
        while (token != NULL)
        {
            entryCount++;
            
            // Trim leading whitespace
            while (*token == ' ') token++;
            
            // Trim trailing whitespace
            char* end = token + strlen(token) - 1;
            while (end > token && *end == ' ') { *end = '\0'; end--; }
            
            sc.AddMessageToLog(SCString().Format("DEBUG: Entry %d: '%s'", entryCount, token), 0);
            
            // Find the pipe separator
            char* pipePos = strchr(token, '|');
            
            ChartConfig cfg;
            cfg.Priority = entryCount - 1;  // Priority based on order in settings (0 = first)
            
            if (pipePos != NULL)
            {
                // Check for multiple pipes (invalid format)
                char* secondPipe = strchr(pipePos + 1, '|');
                if (secondPipe != NULL)
                {
                    SCString error;
                    error.Format("ERROR: Entry '%s' has multiple pipes - format is ChartNum|Name\n", token);
                    validationErrors += error;
                    sc.AddMessageToLog(error.GetChars(), 1);
                    token = strtok(NULL, ",");
                    continue;
                }
                
                // Split at pipe
                *pipePos = '\0';  // Terminate chart number string
                char* chartNumStr = token;
                char* customName = pipePos + 1;
                
                // Validate chart number is numeric
                bool isNumeric = true;
                for (char* c = chartNumStr; *c; c++)
                {
                    if (*c != ' ' && (*c < '0' || *c > '9'))
                    {
                        isNumeric = false;
                        break;
                    }
                }
                
                if (!isNumeric)
                {
                    SCString error;
                    error.Format("ERROR: '%s' is not a valid chart number (must be numeric)\n", chartNumStr);
                    validationErrors += error;
                    sc.AddMessageToLog(error.GetChars(), 1);
                    token = strtok(NULL, ",");
                    continue;
                }
                
                cfg.ChartNumber = atoi(chartNumStr);
                cfg.CustomName = customName;
                
                sc.AddMessageToLog(SCString().Format("DEBUG: Parsed chart #%d with name '%s' (priority %d)", 
                                   cfg.ChartNumber, customName, cfg.Priority), 0);
            }
            else
            {
                // Just chart number, use symbol as name
                cfg.ChartNumber = atoi(token);
                cfg.CustomName = sc.GetChartSymbol(cfg.ChartNumber);
                sc.AddMessageToLog(SCString().Format("DEBUG: Parsed chart #%d (no custom name, priority %d)", 
                                   cfg.ChartNumber, cfg.Priority), 0);
            }
            
            if (cfg.ChartNumber > 0)
            {
                chartsToScan.push_back(cfg);
            }
            else
            {
                SCString error;
                error.Format("ERROR: Invalid chart entry '%s' - chart number must be > 0\n", token);
                validationErrors += error;
                sc.AddMessageToLog(error.GetChars(), 1);
            }
            
            token = strtok(NULL, ",");
        }
        
        sc.AddMessageToLog(SCString().Format("DEBUG: Total chart entries parsed: %d", entryCount), 0);
    }
    
    // Check for validation errors
    if (!validationErrors.IsEmpty())
    {
        SCString helpMessage;
        helpMessage = "=== OBSIDIAN EXPORTER: CONFIGURATION ERRORS ===\n\n";
        helpMessage += validationErrors;
        helpMessage += "\n--- CORRECT FORMAT ---\n\n";
        helpMessage += "CHART CONFIG: ChartNum|CustomName (comma-separated)\n";
        helpMessage += "  Valid examples:\n";
        helpMessage += "    1|1 MIN,9|Weekly,3|Daily TPO\n";
        helpMessage += "    5|Main Chart,12|Volume Profile\n";
        helpMessage += "    7  (uses symbol as name)\n\n";
        helpMessage += "  - ChartNum must be a positive integer (the Sierra Chart number)\n";
        helpMessage += "  - CustomName is optional text after the pipe\n";
        helpMessage += "  - Entries are separated by commas\n";
        helpMessage += "  - Do NOT use pipes in custom names\n\n";
        helpMessage += "LABELS: Label (comma-separated, add |All for all instances)\n";
        helpMessage += "  Valid examples:\n";
        helpMessage += "    PDC,PDH,PDL,BZ\n";
        helpMessage += "    Support|All,Resistance|All,PDC\n";
        
        // Copy to clipboard
        if (OpenClipboard(NULL))
        {
            EmptyClipboard();
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, helpMessage.GetLength() + 1);
            if (hMem)
            {
                char* pMem = (char*)GlobalLock(hMem);
                if (pMem)
                {
                    strcpy(pMem, helpMessage.GetChars());
                    GlobalUnlock(hMem);
                    SetClipboardData(CF_TEXT, hMem);
                }
            }
            CloseClipboard();
        }
        
        sc.AddMessageToLog("*** CONFIGURATION ERRORS - Help copied to clipboard ***", 1);
        MessageBeep(MB_ICONERROR);
        return;
    }
    
    // Collect all matching levels from all charts
    std::vector<LevelEntry> allLevels;
    
    // Track which labels have been found GLOBALLY (for non-All labels, only first instance is exported)
    std::vector<bool> labelFoundGlobally(targetLabels.size(), false);
    
    sc.AddMessageToLog(SCString().Format("DEBUG: Starting scan of %d chart(s)...", (int)chartsToScan.size()), 0);
    
    for (size_t ci = 0; ci < chartsToScan.size(); ci++)
    {
        ChartConfig& chartCfg = chartsToScan[ci];
        int chartNum = chartCfg.ChartNumber;
        int drawingIndex = 0;
        s_UseTool Drawing;
        int totalDrawingsOnChart = 0;
        int matchingDrawingsOnChart = 0;
        
        sc.AddMessageToLog(SCString().Format("DEBUG: Scanning Chart #%d (%s)...", 
                           chartNum, chartCfg.CustomName.GetChars()), 0);
        
        while (sc.GetUserDrawnChartDrawing(chartNum, 0, Drawing, drawingIndex) != 0)
        {
            totalDrawingsOnChart++;
            
            // Check if this drawing has text
            if (Drawing.Text.GetLength() > 0)
            {
                // Use C string functions to parse - GetSubString has issues
                char textBuffer[512];
                strncpy(textBuffer, Drawing.Text.GetChars(), sizeof(textBuffer) - 1);
                textBuffer[sizeof(textBuffer) - 1] = '\0';
                
                char* labelPart = textBuffer;
                char* descriptionPart = NULL;
                
                // Check for pipe separator: "LABEL|Description"
                char* pipePos = strchr(textBuffer, '|');
                if (pipePos != NULL)
                {
                    *pipePos = '\0';  // Terminate label
                    descriptionPart = pipePos + 1;
                }
                
                // Trim leading whitespace from label
                while (*labelPart == ' ') labelPart++;
                
                // Trim trailing whitespace from label
                if (strlen(labelPart) > 0)
                {
                    char* end = labelPart + strlen(labelPart) - 1;
                    while (end > labelPart && *end == ' ') { *end = '\0'; end--; }
                }
                
                // Trim whitespace from description if present
                if (descriptionPart != NULL)
                {
                    while (*descriptionPart == ' ') descriptionPart++;
                    if (strlen(descriptionPart) > 0)
                    {
                        char* end = descriptionPart + strlen(descriptionPart) - 1;
                        while (end > descriptionPart && *end == ' ') { *end = '\0'; end--; }
                    }
                }
                
                // Check if label matches any target (that hasn't been found GLOBALLY yet, unless it's a FindAll label)
                for (size_t li = 0; li < targetLabels.size(); li++)
                {
                    // Skip if this label was already found GLOBALLY AND it's not a "find all" label
                    if (labelFoundGlobally[li] && !labelFindAll[li])
                        continue;
                    
                    // Check for exact match or if label contains target
                    if (strstr(labelPart, targetLabels[li].GetChars()) != NULL)
                    {
                        LevelEntry entry;
                        entry.Label = targetLabels[li];
                        // If descriptionPart is NULL (no pipe found), use the label part (the text itself) as the description
                        entry.Description = (descriptionPart != NULL && strlen(descriptionPart) > 0) ? descriptionPart : labelPart;
                        entry.SourceChart = chartNum;
                        entry.ChartName = chartCfg.CustomName;
                        entry.Price = Drawing.BeginValue;
                        entry.ChartPriority = chartCfg.Priority;
                        
                        allLevels.push_back(entry);
                        matchingDrawingsOnChart++;
                        
                        // Mark this label as found GLOBALLY (for non-FindAll labels, prevents duplicates across all charts)
                        labelFoundGlobally[li] = true;
                        
                        if (labelFindAll[li])
                        {
                            sc.AddMessageToLog(SCString().Format("  -> Found: %s at %.2f (Desc: %s) [find all]", 
                                               targetLabels[li].GetChars(), Drawing.BeginValue, 
                                               (descriptionPart != NULL) ? descriptionPart : ""), 0);
                        }
                        else
                        {
                            sc.AddMessageToLog(SCString().Format("  -> Found: %s at %.2f (Desc: %s) [first globally]", 
                                               targetLabels[li].GetChars(), Drawing.BeginValue, 
                                               (descriptionPart != NULL) ? descriptionPart : ""), 0);
                        }
                        break;
                    }
                }
            }
            
            drawingIndex++;
            if (drawingIndex > 10000) break;
        }
        
        sc.AddMessageToLog(SCString().Format("DEBUG: Chart #%d - Total drawings: %d, Matching: %d", 
                           chartNum, totalDrawingsOnChart, matchingDrawingsOnChart), 0);
    }
    
    // Debug: Show levels before sorting
    sc.AddMessageToLog("DEBUG: Levels BEFORE sorting:", 0);
    for (size_t i = 0; i < allLevels.size(); i++)
    {
        sc.AddMessageToLog(SCString().Format("  [%d] Priority=%d, Price=%.2f, Label=%s, Chart=%s", 
                           (int)i, allLevels[i].ChartPriority, allLevels[i].Price,
                           allLevels[i].Label.GetChars(), allLevels[i].ChartName.GetChars()), 0);
    }
    
    // Sort levels based on user inputs
    int sort1 = Input_SortBy1.GetIndex();
    int sort2 = Input_SortBy2.GetIndex();

    // Helper lambda for comparison
    auto CompareLevels = [](const LevelEntry& a, const LevelEntry& b, int sortType) -> int {
        // Returns 1 if a < b (a comes first), -1 if a > b, 0 if equal
        switch (sortType) {
            case 0: // Chart Priority
                if (a.ChartPriority < b.ChartPriority) return 1;
                if (a.ChartPriority > b.ChartPriority) return -1;
                return 0;
            case 1: // Price Descending
                if (a.Price > b.Price) return 1;
                if (a.Price < b.Price) return -1;
                return 0;
            case 2: // Price Ascending
                if (a.Price < b.Price) return 1;
                if (a.Price > b.Price) return -1;
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
    
    // Debug: Show levels after sorting
    sc.AddMessageToLog("DEBUG: Levels AFTER sorting:", 0);
    for (size_t i = 0; i < allLevels.size(); i++)
    {
        sc.AddMessageToLog(SCString().Format("  [%d] Priority=%d, Price=%.2f, Label=%s, Chart=%s", 
                           (int)i, allLevels[i].ChartPriority, allLevels[i].Price,
                           allLevels[i].Label.GetChars(), allLevels[i].ChartName.GetChars()), 0);
    }
    

    // Store sorted levels in persistent state for display
    p_State->Levels = allLevels;
    p_State->HasScanned = true;
    p_State->LastScanTime = sc.CurrentSystemDateTime;
    
    // Force a redraw so table updates immediately
    showTable = Input_ShowTable.GetYesNo() && p_State->IsTableVisible;
    DrawTable(sc, p_State, showTable, Input_TableX.GetInt(), Input_TableY.GetInt(),
              Input_FontSize.GetInt(), Input_FontColor.GetColor(), Input_BackgroundColor.GetColor(), sc.Input[18].GetColor());

    // Build the output string
    SCString output;
    int includeChartName = Input_IncludeChartName.GetYesNo();
    int includeDescription = Input_IncludeDescription.GetYesNo();
    
    // Check if using external template
    if (Input_UseTemplate.GetYesNo())
    {
        SCString templatePath = Input_TemplatePath.GetPathAndFileName();
        
        // Check if template file exists
        HANDLE hFile = CreateFile(templatePath.GetChars(), GENERIC_READ, FILE_SHARE_READ, 
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        
        if (hFile == INVALID_HANDLE_VALUE)
        {
            // Template doesn't exist - create default template
            sc.AddMessageToLog("Template file not found - creating default template...", 0);
            
            SCString defaultTemplate;
            defaultTemplate += "# Obsidian Level Export Template\n";
            defaultTemplate += "# Available tags:\n";
            defaultTemplate += "#   {{HEADER}} - everything before {{LEVEL_LINE}} is header\n";
            defaultTemplate += "#   {{LEVEL_LINE}} - marks the repeating level line (one per level)\n";
            defaultTemplate += "#   {{FOOTER}} - everything after levels\n";
            defaultTemplate += "# Per-level tags (use within LEVEL_LINE):\n";
            defaultTemplate += "#   {{LABEL}} - the matched label (PDC, BZ, etc)\n";
            defaultTemplate += "#   {{DESCRIPTION}} - description after pipe (if any)\n";
            defaultTemplate += "#   {{DISPLAY_NAME}} - description if exists, else label\n";
            defaultTemplate += "#   {{PRICE}} - the price value\n";
            defaultTemplate += "#   {{CHART_NAME}} - custom chart name from settings\n";
            defaultTemplate += "#   {{CHART_NUMBER}} - chart number\n";
            defaultTemplate += "#\n";
            defaultTemplate += "---\n";
            defaultTemplate += "# Chart Levels Export\n";
            defaultTemplate += "\n";
            defaultTemplate += "{{LEVEL_LINE}}{{DISPLAY_NAME}} ({{CHART_NAME}}):: {{PRICE}}\n";
            defaultTemplate += "---\n";
            
            HANDLE hWrite = CreateFile(templatePath.GetChars(), GENERIC_WRITE, 0, NULL, 
                                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hWrite != INVALID_HANDLE_VALUE)
            {
                DWORD bytesWritten;
                WriteFile(hWrite, defaultTemplate.GetChars(), defaultTemplate.GetLength(), &bytesWritten, NULL);
                CloseHandle(hWrite);
                sc.AddMessageToLog(SCString().Format("Default template created: %s", templatePath.GetChars()), 0);
                sc.AddMessageToLog("Edit the template and run again to use custom format.", 0);
            }
            
            // Use built-in format for this run
            goto use_builtin_format;
        }
        else
        {
            // Read template file
            DWORD fileSize = GetFileSize(hFile, NULL);
            if (fileSize > 0 && fileSize < 65536)
            {
                char* templateBuffer = new char[fileSize + 1];
                DWORD bytesRead;
                ReadFile(hFile, templateBuffer, fileSize, &bytesRead, NULL);
                templateBuffer[bytesRead] = '\0';
                CloseHandle(hFile);
                
                // Preprocess: Remove carriage returns (handle Windows \r\n line endings)
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
                
                // Update bytesRead to reflect stripped content
                bytesRead = (DWORD)(writePtr - templateBuffer);
                
                // Work with the raw C string to avoid SCString issues
                const char* templateData = templateBuffer;
                
                // Find the LEVEL_LINE marker - must be at START of a line (not inside comments)
                const char* levelLinePtr = NULL;
                const char* searchStart = templateData;
                
                // Check if {{LEVEL_LINE}} is at the very start of the file
                if (strncmp(templateData, "{{LEVEL_LINE}}", 14) == 0)
                {
                    levelLinePtr = templateData;
                }
                else
                {
                    // Search for \n{{LEVEL_LINE}} to find it at start of a line
                    while ((searchStart = strstr(searchStart, "\n{{LEVEL_LINE}}")) != NULL)
                    {
                        levelLinePtr = searchStart + 1; // Point to the {{ after the newline
                        break;
                    }
                }
                
                if (levelLinePtr != NULL)
                {
                    // Extract header: everything before {{LEVEL_LINE}}, skipping comment lines
                    SCString header;
                    const char* lineStart = templateData;
                    while (lineStart < levelLinePtr)
                    {
                        // Find end of this line
                        const char* lineEnd = strchr(lineStart, '\n');
                        if (lineEnd == NULL || lineEnd > levelLinePtr)
                            lineEnd = levelLinePtr;
                        
                        // Check if line starts with # (comment) or is empty
                        if (lineStart < lineEnd && *lineStart != '#')
                        {
                            // Add this line to header
                            size_t lineLen = lineEnd - lineStart;
                            char lineBuf[1024];
                            if (lineLen >= sizeof(lineBuf)) lineLen = sizeof(lineBuf) - 1;
                            strncpy(lineBuf, lineStart, lineLen);
                            lineBuf[lineLen] = '\0';
                            
                            // Only add non-empty lines
                            if (lineLen > 0)
                            {
                                header += lineBuf;
                                if (lineEnd < levelLinePtr) header += "\n";
                            }
                        }
                        
                        // Move to next line
                        if (*lineEnd == '\n')
                            lineStart = lineEnd + 1;
                        else
                            break;
                    }
                    
                    // Find the level line template (rest of line after {{LEVEL_LINE}})
                    const char* levelLineContent = levelLinePtr + 14; // Skip "{{LEVEL_LINE}}"
                    const char* levelLineEnd = strchr(levelLineContent, '\n');
                    if (levelLineEnd == NULL) levelLineEnd = templateData + bytesRead;
                    
                    // Extract level line template
                    size_t levelTemplateLen = levelLineEnd - levelLineContent;
                    char levelTemplateBuf[1024];
                    if (levelTemplateLen >= sizeof(levelTemplateBuf)) levelTemplateLen = sizeof(levelTemplateBuf) - 1;
                    strncpy(levelTemplateBuf, levelLineContent, levelTemplateLen);
                    levelTemplateBuf[levelTemplateLen] = '\0';
                    
                    // Debug: Show what we extracted
                    sc.AddMessageToLog(SCString().Format("DEBUG Template: Level template = '%s'", levelTemplateBuf), 0);
                    
                    // Extract footer: everything after level line, skipping comment lines
                    SCString footer;
                    if (*levelLineEnd == '\n')
                    {
                        lineStart = levelLineEnd + 1;
                        const char* dataEnd = templateData + bytesRead;
                        
                        while (lineStart < dataEnd)
                        {
                            const char* lineEnd = strchr(lineStart, '\n');
                            if (lineEnd == NULL) lineEnd = dataEnd;
                            
                            // Check if line starts with # (comment)
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
                    
                    // Build output using template
                    output = header;
                    
                    for (size_t i = 0; i < allLevels.size(); i++)
                    {
                        // Determine display name
                        SCString displayName;
                        if (includeDescription && allLevels[i].Description.GetLength() > 0)
                            displayName = allLevels[i].Description;
                        else
                            displayName = allLevels[i].Label;
                        
                        // Format price
                        SCString priceStr;
                        priceStr.Format("%.2f", allLevels[i].Price);
                        
                        SCString chartNumStr;
                        chartNumStr.Format("%d", allLevels[i].SourceChart);
                        
                        // Replace tags in level template
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
                    
                    sc.AddMessageToLog("Using external template format.", 0);
                }
                else
                {
                    delete[] templateBuffer;
                    sc.AddMessageToLog("Template missing {{LEVEL_LINE}} marker - using built-in format.", 1);
                    goto use_builtin_format;
                }
            }
            else
            {
                CloseHandle(hFile);
                sc.AddMessageToLog("Template file too large or empty - using built-in format.", 1);
                goto use_builtin_format;
            }
        }
    }
    else
    {
use_builtin_format:
        // Built-in format
        output += "---\n";
        output += "# Chart Levels Export\n\n";
        
        for (size_t i = 0; i < allLevels.size(); i++)
        {
            SCString line;
            
            SCString displayName;
            if (includeDescription && allLevels[i].Description.GetLength() > 0)
                displayName = allLevels[i].Description;
            else
                displayName = allLevels[i].Label;
            
            if (includeChartName)
            {
                line.Format("%s (%s):: %.2f\n", 
                           displayName.GetChars(),
                           allLevels[i].ChartName.GetChars(),
                           allLevels[i].Price);
            }
            else
            {
                line.Format("%s:: %.2f\n", 
                           displayName.GetChars(),
                           allLevels[i].Price);
            }
            output += line;
        }
        
        output += "---\n";
    }
    
    // Print header to message log
    sc.AddMessageToLog("", 0);
    sc.AddMessageToLog("========== OBSIDIAN LEVELS EXPORT ==========", 0);
    sc.AddMessageToLog(SCString().Format("Scanned %d chart(s), found %d matching level(s)", 
                       (int)chartsToScan.size(), (int)allLevels.size()), 0);
    sc.AddMessageToLog("", 0);
    
    // Print each level to the message log
    for (size_t i = 0; i < allLevels.size(); i++)
    {
        SCString line;
        
        // Use same display logic as file output
        SCString displayName;
        if (includeDescription && allLevels[i].Description.GetLength() > 0)
        {
            displayName = allLevels[i].Description;
        }
        else
        {
            displayName = allLevels[i].Label;
        }
        
        if (includeChartName)
        {
            line.Format("%s (%s):: %.2f", 
                       displayName.GetChars(),
                       allLevels[i].ChartName.GetChars(),
                       allLevels[i].Price);
        }
        else
        {
            line.Format("%s:: %.2f", 
                       displayName.GetChars(),
                       allLevels[i].Price);
        }
        sc.AddMessageToLog(line, 0);
    }
    
    sc.AddMessageToLog("", 0);
    sc.AddMessageToLog("=============================================", 0);
    
    // Handle output destination: 0=Log Only, 1=Clipboard, 2=File, 3=Clipboard+File
    int outputDest = Input_OutputDestination.GetIndex();
    bool writeToClipboard = (outputDest == 1 || outputDest == 3);
    bool writeToFile = (outputDest == 2 || outputDest == 3);
    
    // Write to clipboard if selected
    if (writeToClipboard)
    {
        if (OpenClipboard(NULL))
        {
            EmptyClipboard();
            
            // Allocate global memory for the text
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, output.GetLength() + 1);
            if (hMem != NULL)
            {
                char* pMem = (char*)GlobalLock(hMem);
                if (pMem != NULL)
                {
                    strcpy(pMem, output.GetChars());
                    GlobalUnlock(hMem);
                    
                    SetClipboardData(CF_TEXT, hMem);
                    
                    sc.AddMessageToLog("", 0);
                    sc.AddMessageToLog("*** COPIED TO CLIPBOARD ***", 0);
                    MessageBeep(MB_OK);
                }
                else
                {
                    GlobalFree(hMem);
                    sc.AddMessageToLog("ERROR: Could not lock clipboard memory", 1);
                }
            }
            else
            {
                sc.AddMessageToLog("ERROR: Could not allocate clipboard memory", 1);
            }
            
            CloseClipboard();
        }
        else
        {
            sc.AddMessageToLog("ERROR: Could not open clipboard", 1);
            MessageBeep(MB_ICONERROR);
        }
    }
    
    // Write to file if selected
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
            
            sc.AddMessageToLog("", 0);
            sc.AddMessageToLog("*** FILE WRITE COMPLETE ***", 0);
            sc.AddMessageToLog(SCString().Format("Saved to: %s", filePath.GetChars()), 0);
            MessageBeep(MB_OK);
        }
        else
        {
            sc.AddMessageToLog(SCString().Format("ERROR: Could not write to file: %s", filePath.GetChars()), 1);
            MessageBeep(MB_ICONERROR);
        }
    }
    
    // If Log Only, just confirm
    if (outputDest == 0)
    {
        sc.AddMessageToLog("", 0);
        sc.AddMessageToLog("*** OUTPUT TO LOG ONLY ***", 0);
        MessageBeep(MB_OK);
    }
}