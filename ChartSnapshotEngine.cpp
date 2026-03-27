// ChartSnapshotEngine.cpp
// Sierra Chart Custom Study - Headless Chart Snapshot Pipeline
// 
// This study acts as a "Master Controller" that:
// 1. Recreates ghost chart windows every 120 minutes to prevent state-drift
// 2. Takes screenshots of specified charts every 30 seconds
// 3. Exports images to C:\SierraChart\Exports for Python sidecar pickup
//
// Usage:
// - Add this study to ONE chart (the master controller)
// - Input the chart numbers you want to snapshot (e.g., "1,5,10")
// - Run watcher.py to bridge images to Telegram

#include "sierrachart.h"
#include <cstdlib>  // atoi
#include <cstdio>   // FILE, fopen, fputs, fclose

SCDLLName("ChartSnapshotEngine")

// Constants for timing (in seconds)
// Persistent variable indices
const int PV_LAST_SNAPSHOT_TIME = 1;
const int PV_INITIALIZATION_DONE = 2;
const int PV_SNAPSHOT_PENDING = 4;
const int PV_TELEGRAM_ENABLED = 5;  // Toggle state for toolbar button

const int PV_SNAPSHOT_ENABLED = 35;  // Toggle state for snapshot button

// ACS Control Bar Button IDs (now configurable via inputs)
// const int ACSBUTTON_SNAPSHOT_TOGGLE = 1;  -- moved to input
// const int ACSBUTTON_TELEGRAM_TOGGLE = 2;  -- moved to input

// Helper function to write manifest with current status
void WriteManifestStatus(SCStudyInterfaceRef& sc, const SCString& ExportPath, int SnapshotEnabled, int TelegramEnabled, int SnapshotIntervalSec)
{
    SCDateTime CurrentDateTime = sc.CurrentSystemDateTime;
    SCString TimeStr = sc.FormatDateTime(CurrentDateTime);
    
    SCString ManifestJson = "{\n  \"charts\": [],\n";
    
    SCString ManifestFooter;
    ManifestFooter.Format("  \"snapshotIntervalSec\": %d,\n  \"snapshotsEnabled\": %s,\n  \"telegramEnabled\": %s,\n  \"lastUpdated\": \"%s\"\n}",
        SnapshotIntervalSec, 
        SnapshotEnabled ? "true" : "false",
        TelegramEnabled ? "true" : "false",
        TimeStr.GetChars());
    ManifestJson += ManifestFooter;
    
    SCString ManifestPath;
    ManifestPath.Format("%s\\manifest.json", ExportPath.GetChars());
    
    FILE* ManifestFile = fopen(ManifestPath.GetChars(), "w");
    if (ManifestFile)
    {
        fputs(ManifestJson.GetChars(), ManifestFile);
        fclose(ManifestFile);
    }
}


