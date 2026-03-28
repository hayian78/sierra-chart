#include "sierrachart.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

SCDLLName("Drawing Cleaner")

// -----------------------------
// Small helpers for string manipulation (standard std::string based)
// -----------------------------
static inline std::string TrimCopy(const std::string& s)
{
    size_t start = 0;
    while (start < s.size() && std::isspace((unsigned char)s[start])) ++start;

    size_t end = s.size();
    while (end > start && std::isspace((unsigned char)s[end - 1])) --end;

    return s.substr(start, end - start);
}

static inline std::string ToUpperCopy(std::string s)
{
    for (char& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}

SCSFExport scsf_DrawingCleaner(SCStudyInterfaceRef sc)
{
    if (sc.SetDefaults)
    {
        sc.GraphName = "Tool: Drawing Cleaner";
        sc.StudyDescription = "Adds a customizable button to the control bar to delete specific types of drawings from the chart.";
        sc.AutoLoop = 0; // Manual looping
        sc.GraphRegion = 0;
        sc.UpdateAlways = 0; // Performance: Only update on data or menu events

        sc.Input[0].Name = "Control Bar Button # (1-150)";
        sc.Input[0].SetInt(1);
        sc.Input[0].SetIntLimits(1, 150);

        sc.Input[1].Name = "Delete Horizontal Lines (Infinite)";
        sc.Input[1].SetYesNo(0);

        sc.Input[2].Name = "Delete Horizontal Rays";
        sc.Input[2].SetYesNo(0);

        sc.Input[3].Name = "Delete Vertical Lines";
        sc.Input[3].SetYesNo(0);

        sc.Input[4].Name = "Delete Trendlines (Line)";
        sc.Input[4].SetYesNo(0);

        sc.Input[5].Name = "Delete Rays";
        sc.Input[5].SetYesNo(0);

        sc.Input[6].Name = "Delete Extended Lines";
        sc.Input[6].SetYesNo(0);

        sc.Input[7].Name = "Delete Parallel Lines";
        sc.Input[7].SetYesNo(0);

        sc.Input[8].Name = "Delete Parallel Rays";
        sc.Input[8].SetYesNo(0);

        sc.Input[9].Name = "Delete Horizontal Line Non-Extended (Segment)";
        sc.Input[9].SetYesNo(0);

        sc.Input[10].Name = "Button Text";
        sc.Input[10].SetString("Clean Drawings");

        sc.Input[11].Name = "Exclusion Text (comma-separated)";
        sc.Input[11].SetString("");

        return;
    }

    int ButtonID = sc.Input[0].GetInt();

    // PERFORMANCE OPTIMIZATION: 
    // Early exit if this is not a button click and we are not on the very first bar of a load/recalculation.
    // This ensures no loops or checks run during normal chart ticking.
    if (sc.UpdateStartIndex > 0 && sc.MenuEventID != ButtonID)
        return;

    // Initialize button text/state only when needed
    if (sc.UpdateStartIndex == 0)
    {
        sc.SetCustomStudyControlBarButtonText(ButtonID, sc.Input[10].GetString());
        sc.SetCustomStudyControlBarButtonEnable(ButtonID, 1);
    }

    // MAIN LOGIC: Only executes on button click
    if (sc.MenuEventID == ButtonID)
    {
        // Parse exclusions into uppercase std::string vector
        std::vector<std::string> exclusions;
        const char* exclusionRaw = sc.Input[11].GetString();
        if (exclusionRaw && exclusionRaw[0] != '\0')
        {
            std::string s(exclusionRaw);
            std::string cur;
            for (char ch : s)
            {
                if (ch == ',')
                {
                    std::string t = ToUpperCopy(TrimCopy(cur));
                    if (!t.empty()) exclusions.push_back(t);
                    cur.clear();
                }
                else
                {
                    cur.push_back(ch);
                }
            }
            std::string t = ToUpperCopy(TrimCopy(cur));
            if (!t.empty()) exclusions.push_back(t);
        }

        std::vector<int> lineNumbersToDelete;
        s_UseTool Drawing;
        int drawingIndex = 0;

        // Pass 1: Scan all drawings on the chart
        while (sc.GetUserDrawnChartDrawing(sc.ChartNumber, 0, Drawing, drawingIndex) != 0)
        {
            bool shouldDelete = false;
            switch (Drawing.DrawingType)
            {
                case DRAWING_HORIZONTALLINE:
                    if (sc.Input[1].GetYesNo()) shouldDelete = true;
                    break;
                case DRAWING_HORIZONTAL_RAY:
                    if (sc.Input[2].GetYesNo()) shouldDelete = true;
                    break;
                case DRAWING_HORIZONTAL_LINE_NON_EXTENDED:
                    if (sc.Input[9].GetYesNo()) shouldDelete = true;
                    break;
                case DRAWING_VERTICALLINE:
                    if (sc.Input[3].GetYesNo()) shouldDelete = true;
                    break;
                case DRAWING_LINE: // Trendline
                    // Check if it's specifically a horizontal segment (non-extended)
                    if (sc.Input[9].GetYesNo() && (Drawing.BeginValue == Drawing.EndValue))
                    {
                        shouldDelete = true;
                    }
                    // Otherwise check if all trendlines should be deleted
                    else if (sc.Input[4].GetYesNo())
                    {
                        shouldDelete = true;
                    }
                    break;
                case DRAWING_RAY:
                    if (sc.Input[5].GetYesNo()) shouldDelete = true;
                    break;
                case DRAWING_EXTENDED_LINE:
                    if (sc.Input[6].GetYesNo()) shouldDelete = true;
                    break;
                case DRAWING_PARALLEL_LINES:
                    if (sc.Input[7].GetYesNo()) shouldDelete = true;
                    break;
                case DRAWING_PARALLEL_RAYS:
                    if (sc.Input[8].GetYesNo()) shouldDelete = true;
                    break;
            }

            if (shouldDelete)
            {
                // Check exclusions (case-insensitive)
                if (!exclusions.empty())
                {
                    const char* dText = Drawing.Text.GetChars();
                    std::string drawTxtUpper = ToUpperCopy(dText ? dText : "");
                    
                    for (const auto& ex : exclusions)
                    {
                        if (drawTxtUpper.find(ex) != std::string::npos)
                        {
                            shouldDelete = false;
                            break;
                        }
                    }
                }
            }

            if (shouldDelete)
            {
                lineNumbersToDelete.push_back(Drawing.LineNumber);
            }
            drawingIndex++;
        }

        // Pass 2: Safety deletion of collected line numbers
        for (size_t i = 0; i < lineNumbersToDelete.size(); ++i)
        {
            sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, lineNumbersToDelete[i]);
        }
    }
}
