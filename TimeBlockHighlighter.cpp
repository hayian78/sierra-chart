#include "sierrachart.h"
#include <string.h>
#include <stdio.h>

SCDLLName("Time Block Highlighter")

SCSFExport scsf_TimeBlockHighlighter(SCStudyInterfaceRef sc)
{
    // Session A colors
    SCSubgraphRef BorderA = sc.Subgraph[0];
    SCSubgraphRef FillA   = sc.Subgraph[1];

    // Session B colors
    SCSubgraphRef BorderB = sc.Subgraph[2];
    SCSubgraphRef FillB   = sc.Subgraph[3];

    // Session A
    SCInputRef EnableAInput      = sc.Input[0];
    SCInputRef StartTimeAInput   = sc.Input[1];
    SCInputRef EndTimeAInput     = sc.Input[2];
    SCInputRef LabelAInput       = sc.Input[3];

    // Session B
    SCInputRef EnableBInput      = sc.Input[4];
    SCInputRef StartTimeBInput   = sc.Input[5];
    SCInputRef EndTimeBInput     = sc.Input[6];
    SCInputRef LabelBInput       = sc.Input[7];

    // Shared box placement
    SCInputRef VerticalOffset    = sc.Input[8];
    SCInputRef HeightPct         = sc.Input[9];
    SCInputRef LocationInput     = sc.Input[10];
    SCInputRef TransparencyInput = sc.Input[11];

    // Shared filters + label style
    SCInputRef OnlyTodayInput           = sc.Input[12];
    SCInputRef DayFilterInput           = sc.Input[13];
    SCInputRef TextAlignment            = sc.Input[14];
    SCInputRef HighlightLabelYOffsetPct = sc.Input[15];
    SCInputRef LabelFontSize            = sc.Input[16];
    SCInputRef LabelColorInput          = sc.Input[17];

    // Optional offset timezone tick LABELS (no vertical lines)
    SCInputRef ShowTzTicksInput      = sc.Input[18];
    SCInputRef TzOffsetHHMMInput     = sc.Input[19];
    SCInputRef TickIntervalMinutes   = sc.Input[20];
    SCInputRef TickColorInput        = sc.Input[21];
    SCInputRef TickFontSizeInput     = sc.Input[22];
    SCInputRef TickLabelFormatInput  = sc.Input[23];
    SCInputRef MaxTzTicksInput       = sc.Input[24];

    // Section headers
    SCInputRef HeaderSessionA  = sc.Input[28];
    SCInputRef HeaderSessionB  = sc.Input[29];
    SCInputRef HeaderStyling   = sc.Input[30];
    SCInputRef HeaderMarkers   = sc.Input[31];
    SCInputRef HeaderTzTicks   = sc.Input[32];
    SCInputRef HeaderFilters   = sc.Input[33];
    SCInputRef HeaderPerf      = sc.Input[34];
    
    // Time Markers
    SCInputRef EnableMarkersInput              = sc.Input[35];
    SCInputRef MarkersListInput                = sc.Input[36];
    SCInputRef MarkerLabelVerticalOffsetInput  = sc.Input[37];
    SCInputRef MarkerDefaultColorInput         = sc.Input[38];
    SCInputRef MarkerLineWidthInput            = sc.Input[39];
    SCInputRef MarkerLineStyleInput            = sc.Input[40];
    SCInputRef MarkerLabelFontSizeInput        = sc.Input[41];
    SCInputRef MarkerLabelColorInput           = sc.Input[42];
    SCInputRef MaxMarkersPerDayInput           = sc.Input[43];
    
    // Forward Projections
    SCInputRef HeaderProjections          = sc.Input[44];
    SCInputRef ExtendActiveBlockInput     = sc.Input[45];
    SCInputRef PreviewUpcomingBlockInput  = sc.Input[46];
    SCInputRef PreviewMinutesInput        = sc.Input[47];
    
    // Rendering mode
    SCInputRef StableRenderInput     = sc.Input[25];
    
    // Performance enhancements
    SCInputRef EnableHashCachingInput = sc.Input[26];
    SCInputRef IncrementalModeInput   = sc.Input[27];

    if (sc.SetDefaults)
    {
        sc.GraphName    = "Time Block Highlighter";
        sc.GraphRegion  = 0;
        sc.AutoLoop     = 0;
        sc.UpdateAlways = 1; // stability > micro-optimizations

        // Session A colors
        BorderA.Name         = "Session A Border";
        BorderA.DrawStyle    = DRAWSTYLE_IGNORE;
        BorderA.PrimaryColor = RGB(255, 255, 0);
        BorderA.LineWidth    = 1;
        BorderA.LineStyle    = LINESTYLE_SOLID;

        FillA.Name         = "Session A Fill";
        FillA.DrawStyle    = DRAWSTYLE_IGNORE;
        FillA.PrimaryColor = RGB(128, 128, 0);

        // Session B colors
        BorderB.Name         = "Session B Border";
        BorderB.DrawStyle    = DRAWSTYLE_IGNORE;
        BorderB.PrimaryColor = RGB(0, 180, 255);
        BorderB.LineWidth    = 1;
        BorderB.LineStyle    = LINESTYLE_SOLID;

        FillB.Name         = "Session B Fill";
        FillB.DrawStyle    = DRAWSTYLE_IGNORE;
        FillB.PrimaryColor = RGB(0, 80, 128);

        // ========== Display Order Grouping ==========
        int dispOrder = 1;

        // ===== SESSION A =====
        HeaderSessionA.Name = "===== SESSION A =====";
        HeaderSessionA.SetInt(0);
        HeaderSessionA.SetIntLimits(0, 0);
        HeaderSessionA.DisplayOrder = dispOrder++;

        EnableAInput.Name = "Enable Session A";
        EnableAInput.SetYesNo(1);
        EnableAInput.DisplayOrder = dispOrder++;

        StartTimeAInput.Name = "Session A Start Time";
        StartTimeAInput.SetTime(HMS_TIME(9, 30, 0));
        StartTimeAInput.DisplayOrder = dispOrder++;

        EndTimeAInput.Name = "Session A End Time";
        EndTimeAInput.SetTime(HMS_TIME(16, 0, 0));
        EndTimeAInput.DisplayOrder = dispOrder++;

        LabelAInput.Name = "Session A Label Text";
        LabelAInput.SetString("RTH");
        LabelAInput.DisplayOrder = dispOrder++;

        // ===== SESSION B =====
        HeaderSessionB.Name = "===== SESSION B =====";
        HeaderSessionB.SetInt(0);
        HeaderSessionB.SetIntLimits(0, 0);
        HeaderSessionB.DisplayOrder = dispOrder++;

        EnableBInput.Name = "Enable Session B";
        EnableBInput.SetYesNo(1);
        EnableBInput.DisplayOrder = dispOrder++;

        StartTimeBInput.Name = "Session B Start Time";
        StartTimeBInput.SetTime(HMS_TIME(18, 0, 0));
        StartTimeBInput.DisplayOrder = dispOrder++;

        EndTimeBInput.Name = "Session B End Time";
        EndTimeBInput.SetTime(HMS_TIME(9, 29, 59));
        EndTimeBInput.DisplayOrder = dispOrder++;

        LabelBInput.Name = "Session B Label Text";
        LabelBInput.SetString("Globex");
        LabelBInput.DisplayOrder = dispOrder++;

        // ===== SESSION STYLING =====
        HeaderStyling.Name = "===== SESSION STYLING =====";
        HeaderStyling.SetInt(0);
        HeaderStyling.SetIntLimits(0, 0);
        HeaderStyling.DisplayOrder = dispOrder++;

        LocationInput.Name  = "Location";
        LocationInput.SetCustomInputStrings("From Bottom;From Top;Custom");
        LocationInput.SetCustomInputIndex(1);
        LocationInput.DisplayOrder = dispOrder++;

        VerticalOffset.Name = "Vertical Offset (%)";
        VerticalOffset.SetFloat(0.0f);
        VerticalOffset.DisplayOrder = dispOrder++;

        HeightPct.Name  = "Height (%)";
        HeightPct.SetFloat(4.0f);
        HeightPct.DisplayOrder = dispOrder++;

        TransparencyInput.Name = "Transparency (0-100)";
        TransparencyInput.SetInt(60);
        TransparencyInput.DisplayOrder = dispOrder++;

        TextAlignment.Name = "Text Alignment (0=Left, 1=Center, 2=Right)";
        TextAlignment.SetInt(1);
        TextAlignment.DisplayOrder = dispOrder++;

        HighlightLabelYOffsetPct.Name = "Highlight Label Y Offset (% of box height, -50..+50)";
        HighlightLabelYOffsetPct.SetFloat(25.0f);
        HighlightLabelYOffsetPct.DisplayOrder = dispOrder++;

        LabelFontSize.Name = "Label Font Size";
        LabelFontSize.SetInt(12);
        LabelFontSize.DisplayOrder = dispOrder++;

        LabelColorInput.Name = "Label Color";
        LabelColorInput.SetColor(RGB(255, 255, 255));
        LabelColorInput.DisplayOrder = dispOrder++;

        // ===== TIME MARKERS =====
        HeaderMarkers.Name = "===== TIME MARKERS =====";
        HeaderMarkers.SetInt(0);
        HeaderMarkers.SetIntLimits(0, 0);
        HeaderMarkers.DisplayOrder = dispOrder++;

        EnableMarkersInput.Name = "Enable Time Markers";
        EnableMarkersInput.SetYesNo(0);
        EnableMarkersInput.DisplayOrder = dispOrder++;

        MarkersListInput.Name = "Time Markers List (HH:MM|Label|ColorName, comma-separated)";
        MarkersListInput.SetString("");
        MarkersListInput.DisplayOrder = dispOrder++;

        MarkerLabelVerticalOffsetInput.Name = "Marker Label Vertical Offset (% from session box)";
        MarkerLabelVerticalOffsetInput.SetFloat(0.0f);
        MarkerLabelVerticalOffsetInput.SetFloatLimits(-50.0f, 50.0f);
        MarkerLabelVerticalOffsetInput.DisplayOrder = dispOrder++;

        MarkerDefaultColorInput.Name = "Marker Default Color";
        MarkerDefaultColorInput.SetColor(RGB(255, 165, 0));
        MarkerDefaultColorInput.DisplayOrder = dispOrder++;

        MarkerLineWidthInput.Name = "Marker Line Width";
        MarkerLineWidthInput.SetInt(1);
        MarkerLineWidthInput.SetIntLimits(1, 6);
        MarkerLineWidthInput.DisplayOrder = dispOrder++;

        MarkerLineStyleInput.Name = "Marker Line Style";
        MarkerLineStyleInput.SetCustomInputStrings("Solid;Dashed;Dotted");
        MarkerLineStyleInput.SetCustomInputIndex(1);
        MarkerLineStyleInput.DisplayOrder = dispOrder++;

        MarkerLabelFontSizeInput.Name = "Marker Label Font Size";
        MarkerLabelFontSizeInput.SetInt(10);
        MarkerLabelFontSizeInput.SetIntLimits(6, 36);
        MarkerLabelFontSizeInput.DisplayOrder = dispOrder++;

        MarkerLabelColorInput.Name = "Marker Label Color";
        MarkerLabelColorInput.SetColor(RGB(255, 255, 255));
        MarkerLabelColorInput.DisplayOrder = dispOrder++;

        MaxMarkersPerDayInput.Name = "Max Time Markers Per Day";
        MaxMarkersPerDayInput.SetInt(50);
        MaxMarkersPerDayInput.SetIntLimits(1, 200);
        MaxMarkersPerDayInput.DisplayOrder = dispOrder++;

        // ===== TZ TICK LABELS =====
        HeaderTzTicks.Name = "===== TZ TICK LABELS =====";
        HeaderTzTicks.SetInt(0);
        HeaderTzTicks.SetIntLimits(0, 0);
        HeaderTzTicks.DisplayOrder = dispOrder++;

        ShowTzTicksInput.Name = "Show TZ Tick Labels";
        ShowTzTicksInput.SetYesNo(0);
        ShowTzTicksInput.DisplayOrder = dispOrder++;

        TzOffsetHHMMInput.Name = "Local Time Offset from Chart (HHMM, e.g. 1500, -430)";
        TzOffsetHHMMInput.SetInt(0);
        TzOffsetHHMMInput.DisplayOrder = dispOrder++;

        TickIntervalMinutes.Name = "Tick Label Interval Minutes (0=auto scale by zoom)";
        TickIntervalMinutes.SetInt(0);
        TickIntervalMinutes.DisplayOrder = dispOrder++;

        TickColorInput.Name = "TZ Tick Label Color";
        TickColorInput.SetColor(RGB(200, 200, 200));
        TickColorInput.DisplayOrder = dispOrder++;

        TickFontSizeInput.Name = "TZ Tick Label Font Size";
        TickFontSizeInput.SetInt(9);
        TickFontSizeInput.DisplayOrder = dispOrder++;

        TickLabelFormatInput.Name = "TZ Tick Label Format (0=HH:MM, 1=HH:MM:SS, 2=HH)";
        TickLabelFormatInput.SetInt(0);
        TickLabelFormatInput.DisplayOrder = dispOrder++;

        MaxTzTicksInput.Name = "Max TZ Tick Labels to Draw (0=unlimited)";
        MaxTzTicksInput.SetInt(200);
        MaxTzTicksInput.DisplayOrder = dispOrder++;

        // ===== FILTERS & LABELS =====
        HeaderFilters.Name = "===== FILTERS & LABELS =====";
        HeaderFilters.SetInt(0);
        HeaderFilters.SetIntLimits(0, 0);
        HeaderFilters.DisplayOrder = dispOrder++;

        OnlyTodayInput.Name = "Only Current Trading Day";
        OnlyTodayInput.SetYesNo(0);
        OnlyTodayInput.DisplayOrder = dispOrder++;

        DayFilterInput.Name = "Allowed Days (Mon,Tue,Wed,Thu,Fri,Sat,Sun)";
        DayFilterInput.SetString("Mon,Tue,Wed,Thu,Fri");
        DayFilterInput.DisplayOrder = dispOrder++;

        // ===== PROJECTIONS =====
        HeaderProjections.Name = "===== PROJECTIONS =====";
        HeaderProjections.SetInt(0);
        HeaderProjections.SetIntLimits(0, 0);
        HeaderProjections.DisplayOrder = dispOrder++;

        ExtendActiveBlockInput.Name = "Extend Active Block into Future";
        ExtendActiveBlockInput.SetYesNo(1);
        ExtendActiveBlockInput.DisplayOrder = dispOrder++;

        PreviewUpcomingBlockInput.Name = "Preview Upcoming Block";
        PreviewUpcomingBlockInput.SetYesNo(1);
        PreviewUpcomingBlockInput.DisplayOrder = dispOrder++;

        PreviewMinutesInput.Name = "Preview Minutes Before Start";
        PreviewMinutesInput.SetInt(60);
        PreviewMinutesInput.SetIntLimits(1, 1440);
        PreviewMinutesInput.DisplayOrder = dispOrder++;

        // ===== PERFORMANCE =====
        HeaderPerf.Name = "===== PERFORMANCE =====";
        HeaderPerf.SetInt(0);
        HeaderPerf.SetIntLimits(0, 0);
        HeaderPerf.DisplayOrder = dispOrder++;

        StableRenderInput.Name = "Stable Rendering Mode (recommended)";
        StableRenderInput.SetYesNo(1);
        StableRenderInput.DisplayOrder = dispOrder++;
        
        EnableHashCachingInput.Name = "Enable Hash Caching (skip redraw if nothing changed)";
        EnableHashCachingInput.SetYesNo(1);
        EnableHashCachingInput.DisplayOrder = dispOrder++;
        
        IncrementalModeInput.Name = "Incremental Update Mode (process only new bars)";
        IncrementalModeInput.SetYesNo(1);
        IncrementalModeInput.DisplayOrder = dispOrder++;

        return;
    }

    if (sc.LastCallToFunction || sc.ArraySize == 0)
        return;

    // Persistent counters used only for cleanup
    int& LastSegCountA       = sc.GetPersistentInt(0);
    int& LastSegCountB       = sc.GetPersistentInt(1);
    int& LastTzTickDrawCount = sc.GetPersistentInt(2);
    int& LastMarkerDrawCount = sc.GetPersistentInt(6);  // Time Markers cleanup counter
    
    // Hash caching and incremental mode
    int& LastInputHash       = sc.GetPersistentInt(3);
    int& LastArraySize       = sc.GetPersistentInt(4);
    int& LastProcessedIndex  = sc.GetPersistentInt(5);

    // Session times
    const int    enableA = EnableAInput.GetYesNo();
    const double startA  = StartTimeAInput.GetTime();
    const double endA    = EndTimeAInput.GetTime();
    const char*  labelA  = LabelAInput.GetString();

    const int    enableB = EnableBInput.GetYesNo();
    const double startB  = StartTimeBInput.GetTime();
    const double endB    = EndTimeBInput.GetTime();
    const char*  labelB  = LabelBInput.GetString();

    // Shared settings
    const float    offsetPct    = VerticalOffset.GetFloat();
    const float    heightPct    = HeightPct.GetFloat();
    const int      location     = LocationInput.GetIndex();
    const int      transparency = TransparencyInput.GetInt();
    const int      onlyToday    = OnlyTodayInput.GetYesNo();
    const int      labelSize    = LabelFontSize.GetInt();
    const COLORREF labelColor   = LabelColorInput.GetColor();
    const float    labelYOffset = HighlightLabelYOffsetPct.GetFloat();

    // TZ tick labels
    const int      showTzTicks   = ShowTzTicksInput.GetYesNo();
    const int      tzOffsetHHMM  = TzOffsetHHMMInput.GetInt();
    int            tickEveryMin  = TickIntervalMinutes.GetInt();
    const COLORREF tickColor     = TickColorInput.GetColor();
    const int      tickFontSize  = TickFontSizeInput.GetInt();
    const int      tickFmt       = TickLabelFormatInput.GetInt();
    const int      maxTzTicks    = MaxTzTicksInput.GetInt();

    const int stableMode = StableRenderInput.GetYesNo();
    const int enableHashCaching = EnableHashCachingInput.GetYesNo();
    const int incrementalMode = IncrementalModeInput.GetYesNo();

    // Base line number for this study instance
    const int baseLine = sc.StudyGraphInstanceID * 100000;

    if (heightPct <= 0.0f)
        return;

    // latest trading day (for OnlyToday)
    SCDateTime latestDT = sc.BaseDateTimeIn[sc.ArraySize - 1];
    const int  latestDate = (int)latestDT.GetDate();

    // Parse HHMM offset (signed) into seconds for both Ticks and Markers
    int sign = (tzOffsetHHMM < 0) ? -1 : 1;
    int absHHMM = (tzOffsetHHMM < 0) ? -tzOffsetHHMM : tzOffsetHHMM;
    int offH = absHHMM / 100;
    int offM = absHHMM % 100;
    if (offM > 59) offM = 59;
    const int offsetSeconds = sign * (offH * 3600 + offM * 60);
    const double secondsPerDay = 86400.0;

    // Day-of-week filter - pre-compute bitmask for performance
    SCString dowFilterSC = DayFilterInput.GetString();
    const char* dowFilter = dowFilterSC.GetChars();
    
    static const char* dowNames[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    uint8_t allowedDaysMask = 0;
    
    if (dowFilter == NULL || dowFilter[0] == '\0')
    {
        allowedDaysMask = 0x7F; // All days allowed (bits 0-6 set)
    }
    else
    {
        for (int d = 0; d < 7; ++d)
        {
            if (strstr(dowFilter, dowNames[d]) != NULL)
                allowedDaysMask |= (1 << d);
        }
    }

    auto IsDayAllowed = [&](int scDayOfWeek) -> bool
    {
        if (scDayOfWeek < 0 || scDayOfWeek > 6)
            return false;
        return (allowedDaysMask & (1 << scDayOfWeek)) != 0;
    };

    auto IsTimeInWindow = [&](double t, double s, double e) -> bool
    {
        if (s <= e)
            return t >= s && t <= e;
        return (t >= s) || (t <= e);
    };

    auto ShouldHighlight = [&](const SCDateTime& dt, double s, double e) -> bool
    {
        const int date = (int)dt.GetDate();
        if (onlyToday && date != latestDate)
            return false;
        const int dow = dt.GetDayOfWeek();
        if (!IsDayAllowed(dow))
            return false;
        return IsTimeInWindow(dt.GetTime(), s, e);
    };

    // Helper: compute the bounds of the current or next session occurrence
    // relative to 'now'.  When skipCurrent=true the session that 'now' is
    // inside is skipped and the *next* occurrence is returned instead.
    auto GetNextSessionBounds = [&](double s, double e, const SCDateTime& now,
                                    SCDateTime& outStart, SCDateTime& outEnd,
                                    bool skipCurrent = false)
    {
        SCDateTime todayStart = now.GetDate() + (s / 86400.0);
        SCDateTime todayEnd   = now.GetDate() + (e / 86400.0);
        if (s > e) todayEnd += 1.0;  // overnight session

        // Check if we are inside the tail of an overnight session that started yesterday
        if (s > e)
        {
            SCDateTime yesterdayStart = todayStart - 1.0;
            SCDateTime yesterdayEnd   = todayEnd   - 1.0;
            if (now >= yesterdayStart && now < yesterdayEnd)
            {
                if (skipCurrent)
                {
                    outStart = todayStart;
                    outEnd   = todayEnd;
                }
                else
                {
                    outStart = yesterdayStart;
                    outEnd   = yesterdayEnd;
                }
                return;
            }
        }

        if (now < todayStart)
        {
            outStart = todayStart;
            outEnd   = todayEnd;
        }
        else if (now < todayEnd && !skipCurrent)
        {
            outStart = todayStart;
            outEnd   = todayEnd;
        }
        else
        {
            outStart = todayStart + 1.0;
            outEnd   = todayEnd   + 1.0;
        }

        // Skip to next allowed day-of-week
        for (int i = 0; i < 7; i++)
        {
            if (IsDayAllowed(outStart.GetDayOfWeek()))
                break;
            outStart += 1.0;
            outEnd   += 1.0;
        }
    };
    
    // -------------------- Read Time Markers inputs (used by hash and drawing) --------------------
    const int enableMarkers = EnableMarkersInput.GetYesNo();
    SCString markersListStr = MarkersListInput.GetString();
    const char* markersList = markersListStr.GetChars();
    const float markerLabelVerticalOffset = MarkerLabelVerticalOffsetInput.GetFloat();
    const COLORREF markerDefaultColor = MarkerDefaultColorInput.GetColor();
    const int markerLineWidth = MarkerLineWidthInput.GetInt();
    const int markerLineStyle = MarkerLineStyleInput.GetInt();
    const int markerLabelFontSize = MarkerLabelFontSizeInput.GetInt();
    const COLORREF markerLabelColor = MarkerLabelColorInput.GetColor();
    const int maxMarkersPerDay = MaxMarkersPerDayInput.GetInt();
    
    // Projections
    const int extendActive    = ExtendActiveBlockInput.GetYesNo();
    const int previewUpcoming = PreviewUpcomingBlockInput.GetYesNo();
    const int previewMinutes  = PreviewMinutesInput.GetInt();
    
    // -------------------- Hash Caching: Skip redraw if nothing changed --------------------
    if (enableHashCaching)
    {
        // Compute hash of all relevant inputs
        int currentHash = 0;
        currentHash ^= enableA;
        currentHash ^= enableB;
        currentHash ^= (int)(startA * 1000);
        currentHash ^= (int)(endA * 1000);
        currentHash ^= (int)(startB * 1000);
        currentHash ^= (int)(endB * 1000);
        currentHash ^= (int)(offsetPct * 100);
        currentHash ^= (int)(heightPct * 100);
        currentHash ^= location;
        currentHash ^= transparency;
        currentHash ^= onlyToday;
        currentHash ^= labelSize;
        currentHash ^= (int)labelColor;
        currentHash ^= showTzTicks;
        currentHash ^= tzOffsetHHMM;
        currentHash ^= tickEveryMin;
        currentHash ^= stableMode;
        currentHash ^= sc.ArraySize;
        
        // Projections
        currentHash ^= extendActive;
        currentHash ^= previewUpcoming;
        currentHash ^= previewMinutes;
        
        // Time Markers
        currentHash ^= enableMarkers;
        currentHash ^= (int)(markerLabelVerticalOffset * 100);
        currentHash ^= (int)markerDefaultColor;
        currentHash ^= markerLineWidth;
        currentHash ^= markerLineStyle;
        currentHash ^= markerLabelFontSize;
        currentHash ^= (int)markerLabelColor;
        currentHash ^= maxMarkersPerDay;
        
        // Simple hash of day filter and labels
        if (dowFilter != NULL)
            for (const char* p = dowFilter; *p; ++p) currentHash ^= *p;
        if (labelA != NULL)
            for (const char* p = labelA; *p; ++p) currentHash ^= *p;
        if (labelB != NULL)
            for (const char* p = labelB; *p; ++p) currentHash ^= *p;
        if (markersList != NULL)
            for (const char* p = markersList; *p; ++p) currentHash ^= *p;
        
        // Lazy Sticky Edge: REDRAW logic only executes on 30-bar quantized changes
        int quantizedFirstBar = sc.IndexOfFirstVisibleBar / 30;
        int quantizedLastBar = sc.IndexOfLastVisibleBar / 30;
        currentHash ^= quantizedFirstBar * 31;
        currentHash ^= quantizedLastBar * 37;
        
        // If nothing changed, skip redraw
        if (currentHash == LastInputHash && sc.ArraySize == LastArraySize && !sc.IsFullRecalculation)
        {
            return;
        }
        
        LastInputHash = currentHash;
        LastArraySize = sc.ArraySize;
    }

    // -------------------- Compute visible range once (for both sessions and TZ ticks) --------------------
    int visibleStart = 0;
    int visibleEnd = sc.ArraySize - 1;
    
    if (sc.IndexOfFirstVisibleBar >= 0 && sc.IndexOfLastVisibleBar >= 0 &&
        sc.IndexOfFirstVisibleBar < sc.ArraySize && sc.IndexOfLastVisibleBar < sc.ArraySize &&
        sc.IndexOfFirstVisibleBar <= sc.IndexOfLastVisibleBar)
    {
        visibleStart = sc.IndexOfFirstVisibleBar;
        visibleEnd = sc.IndexOfLastVisibleBar;
    }
    
    // Cache firstToday computation (used multiple times)
    int firstToday = sc.ArraySize - 1;
    if (onlyToday)
    {
        while (firstToday > 0 && (int)sc.BaseDateTimeIn[firstToday - 1].GetDate() == latestDate)
            --firstToday;
    }
    
    // -------------------- Scan range for session boxes --------------------
    int scanStart = 0;
    int scanEnd   = sc.ArraySize - 1;

    if (!stableMode)
    {
        scanStart = visibleStart;
        scanEnd   = visibleEnd;

        const int bufferBars = 200;
        scanStart = (scanStart - bufferBars < 0) ? 0 : (scanStart - bufferBars);
        scanEnd   = (scanEnd + bufferBars >= sc.ArraySize) ? (sc.ArraySize - 1) : (scanEnd + bufferBars);
    }

    if (onlyToday && scanStart < firstToday)
        scanStart = firstToday;
    
    // -------------------- Incremental Mode: Process only when new bars are added --------------------
    //
    // Previously this logic always narrowed the scan range on *any* non-full
    // recalculation, including simple pans/zooms. That caused older session
    // segments (like most of the Globex block) to be deleted and only the
    // most recent portion to remain. Now we only apply the incremental
    // narrowing when the underlying data series has actually grown.
    if (incrementalMode && !sc.IsFullRecalculation && sc.ArraySize > LastProcessedIndex + 1)
    {
        // Only process from the last processed index onwards
        if (LastProcessedIndex >= 0 && LastProcessedIndex < sc.ArraySize)
        {
            // Expand scan range backwards a bit to catch segment boundaries
            const int safetyMargin = 50;
            int incrementalStart = (LastProcessedIndex - safetyMargin < 0) ? 0 : (LastProcessedIndex - safetyMargin);
            
            if (incrementalStart > scanStart)
                scanStart = incrementalStart;
        }
    }
    
    // Update last processed index
    LastProcessedIndex = sc.ArraySize - 1;

    // -------------------- Shared box vertical bounds (relative %) --------------------
    float boxBegin = 0.0f;
    float boxEnd   = 0.0f;

    switch (location)
    {
        case 1: // From Top
            boxEnd   = 100.0f - offsetPct;
            boxBegin = boxEnd - heightPct;
            break;
        case 2: // Custom
            boxBegin = offsetPct;
            boxEnd   = offsetPct + heightPct;
            break;
        default: // From Bottom
            boxBegin = offsetPct;
            boxEnd   = offsetPct + heightPct;
            break;
    }

    if (boxBegin < 0.0f)   boxBegin = 0.0f;
    if (boxBegin > 100.0f) boxBegin = 100.0f;
    if (boxEnd   < 0.0f)   boxEnd   = 0.0f;
    if (boxEnd   > 100.0f) boxEnd   = 100.0f;
    if (boxEnd <= boxBegin)
        return;

    // Helper: Convert a future DateTime to a virtual bar index in SC's forward fill space.
    // DRAWING_RECTANGLEHIGHLIGHT handles future DateTimes natively, but DRAWING_TEXT and
    // DRAWING_LINE do not reliably render at BeginDateTime positions in blank space.
    // Using virtual bar indices beyond sc.ArraySize-1 makes SC render them correctly.
    auto FutureBarIndex = [&](const SCDateTime& futureDT) -> int
    {
        if (sc.ArraySize <= 0 || sc.SecondsPerBar <= 0)
            return sc.ArraySize > 0 ? sc.ArraySize - 1 : 0;
        SCDateTime lastBarDT = sc.BaseDateTimeIn[sc.ArraySize - 1];
        double offsetSec = (futureDT - lastBarDT).GetAsDouble() * 86400.0;
        int barsForward = (int)(offsetSec / sc.SecondsPerBar + 0.5);
        if (barsForward < 1) barsForward = 1;
        return (sc.ArraySize - 1) + barsForward;
    };

    // Draw rectangle + label for a segment
    auto DrawSegment = [&](int segmentIndex, int beginIndex, int endIndex,
                           const SCSubgraphRef& BorderSG, const SCSubgraphRef& FillSG,
                           const char* text,
                           int rectBase)
    {
        if (endIndex < beginIndex)
            return;

        const int rectLine = rectBase + 2 * segmentIndex;
        const int textLine = rectBase + 2 * segmentIndex + 1;

        s_UseTool R;
        R.Clear();
        R.ChartNumber = sc.ChartNumber;
        R.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
        R.Region      = sc.GraphRegion;
        R.BeginIndex  = beginIndex;
        R.EndIndex    = endIndex;
        R.UseRelativeVerticalValues = 1;
        R.BeginValue  = boxBegin;
        R.EndValue    = boxEnd;

        R.Color             = BorderSG.PrimaryColor;
        R.LineWidth         = BorderSG.LineWidth;
        R.LineStyle         = BorderSG.LineStyle;
        R.SecondaryColor    = FillSG.PrimaryColor;
        R.TransparencyLevel = transparency;

        R.AddAsUserDrawnDrawing = 0;
        R.LineNumber            = rectLine;
        R.AddMethod             = UTAM_ADD_OR_ADJUST;
        sc.UseTool(R);

        if (text != NULL && text[0] != '\0')
        {
            // We want the session header (e.g. "Globex", "RTH") to stay
            // centered over the *visible* portion of its session, while
            // never moving outside the actual session bounds.
            //
            // Compute the intersection of this segment with the current
            // visible bar range, and center the label there. If the
            // segment is completely off-screen, we fall back to the
            // original full-segment midpoint, which will also be off-screen.
            int effectiveStart = beginIndex;
            int effectiveEnd   = endIndex;

            if (visibleEnd >= visibleStart)
            {
                // If there is any overlap between [beginIndex,endIndex]
                // and [visibleStart,visibleEnd], clamp to that overlap.
                if (!(endIndex < visibleStart || beginIndex > visibleEnd))
                {
                    if (beginIndex < visibleStart) effectiveStart = visibleStart;
                    if (endIndex   > visibleEnd)   effectiveEnd   = visibleEnd;
                }
            }

            if (effectiveEnd < effectiveStart)
                effectiveStart = effectiveEnd = beginIndex + (endIndex - beginIndex) / 2;

            const int midIndex = effectiveStart + (effectiveEnd - effectiveStart) / 2;

            const float boxH = (boxEnd - boxBegin);
            float labelY = (boxBegin + boxEnd) * 0.5f;

            float yPct = labelYOffset;
            if (yPct < -100.0f) yPct = -100.0f;
            if (yPct >  100.0f) yPct =  100.0f;
            labelY += boxH * (yPct / 100.0f);

            const float padInside = boxH * 0.10f;
            if (labelY < boxBegin + padInside) labelY = boxBegin + padInside;
            if (labelY > boxEnd   - padInside) labelY = boxEnd   - padInside;

            int alignInput = TextAlignment.GetInt();
            int anchorIndex;
            const int padBars = 10;

            switch (alignInput)
            {
                case 0:
                    // Left-aligned: pad from the left of the *visible*
                    // portion, but never outside the true session bounds.
                    anchorIndex = effectiveStart + padBars;
                    if (anchorIndex > effectiveEnd)   anchorIndex = effectiveEnd;
                    if (anchorIndex < beginIndex)     anchorIndex = beginIndex;
                    if (anchorIndex > endIndex)       anchorIndex = endIndex;
                    break;
                case 2:
                    // Right-aligned: pad from the right of the *visible*
                    // portion, but never outside the true session bounds.
                    anchorIndex = effectiveEnd - padBars;
                    if (anchorIndex < effectiveStart) anchorIndex = effectiveStart;
                    if (anchorIndex < beginIndex)     anchorIndex = beginIndex;
                    if (anchorIndex > endIndex)       anchorIndex = endIndex;
                    break;
                default:
                    anchorIndex = midIndex;
                    break;
            }

            s_UseTool T;
            T.Clear();
            T.ChartNumber = sc.ChartNumber;
            T.DrawingType = DRAWING_TEXT;
            T.Region      = sc.GraphRegion;
            T.BeginIndex  = anchorIndex;
            T.UseRelativeVerticalValues = 1;
            T.BeginValue  = labelY;
            T.Text        = text;
            T.FontSize    = labelSize;
            T.TextColor   = labelColor;
            T.TransparentLabelBackground = 1;
            T.TextAlignment = DT_CENTER | DT_VCENTER;
            T.AddAsUserDrawnDrawing = 0;
            T.LineNumber  = textLine;
            T.AddMethod   = UTAM_ADD_OR_ADJUST;
            sc.UseTool(T);
        }
        else
        {
            sc.DeleteACSChartDrawing(sc.ChartNumber, 0, textLine);
        }
    };

    // Bases for drawings
    const int rectBaseA = baseLine + 0;
    const int rectBaseB = baseLine + 20000;
    const int tzBase    = baseLine + 60000;

    // -------------------- Session A segments (O(Sessions) Jump Logic) --------------------
    int segCountA = 0;
    if (enableA)
    {
        int i = scanStart;
        while (i <= scanEnd && segCountA < 9999)
        {
            SCDateTime dt = sc.BaseDateTimeIn[i];
            
            // Skip to next allowed day if current day is filtered
            if (!IsDayAllowed(dt.GetDayOfWeek())) {
                i = sc.GetNearestMatchForSCDateTime(sc.ChartNumber, dt.GetDate() + 1.0);
                if (i < 0 || i > scanEnd) break;
                continue;
            }

            if (ShouldHighlight(dt, startA, endA))
            {
                int segStart = i;
                SCDateTime endDT = dt.GetDate() + endA / 86400.0;
                if (startA > endA) endDT += 1.0; // Overnight wrap
                
                i = sc.GetNearestMatchForSCDateTime(sc.ChartNumber, endDT);
                if (i < 0 || i > scanEnd) i = scanEnd;
                
                DrawSegment(segCountA++, segStart, i, BorderA, FillA, labelA, rectBaseA);
            }
            i++;
        }

        // ---- FUTURE PROJECTION: Extend active session A into blank space ----
        SCDateTime lastBarDT = sc.BaseDateTimeIn[sc.ArraySize - 1];
        if (extendActive && ShouldHighlight(lastBarDT, startA, endA) && scanEnd == sc.ArraySize - 1 && segCountA < 9999)
        {
            SCDateTime sDT, eDT;
            GetNextSessionBounds(startA, endA, lastBarDT, sDT, eDT, false);
            if (eDT > latestDT)
            {
                // Delete the historical segment's label — the floating label below replaces it
                sc.DeleteACSChartDrawing(sc.ChartNumber, 0, rectBaseA + 2 * (segCountA - 1) + 1);

                const int futRectLine = rectBaseA + 2 * segCountA;
                const int futTextLine = rectBaseA + 2 * segCountA + 1;

                // Draw the future extension rectangle using DateTimes
                s_UseTool R;
                R.Clear();
                R.ChartNumber = sc.ChartNumber;
                R.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
                R.Region      = sc.GraphRegion;
                R.BeginDateTime = sc.BaseDateTimeIn[sc.ArraySize - 1];
                R.EndDateTime   = eDT;
                R.UseRelativeVerticalValues = 1;
                R.BeginValue  = boxBegin;
                R.EndValue    = boxEnd;
                R.Color             = BorderA.PrimaryColor;
                R.LineWidth         = BorderA.LineWidth;
                R.LineStyle         = BorderA.LineStyle;
                R.SecondaryColor    = FillA.PrimaryColor;
                R.TransparencyLevel = transparency;
                R.AddAsUserDrawnDrawing = 0;
                R.LineNumber  = futRectLine;
                R.AddMethod   = UTAM_ADD_OR_ADJUST;
                sc.UseTool(R);

                // Floating label centered in the visible portion of the full extended segment
                if (labelA != NULL && labelA[0] != '\0')
                {
                    // Logic remains unchanged for floating labels
                    SCDateTime visStartDT = sc.BaseDateTimeIn[visibleStart];
                    SCDateTime visEndDT   = sc.BaseDateTimeIn[visibleEnd];
                    double barPeriodDays = (sc.SecondsPerBar > 0) ? sc.SecondsPerBar / 86400.0 : 60.0 / 86400.0;
                    SCDateTime visRightEdgeDT = visEndDT + (barPeriodDays * (visibleEnd - visibleStart + 1) * 0.4);

                    SCDateTime effStart = (sDT > visStartDT) ? sDT : visStartDT;
                    SCDateTime effEnd   = (eDT < visRightEdgeDT) ? eDT : visRightEdgeDT;
                    if (effEnd < effStart) effEnd = effStart;

                    SCDateTime midDT = effStart + (effEnd - effStart).GetAsDouble() / 2.0;
                    int labelIdx = FutureBarIndex(midDT);
                    if (midDT <= latestDT) {
                        int bi = sc.GetNearestMatchForSCDateTime(sc.ChartNumber, midDT);
                        if (bi >= 0 && bi < sc.ArraySize) labelIdx = bi;
                    }

                    const float boxH = (boxEnd - boxBegin);
                    float labelY = boxBegin + (boxH * (labelYOffset / 100.0f + 0.5f));

                    s_UseTool T;
                    T.Clear();
                    T.ChartNumber = sc.ChartNumber;
                    T.DrawingType = DRAWING_TEXT;
                    T.Region      = sc.GraphRegion;
                    T.BeginIndex  = labelIdx;
                    T.UseRelativeVerticalValues = 1;
                    T.BeginValue  = labelY;
                    T.Text        = labelA;
                    T.FontSize    = labelSize;
                    T.TextColor   = labelColor;
                    T.TransparentLabelBackground = 1;
                    T.TextAlignment = DT_CENTER | DT_VCENTER;
                    T.AddAsUserDrawnDrawing = 0;
                    T.LineNumber  = futTextLine;
                    T.AddMethod   = UTAM_ADD_OR_ADJUST;
                    sc.UseTool(T);
                }
                segCountA++;
            }
        }

        // ---- FUTURE PROJECTION: Preview upcoming session A ----
        if (previewUpcoming && segCountA < 9999)
        {
            SCDateTime sDT, eDT;
            GetNextSessionBounds(startA, endA, latestDT, sDT, eDT, true);
            if (latestDT < sDT)
            {
                if (!onlyToday || sDT.GetDate() == latestDate)
                {
                    double minsToStart = (sDT - latestDT).GetAsDouble() * 1440.0;
                    if (minsToStart <= previewMinutes && minsToStart >= 0)
                    {
                        const int futRectLine = rectBaseA + 2 * segCountA;
                        const int futTextLine = rectBaseA + 2 * segCountA + 1;

                        s_UseTool R;
                        R.Clear();
                        R.ChartNumber = sc.ChartNumber;
                        R.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
                        R.Region      = sc.GraphRegion;
                        R.BeginDateTime = sDT;
                        R.EndDateTime   = eDT;
                        R.UseRelativeVerticalValues = 1;
                        R.BeginValue  = boxBegin;
                        R.EndValue    = boxEnd;
                        R.Color             = BorderA.PrimaryColor;
                        R.LineWidth         = BorderA.LineWidth;
                        R.LineStyle         = BorderA.LineStyle;
                        R.SecondaryColor    = FillA.PrimaryColor;
                        R.TransparencyLevel = transparency;
                        R.AddAsUserDrawnDrawing = 0;
                        R.LineNumber  = futRectLine;
                        R.AddMethod   = UTAM_ADD_OR_ADJUST;
                        sc.UseTool(R);

                        // Label at center using virtual bar index
                        if (labelA != NULL && labelA[0] != '\0')
                        {
                            SCDateTime midDT = sDT + (eDT - sDT).GetAsDouble() / 2.0;
                            const float boxH = (boxEnd - boxBegin);
                            float labelY = (boxBegin + boxEnd) * 0.5f;
                            float yPct = labelYOffset;
                            if (yPct < -100.0f) yPct = -100.0f;
                            if (yPct >  100.0f) yPct =  100.0f;
                            labelY += boxH * (yPct / 100.0f);
                            const float padInside = boxH * 0.10f;
                            if (labelY < boxBegin + padInside) labelY = boxBegin + padInside;
                            if (labelY > boxEnd   - padInside) labelY = boxEnd   - padInside;

                            s_UseTool T;
                            T.Clear();
                            T.ChartNumber = sc.ChartNumber;
                            T.DrawingType = DRAWING_TEXT;
                            T.Region      = sc.GraphRegion;
                            T.BeginIndex  = FutureBarIndex(midDT);
                            T.UseRelativeVerticalValues = 1;
                            T.BeginValue  = labelY;
                            T.Text        = labelA;
                            T.FontSize    = labelSize;
                            T.TextColor   = labelColor;
                            T.TransparentLabelBackground = 1;
                            T.TextAlignment = DT_CENTER | DT_VCENTER;
                            T.AddAsUserDrawnDrawing = 0;
                            T.LineNumber  = futTextLine;
                            T.AddMethod   = UTAM_ADD_OR_ADJUST;
                            sc.UseTool(T);
                        }
                        else
                        {
                            sc.DeleteACSChartDrawing(sc.ChartNumber, 0, futTextLine);
                        }
                        segCountA++;
                    }
                }
            }
        }
    }

    for (int i = segCountA; i < LastSegCountA; ++i)
    {
        sc.DeleteACSChartDrawing(sc.ChartNumber, 0, rectBaseA + 2 * i);
        sc.DeleteACSChartDrawing(sc.ChartNumber, 0, rectBaseA + 2 * i + 1);
    }
    LastSegCountA = segCountA;

    // -------------------- Session B segments (O(Sessions) Jump Logic) --------------------
    int segCountB = 0;
    if (enableB)
    {
        int i = scanStart;
        while (i <= scanEnd && segCountB < 9999)
        {
            SCDateTime dt = sc.BaseDateTimeIn[i];
            
            // Skip to next allowed day if current day is filtered
            if (!IsDayAllowed(dt.GetDayOfWeek())) {
                i = sc.GetNearestMatchForSCDateTime(sc.ChartNumber, dt.GetDate() + 1.0);
                if (i < 0 || i > scanEnd) break;
                continue;
            }

            if (ShouldHighlight(dt, startB, endB))
            {
                int segStart = i;
                SCDateTime endDT = dt.GetDate() + endB / 86400.0;
                if (startB > endB) endDT += 1.0; // Overnight wrap
                
                i = sc.GetNearestMatchForSCDateTime(sc.ChartNumber, endDT);
                if (i < 0 || i > scanEnd) i = scanEnd;
                
                DrawSegment(segCountB++, segStart, i, BorderB, FillB, labelB, rectBaseB);
            }
            i++;
        }

        // ---- FUTURE PROJECTION: Extend active session B into blank space ----
        SCDateTime lastBarDT = sc.BaseDateTimeIn[sc.ArraySize - 1];
        if (extendActive && ShouldHighlight(lastBarDT, startB, endB) && scanEnd == sc.ArraySize - 1 && segCountB < 9999)
        {
            SCDateTime sDT, eDT;
            GetNextSessionBounds(startB, endB, lastBarDT, sDT, eDT, false);
            if (eDT > latestDT)
            {
                // Delete the historical segment's label — the floating label below replaces it
                sc.DeleteACSChartDrawing(sc.ChartNumber, 0, rectBaseB + 2 * (segCountB - 1) + 1);

                const int futRectLine = rectBaseB + 2 * segCountB;
                const int futTextLine = rectBaseB + 2 * segCountB + 1;

                s_UseTool R;
                R.Clear();
                R.ChartNumber = sc.ChartNumber;
                R.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
                R.Region      = sc.GraphRegion;
                R.BeginDateTime = sc.BaseDateTimeIn[sc.ArraySize - 1];
                R.EndDateTime   = eDT;
                R.UseRelativeVerticalValues = 1;
                R.BeginValue  = boxBegin;
                R.EndValue    = boxEnd;
                R.Color             = BorderB.PrimaryColor;
                R.LineWidth         = BorderB.LineWidth;
                R.LineStyle         = BorderB.LineStyle;
                R.SecondaryColor    = FillB.PrimaryColor;
                R.TransparencyLevel = transparency;
                R.AddAsUserDrawnDrawing = 0;
                R.LineNumber  = futRectLine;
                R.AddMethod   = UTAM_ADD_OR_ADJUST;
                sc.UseTool(R);

                // Floating label centered in visible portion of extended segment
                if (labelB != NULL && labelB[0] != '\0')
                {
                    SCDateTime visStartDT = sc.BaseDateTimeIn[visibleStart];
                    SCDateTime visEndDT   = sc.BaseDateTimeIn[visibleEnd];
                    double barPeriodDays = (sc.SecondsPerBar > 0) ? sc.SecondsPerBar / 86400.0 : 60.0 / 86400.0;
                    SCDateTime visRightEdgeDT = visEndDT + (barPeriodDays * (visibleEnd - visibleStart + 1) * 0.4);

                    SCDateTime effStart = (sDT > visStartDT) ? sDT : visStartDT;
                    SCDateTime effEnd   = (eDT < visRightEdgeDT) ? eDT : visRightEdgeDT;
                    if (effEnd < effStart) effEnd = effStart;

                    SCDateTime midDT = effStart + (effEnd - effStart).GetAsDouble() / 2.0;
                    int labelIdx = FutureBarIndex(midDT);
                    if (midDT <= latestDT)
                    {
                        int bi = sc.GetNearestMatchForSCDateTime(sc.ChartNumber, midDT);
                        if (bi >= 0 && bi < sc.ArraySize) labelIdx = bi;
                    }

                    const float boxH = (boxEnd - boxBegin);
                    float labelY = boxBegin + (boxH * (labelYOffset / 100.0f + 0.5f));

                    s_UseTool T;
                    T.Clear();
                    T.ChartNumber = sc.ChartNumber;
                    T.DrawingType = DRAWING_TEXT;
                    T.Region      = sc.GraphRegion;
                    T.BeginIndex  = labelIdx;
                    T.UseRelativeVerticalValues = 1;
                    T.BeginValue  = labelY;
                    T.Text        = labelB;
                    T.FontSize    = labelSize;
                    T.TextColor   = labelColor;
                    T.TransparentLabelBackground = 1;
                    T.TextAlignment = DT_CENTER | DT_VCENTER;
                    T.AddAsUserDrawnDrawing = 0;
                    T.LineNumber  = futTextLine;
                    T.AddMethod   = UTAM_ADD_OR_ADJUST;
                    sc.UseTool(T);
                }
                segCountB++;
            }
        }

        // ---- FUTURE PROJECTION: Preview upcoming session B ----
        if (previewUpcoming && segCountB < 9999)
        {
            SCDateTime sDT, eDT;
            GetNextSessionBounds(startB, endB, latestDT, sDT, eDT, true);
            if (latestDT < sDT)
            {
                if (!onlyToday || sDT.GetDate() == latestDate)
                {
                    double minsToStart = (sDT - latestDT).GetAsDouble() * 1440.0;
                    if (minsToStart <= previewMinutes && minsToStart >= 0)
                    {
                        const int futRectLine = rectBaseB + 2 * segCountB;
                        const int futTextLine = rectBaseB + 2 * segCountB + 1;

                        s_UseTool R;
                        R.Clear();
                        R.ChartNumber = sc.ChartNumber;
                        R.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
                        R.Region      = sc.GraphRegion;
                        R.BeginDateTime = sDT;
                        R.EndDateTime   = eDT;
                        R.UseRelativeVerticalValues = 1;
                        R.BeginValue  = boxBegin;
                        R.EndValue    = boxEnd;
                        R.Color             = BorderB.PrimaryColor;
                        R.LineWidth         = BorderB.LineWidth;
                        R.LineStyle         = BorderB.LineStyle;
                        R.SecondaryColor    = FillB.PrimaryColor;
                        R.TransparencyLevel = transparency;
                        R.AddAsUserDrawnDrawing = 0;
                        R.LineNumber  = futRectLine;
                        R.AddMethod   = UTAM_ADD_OR_ADJUST;
                        sc.UseTool(R);

                        if (labelB != NULL && labelB[0] != '\0')
                        {
                            SCDateTime midDT = sDT + (eDT - sDT).GetAsDouble() / 2.0;
                            const float boxH = (boxEnd - boxBegin);
                            float labelY = boxBegin + (boxH * (labelYOffset / 100.0f + 0.5f));

                            s_UseTool T;
                            T.Clear();
                            T.ChartNumber = sc.ChartNumber;
                            T.DrawingType = DRAWING_TEXT;
                            T.Region      = sc.GraphRegion;
                            T.BeginIndex  = FutureBarIndex(midDT);
                            T.UseRelativeVerticalValues = 1;
                            T.BeginValue  = labelY;
                            T.Text        = labelB;
                            T.FontSize    = labelSize;
                            T.TextColor   = labelColor;
                            T.TransparentLabelBackground = 1;
                            T.TextAlignment = DT_CENTER | DT_VCENTER;
                            T.AddAsUserDrawnDrawing = 0;
                            T.LineNumber  = futTextLine;
                            T.AddMethod   = UTAM_ADD_OR_ADJUST;
                            sc.UseTool(T);
                        }
                        segCountB++;
                    }
                }
            }
        }
    }

    for (int i = segCountB; i < LastSegCountB; ++i)
    {
        sc.DeleteACSChartDrawing(sc.ChartNumber, 0, rectBaseB + 2 * i);
        sc.DeleteACSChartDrawing(sc.ChartNumber, 0, rectBaseB + 2 * i + 1);
    }
    LastSegCountB = segCountB;

    // -------------------- TZ Tick Labels (anchored to local clock boundaries) --------------------
    
    // Tick labels should be tied to the visible range (even in Stable Rendering Mode),
    // otherwise Max TZ ticks can be consumed far in the past and you see none near the right edge.
    int tickScanStart = scanStart;
    int tickScanEnd   = scanEnd;
    
    if (stableMode)
    {
        const int bufferBars = 500;
        tickScanStart = (visibleStart - bufferBars < 0) ? 0 : (visibleStart - bufferBars);
        tickScanEnd   = (visibleEnd + bufferBars >= sc.ArraySize) ? (sc.ArraySize - 1) : (visibleEnd + bufferBars);
        
        if (onlyToday && tickScanStart < firstToday)
            tickScanStart = firstToday;
    }
    
    int tzTickIndex = 0;
    
    if (showTzTicks)
    {
        // Place tick label inside box near bottom edge
        const float tzTextY = boxBegin + 0.20f * (boxEnd - boxBegin);
        
        // Choose interval
        if (tickEveryMin <= 0)
        {
            double visibleSpanDays = 0.0;
            if (visibleEnd > visibleStart && visibleStart >= 0 && visibleEnd < sc.ArraySize)
            {
                visibleSpanDays = (sc.BaseDateTimeIn[visibleEnd] - sc.BaseDateTimeIn[visibleStart]).GetAsDouble();
            }

            if (visibleSpanDays <= 0.0)
            {
                double barSeconds = sc.SecondsPerBar;
                if (barSeconds <= 0)
                    tickEveryMin = 60;
                else
                {
                    const double barMinutes = barSeconds / 60.0;
                    if (barMinutes <= 1.0)      tickEveryMin = 30;
                    else if (barMinutes <= 5.0) tickEveryMin = 60;
                    else                        tickEveryMin = 120;
                }
            }
            else
            {
                // Dynamic scaling based on visible time span to prevent bunching
                if (visibleSpanDays <= 0.1)        tickEveryMin = 15;   // <= ~2.4 hours
                else if (visibleSpanDays <= 0.25)  tickEveryMin = 30;   // <= 6 hours
                else if (visibleSpanDays <= 0.5)   tickEveryMin = 60;   // <= 12 hours
                else if (visibleSpanDays <= 1.0)   tickEveryMin = 120;  // <= 1 day
                else if (visibleSpanDays <= 2.0)   tickEveryMin = 240;  // <= 2 days
                else if (visibleSpanDays <= 5.0)   tickEveryMin = 480;  // <= 5 days
                else if (visibleSpanDays <= 10.0)  tickEveryMin = 1440; // <= 10 days
                else                               tickEveryMin = 10080;// > 10 days (Weekly)
            }
        }
        if (tickEveryMin < 1)
            tickEveryMin = 1;
        
        const int intervalSeconds = tickEveryMin * 60;
        
        auto FormatLocalTime = [&](const SCDateTime& chartDT, char* out, int outSize)
        {
            SCDateTime localDT = chartDT;
            localDT += (double)offsetSeconds / secondsPerDay;
            
            const int h = localDT.GetHour();
            const int m = localDT.GetMinute();
            const int s = localDT.GetSecond();
            
            if (tickFmt == 2)
                snprintf(out, outSize, "%02d", h);
            else if (tickFmt == 1)
                snprintf(out, outSize, "%02d:%02d:%02d", h, m, s);
            else
                snprintf(out, outSize, "%02d:%02d", h, m);
        };
        
        auto LocalSlotIndex = [&](const SCDateTime& chartDT) -> int
        {
            SCDateTime localDT = chartDT;
            localDT += (double)offsetSeconds / secondsPerDay;
            const int tod = localDT.GetHour() * 3600 + localDT.GetMinute() * 60 + localDT.GetSecond();
            return tod / intervalSeconds;
        };
        
        bool prevInSession = false;
        int prevSlot = -1;
        
        for (int i = tickScanStart; i <= tickScanEnd; ++i)
        {
            if (maxTzTicks > 0 && tzTickIndex >= maxTzTicks)
                break;
            
            const SCDateTime dt = sc.BaseDateTimeIn[i];
            
            const bool inSession =
                ((enableA && ShouldHighlight(dt, startA, endA)) ||
                 (enableB && ShouldHighlight(dt, startB, endB)));
            
            if (!inSession)
            {
                prevInSession = false;
                prevSlot = -1;
                continue;
            }
            
            const int slot = LocalSlotIndex(dt);
            
            if (!prevInSession)
            {
                prevInSession = true;
                prevSlot = slot;
                continue;
            }
            
            if (slot == prevSlot)
                continue;
            
            prevSlot = slot;
            
            const int textNumber = tzBase + tzTickIndex;
            
            char buf[32];
            buf[0] = '\0';
            FormatLocalTime(dt, buf, (int)sizeof(buf));
            
            s_UseTool T;
            T.Clear();
            T.ChartNumber = sc.ChartNumber;
            T.DrawingType = DRAWING_TEXT;
            T.Region      = sc.GraphRegion;
            T.BeginIndex  = i;
            T.UseRelativeVerticalValues = 1;
            T.BeginValue  = tzTextY;
            T.Text        = buf;
            T.FontSize    = tickFontSize;
            T.TextColor   = tickColor;
            T.TransparentLabelBackground = 1;
            T.TextAlignment = DT_CENTER | DT_VCENTER;
            T.AddAsUserDrawnDrawing = 0;
            T.LineNumber  = textNumber;
            T.AddMethod   = UTAM_ADD_OR_ADJUST;
            sc.UseTool(T);
            
            tzTickIndex++;
        }

        int lastFutureTzTickVI = -1;

        // ---- FUTURE PROJECTION: TZ Tick Labels in blank future space ----
        // After drawing historical ticks, generate future ticks from lastBarDT to session end.
        // Uses virtual bar indices (DRAWING_TEXT with BeginIndex) since BeginDateTime is unreliable in blank space.
        auto DrawFutureTick = [&](const SCDateTime& tickDT)
        {
            if (maxTzTicks > 0 && tzTickIndex >= maxTzTicks)
                return;

            int vi = FutureBarIndex(tickDT);
            if (vi == lastFutureTzTickVI)
                return; // Prevent duplicate labels at session crossovers
            lastFutureTzTickVI = vi;

            const int textNumber = tzBase + tzTickIndex;
            char buf[32];
            buf[0] = '\0';
            FormatLocalTime(tickDT, buf, (int)sizeof(buf));

            s_UseTool T;
            T.Clear();
            T.ChartNumber = sc.ChartNumber;
            T.DrawingType = DRAWING_TEXT;
            T.Region      = sc.GraphRegion;
            T.BeginIndex  = vi;  // virtual bar index
            T.UseRelativeVerticalValues = 1;
            T.BeginValue  = tzTextY;
            T.Text        = buf;
            T.FontSize    = tickFontSize;
            T.TextColor   = tickColor;
            T.TransparentLabelBackground = 1;
            T.TextAlignment = DT_CENTER | DT_VCENTER;
            T.AddAsUserDrawnDrawing = 0;
            T.LineNumber  = textNumber;
            T.AddMethod   = UTAM_ADD_OR_ADJUST;
            sc.UseTool(T);

            tzTickIndex++;
        };

        auto DrawFutureRangeTicks = [&](const SCDateTime& rangeStart, const SCDateTime& rangeEnd)
        {
            // Start at the first slot boundary *after* rangeStart
            SCDateTime localStart = rangeStart + (double)offsetSeconds / secondsPerDay;
            // Align localStart to the next interval boundary
            double localSecs = localStart.GetTimeInSeconds(); // uses SCDateTime's built-in time getter
            int startTOD = (int)localSecs;
            int nextSlotTOD = ((startTOD / intervalSeconds) + 1) * intervalSeconds;
            
            SCDateTime currentDT = localStart.GetDate() + (double)nextSlotTOD / secondsPerDay;
            // Convert back to chart time
            currentDT -= (double)offsetSeconds / secondsPerDay;
            
            if (currentDT <= rangeStart) currentDT += (double)intervalSeconds / secondsPerDay;

            while (currentDT < rangeEnd)
            {
                DrawFutureTick(currentDT);
                currentDT += (double)intervalSeconds / secondsPerDay;
            }
        };

        // Extend ticks for active sessions
        if (extendActive && tickScanEnd == sc.ArraySize - 1)
        {
            SCDateTime lastBarDT = sc.BaseDateTimeIn[tickScanEnd];
            if (enableA && ShouldHighlight(lastBarDT, startA, endA))
            {
                SCDateTime sDT, eDT;
                GetNextSessionBounds(startA, endA, lastBarDT, sDT, eDT);
                DrawFutureRangeTicks(lastBarDT, eDT);
            }
            if (enableB && ShouldHighlight(lastBarDT, startB, endB))
            {
                SCDateTime sDT, eDT;
                GetNextSessionBounds(startB, endB, lastBarDT, sDT, eDT);
                DrawFutureRangeTicks(lastBarDT, eDT);
            }
        }

        // Preview ticks for upcoming sessions
        if (previewUpcoming)
        {
            if (enableA)
            {
                SCDateTime sDT, eDT;
                GetNextSessionBounds(startA, endA, latestDT, sDT, eDT, true);
                if (latestDT < sDT && (!onlyToday || sDT.GetDate() == latestDate))
                {
                    double minsToStart = (sDT - latestDT).GetAsDouble() * 1440.0;
                    if (minsToStart <= previewMinutes && minsToStart >= 0)
                        DrawFutureRangeTicks(sDT, eDT);
                }
            }
            if (enableB)
            {
                SCDateTime sDT, eDT;
                GetNextSessionBounds(startB, endB, latestDT, sDT, eDT, true);
                if (latestDT < sDT && (!onlyToday || sDT.GetDate() == latestDate))
                {
                    double minsToStart = (sDT - latestDT).GetAsDouble() * 1440.0;
                    if (minsToStart <= previewMinutes && minsToStart >= 0)
                        DrawFutureRangeTicks(sDT, eDT);
                }
            }
        }
    }
    
    // Always clean up any old TZ tick drawings that are no longer used
    for (int i = tzTickIndex; i < LastTzTickDrawCount; ++i)
        sc.DeleteACSChartDrawing(sc.ChartNumber, 0, tzBase + i);
    
    LastTzTickDrawCount = tzTickIndex;

    // -------------------- Time Markers (vertical lines at specified times) --------------------
    // (Inputs already read above for hash caching)
    
    if (!enableMarkers)
    {
        // Clean up any existing markers
        for (int i = 0; i < LastMarkerDrawCount; ++i)
        {
            sc.DeleteACSChartDrawing(sc.ChartNumber, 0, baseLine + 80000 + 2 * i);      // Line
            sc.DeleteACSChartDrawing(sc.ChartNumber, 0, baseLine + 80000 + 2 * i + 1);  // Label
        }
        LastMarkerDrawCount = 0;
        return;
    }
    
    // Verify markersList string is valid
    if (markersList == NULL || markersList[0] == '\0')
    {
        // No markers defined, clean up
        for (int i = 0; i < LastMarkerDrawCount; ++i)
        {
            sc.DeleteACSChartDrawing(sc.ChartNumber, 0, baseLine + 80000 + 2 * i);
            sc.DeleteACSChartDrawing(sc.ChartNumber, 0, baseLine + 80000 + 2 * i + 1);
        }
        LastMarkerDrawCount = 0;
        return;
    }
    
    // Additional safety: verify string length is reasonable
    int strLen = 0;
    while (markersList[strLen] != '\0' && strLen < 10000) strLen++;
    if (strLen == 0 || strLen >= 10000)
    {
        // Invalid string, clean up
        for (int i = 0; i < LastMarkerDrawCount; ++i)
        {
            sc.DeleteACSChartDrawing(sc.ChartNumber, 0, baseLine + 80000 + 2 * i);
            sc.DeleteACSChartDrawing(sc.ChartNumber, 0, baseLine + 80000 + 2 * i + 1);
        }
        LastMarkerDrawCount = 0;
        return;
    }
    
    // Named color mapping
    auto ParseNamedColor = [&](const char* colorName) -> COLORREF
    {
        if (colorName == NULL || colorName[0] == '\0')
            return markerDefaultColor;
        
        // Simple case-insensitive compare
        char lower[32] = {0};
        int idx = 0;
        while (colorName[idx] && idx < 31)
        {
            lower[idx] = (colorName[idx] >= 'A' && colorName[idx] <= 'Z') ? (colorName[idx] + 32) : colorName[idx];
            idx++;
        }
        lower[idx] = '\0';
        
        if (strcmp(lower, "red") == 0)     return RGB(255, 0, 0);
        if (strcmp(lower, "green") == 0)   return RGB(0, 255, 0);
        if (strcmp(lower, "blue") == 0)    return RGB(0, 128, 255);
        if (strcmp(lower, "yellow") == 0)  return RGB(255, 255, 0);
        if (strcmp(lower, "orange") == 0)  return RGB(255, 165, 0);
        if (strcmp(lower, "cyan") == 0)    return RGB(0, 255, 255);
        if (strcmp(lower, "magenta") == 0) return RGB(255, 0, 255);
        if (strcmp(lower, "white") == 0)   return RGB(255, 255, 255);
        if (strcmp(lower, "gray") == 0)    return RGB(128, 128, 128);
        if (strcmp(lower, "grey") == 0)    return RGB(128, 128, 128);
        
        return markerDefaultColor;
    };
    
    // Parse markers list: HH:MM|Label|ColorName (comma-separated)
    struct TimeMarker
    {
        double time;
        char label[128];
        COLORREF color;
    };
    
    TimeMarker markers[200];
    int markerCount = 0;
    
    // Simple parsing
    const char* p = markersList;
    while (*p && markerCount < 200)
    {
        // Skip whitespace
        while (*p == ' ' || *p == '\t') ++p;
        if (*p == '\0') break;
        
        // Parse time HH:MM
        int hh = 0, mm = 0;
        if (*p >= '0' && *p <= '9')
        {
            hh = (*p++ - '0');
            if (*p >= '0' && *p <= '9') hh = hh * 10 + (*p++ - '0');
        }
        
        if (*p == ':') ++p;
        
        if (*p >= '0' && *p <= '9')
        {
            mm = (*p++ - '0');
            if (*p >= '0' && *p <= '9') mm = mm * 10 + (*p++ - '0');
        }
        
        if (hh > 23) hh = 23;
        if (mm > 59) mm = 59;
        
        markers[markerCount].time = HMS_TIME(hh, mm, 0);
        
        // Parse label (up to '|' or ',')
        int labelIdx = 0;
        if (*p == '|')
        {
            ++p;
            while (*p && *p != ',' && *p != '|' && labelIdx < 127)
            {
                markers[markerCount].label[labelIdx++] = *p++;
            }
        }
        markers[markerCount].label[labelIdx] = '\0';
        
        // Parse optional color
        if (*p == '|')
        {
            ++p;
            char colorName[32] = {0};
            int colorIdx = 0;
            while (*p && *p != ',' && colorIdx < 31)
            {
                colorName[colorIdx++] = *p++;
            }
            colorName[colorIdx] = '\0';
            markers[markerCount].color = ParseNamedColor(colorName);
        }
        else
        {
            markers[markerCount].color = markerDefaultColor;
        }
        
        markerCount++;
        
        // Skip to next marker (past comma)
        while (*p && *p != ',') ++p;
        if (*p == ',') ++p;
    }
    
    // Draw markers
    int lineStyleValue = LINESTYLE_SOLID;
    if (markerLineStyle == 1) lineStyleValue = LINESTYLE_DASH;
    else if (markerLineStyle == 2) lineStyleValue = LINESTYLE_DOT;
    
    const int markerBase = baseLine + 80000;
    int markerDrawIndex = 0;

    // ---- FUTURE PROJECTION: Time Markers (PRIORITY — draw before historical) ----
    // Future markers are drawn first so they get budget priority.
    // Uses virtual bar indices for both DRAWING_LINE and DRAWING_TEXT.
    auto DrawFutureMarker = [&](const SCDateTime& markerDT, const TimeMarker& marker)
    {
        if (markerDrawIndex >= maxMarkersPerDay)
            return;

        const int lineNum  = markerBase + 2 * markerDrawIndex;
        const int labelNum = markerBase + 2 * markerDrawIndex + 1;
        int vi = FutureBarIndex(markerDT);
        // sc.AddMessageToLog(sc.GraphShortName + SCString(": DrawFutureMarker called, vi=") + SCString(vi) + SCString(" label=") + SCString(marker.label), 0);

        // DRAWING_LINE with virtual bar index for future space
        s_UseTool L;
        L.Clear();
        L.ChartNumber = sc.ChartNumber;
        L.DrawingType = DRAWING_LINE;
        L.Region      = sc.GraphRegion;
        L.BeginIndex  = vi;
        L.EndIndex    = vi;
        L.UseRelativeVerticalValues = 1;
        L.BeginValue  = 100.0f;
        L.EndValue    = 100.0f - 5.0f * (boxEnd - boxBegin);
        L.Color       = marker.color;
        L.LineWidth   = markerLineWidth;
        L.LineStyle   = (SubgraphLineStyles)lineStyleValue;
        L.AddAsUserDrawnDrawing = 0;
        L.LineNumber  = lineNum;
        L.AddMethod   = UTAM_ADD_OR_ADJUST;
        sc.UseTool(L);

        if (marker.label[0] != '\0')
        {
            const float labelY = boxBegin - 1.0f + markerLabelVerticalOffset;

            s_UseTool T;
            T.Clear();
            T.ChartNumber = sc.ChartNumber;
            T.DrawingType = DRAWING_TEXT;
            T.Region      = sc.GraphRegion;
            T.BeginIndex  = vi;  // virtual bar index for future text
            T.UseRelativeVerticalValues = 1;
            T.BeginValue  = labelY;
            T.Text        = marker.label;
            T.FontSize    = markerLabelFontSize;
            // Handle fallback to marker color if markerLabelColor is 0, just like the historical loop!
            T.TextColor   = (markerLabelColor == 0) ? marker.color : markerLabelColor;
            T.TransparentLabelBackground = 1;
            T.TextAlignment = DT_CENTER | DT_VCENTER;
            T.AddAsUserDrawnDrawing = 0;
            T.LineNumber  = labelNum;
            T.AddMethod   = UTAM_ADD_OR_ADJUST;
            sc.UseTool(T);
        }
        else
        {
            sc.DeleteACSChartDrawing(sc.ChartNumber, 0, labelNum);
        }

        markerDrawIndex++;
    };

    auto DrawFutureMarkersForSession = [&](double s, double e,
                                           const SCDateTime& anchorDT, bool skipCurrent)
    {
        SCDateTime sDT, eDT;
        GetNextSessionBounds(s, e, anchorDT, sDT, eDT, skipCurrent);

        if (eDT > latestDT)
        {
        // sc.AddMessageToLog(sc.GraphShortName + SCString(": FutureMarkersForSession s=") + SCString((int)(s*1440)) + SCString("m e=") + SCString((int)(e*1440)) + SCString("m sDT=") + sc.DateTimeToString(sDT, FLAG_DT_COMPLETE_DATETIME) + SCString(" eDT=") + sc.DateTimeToString(eDT, FLAG_DT_COMPLETE_DATETIME) + SCString(" mrkCnt=") + SCString(markerCount), 0);
            for (int m = 0; m < markerCount; ++m)
            {
                if (IsTimeInWindow(markers[m].time, s, e))
                {
                    SCDateTime mDT = sDT.GetDate() + (markers[m].time / 86400.0);
                    if (mDT < sDT) mDT += 1.0;  // handle overnight wrap

                    if (mDT > latestDT && mDT <= eDT)
                    {
                        DrawFutureMarker(mDT, markers[m]);
                    }
                }
            }
        }
    };

    // Draw future markers for active sessions
    if (extendActive)
    {
        // sc.AddMessageToLog(sc.GraphShortName + SCString(": extendActive=true, enableA=") + SCString(enableA) + SCString(" enableB=") + SCString(enableB), 0);
        if (enableA && ShouldHighlight(latestDT, startA, endA))
        {
            // sc.AddMessageToLog(sc.GraphShortName + SCString(": Drawing future markers for active session A"), 0);
            DrawFutureMarkersForSession(startA, endA, latestDT, false);
        }
        if (enableB && ShouldHighlight(latestDT, startB, endB))
        {
            // sc.AddMessageToLog(sc.GraphShortName + SCString(": Drawing future markers for active session B"), 0);
            DrawFutureMarkersForSession(startB, endB, latestDT, false);
        }
    }

    // Draw future markers for upcoming preview sessions
    if (previewUpcoming)
    {
        // sc.AddMessageToLog(sc.GraphShortName + SCString(": previewUpcoming=true"), 0);
        if (enableA)
        {
            SCDateTime sDT, eDT;
            GetNextSessionBounds(startA, endA, latestDT, sDT, eDT, true);
            if (latestDT < sDT && (!onlyToday || sDT.GetDate() == latestDate))
            {
                double minsToStart = (sDT - latestDT).GetAsDouble() * 1440.0;
                if (minsToStart <= previewMinutes && minsToStart >= 0)
                    DrawFutureMarkersForSession(startA, endA, latestDT, true);
            }
        }
        if (enableB)
        {
            SCDateTime sDT, eDT;
            GetNextSessionBounds(startB, endB, latestDT, sDT, eDT, true);
            if (latestDT < sDT && (!onlyToday || sDT.GetDate() == latestDate))
            {
                double minsToStart = (sDT - latestDT).GetAsDouble() * 1440.0;
                if (minsToStart <= previewMinutes && minsToStart >= 0)
                    DrawFutureMarkersForSession(startB, endB, latestDT, true);
            }
        }
    }

    // ---- HISTORICAL MARKERS (remaining budget after future markers) ----
    // Independent scan range for markers so they remain stable and do not
    // get skipped by incremental session scanning.
    int markerScanStart = 0;
    int markerScanEnd   = sc.ArraySize - 1;
    if (onlyToday && markerScanStart < firstToday)
        markerScanStart = firstToday;
    
    // Iterate backwards (newest to oldest) so recent markers are drawn first
    for (int d = markerScanEnd; d >= markerScanStart && markerDrawIndex < maxMarkersPerDay; --d)
    {
        const SCDateTime dt = sc.BaseDateTimeIn[d];
        
        // Check day filter
        const int dow = dt.GetDayOfWeek();
        if (!IsDayAllowed(dow))
            continue;
        
        // Check onlyToday filter
        if (onlyToday && (int)dt.GetDate() != latestDate)
            continue;
        
        const double barTime = dt.GetTime();
        
        // Check if this bar matches any marker time
        for (int m = 0; m < markerCount; ++m)
        {
            // Match if bar time is at or just after marker time
            if (barTime >= markers[m].time && barTime < markers[m].time + 0.0001)
            {
                const int lineNum = markerBase + 2 * markerDrawIndex;
                const int labelNum = markerBase + 2 * markerDrawIndex + 1;
                
                // Draw line from top of screen down to twice the box height
                s_UseTool L;
                L.Clear();
                L.ChartNumber = sc.ChartNumber;
                L.DrawingType = DRAWING_LINE;
                L.Region = sc.GraphRegion;
                L.BeginDateTime = dt;
                L.EndDateTime = dt;
                L.UseRelativeVerticalValues = 1;
                L.BeginValue = 100.0f;
                L.EndValue = 100.0f - 5.0f * (boxEnd - boxBegin);
                L.Color = markers[m].color;
                L.LineWidth = markerLineWidth;
                L.LineStyle = (SubgraphLineStyles)lineStyleValue;
                L.AddAsUserDrawnDrawing = 0;
                L.LineNumber = lineNum;
                L.AddMethod = UTAM_ADD_OR_ADJUST;
                sc.UseTool(L);
                
                // Draw label with vertical offset from session box
                if (markers[m].label[0] != '\0')
                {
                    const float labelY = boxBegin - 1.0f + markerLabelVerticalOffset;
                    
                    s_UseTool T;
                    T.Clear();
                    T.ChartNumber = sc.ChartNumber;
                    T.DrawingType = DRAWING_TEXT;
                    T.Region = sc.GraphRegion;
                    T.BeginDateTime = dt;
                    T.UseRelativeVerticalValues = 1;
                    T.BeginValue = labelY;
                    T.Text = markers[m].label;
                    T.FontSize = markerLabelFontSize;
                    T.TextColor = markerLabelColor;
                    T.TransparentLabelBackground = 1;
                    T.TextAlignment = DT_CENTER | DT_VCENTER;
                    T.AddAsUserDrawnDrawing = 0;
                    T.LineNumber = labelNum;
                    T.AddMethod = UTAM_ADD_OR_ADJUST;
                    sc.UseTool(T);
                }
                else
                {
                    sc.DeleteACSChartDrawing(sc.ChartNumber, 0, labelNum);
                }
                
                markerDrawIndex++;
                break;  // Only process first matching marker per bar
            }
        }
    }
    
    // Clean up old markers
    for (int i = markerDrawIndex; i < LastMarkerDrawCount; ++i)
    {
        sc.DeleteACSChartDrawing(sc.ChartNumber, 0, markerBase + 2 * i);
        sc.DeleteACSChartDrawing(sc.ChartNumber, 0, markerBase + 2 * i + 1);
    }
    
    LastMarkerDrawCount = markerDrawIndex;
}