SCSFExport scsf_ChartSnapshotEngine(SCStudyInterfaceRef sc)
{
    // Input definitions
    SCInputRef Input_SourceChartNumbers = sc.Input[0];
    SCInputRef Input_ExportPath = sc.Input[1];
    SCInputRef Input_SnapshotInterval = sc.Input[2];

    SCInputRef Input_EnableLogging = sc.Input[4];
    SCInputRef Input_ImageWidth = sc.Input[5];
    SCInputRef Input_ImageHeight = sc.Input[6];
    SCInputRef Input_SnapshotButtonID = sc.Input[7];
    SCInputRef Input_TelegramButtonID = sc.Input[8];

    if (sc.SetDefaults)
    {
        sc.GraphName = "Chart Snapshot Engine";
        sc.GraphRegion = 0;
        sc.AutoLoop = 0;  // Manual looping for timer-based operation
        
        sc.UpdateAlways = 1;  // Required for periodic updates
        
        Input_SourceChartNumbers.Name = "Source Chart Numbers (comma-separated)";
        Input_SourceChartNumbers.SetString("1,5,10");
        
        Input_ExportPath.Name = "Export Path";
        Input_ExportPath.SetString("C:\\SierraChart\\Exports");
        
        Input_SnapshotInterval.Name = "Snapshot Interval (seconds)";
        Input_SnapshotInterval.SetInt(30);
        Input_SnapshotInterval.SetIntLimits(5, 3600);
        
        Input_SnapshotInterval.SetIntLimits(5, 3600);

        
        Input_EnableLogging.Name = "Enable Debug Logging";
        Input_EnableLogging.SetYesNo(0);
        
        Input_ImageWidth.Name = "Image Width (pixels)";
        Input_ImageWidth.SetInt(1920);
        Input_ImageWidth.SetIntLimits(640, 3840);
        
        Input_ImageHeight.Name = "Image Height (pixels)";
        Input_ImageHeight.SetInt(1080);
        Input_ImageHeight.SetIntLimits(480, 2160);
        
        Input_SnapshotButtonID.Name = "Snapshot Button ID (ACS Control Bar)";
        Input_SnapshotButtonID.SetInt(1);
        Input_SnapshotButtonID.SetIntLimits(1, 150);
        
        Input_TelegramButtonID.Name = "Telegram Button ID (ACS Control Bar)";
        Input_TelegramButtonID.SetInt(2);
        Input_TelegramButtonID.SetIntLimits(1, 150);
        
        // Setup ACS Control Bar buttons
        sc.AllowMultipleEntriesInSameDirection = true;
        
        return;
    }
    
    // Get button IDs from inputs
    int SnapshotButtonID = Input_SnapshotButtonID.GetInt();
    int TelegramButtonID = Input_TelegramButtonID.GetInt();
    
    // Handle ACS Control Bar buttons - get persistent states
    int& SnapshotEnabled = sc.GetPersistentInt(PV_SNAPSHOT_ENABLED);
    int& TelegramEnabled = sc.GetPersistentInt(PV_TELEGRAM_ENABLED);
    
    // Initialize buttons on first run (default: snapshots ON, telegram OFF)
    if (sc.UpdateStartIndex == 0)
    {
        // Default snapshot to enabled if never set
        if (SnapshotEnabled == 0 && sc.GetPersistentInt(PV_INITIALIZATION_DONE) == 0)
        {
            SnapshotEnabled = 1;  // Start with snapshots enabled
        }
        
        // Set up buttons with proper labels
        sc.SetCustomStudyControlBarButtonText(SnapshotButtonID, 
            SnapshotEnabled ? "SNAP: ON" : "SNAP: OFF");
        sc.SetCustomStudyControlBarButtonEnable(SnapshotButtonID, true);
        
        sc.SetCustomStudyControlBarButtonText(TelegramButtonID, 
            TelegramEnabled ? "TG: ON" : "TG: OFF");
        sc.SetCustomStudyControlBarButtonEnable(TelegramButtonID, true);
    }
    
    // Check if Snapshot button was clicked
    if (sc.MenuEventID == SnapshotButtonID)
    {
        SnapshotEnabled = SnapshotEnabled ? 0 : 1;  // Toggle
        
        sc.SetCustomStudyControlBarButtonText(SnapshotButtonID, 
            SnapshotEnabled ? "SNAP: ON" : "SNAP: OFF");
        
        SCString LogMsg;
        LogMsg.Format("ChartSnapshotEngine: Snapshots %s", SnapshotEnabled ? "ENABLED" : "DISABLED");
        sc.AddMessageToLog(LogMsg, 0);
        
        // Update manifest immediately with new state
        SCString ExportPath = Input_ExportPath.GetString();
        int SnapshotIntervalSec = Input_SnapshotInterval.GetInt();
        WriteManifestStatus(sc, ExportPath, SnapshotEnabled, TelegramEnabled, SnapshotIntervalSec);
        
        return;  // Don't process rest of study on button click
    }
    
    // Check if Telegram button was clicked
    if (sc.MenuEventID == TelegramButtonID)
    {
        TelegramEnabled = TelegramEnabled ? 0 : 1;  // Toggle
        
        sc.SetCustomStudyControlBarButtonText(TelegramButtonID, 
            TelegramEnabled ? "TG: ON" : "TG: OFF");
        
        SCString LogMsg;
        LogMsg.Format("ChartSnapshotEngine: Telegram export %s", TelegramEnabled ? "ENABLED" : "DISABLED");
        sc.AddMessageToLog(LogMsg, 0);
        
        // Update manifest immediately with new state
        SCString ExportPath = Input_ExportPath.GetString();
        int SnapshotIntervalSec = Input_SnapshotInterval.GetInt();
        WriteManifestStatus(sc, ExportPath, SnapshotEnabled, TelegramEnabled, SnapshotIntervalSec);
        
        return;  // Don't process rest of study on button click
    }
    
    // Use SCDateTime for persistent time tracking (not int - it overflows!)
    SCDateTime& LastSnapshotDateTime = sc.GetPersistentSCDateTime(PV_LAST_SNAPSHOT_TIME);
    int& InitializationDone = sc.GetPersistentInt(PV_INITIALIZATION_DONE);
    int& SnapshotPending = sc.GetPersistentInt(PV_SNAPSHOT_PENDING);
    
    // Get intervals from inputs FIRST for early exit check
    int SnapshotIntervalSec = Input_SnapshotInterval.GetInt();
    
    // Get current time
    SCDateTime CurrentDateTime = sc.CurrentSystemDateTime;
    
    // OPTIMIZATION: Early exit if not enough time has passed and no pending work
    // This avoids all parsing/allocation overhead on most calls
    if (InitializationDone != 0 && SnapshotPending == 0)
    {
        // Quick time check using days (avoid expensive GetAsDouble on most calls)
        double DaysSinceSnapshot = (CurrentDateTime - LastSnapshotDateTime).GetAsDouble();
        double MinDaysNeeded = SnapshotIntervalSec / 86400.0;
        
        if (DaysSinceSnapshot < MinDaysNeeded * 0.9)  // Not even close to snapshot time
        {
            return;  // EARLY EXIT - nothing to do
        }
    }
    
    bool EnableLogging = Input_EnableLogging.GetYesNo() != 0;
    
    // OPTIMIZATION: Cache parsed chart numbers in static storage
    // Only re-parse when input string changes
    static SCString CachedChartNumbersStr;
    static int CachedChartNumbers[20];
    static int CachedChartCount = 0;
    
    SCString ChartNumbersStr = Input_SourceChartNumbers.GetString();
    
    // Check if we need to re-parse (input changed or first time)
    if (ChartNumbersStr != CachedChartNumbersStr || CachedChartCount == 0)
    {
        CachedChartNumbersStr = ChartNumbersStr;
        CachedChartCount = 0;
        
        // Parse comma-separated chart numbers
        int StartPos = 0;
        int StrLen = ChartNumbersStr.GetLength();
        for (int i = 0; i <= StrLen && CachedChartCount < 20; i++)
        {
            if (i == StrLen || ChartNumbersStr[i] == ',')
            {
                if (i > StartPos)
                {
                    SCString NumStr = ChartNumbersStr.GetSubString(i - StartPos, StartPos);
                    int ChartNum = atoi(NumStr.GetChars());
                    if (ChartNum > 0)
                    {
                        CachedChartNumbers[CachedChartCount++] = ChartNum;
                    }
                }
                StartPos = i + 1;
            }
        }
        
        if (EnableLogging && CachedChartCount > 0)
        {
            SCString LogMsg;
            LogMsg.Format("ChartSnapshotEngine: Parsed %d chart number(s)", CachedChartCount);
            sc.AddMessageToLog(LogMsg, 0);
        }
    }
    
    if (CachedChartCount == 0)
    {
        sc.AddMessageToLog("ChartSnapshotEngine: No valid chart numbers specified", 1);
        return;
    }
    
    // Initialize ONLY on first run (not on every full recalculation)
    if (InitializationDone == 0)
    {
        SCString LogMsg;
        LogMsg.Format("ChartSnapshotEngine: *** FIRST INITIALIZATION *** with %d chart(s)", CachedChartCount);
        sc.AddMessageToLog(LogMsg, 0);
        

        LastSnapshotDateTime = CurrentDateTime - SCDateTime::SECONDS(SnapshotIntervalSec);  // Force immediate snapshot
        InitializationDone = 1;
        SnapshotPending = 1;  // Trigger immediate first snapshot
        
        // Don't return - continue to snapshot logic below
    }
    
    // Handle cleanup on removal
    if (sc.LastCallToFunction)
    {
        InitializationDone = 0;
        return;
    }
    

    
    // Check for snapshot timer (every 30 seconds by default)
    double DaysSinceSnapshot = (CurrentDateTime - LastSnapshotDateTime).GetAsDouble();
    int SecondsSinceSnapshot = static_cast<int>(DaysSinceSnapshot * 86400.0);
    bool TimeToSnapshot = SecondsSinceSnapshot >= SnapshotIntervalSec;
    
    // Debug snapshot trigger condition
    if (EnableLogging && TimeToSnapshot)
    {
        SCString DebugMsg;
        DebugMsg.Format("ChartSnapshotEngine: TimeToSnapshot=TRUE! (elapsed=%d, needed=%d)", 
            SecondsSinceSnapshot, SnapshotIntervalSec);
        sc.AddMessageToLog(DebugMsg, 0);
    }
    
    // Handle snapshot timer (only if snapshots are enabled via toolbar button)
    if (SnapshotEnabled && (TimeToSnapshot || SnapshotPending))
    {
        SCString ExportPath = Input_ExportPath.GetString();
        int ImageWidth = Input_ImageWidth.GetInt();
        int ImageHeight = Input_ImageHeight.GetInt();
        
        // Always log snapshot activity so user knows it's working
        SCString TimeStr = sc.FormatDateTime(CurrentDateTime);
        SCString StatusMsg;
        StatusMsg.Format("ChartSnapshotEngine: Taking %d snapshot(s) at %s", 
            CachedChartCount, TimeStr.GetChars());
        sc.AddMessageToLog(StatusMsg, 0);
        
        // Build manifest JSON for PowerShell watcher
        SCString ManifestJson = "{\n  \"charts\": [\n";
        
        for (int i = 0; i < CachedChartCount; i++)
        {
            int ChartNum = CachedChartNumbers[i];
            
            // Build the export filename
            SCString FileName;
            FileName.Format("%s\\Chart_%d.png", ExportPath.GetChars(), ChartNum);
            
            // Delete old file to ensure fresh overwrite (Windows can be finicky)
            DeleteFileA(FileName.GetChars());
            
            // Enforce "Live" positioning on the source chart
            // Note: We rely on the user ensuring "Scroll To End" is enabled 
            // when they toggle the "SNAP: ON" button.
            
            // Explicitly MAXIMIZE the window as requested
            // This ensures consistent state and overrides any "restore" behavior
            sc.SetChartWindowState(ChartNum, CWS_MAXIMIZE);
            
            // Small delay to allow chart to update its position/state
            Sleep(50);
            
            // Take the screenshot directly from the source chart
            sc.SaveChartImageToFileExtended(ChartNum, FileName, ImageWidth, ImageHeight, 0);
            
            // Get chart name for manifest
            SCString ChartName = sc.GetChartName(ChartNum);
            
            // Escape backslashes for JSON
            SCString EscapedPath;
            for (int j = 0; j < FileName.GetLength(); j++)
            {
                if (FileName[j] == '\\')
                    EscapedPath += "\\\\";
                else
                    EscapedPath += FileName[j];
            }
            
            // Add to manifest JSON
            SCString ChartEntry;
            ChartEntry.Format("    {\"chartNumber\": %d, \"name\": \"%s\", \"imagePath\": \"%s\"}",
                ChartNum, ChartName.GetChars(), EscapedPath.GetChars());
            ManifestJson += ChartEntry;
            if (i < CachedChartCount - 1)
                ManifestJson += ",";
            ManifestJson += "\n";
            
            if (EnableLogging)
            {
                SCString LogMsg;
                LogMsg.Format("ChartSnapshotEngine: Snapshotted Chart %d -> %s", ChartNum, FileName.GetChars());
                sc.AddMessageToLog(LogMsg, 0);
            }
        }
        
        // Close manifest JSON
        SCString ManifestFooter;
        ManifestFooter.Format("  ],\n  \"snapshotIntervalSec\": %d,\n  \"snapshotsEnabled\": %s,\n  \"telegramEnabled\": %s,\n  \"lastUpdated\": \"%s\"\n}",
            SnapshotIntervalSec, 
            SnapshotEnabled ? "true" : "false",
            TelegramEnabled ? "true" : "false",
            TimeStr.GetChars());
        ManifestJson += ManifestFooter;
        
        // Always write manifest file so PowerShell can see the current state
        SCString ManifestPath;
        ManifestPath.Format("%s\\manifest.json", ExportPath.GetChars());
        
        FILE* ManifestFile = fopen(ManifestPath.GetChars(), "w");
        if (ManifestFile)
        {
            fputs(ManifestJson.GetChars(), ManifestFile);
            fclose(ManifestFile);
        }
        
        LastSnapshotDateTime = CurrentDateTime;
        SnapshotPending = 0;
    }
}