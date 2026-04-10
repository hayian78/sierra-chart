#include "sierrachart.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

SCDLLName("Balance Zone Manager")

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cmath>

SCSFExport scsf_BalanceZoneManager(SCStudyInterfaceRef sc);

// -----------------------------
// Helper Structures & Logic (Mirrored from BZE for compatibility)
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

static inline bool IsDigits(const std::string& s)
{
    if (s.empty()) return false;
    for (char c : s) if (!std::isdigit((unsigned char)c)) return false;
    return true;
}

enum class ZoneTier { Focus, Context, Archive };

struct AnchorTextOverrides
{
    std::string BaseLabel;
    bool HasUp = false;
    bool HasDown = false;
    bool UpIsX = false;
    bool DownIsX = false;
    int  Up = 0;
    int  Down = 0;

    bool HasTierOverride = false;
    ZoneTier ExplicitTier = ZoneTier::Focus;
    std::string OriginalTierKeyword; // Preserve "LOCK", "FIXED", etc.
    bool HasNiceStar = false;
    std::string Description; // Preserve text after '|'
};

static inline AnchorTextOverrides ParseAnchorTextOverrides(const SCString& toolText)
{
    AnchorTextOverrides out;
    const char* raw = toolText.GetChars();
    if (raw == nullptr) return out;

    std::string full(raw);
    
    // 1. Separate Description
    size_t pipePos = full.find('|');
    std::string labelPart = full;
    if (pipePos != std::string::npos)
    {
        labelPart = full.substr(0, pipePos);
        out.Description = TrimCopy(full.substr(pipePos + 1));
    }

    std::string s = TrimCopy(labelPart);
    if (s.empty()) return out;

    size_t i = 0;
    while (i < s.size() && !std::isspace((unsigned char)s[i])) ++i;

    out.BaseLabel = s.substr(0, i);

    if (!out.BaseLabel.empty() && out.BaseLabel.back() == '*')
    {
        out.HasNiceStar = true;
        out.HasTierOverride = true;
        out.ExplicitTier = ZoneTier::Focus;
        out.BaseLabel.pop_back();
        out.BaseLabel = TrimCopy(out.BaseLabel);
    }

    std::string rem = (i < s.size()) ? TrimCopy(s.substr(i)) : std::string();
    if (rem.empty()) return out;

    std::vector<std::string> parts;
    {
        std::string cur;
        for (char ch : rem)
        {
            if (ch == ',')
            {
                std::string t = TrimCopy(cur);
                if (!t.empty()) parts.push_back(t);
                cur.clear();
            }
            else { cur.push_back(ch); }
        }
        std::string t = TrimCopy(cur);
        if (!t.empty()) parts.push_back(t);
    }

    auto ParseSignedTerm = [&](const std::string& term)
    {
        std::string t = ToUpperCopy(TrimCopy(term));
        if (t.empty()) return;

        // Tier Keywords
        if (t == "T1" || t == "FOCUS") 
        { 
            out.HasTierOverride = true; out.ExplicitTier = ZoneTier::Focus; 
            out.OriginalTierKeyword = term; return; 
        }
        if (t == "T2" || t == "CONTEXT") 
        { 
            out.HasTierOverride = true; out.ExplicitTier = ZoneTier::Context; 
            out.OriginalTierKeyword = term; return; 
        }
        if (t == "T3" || t == "ARCHIVE" || t == "LOCK" || t == "FIXED") 
        { 
            out.HasTierOverride = true; out.ExplicitTier = ZoneTier::Archive; 
            out.OriginalTierKeyword = term; return; 
        }

        bool isX = false;
        if (!t.empty() && t.back() == 'X') { isX = true; t.pop_back(); t = TrimCopy(t); }

        char sign = 0;
        std::string n;
        if (std::isdigit((unsigned char)t[0])) { sign = '+'; n = t; }
        else if (t[0] == '+' || t[0] == '-') { sign = t[0]; n = TrimCopy(t.substr(1)); }
        else return;

        if (!IsDigits(n)) return;
        int v = std::atoi(n.c_str());
        if (v < 1) v = 1; if (v > 30) v = 30;

        if (sign == '+') { out.HasUp = true; out.Up = v; out.UpIsX = isX; }
        else { out.HasDown = true; out.Down = v; out.DownIsX = isX; }
    };

    for (const auto& p : parts) ParseSignedTerm(p);
    return out;
}

// -----------------------------
// Study Implementation
// -----------------------------

