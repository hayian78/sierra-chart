#include "sierrachart.h"
// Undefine min/max macros from SierraChart/Windows headers to prevent conflicts with C++ standard libraries
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <cstdlib>
#include <ctime>
#include <cmath>
#include <random>
#include <algorithm>

SCDLLName("Trade Entry Drills")

SCSFExport scsf_TradeEntryDrillMultipleContracts(SCStudyInterfaceRef sc)
{
    // --- SUBGRAPHS ---------------------------------------------------------
    SCSubgraphRef LabelStyle = sc.Subgraph[0];

    // --- INPUTS WITH GROUP HEADERS ----------------------------------------
// 0: CORE DRILL CONTROL (header)
SCInputRef In_HeaderCore              = sc.Input[0];
SCInputRef In_RunDrill                = sc.Input[1];
SCInputRef In_Paused                  = sc.Input[2];
SCInputRef In_ControlBarButtonNum     = sc.Input[3];

// 4: DIRECTION & TIMING (header)
SCInputRef In_HeaderDirTime           = sc.Input[4];
SCInputRef In_LongShortMode           = sc.Input[5];
SCInputRef In_GapBetweenSetsMS      = sc.Input[6];
SCInputRef In_DelayEntryToStopMS      = sc.Input[7];
SCInputRef In_DelayStopToTargetMS     = sc.Input[8];

// 9: ENTRY / STOP / TP1 (header)
SCInputRef In_HeaderEntryStopTP1      = sc.Input[9];
SCInputRef In_EntryMinTicks           = sc.Input[10];
SCInputRef In_EntryMaxTicks           = sc.Input[11];
SCInputRef In_StopMinTicks            = sc.Input[12];
SCInputRef In_StopMaxTicks            = sc.Input[13];
SCInputRef In_TPMinTicks              = sc.Input[14];
SCInputRef In_TPMaxTicks              = sc.Input[15];

// 16: DISPLAY & APPEARANCE (header)
SCInputRef In_HeaderDisplay           = sc.Input[16];
SCInputRef In_EnableTextPrompts       = sc.Input[17];
SCInputRef In_EnableLinePrompts       = sc.Input[18];
SCInputRef In_EnableLineValues        = sc.Input[19];
SCInputRef In_LineTextAtEnd           = sc.Input[20];
SCInputRef In_VerticalPositionPercent = sc.Input[21];
SCInputRef In_HorizontalBarsRight     = sc.Input[22];
SCInputRef In_LongBackgroundColor     = sc.Input[23];
SCInputRef In_ShortBackgroundColor    = sc.Input[24];
SCInputRef In_TextColor               = sc.Input[25];
SCInputRef In_LabelEntry              = sc.Input[26];
SCInputRef In_LabelStop               = sc.Input[27];
SCInputRef In_LabelTarget             = sc.Input[28];
SCInputRef In_LineStyle               = sc.Input[29];

// 30: ADVANCED: MULTI-CONTRACT (header)
SCInputRef In_HeaderAdvancedMC        = sc.Input[30];
SCInputRef In_TotalContracts          = sc.Input[31];
SCInputRef In_MaxTPCount              = sc.Input[32];
SCInputRef In_RandomiseTPCount        = sc.Input[33];
SCInputRef In_MinContractsPerTP       = sc.Input[34];
SCInputRef In_TPContractAllocation    = sc.Input[35];   // NEW: Even/Random

// 36: ADVANCED: EXTRA TP DISTANCES (header)
SCInputRef In_HeaderTPExtra           = sc.Input[36];
SCInputRef In_TP2Ticks                = sc.Input[37];
SCInputRef In_TP3Ticks                = sc.Input[38];
SCInputRef In_TP4Ticks                = sc.Input[39];
SCInputRef In_TP5Ticks                = sc.Input[40];
// 41: SESSION TRACKER (header)
SCInputRef In_HeaderSession            = sc.Input[41];
SCInputRef In_SessionButtonNum         = sc.Input[42];
SCInputRef In_SessionHeaderText        = sc.Input[43];
SCInputRef In_SessionTextBarsRight     = sc.Input[44];
SCInputRef In_SessionTextTicksAbove    = sc.Input[45];
SCInputRef In_SessionTextPositionMode  = sc.Input[46];



// --- DEFAULTS ----------------------------------------------------------
if (sc.SetDefaults)
{
    sc.GraphName        = "Trade Entry Drills";
    sc.StudyDescription = "Rapid-fire Entry/Stop/TP drill near current price. Single-contract by default; optional multi-contract / multi-TP mode.";
    sc.GraphRegion      = 0;
    sc.AutoLoop         = 0;
    sc.UpdateAlways     = 1;

    // Text label appearance
    LabelStyle.Name               = "Label Style";
    LabelStyle.DrawStyle          = DRAWSTYLE_CUSTOM_TEXT;
    LabelStyle.PrimaryColor       = RGB(255, 255, 255);
    LabelStyle.SecondaryColorUsed = 1;
    LabelStyle.SecondaryColor     = RGB(0, 200, 0);
    LabelStyle.LineWidth          = 18;
    LabelStyle.DrawZeros          = 0;

    // --- Group headers -------------------------------------------------
    In_HeaderCore.Name        = "═════ CORE DRILL CONTROL ═════";
    In_HeaderCore.SetDescription("CORE DRILL CONTROL");
    In_HeaderCore.SetInt(0);

    In_HeaderDirTime.Name     = "═════ DIRECTION & TIMING ═════";
    In_HeaderDirTime.SetDescription("DIRECTION & TIMING");
    In_HeaderDirTime.SetInt(0);

    In_HeaderEntryStopTP1.Name = "═════ ENTRY / STOP / DEFAULT TP1 ═════";
    In_HeaderEntryStopTP1.SetDescription("ENTRY / STOP / DEFAULT TP1");
    In_HeaderEntryStopTP1.SetInt(0);

    In_HeaderDisplay.Name     = "═════ DISPLAY & APPEARANCE ═════";
    In_HeaderDisplay.SetDescription("DISPLAY & APPEARANCE");
    In_HeaderDisplay.SetInt(0);

    In_HeaderAdvancedMC.Name  = "═════ ADVANCED: MULTI-CONTRACT / MULTI-TP ═════";
    In_HeaderAdvancedMC.SetDescription("ADVANCED: MULTI-CONTRACT / MULTI-TP");
    In_HeaderAdvancedMC.SetInt(0);

    In_HeaderTPExtra.Name     = "═════ ADVANCED: EXTRA TP DISTANCES (TP2–TP5) ═════";
    In_HeaderTPExtra.SetDescription("ADVANCED: EXTRA TP DISTANCES (TP2–TP5)");
    In_HeaderTPExtra.SetInt(0);

    // --- CORE DRILL CONTROL -------------------------------------------
    In_RunDrill.Name = "1. Run Drill";
    In_RunDrill.SetYesNo(true);

    In_Paused.Name = "2. Paused";
    In_Paused.SetYesNo(false);

    In_ControlBarButtonNum.Name = "3. Control Bar Button # (0 = None)";
    In_ControlBarButtonNum.SetInt(1);
    In_ControlBarButtonNum.SetIntLimits(0, 150);

    // --- DIRECTION & TIMING -------------------------------------------
    In_LongShortMode.Name = "4. Trade Direction Mode";
    In_LongShortMode.SetCustomInputStrings("Long Only;Short Only;Both");
    In_LongShortMode.SetCustomInputIndex(0);

    In_GapBetweenSetsMS.Name = "5. Gap Between Sets (ms)";
    In_GapBetweenSetsMS.SetInt(3000);

    In_DelayEntryToStopMS.Name = "6. Delay Entry -> Stop (ms)";
    In_DelayEntryToStopMS.SetInt(1000);

    In_DelayStopToTargetMS.Name = "7. Delay Stop -> TP (ms)";
    In_DelayStopToTargetMS.SetInt(1500);

    // --- ENTRY / STOP / TP1 -------------------------------------------
    In_EntryMinTicks.Name = "8. Entry Dist Min (ticks)";
    In_EntryMinTicks.SetInt(4);
    In_EntryMinTicks.SetIntLimits(1, 1000);

    In_EntryMaxTicks.Name = "9. Entry Dist Max (ticks)";
    In_EntryMaxTicks.SetInt(8);
    In_EntryMaxTicks.SetIntLimits(1, 1000);

    In_StopMinTicks.Name = "10. Stop Dist Min (ticks)";
    In_StopMinTicks.SetInt(5);
    In_StopMinTicks.SetIntLimits(1, 1000);

    In_StopMaxTicks.Name = "11. Stop Dist Max (ticks)";
    In_StopMaxTicks.SetInt(12);
    In_StopMaxTicks.SetIntLimits(1, 1000);

    In_TPMinTicks.Name = "12. TP Dist Min (ticks)";
    In_TPMinTicks.SetInt(10);
    In_TPMinTicks.SetIntLimits(1, 1000);

    In_TPMaxTicks.Name = "13. TP Dist Max (ticks)";
    In_TPMaxTicks.SetInt(20);
    In_TPMaxTicks.SetIntLimits(1, 1000);

    // --- DISPLAY & APPEARANCE (your custom order stays as-is here) ------
    In_EnableTextPrompts.Name  = "17. Show Text Prompts";
    In_EnableTextPrompts.SetYesNo(true);

    In_LabelEntry.Name  = "18. Label for Entry";
    In_LabelEntry.SetString("ENTRY : ");

    In_LabelStop.Name   = "19. Label for Stop";
    In_LabelStop.SetString("STOP  : ");

    In_LabelTarget.Name = "20. Label for Targets (base)";
    In_LabelTarget.SetString("TP");

    In_EnableLinePrompts.Name  = "21. Show Line Prompts";
    In_EnableLinePrompts.SetYesNo(false);

    In_LineStyle.Name = "22. Line Style (Solid/Dash/Dot)";
    In_LineStyle.SetCustomInputStrings("Solid;Dash;Dot");
    In_LineStyle.SetCustomInputIndex(0);

    In_EnableLineValues.Name   = "23. Show Values On Lines";
    In_EnableLineValues.SetYesNo(true);

    In_LineTextAtEnd.Name      = "24. Draw Line Text At End (Right)";
    In_LineTextAtEnd.SetYesNo(true);

    In_VerticalPositionPercent.Name = "25. Text Vertical Position % (0=bottom,100=top)";
    In_VerticalPositionPercent.SetInt(85);
    In_VerticalPositionPercent.SetIntLimits(0, 100);

    In_HorizontalBarsRight.Name = "26. Text Bars To Right Of Last Bar";
    In_HorizontalBarsRight.SetInt(3);
    In_HorizontalBarsRight.SetIntLimits(0, 200);

    In_LongBackgroundColor.Name = "27. Long Text Background Color";
    In_LongBackgroundColor.SetColor(RGB(0, 160, 0));

    In_ShortBackgroundColor.Name = "28. Short Text Background Color";
    In_ShortBackgroundColor.SetColor(RGB(200, 0, 0));

    In_TextColor.Name = "29. Text Color";
    In_TextColor.SetColor(RGB(255, 255, 255));

    // --- ADVANCED: MULTI-CONTRACT -------------------------------------
    In_TotalContracts.Name = "30. Total Contracts";
    In_TotalContracts.SetInt(1);
    In_TotalContracts.SetIntLimits(1, 5);

    In_MaxTPCount.Name = "31. Max TP Count";
    In_MaxTPCount.SetInt(3);
    In_MaxTPCount.SetIntLimits(1, 5);

    In_RandomiseTPCount.Name = "32. Randomise TP Count";
    In_RandomiseTPCount.SetYesNo(0);

    In_MinContractsPerTP.Name = "33. Min Contracts Per TP (0 or 1)";
    In_MinContractsPerTP.SetInt(1);
    In_MinContractsPerTP.SetIntLimits(0, 1);

    In_TPContractAllocation.Name = "34. TP Contract Allocation";
    In_TPContractAllocation.SetCustomInputStrings("Even;Random");
    In_TPContractAllocation.SetCustomInputIndex(1); // default Random

    // --- ADVANCED: EXTRA TP DISTANCES ---------------------------------
    In_TP2Ticks.Name = "35. TP2 Base Distance (ticks)";
    In_TP2Ticks.SetInt(8);

    In_TP3Ticks.Name = "36. TP3 Base Distance (ticks)";
    In_TP3Ticks.SetInt(12);

    In_TP4Ticks.Name = "37. TP4 Base Distance (ticks)";
    In_TP4Ticks.SetInt(16);

    In_TP5Ticks.Name = "38. TP5 Base Distance (ticks)";
    In_TP5Ticks.SetInt(20);

    // --- SESSION TRACKER ----------------------------------------------
    In_HeaderSession.Name = "═════ SESSION TRACKER ═════";
    In_HeaderSession.SetString("SESSION TRACKER");

    In_SessionButtonNum.Name = "39. Session Tracker Button # (0 = None)";
    In_SessionButtonNum.SetInt(2);
    In_SessionButtonNum.SetIntLimits(0, 150);

    In_SessionHeaderText.Name = "40. Session Text Header";
    In_SessionHeaderText.SetString("Trade Entry Drill Details");

    In_SessionTextBarsRight.Name = "41. Session Text Bars Right of Last Bar";
    In_SessionTextBarsRight.SetInt(10);
    In_SessionTextBarsRight.SetIntLimits(0, 200);

    In_SessionTextTicksAbove.Name = "42. Session Text Ticks Above Last Price";
    In_SessionTextTicksAbove.SetInt(20);
    In_SessionTextTicksAbove.SetIntLimits(-200, 400);

    In_SessionTextPositionMode.Name = "43. Session Text Position Mode";
    In_SessionTextPositionMode.SetCustomInputStrings("Auto (stick to last bar);Manual (draggable)");
    In_SessionTextPositionMode.SetCustomInputIndex(1);
    return;
}









    // --- PERSISTENT STATE --------------------------------------------------
    enum DrillState
    {
        STATE_GAP = 0,
        STATE_SHOW_ENTRY,
        STATE_SHOW_STOP,
        STATE_SHOW_TP,
        STATE_SHOW_TARGETS
    };

    int&   State            = sc.GetPersistentInt(0);
    int&   CurrentSide      = sc.GetPersistentInt(1);  // +1 = long, -1 = short
    int&   LabelLineNumber  = sc.GetPersistentInt(2);
    int&   EntryLineNumber  = sc.GetPersistentInt(3);
    int&   StopLineNumber   = sc.GetPersistentInt(4);
    int&   TPLineNumber1    = sc.GetPersistentInt(5);
    int&   TPLineNumber2    = sc.GetPersistentInt(6);
    int&   TPLineNumber3    = sc.GetPersistentInt(7);
    int&   TPLineNumber4    = sc.GetPersistentInt(8);
    int&   TPLineNumber5    = sc.GetPersistentInt(9);
    int&   RNGSeeded        = sc.GetPersistentInt(10);
    int&   TPCount          = sc.GetPersistentInt(11);
    int&   TPContracts1     = sc.GetPersistentInt(12);
    int&   TPContracts2     = sc.GetPersistentInt(13);
    int&   TPContracts3     = sc.GetPersistentInt(14);
    int&   TPContracts4     = sc.GetPersistentInt(15);
    int&   TPContracts5     = sc.GetPersistentInt(16);
    int&   CurrentTPIndex   = sc.GetPersistentInt(17);

    float& EntryPrice  = sc.GetPersistentFloat(0);
    float& StopPrice   = sc.GetPersistentFloat(1);
    float& TPPrice1    = sc.GetPersistentFloat(2);
    float& TPPrice2    = sc.GetPersistentFloat(3);
    float& TPPrice3    = sc.GetPersistentFloat(4);
    float& TPPrice4    = sc.GetPersistentFloat(5);
    float& TPPrice5    = sc.GetPersistentFloat(6);

    SCDateTime& NextEventTime = sc.GetPersistentSCDateTime(0);

    if (sc.IsFullRecalculation && sc.UpdateStartIndex == 0)
    {
        State            = STATE_GAP;
        CurrentSide      = 1;
        LabelLineNumber  = 0;
        EntryLineNumber  = 0;
        StopLineNumber   = 0;
        TPLineNumber1    = 0;
        TPLineNumber2    = 0;
        TPLineNumber3    = 0;
        TPLineNumber4    = 0;
        TPLineNumber5    = 0;
        RNGSeeded        = 0;
        TPCount          = 0;
        TPContracts1     = TPContracts2 = TPContracts3 = TPContracts4 = TPContracts5 = 0;
        CurrentTPIndex   = 0;
        EntryPrice       = 0.0f;
        StopPrice        = 0.0f;
        TPPrice1 = TPPrice2 = TPPrice3 = TPPrice4 = TPPrice5 = 0.0f;
        NextEventTime    = sc.CurrentSystemDateTime;
    }

    bool Paused       = In_Paused.GetYesNo() != 0;
    int  ButtonNumber = In_ControlBarButtonNum.GetInt();
    int  SessionButtonNumber     = In_SessionButtonNum.GetInt();
    int  SessionPosMode          = In_SessionTextPositionMode.GetIndex(); // 0=Auto,1=Manual

    // Sierra can provide sc.MenuEventID as either:
    //  - the raw CS# (1..150), OR
    //  - the menu ID (ACS_BUTTON_1 ..)
    // depending on which control bar layout is active.
    auto ButtonMenuIDFromNumber = [&](int n) -> int
    {
        if (n <= 0)
            return 0;
        return ACS_BUTTON_1 + (n - 1);
    };

    auto MatchesButton = [&](int n) -> bool
    {
        if (n <= 0)
            return false;
        const int menuID = ButtonMenuIDFromNumber(n);
        return sc.MenuEventID == n || (menuID != 0 && sc.MenuEventID == menuID);
    };



    auto UpdateButtonUI = [&](bool paused)
    {
        if (ButtonNumber <= 0)
            return;

        const char* caption = paused ? "Resume" : "Pause";
        sc.SetCustomStudyControlBarButtonText(ButtonNumber, caption);

        SCString hover;
        hover.Format("Toggle Trade Entry Drill (%s)",
                     paused ? "Paused" : "Running");
        sc.SetCustomStudyControlBarButtonHoverText(ButtonNumber, hover);

        sc.SetCustomStudyControlBarButtonEnable(ButtonNumber, paused ? 1 : 0);
    };

    // One-time button labels / hover text for session tracker utilities.
    if (sc.IsFullRecalculation && sc.UpdateStartIndex == 0)
    {
        if (SessionButtonNumber > 0)
        {
            sc.SetCustomStudyControlBarButtonText(SessionButtonNumber, "Reset Session");
            sc.SetCustomStudyControlBarButtonHoverText(SessionButtonNumber, "Reset/Create the session counter box");
        }
    }

    auto ClearTPLine = [&](int& lineNum)
    {
        if (lineNum != 0)
        {
            sc.DeleteACSChartDrawing(sc.ChartNumber, 0, lineNum);
            lineNum = 0;
        }
    };

    auto ClearAllLines = [&]()
    {
        if (EntryLineNumber != 0)
        {
            sc.DeleteACSChartDrawing(sc.ChartNumber, 0, EntryLineNumber);
            EntryLineNumber = 0;
        }
        if (StopLineNumber != 0)
        {
            sc.DeleteACSChartDrawing(sc.ChartNumber, 0, StopLineNumber);
            StopLineNumber = 0;
        }
        ClearTPLine(TPLineNumber1);
        ClearTPLine(TPLineNumber2);
        ClearTPLine(TPLineNumber3);
        ClearTPLine(TPLineNumber4);
        ClearTPLine(TPLineNumber5);
    };

    auto ClearLabel = [&]()
    {
        if (LabelLineNumber != 0)
        {
            sc.DeleteACSChartDrawing(sc.ChartNumber, 0, LabelLineNumber);
            LabelLineNumber = 0;
        }
    };

    if (sc.LastCallToFunction)
    {
        if (ButtonNumber > 0)
        {
            sc.SetCustomStudyControlBarButtonHoverText(ButtonNumber, "");
            sc.SetCustomStudyControlBarButtonText(ButtonNumber, "");
            sc.SetCustomStudyControlBarButtonEnable(ButtonNumber, 0);
        }
        return;
    }

    if (ButtonNumber > 0 && sc.UpdateStartIndex == 0)
        UpdateButtonUI(Paused);

    const float TickSize = (float)sc.TickSize;

    // Persist the last used ticks to prevent exact duplicates where possible.
    int& LastEntryTicks = sc.GetPersistentInt(20);
    int& LastStopTicks  = sc.GetPersistentInt(21);
    int& LastTPTicks    = sc.GetPersistentInt(22);

    auto GetRandomTicks = [&](int minTicks, int maxTicks, int& lastVal) -> int
    {
        if (maxTicks < minTicks) maxTicks = minTicks;
        
        static std::mt19937 rng(std::random_device{}()); 
        // Note: static rng means it's shared across instances but seeded once. 
        // For a study this is usually fine/desired for randomness.
        // If exact reproducibility per instance is needed, we'd persist the seed.
        
        std::uniform_int_distribution<int> dist(minTicks, maxTicks);
        
        int val = dist(rng);
        
        // Anti-duplicate logic:
        // If range allows for >1 option, ensure we don't pick the exact same one twice in a row.
        // Try up to 3 times to get a different number.
        if (maxTicks > minTicks)
        {
            int attempts = 0;
            while (val == lastVal && attempts < 3)
            {
                val = dist(rng);
                attempts++;
            }
        }
        
        lastVal = val;
        return val;
    };
    
    // Legacy helper kept just in case but likely unused now
    auto RandInt = [](int minVal, int maxVal) -> int
    {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(minVal, maxVal);
        return dist(rng);
    };

    auto FormatPrice2 = [](float value) -> SCString
    {
        SCString s;
        s.Format("%.2f", value);
        return s;
    };

    auto GetLineStyle = [&](int styleIndex) -> SubgraphLineStyles
    {
        switch (styleIndex)
        {
        default:
        case 0: return LINESTYLE_SOLID;
        case 1: return LINESTYLE_DASH;
        case 2: return LINESTYLE_DOT;
        }
    };

    const bool showText       = In_EnableTextPrompts.GetYesNo();
    const bool showLines      = In_EnableLinePrompts.GetYesNo();
    const bool showLineValues = In_EnableLineValues.GetYesNo();
    const bool drawTextAtEnd  = In_LineTextAtEnd.GetYesNo();
    const bool multiEnabled   = false; // reserved flag now unused

    SCDateTime Now = sc.CurrentSystemDateTime;

    // --- SESSION TRACKER (manual counter; no DOM trade detection) ------
    const int SessCountKey = 901;
    const int SessStartKey = 902; // double (SCDateTime) stored in persistent double
    const int SessEndKey   = 903; // double (SCDateTime) stored in persistent double

    auto FormatHMS = [&](const SCDateTime& dt) -> SCString
    {
        SCString s;
        s.Format("%02d:%02d:%02d", dt.GetHour(), dt.GetMinute(), dt.GetSecond());
        return s;
    };

    auto DrawSessionText = [&]()
    {
        SCString header = In_SessionHeaderText.GetString();
        if (header.GetLength() == 0)
            header = "Trade Entry Drill Details";

        const int count = sc.GetPersistentInt(SessCountKey);
        const double startD = sc.GetPersistentDouble(SessStartKey);
        const double endD   = sc.GetPersistentDouble(SessEndKey);

        SCString startStr = "--:--:--";
        SCString endStr   = "--:--:--";

        if (startD > 0.0)
        {
            SCDateTime dt;
            dt.SetDateTimeAsDouble(startD);
            startStr = FormatHMS(dt);
        }
        if (endD > 0.0)
        {
            SCDateTime dt;
            dt.SetDateTimeAsDouble(endD);
            endStr = FormatHMS(dt);
        }

        // Include a compact snapshot of key settings (kept to one short line).
        SCString modeStr;
        switch (In_LongShortMode.GetIndex())
        {
        default:
        case 0: modeStr = "Long";  break;
        case 1: modeStr = "Short"; break;
        case 2: modeStr = "Both";  break;
        }

        const double gapSec   = (double)In_GapBetweenSetsMS.GetInt() / 1000.0;
        const double entrySec = (double)In_DelayEntryToStopMS.GetInt() / 1000.0;
        const double stopSec  = (double)In_DelayStopToTargetMS.GetInt() / 1000.0;

        SCString text;
        text.Format(
            "%s\n"
            "Mode: %s  Set: %.1fs  Stop: %.1fs  Profit: %.1fs\n"
            "Start Time: %s\n"
            "End Time: %s\n"
            "Drill Trades Taken: %d",
            header.GetChars(),
            modeStr.GetChars(),
            gapSec,
            entrySec,
            stopSec,
            startStr.GetChars(),
            endStr.GetChars(),
            count
        );

        const int barsRight = In_SessionTextBarsRight.GetInt();
        const int ticksUp   = In_SessionTextTicksAbove.GetInt();
        const int lastIndex = sc.ArraySize - 1;
        const int drawIndex = lastIndex + barsRight;
        const float basePrice = (float)sc.Close[lastIndex];
        const float drawPrice = basePrice + (float)ticksUp * (float)sc.TickSize;

        int finalIndex = drawIndex;
        float finalPrice = drawPrice;

        // If in Manual mode, preserve the user's dragged location by reading the existing drawing position.
        if (SessionPosMode == 1)
        {
            s_UseTool existing;
            existing.Clear();
            existing.ChartNumber = sc.ChartNumber;
            existing.LineNumber  = 600000 + sc.StudyGraphInstanceID;
            
            // Preserve the user's dragged location by reading the existing user-drawn drawing.
            if (sc.GetUserDrawnDrawingByLineNumber(sc.ChartNumber, existing.LineNumber, existing))
            {
                finalIndex = existing.BeginIndex;
                finalPrice = (float)existing.BeginValue;
            }
        }

        const int lineNumber = 600000 + sc.StudyGraphInstanceID;

        s_UseTool t;
        t.Clear();
        t.ChartNumber  = sc.ChartNumber;
        t.DrawingType  = DRAWING_TEXT;
        t.Region       = 0;
        t.BeginIndex   = finalIndex;
        t.BeginValue   = finalPrice;
        t.LineNumber   = lineNumber;
        t.AddAsUserDrawnDrawing = 1;
        t.LockDrawing  = 0;
        t.AddMethod    = UTAM_ADD_OR_ADJUST;
        t.Color        = RGB(255,255,255);
        t.FontBackColor = sc.ChartBackgroundColor;
        t.TransparentLabelBackground = 0;
        t.Text         = text;
        sc.UseTool(t);
    };

    auto ResetSession = [&]()
    {
        sc.SetPersistentInt(SessCountKey, 0);
        sc.SetPersistentDouble(SessStartKey, Now.GetAsDouble());
        sc.SetPersistentDouble(SessEndKey, 0.0);
        DrawSessionText();
    };

    auto IncrementSession = [&]()
    {
        const double startD = sc.GetPersistentDouble(SessStartKey);
        if (startD <= 0.0)
            return; // session not started (requires button press)

        int count = sc.GetPersistentInt(SessCountKey);
        count++;
        sc.SetPersistentInt(SessCountKey, count);
        sc.SetPersistentDouble(SessEndKey,   Now.GetAsDouble());
        DrawSessionText();
    };


    if (MatchesButton(ButtonNumber))
    {
        bool isOn = (sc.PointerEventType == SC_ACS_BUTTON_ON);
        In_Paused.SetYesNo(isOn ? 1 : 0);
        Paused = isOn;

        UpdateButtonUI(Paused);

        State         = STATE_GAP;
        NextEventTime = sc.CurrentSystemDateTime;
        ClearAllLines();
        ClearLabel();
    }

    if (MatchesButton(SessionButtonNumber))
    {
        // Create/reset the session text box (manual tracking only)
        ResetSession();

        // Make the tracker button momentary (always clickable).
        if (SessionButtonNumber > 0)
            sc.SetCustomStudyControlBarButtonEnable(SessionButtonNumber, 0);
    }

    
    if (!In_RunDrill.GetYesNo() || Paused || sc.ArraySize < 1)
        return;

    if (Now >= NextEventTime)
    {
        switch (State)
        {
        case STATE_GAP:
        {
            ClearAllLines();
            ClearLabel();

            int mode = In_LongShortMode.GetIndex(); // 0=Long,1=Short,2=Both
            if (mode == 0)
                CurrentSide = 1;
            else if (mode == 1)
                CurrentSide = -1;
            else
                CurrentSide = (RandInt(0, 1) == 0) ? 1 : -1;

            const int lastIndex = sc.ArraySize - 1;
            float lastPrice     = (float)sc.Close[lastIndex];

            // ENTRY
            int entryTicks = GetRandomTicks(In_EntryMinTicks.GetInt(), In_EntryMaxTicks.GetInt(), LastEntryTicks);
            float entryOffset = (float)entryTicks * TickSize;

            // STOP
            int stopTicks  = GetRandomTicks(In_StopMinTicks.GetInt(), In_StopMaxTicks.GetInt(), LastStopTicks);
            float stopOffset = (float)stopTicks * TickSize;

            if (CurrentSide == 1)
            {
                EntryPrice = lastPrice + entryOffset;
                StopPrice  = EntryPrice - stopOffset;
            }
            else
            {
                EntryPrice = lastPrice - entryOffset;
                StopPrice  = EntryPrice + stopOffset;
            }

            // --- TP Calculation ---
            int totalContracts = In_TotalContracts.GetInt();
            if (totalContracts < 1) totalContracts = 1;
            if (totalContracts > 5) totalContracts = 5;

            // Simplified: We now use TP Min/Max ticks for the logic.
            // If using multi-contract, we might need multiple TPs.
            // For now, we apply the simpler logic: uniform random TPs usually.
            
            // Note: The previous logic had a complex Percent-based approach.
            // We'll replace it with:
            // TP1 = Random(Min, Max)
            // Extra TPs (TP2-TP5) currently use fixed inputs (In_TP2Ticks, etc). 
            // The user only asked to simplify the *main* random generation.
            // We will respect that. For the PRIMARY random generation (TP1 or uniform), we use Min/Max.

            TPCount        = 0;
            TPContracts1   = TPContracts2 = TPContracts3 = TPContracts4 = TPContracts5 = 0;
            TPPrice1       = TPPrice2 = TPPrice3 = TPPrice4 = TPPrice5 = 0.0f;
            CurrentTPIndex = 0;

            bool useMulti = true; 

            if (useMulti)
            {
                int maxTPCount = In_MaxTPCount.GetInt();
                if (maxTPCount < 1) maxTPCount = 1;
                if (maxTPCount > 5) maxTPCount = 5;

                bool randTPCount      = In_RandomiseTPCount.GetYesNo() != 0;
                bool randTPContracts = (In_TPContractAllocation.GetIndex() == 1); // 0=Even, 1=Random

                int  minContractsPerTP= In_MinContractsPerTP.GetInt();
                if (minContractsPerTP < 0) minContractsPerTP = 0;
                if (minContractsPerTP > 1) minContractsPerTP = 1;

                if (randTPCount)
                    TPCount = RandInt(1, maxTPCount);
                else
                    TPCount = maxTPCount;

                if (TPCount < 1) TPCount = 1;
                if (TPCount > 5) TPCount = 5;

                if (minContractsPerTP == 1 && TPCount > totalContracts)
                    TPCount = totalContracts;

                if (TPCount < 1) TPCount = 1;

                // For TP logic:
                // TP1 is random within the NEW Min/Max range.
                // But wait, the original code had TP1Base, TP2Ticks (Fixed), etc.
                // The user complained about randomness.
                // We should probably randomize ALL TPs if they are meant to be random, 
                // OR just TP1? 
                // The original code used `tpTicks[0] = In_TP1Base` then applied a random %.
                // TP2..5 were fixed inputs.
                // If we want "Better randomness", we should probably apply the Random Min/Max to ALL of them?
                // OR maybe just TP1 is the dynamic one?
                // Let's stick to the interpretation: TP1 is the main dynamic one. 
                // TP2..5 are "Advanced Extra" which are fixed.
                
                // However, the original code APPLIED the random percent to ALL TPs in the loop!
                // `double pct = RandFloat(tpMinPct, tpMaxPct);` was inside the loop.
                // So all TPs were jittered.
                
                // We should replicate this "jitter" or random selection.
                // New logic:
                // For each TP, we want a target. 
                // IF it's TP1, we use GetRandomTicks(Min, Max).
                // IF it's TP2..5, we use their fixed ticks... but should we jitter them?
                // The user said "Get what I want from ... profit".
                // Simple approach: 
                // TP1 = Random(Min, Max).
                // TP2..5 = Fixed inputs (as defined in Advanced). 
                // If the user wants random TP2, they can request it, but usually drill = 1 main target.
                
                // Let's implement: TP1 is random. TP2..5 are fixed (as per their inputs).
                
                float* TPPrices[5] = { &TPPrice1, &TPPrice2, &TPPrice3, &TPPrice4, &TPPrice5 };
                int*   TPContracts[5] = { &TPContracts1, &TPContracts2, &TPContracts3,
                                          &TPContracts4, &TPContracts5 };

                for (int i = 0; i < TPCount; ++i)
                {
                    int distTicks = 0;
                    if (i == 0)
                    {
                        // TP1 uses the dynamic random range
                        distTicks = GetRandomTicks(In_TPMinTicks.GetInt(), In_TPMaxTicks.GetInt(), LastTPTicks);
                    }
                    else
                    {
                        // TP2..5 use their specific inputs (no jitter for now, exact fixed distance)
                        // Accessing inputs In_TP2Ticks(37) to In_TP5Ticks(40)
                        s_SCInput_145* extraTPInputs[] = { &In_TP2Ticks, &In_TP3Ticks, &In_TP4Ticks, &In_TP5Ticks };
                        distTicks = extraTPInputs[i-1]->GetInt();
                    }
                    
                    float dist = (float)distTicks * TickSize;
                    
                    if (CurrentSide == 1)
                        *TPPrices[i] = EntryPrice + dist;
                    else
                        *TPPrices[i] = EntryPrice - dist;
                }

                if (!randTPContracts)
                {
                    if (minContractsPerTP == 1)
                    {
                        for (int i = 0; i < TPCount; ++i)
                            *TPContracts[i] = 1;

                        int remaining = totalContracts - TPCount;
                        if (remaining < 0) remaining = 0;

                        int perExtra = (TPCount > 0) ? (remaining / TPCount) : 0;
                        int rem      = (TPCount > 0) ? (remaining % TPCount) : 0;

                        for (int i = 0; i < TPCount; ++i)
                        {
                            *TPContracts[i] += perExtra;
                            if (i < rem)
                                (*TPContracts[i])++;
                        }
                    }
                    else
                    {
                        int base = (TPCount > 0) ? (totalContracts / TPCount) : 0;
                        int rem  = (TPCount > 0) ? (totalContracts % TPCount) : 0;

                        for (int i = 0; i < TPCount; ++i)
                            *TPContracts[i] = base;

                        for (int i = 0; i < rem; ++i)
                            (*TPContracts[i])++;
                    }
                }
                else
                {
                    if (minContractsPerTP == 1)
                    {
                        for (int i = 0; i < TPCount; ++i)
                            *TPContracts[i] = 1;

                        int remaining = totalContracts - TPCount;
                        if (remaining < 0) remaining = 0;

                        for (int k = 0; k < remaining; ++k)
                        {
                            int idx = RandInt(0, TPCount - 1);
                            (*TPContracts[idx])++;
                        }
                    }
                    else
                    {
                        for (int i = 0; i < TPCount; ++i)
                            *TPContracts[i] = 0;

                        for (int k = 0; k < totalContracts; ++k)
                        {
                            int idx = RandInt(0, TPCount - 1);
                            (*TPContracts[idx])++;
                        }
                    }
                }

                CurrentTPIndex = 0;
            }
            else
            {
                // (not used now, but kept for structure)
                TPCount      = 1;
                // uses TP1 Random
                int distTicks = GetRandomTicks(In_TPMinTicks.GetInt(), In_TPMaxTicks.GetInt(), LastTPTicks);
                float dist = (float)distTicks * TickSize;
                
                if (CurrentSide == 1)
                     TPPrice1 = EntryPrice + dist;
                else
                     TPPrice1 = EntryPrice - dist;

                TPContracts1 = totalContracts;
                TPContracts2 = TPContracts3 = TPContracts4 = TPContracts5 = 0;
                CurrentTPIndex = 0;
            }

            IncrementSession();

            State = STATE_SHOW_ENTRY;

            double msToDays =
                (double)In_DelayEntryToStopMS.GetInt() / 86400000.0;
            NextEventTime = Now + msToDays;
        }
        break;

        case STATE_SHOW_ENTRY:
        {
            State = STATE_SHOW_STOP;

            double msToDays =
                (double)In_DelayStopToTargetMS.GetInt() / 86400000.0;
            NextEventTime = Now + msToDays;
        }
        break;

        case STATE_SHOW_STOP:
        {
            if (TPCount > 0)
            {
                State = STATE_SHOW_TP;
                CurrentTPIndex = 0;

                double msToDays =
                    (double)In_DelayStopToTargetMS.GetInt() / 86400000.0;
                NextEventTime = Now + msToDays;
            }
            else
            {
                State = STATE_SHOW_TARGETS;
                double secGap =
                    (double)In_GapBetweenSetsMS.GetInt() / 86400000.0;
                NextEventTime = Now + secGap;
            }
        }
        break;

        case STATE_SHOW_TP:
        {
            CurrentTPIndex++;

            if (CurrentTPIndex >= TPCount)
            {
                State = STATE_SHOW_TARGETS;
                double secGap =
                    (double)In_GapBetweenSetsMS.GetInt() / 86400000.0;
                NextEventTime = Now + secGap;
            }
            else
            {
                double msToDays =
                    (double)In_DelayStopToTargetMS.GetInt() / 86400000.0;
                NextEventTime = Now + msToDays;
            }
        }
        break;

        case STATE_SHOW_TARGETS:
        default:
        {
            ClearAllLines();
            ClearLabel();

            State = STATE_GAP;

            double secGap =
                (double)In_GapBetweenSetsMS.GetInt() / 86400000.0;
            NextEventTime = Now + secGap;

            return;
        }
        }
    }

    if (State == STATE_GAP)
    {
        ClearAllLines();
        ClearLabel();
        return;
    }

    // --- BUILD TEXT BLOCK --------------------------------------------------
    int totalContracts = In_TotalContracts.GetInt();
    if (totalContracts < 1) totalContracts = 1;
    if (totalContracts > 5) totalContracts = 5;

    SCString labelEntry  = In_LabelEntry.GetString();
    SCString labelStop   = In_LabelStop.GetString();

    float  tpPrices[5]    = { TPPrice1, TPPrice2, TPPrice3, TPPrice4, TPPrice5 };
    int    tpContracts[5] = { TPContracts1, TPContracts2, TPContracts3,
                              TPContracts4, TPContracts5 };

    SCString text;

    text.Format("%s %d\n", (CurrentSide == 1 ? "LONG" : "SHORT"), totalContracts);

    text += labelEntry;
    text += FormatPrice2(EntryPrice);
    text += "\n";

    if (State >= STATE_SHOW_STOP)
    {
        text += labelStop;
        text += FormatPrice2(StopPrice);
        text += "\n";
    }

    if (TPCount > 0)
    {
        if (State == STATE_SHOW_TP)
        {
            for (int i = 0; i <= CurrentTPIndex && i < TPCount; ++i)
            {
                if (tpContracts[i] <= 0)
                    continue;

                SCString line;
                line.Format("TP%d   : ", i + 1);
                line += FormatPrice2(tpPrices[i]);
                line += " (";
                {
                    SCString tmp;
                    tmp.Format("%d", tpContracts[i]);
                    line += tmp;
                }
                line += ")";

                text += line;

                if (i < CurrentTPIndex && i != TPCount - 1)
                    text += "\n";
            }
        }
        else if (State >= STATE_SHOW_TARGETS)
        {
            bool firstPrinted = false;
            for (int i = 0; i < TPCount; ++i)
            {
                if (tpContracts[i] <= 0)
                    continue;

                if (firstPrinted)
                    text += "\n";

                SCString line;
                line.Format("TP%d   : ", i + 1);
                line += FormatPrice2(tpPrices[i]);
                line += " (";
                {
                    SCString tmp;
                    tmp.Format("%d", tpContracts[i]);
                    line += tmp;
                }
                line += ")";

                text += line;
                firstPrinted = true;
            }
        }
    }

    // --- DRAW / REMOVE TEXT PROMPTS ---------------------------------------
    if (showText)
    {
        const int lastIndex = sc.ArraySize - 1;
        SCDateTime lastDT   = sc.BaseDateTimeIn[lastIndex];

        int barsRight = In_HorizontalBarsRight.GetInt();
        if (barsRight < 0) barsRight = 0;

        double secondsPerBar = sc.SecondsPerBar;
        if (secondsPerBar <= 0.0)
            secondsPerBar = 60.0;

        double offsetDays = (double)barsRight * secondsPerBar / 86400.0;
        SCDateTime labelDT = lastDT + offsetDays;

        s_UseTool Tool;
        Tool.Clear();
        Tool.ChartNumber = sc.ChartNumber;
        Tool.DrawingType = DRAWING_TEXT;
        Tool.Region      = sc.GraphRegion;

        Tool.BeginDateTime = labelDT;

        Tool.UseRelativeVerticalValues = 1;
        Tool.BeginValue                = (float)In_VerticalPositionPercent.GetInt();

        Tool.Text           = text;
        Tool.MultiLineLabel = 1;
        Tool.TextAlignment  = DT_LEFT | DT_TOP;

        Tool.FontSize = LabelStyle.LineWidth;
        Tool.FontBold = 1;

        Tool.Color         = In_TextColor.GetColor();
        Tool.FontBackColor = (CurrentSide == 1
                              ? In_LongBackgroundColor.GetColor()
                              : In_ShortBackgroundColor.GetColor());

        Tool.TransparentLabelBackground = 0;
        Tool.AddMethod                  = UTAM_ADD_OR_ADJUST;

        if (LabelLineNumber != 0)
            Tool.LineNumber = LabelLineNumber;

        sc.UseTool(Tool);
        LabelLineNumber = Tool.LineNumber;
    }
    else
    {
        ClearLabel();
    }

    // --- ENTRY / STOP / TP LINES ------------------------------------------
    if (showLines)
    {
        const SubgraphLineStyles lineStyleEnum =
            GetLineStyle(In_LineStyle.GetIndex());

        const int lastIndex = sc.ArraySize - 1;
        SCDateTime lastDT   = sc.BaseDateTimeIn[lastIndex];

        double secondsPerBar = sc.SecondsPerBar;
        if (secondsPerBar <= 0.0)
            secondsPerBar = 60.0;

        double leftDays  = (secondsPerBar * 60.0) / 86400.0;
        double rightDays = (secondsPerBar * 20.0) / 86400.0;

        SCDateTime beginDT = lastDT - leftDays;
        SCDateTime endDT   = lastDT + rightDays;

        auto BuildLineLabel = [&](const SCString& base, float price, int contracts) -> SCString
        {
            if (!showLineValues)
                return base;

            SCString out = base;
            out += FormatPrice2(price);

            if (contracts > 0)
            {
                out += " (";
                SCString tmp;
                tmp.Format("%d", contracts);
                out += tmp;
                out += ")";
            }
            return out;
        };

        // ENTRY line
        if (State >= STATE_SHOW_ENTRY)
        {
            s_UseTool line;
            line.Clear();
            line.ChartNumber   = sc.ChartNumber;
            line.DrawingType   = DRAWING_LINE;
            line.Region        = sc.GraphRegion;
            line.BeginDateTime = beginDT;
            line.EndDateTime   = endDT;
            line.BeginValue    = EntryPrice;
            line.EndValue      = EntryPrice;
            line.Color         = RGB(200, 200, 0);
            line.LineWidth     = 2;
            line.LineStyle     = lineStyleEnum;

            SCString base = In_LabelEntry.GetString();
            line.Text          = BuildLineLabel(base, EntryPrice, -1);
            line.DrawTextAtEnd = drawTextAtEnd ? 1 : 0;
            line.AddMethod     = UTAM_ADD_OR_ADJUST;

            if (EntryLineNumber != 0)
                line.LineNumber = EntryLineNumber;

            sc.UseTool(line);
            EntryLineNumber = line.LineNumber;
        }
        else
        {
            if (EntryLineNumber != 0)
            {
                sc.DeleteACSChartDrawing(sc.ChartNumber, 0, EntryLineNumber);
                EntryLineNumber = 0;
            }
        }

        // STOP line
        if (State >= STATE_SHOW_STOP)
        {
            s_UseTool line;
            line.Clear();
            line.ChartNumber   = sc.ChartNumber;
            line.DrawingType   = DRAWING_LINE;
            line.Region        = sc.GraphRegion;
            line.BeginDateTime = beginDT;
            line.EndDateTime   = endDT;
            line.BeginValue    = StopPrice;
            line.EndValue      = StopPrice;
            line.Color         = RGB(255, 0, 0);
            line.LineWidth     = 2;
            line.LineStyle     = lineStyleEnum;

            SCString base = In_LabelStop.GetString();
            line.Text          = BuildLineLabel(base, StopPrice, -1);
            line.DrawTextAtEnd = drawTextAtEnd ? 1 : 0;
            line.AddMethod     = UTAM_ADD_OR_ADJUST;

            if (StopLineNumber != 0)
                line.LineNumber = StopLineNumber;

            sc.UseTool(line);
            StopLineNumber = line.LineNumber;
        }
        else
        {
            if (StopLineNumber != 0)
            {
                sc.DeleteACSChartDrawing(sc.ChartNumber, 0, StopLineNumber);
                StopLineNumber = 0;
            }
        }

        // TP lines
        int   tpContractsPtr[5] = { TPContracts1, TPContracts2, TPContracts3, TPContracts4, TPContracts5 };
        float tpPricesPtr[5]    = { TPPrice1, TPPrice2, TPPrice3, TPPrice4, TPPrice5 };
        int*  tpLineNums[5]     = { &TPLineNumber1, &TPLineNumber2, &TPLineNumber3, &TPLineNumber4, &TPLineNumber5 };

        int visibleUpTo = -1;
        if (State == STATE_SHOW_TP)
            visibleUpTo = CurrentTPIndex;
        else if (State >= STATE_SHOW_TARGETS)
            visibleUpTo = TPCount - 1;

        for (int i = 0; i < 5; ++i)
        {
            int&  lineNum   = *tpLineNums[i];
            int   contracts = tpContractsPtr[i];
            float price     = tpPricesPtr[i];

            if (i > visibleUpTo || i >= TPCount || contracts <= 0 || price == 0.0f)
            {
                if (lineNum != 0)
                {
                    sc.DeleteACSChartDrawing(sc.ChartNumber, 0, lineNum);
                    lineNum = 0;
                }
                continue;
            }

            s_UseTool line;
            line.Clear();
            line.ChartNumber   = sc.ChartNumber;
            line.DrawingType   = DRAWING_LINE;
            line.Region        = sc.GraphRegion;
            line.BeginDateTime = beginDT;
            line.EndDateTime   = endDT;
            line.BeginValue    = price;
            line.EndValue      = price;
            line.Color         = RGB(0, 200, 0);
            line.LineWidth     = 2;
            line.LineStyle     = lineStyleEnum;

            SCString base;
            base.Format("TP%d : ", i + 1);
            line.Text          = BuildLineLabel(base, price, contracts);
            line.DrawTextAtEnd = drawTextAtEnd ? 1 : 0;
            line.AddMethod     = UTAM_ADD_OR_ADJUST;

            if (lineNum != 0)
                line.LineNumber = lineNum;

            sc.UseTool(line);
            lineNum = line.LineNumber;
        }
    }
    else
    {
        ClearAllLines();
    }
}