SCSFExport scsf_BalanceZoneManager(SCStudyInterfaceRef sc)
{
    if (sc.SetDefaults)
    {
        sc.GraphName = "Tool: Balance Zone Manager";
        sc.StudyDescription = "Maintenance tool for Balance Zone anchors. Includes 'Shrink-Wrap' logic to optimize extensions based on historical price action.";
        sc.AutoLoop = 0;
        sc.GraphRegion = 0;

        sc.Input[0].Name = "Run Shrink-Wrap Now";
        sc.Input[0].SetYesNo(0);

        sc.Input[1].Name = "Base Labels (comma-separated)";
        sc.Input[1].SetString("BZ");

        sc.Input[2].Name = "Minimum Multiplier (to keep)";
        sc.Input[2].SetInt(0);
        sc.Input[2].SetIntLimits(0, 30);

        sc.Input[3].Name = "Force Update (Bypass explicit check)";
        sc.Input[3].SetYesNo(0);

        sc.Input[4].Name = "Enable Debug Logging";
        sc.Input[4].SetYesNo(0);

        sc.UpdateAlways = 0;
        sc.CalculationPrecedence = LOW_PREC_LEVEL;

        return;
    }

    // Zero-Lag Optimization: Only run on full recalculation (triggered by input change or manual reset)
    if (!sc.IsFullRecalculation)
        return;

    // Early exit if trigger is not set
    if (!sc.Input[0].GetYesNo())
        return;

    // Reset the input immediately to act as a one-time trigger
    sc.Input[0].SetYesNo(0);
        
    bool debugLog = sc.Input[4].GetYesNo();
    std::vector<std::string> baseLabels;
    {
        SCString raw = sc.Input[1].GetString();
        std::string s = raw.GetChars();
        std::string cur;
        for (char ch : s)
        {
            if (ch == ',') { if (!cur.empty()) baseLabels.push_back(ToUpperCopy(TrimCopy(cur))); cur.clear(); }
            else cur.push_back(ch);
        }
        if (!cur.empty()) baseLabels.push_back(ToUpperCopy(TrimCopy(cur)));
    }

    s_UseTool Tool;
    int drawIndex = 0;
    int updatedCount = 0;

    while (sc.GetUserDrawnChartDrawing(sc.ChartNumber, DRAWING_RECTANGLEHIGHLIGHT, Tool, drawIndex) != 0)
    {
        drawIndex++;
        AnchorTextOverrides ov = ParseAnchorTextOverrides(Tool.Text);
        
        // Check if base label matches
        bool labelMatch = false;
        std::string upperBase = ToUpperCopy(ov.BaseLabel);
        for (const auto& bl : baseLabels) { if (upperBase == bl) { labelMatch = true; break; } }
        if (!labelMatch) continue;

        // EXCLUSION MANDATE: Skip if multipliers are explicitly specified (unless forced)
        if (!sc.Input[3].GetYesNo() && (ov.HasUp || ov.HasDown)) continue;

        // Calculation Range: Only within the anchor's horizontal bounds
        int startIdx = std::min(Tool.BeginIndex, Tool.EndIndex);
        int endIdx = std::max(Tool.BeginIndex, Tool.EndIndex);
        
        if (startIdx < 0) startIdx = 0;
        if (endIdx >= sc.ArraySize) endIdx = sc.ArraySize - 1;
        if (startIdx > endIdx) continue;

        double maxPrice = -1.0e30;
        double minPrice = 1.0e30;

        for (int i = startIdx; i <= endIdx; ++i)
        {
            if (sc.High[i] > maxPrice) maxPrice = sc.High[i];
            if (sc.Low[i] < minPrice) minPrice = sc.Low[i];
        }

        double top = std::max(Tool.BeginValue, Tool.EndValue);
        double bot = std::min(Tool.BeginValue, Tool.EndValue);
        double height = top - bot;

        // Diagnostic Logging
        if (debugLog)
        {
            SCString diag;
            diag.Format("BZ Debug: Label=%s, Top=%.2f, Bot=%.2f, H=%.2f, MaxP=%.2f, MinP=%.2f", 
                        Tool.Text.GetChars(), top, bot, height, maxPrice, minPrice);
            sc.AddMessageToLog(diag, 0);
        }

        if (height <= 0) continue;

        int reqUp = 0;
        if (maxPrice > top) reqUp = (int)std::ceil((maxPrice - top) / height);
        
        int reqDn = 0;
        if (minPrice < bot) reqDn = (int)std::ceil((bot - minPrice) / height);

        int minKeep = sc.Input[2].GetInt();
        if (reqUp < minKeep) reqUp = minKeep;
        if (reqDn < minKeep) reqDn = minKeep;

        // Cap at 30 (BZE limit)
        if (reqUp > 30) reqUp = 30;
        if (reqDn > 30) reqDn = 30;

        // Construct new label
        SCString newLabel;
        newLabel.Format("%s%s +%d,-%d", ov.BaseLabel.c_str(), ov.HasNiceStar ? "*" : "", reqUp, reqDn);

        // Re-append Tier overrides if present
        if (ov.HasTierOverride && !ov.HasNiceStar) // NiceStar already implied T1
        {
            if (!ov.OriginalTierKeyword.empty())
            {
                newLabel += " ";
                newLabel += ov.OriginalTierKeyword.c_str();
            }
            else
            {
                if (ov.ExplicitTier == ZoneTier::Focus) newLabel += " T1";
                else if (ov.ExplicitTier == ZoneTier::Context) newLabel += " T2";
                else if (ov.ExplicitTier == ZoneTier::Archive) newLabel += " T3";
            }
        }

        // Re-append Description if present
        if (!ov.Description.empty())
        {
            newLabel += " | ";
            newLabel += ov.Description.c_str();
        }

        // Update the drawing
        Tool.Text = newLabel;
        Tool.AddMethod = UTAM_ADD_OR_ADJUST;
        if (sc.UseTool(Tool)) updatedCount++;
    }

    SCString msg;
    msg.Format("BZ Manager: Shrink-wrapped %d anchors.", updatedCount);
    sc.AddMessageToLog(msg, 0);
}
