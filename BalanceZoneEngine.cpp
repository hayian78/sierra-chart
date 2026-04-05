// ============================================================================
// Balance Zone Engine (Sierra Chart ACSIL)
// Version: 1.0.3
// Build:   2026-02-09
// Author:  Ian @hayian0078
// Docs:    https://tinyurl.com/BalanceZoneEngine
//
// Summary:
//  - Projects balance zones above/below user anchor rectangles.
//  - Supports per-anchor overrides via rectangle text:
//
//      BZ
//      BZ +2,-4
//      BZ +6x
//      BZ  +6x,-2x
//
//    Where:
//      - "+N" / "-N"    => set max multipliers directly (1..30)
//      - "+Nx" / "-Nx"  => "group-based" max using zone groups:
//            Nx <= 2  -> 2
//            Nx <= 4  -> 6
//            Nx <= 8  -> 14
//            Nx >  8  -> 30
//      - "*" suffix     => force detailed ("nice") rendering (borders, midlines, labels)
//
//    Examples:
//      BZ 2,4     => Up 2 multipliers, Down 4 multipliers (direct counts)
//      BZ 2x,4x   => Up group(2x)=2, Down group(4x)=6   (group-based)
//      BZ +6x     => Up group(6x)=14, Down uses study setting
//      BZ*        => Force detailed rendering for this anchor
//      BZ* +4x    => Force detailed + Up group(4x)=6
//
//  - Performance controls: process visible anchors only, nice-vs-basic rendering.
//
// Compatibility:
//  - Sierra Chart: v2813+
// ============================================================================

#include "sierrachart.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <math.h>
#include <vector>
#include <algorithm>
#include <string>
#include <cctype>
#include <cstdlib> // atoi
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <cstring> // strncmp

// Stage 1: Smart Cleanup Optimization
static constexpr int  BZE_BUILD = 2026020901; // YYYYMMDDNN

// Uncomment to enable debug logging (has minor perf overhead)
// #define BZE_DEBUG_LOGGING
static constexpr char BZE_BUILD_STR[] = "2026-02-09";


// -----------------------------
// Graph value formatting compatibility
// - Prefer sc.FormatGraphValue() in whatever signature is available
// - Returns empty string if not supported
// -----------------------------
struct BZE_FmtPriority0 {};
struct BZE_FmtPriority1 : BZE_FmtPriority0 {};
struct BZE_FmtPriority2 : BZE_FmtPriority1 {};
struct BZE_FmtPriority3 : BZE_FmtPriority2 {};

// Try: FormatGraphValue(double, SCString&)
template <typename SC>
static auto BZE_FormatGraphValue_Impl(SC& sc, double Value, BZE_FmtPriority3)
    -> decltype(sc.FormatGraphValue(Value, std::declval<SCString&>()), SCString())
{
    SCString Out;
    sc.FormatGraphValue(Value, Out);
    return Out;
}

// Try: FormatGraphValue(SCString&, double)
template <typename SC>
static auto BZE_FormatGraphValue_Impl(SC& sc, double Value, BZE_FmtPriority2)
    -> decltype(sc.FormatGraphValue(std::declval<SCString&>(), Value), SCString())
{
    SCString Out;
    sc.FormatGraphValue(Out, Value);
    return Out;
}

// Try: FormatGraphValue(double) -> SCString
template <typename SC>
static auto BZE_FormatGraphValue_Impl(SC& sc, double Value, BZE_FmtPriority1)
    -> decltype(sc.FormatGraphValue(Value), SCString())
{
    return sc.FormatGraphValue(Value);
}

// Fallback: unsupported
template <typename SC>
static SCString BZE_FormatGraphValue_Impl(SC&, double, BZE_FmtPriority0)
{
    SCString Out;
    return Out;
}

template <typename SC>
static SCString BZE_FormatGraphValueCompat(SC& sc, double Value)
{
    return BZE_FormatGraphValue_Impl(sc, Value, BZE_FmtPriority3{});
}
static constexpr char BZE_VERSION[] = "1.0.3";

SCDLLName("Balance Zone Engine")

// -----------------------------
// Input index map (settings-only styling; subgraphs no longer used for colors)
// -----------------------------
enum InputIx
{
    IN_HELP_URL = 0,

    IN_HDR_CORE,
    IN_ANCHOR_LABELS,
    IN_ONLY_MOST_RECENT,
    IN_SHOW_RECT_PRICES,
    IN_LABEL_DECIMALS,
    IN_LABEL_BAR_OFFSET,
    IN_LABEL_TEXT_COLOR,

    IN_HDR_RANGE,
    IN_ENABLE_UP,
    IN_ENABLE_DOWN,
    IN_MAX_UP_GROUPS,     // 0..4 => 0/2/6/14/30
    IN_MAX_DOWN_GROUPS,   // 0..4 => 0/2/6/14/30

    IN_HDR_FMT,           // "Balance Zone Formatting"
    IN_BASIC_DRAW_BORDER,
    IN_BORDER_COLOR_MODE, // 0=same as fill, 1=custom
    IN_CUSTOM_BORDER_COLOR,

    // Zone-name labels (zone bands; latest anchor only)
    IN_ZONE_NAME_LABEL_MODE,   // 0=Off,1=Nice Only,2=All
    IN_ZONE_LABEL_FONT_SIZE,
    IN_ZONE_LABEL_FONT_COLOR,
    IN_ZONE_LABEL_LEFT_OFFSET, // bars to the right
    IN_COST_BASIS_ZONE_LABEL,
    IN_UP_ZONE_LABEL_TEMPLATE,
    IN_DOWN_ZONE_LABEL_TEMPLATE,

    IN_HDR_UP,
    IN_UP_TRANS_12,
    IN_UP_FILL_12,
    IN_UP_TRANS_36,
    IN_UP_FILL_36,
    IN_UP_TRANS_714,
    IN_UP_FILL_714,
    IN_UP_TRANS_1530,
    IN_UP_FILL_1530,

    IN_HDR_DOWN,
    IN_DN_TRANS_12,
    IN_DN_FILL_12,
    IN_DN_TRANS_36,
    IN_DN_FILL_36,
    IN_DN_TRANS_714,
    IN_DN_FILL_714,
    IN_DN_TRANS_1530,
    IN_DN_FILL_1530,

    IN_HDR_MIDLINE,
    IN_DRAW_MIDLINE,
    IN_MIDLINE_STYLE,      // 0=None,1=Solid,2=Dash,3=Dot
    IN_MIDLINE_COLOR_MODE, // 0=same as fill, 1=custom
    IN_CUSTOM_MIDLINE_COLOR,
    IN_SHOW_MIDLINE_PRICE,
    IN_MIDLINE_WIDTH,

    IN_HDR_PERF,
    IN_AUTO_REDRAW,
    IN_FORCE_REDRAW_NOW,
    IN_CLEAR_OLD_PROJECTIONS,

    IN_HDR_SCOPE,
    IN_PROCESS_VISIBLE_ONLY,
    IN_NICE_ANCHORS_MODE,
    IN_NICE_ANCHORS_COUNT,
    IN_AUTO_EXTEND_MODE,
    IN_AUTO_EXTEND_BUTTON_ID,
    IN_REFRESH_BUTTON_ID,

    IN_HDR_ALERTS,
    IN_ALERT_PRICE_SOURCE, // 0=Last Trade (fallback Close), 1=Bar Close
    IN_ALERT_USE_GAP_DETECTION,
    IN_ALERT_INCLUDE_BAR_HL
};


// -----------------------------
// Small helpers
// -----------------------------
static inline std::string TrimCopy(const std::string& s)
{
    size_t start = 0;
    while (start < s.size() && std::isspace((unsigned char)s[start])) ++start;

    size_t end = s.size();
    while (end > start && std::isspace((unsigned char)s[end - 1])) --end;

    return s.substr(start, end - start);
}

static inline std::vector<std::string> SplitAndTrimCSV(const char* csv)
{
    std::vector<std::string> out;
    if (csv == nullptr) return out;

    std::string s(csv);
    std::string cur;

    for (char ch : s)
    {
        if (ch == ',')
        {
            std::string t = TrimCopy(cur);
            if (!t.empty()) out.push_back(t);
            cur.clear();
        }
        else
        {
            cur.push_back(ch);
        }
    }

    std::string t = TrimCopy(cur);
    if (!t.empty()) out.push_back(t);

    return out;
}

static inline std::string ToUpperCopy(std::string s)
{
    for (char& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}

static inline bool IsDigits(const std::string& s)
{
    if (s.empty()) return false;
    for (char c : s)
        if (!std::isdigit((unsigned char)c))
            return false;
    return true;
}

static inline void HashString64(uint64_t& h, const std::string& s)
{
    for (unsigned char c : s)
    {
        h ^= (uint64_t)c;
        h *= 1099511628211ULL;
    }
}

static inline int MapXToMaxMultiplier(int x)
{
    if (x <= 2) return 2;
    if (x <= 4) return 6;
    if (x <= 8) return 14;
    return 30;
}

static inline int GroupFromMultiplier(int m) // 0..3
{
    if (m <= 2) return 0;
    if (m <= 6) return 1;
    if (m <= 14) return 2;
    return 3;
}

enum class ZoneSide { Up, Down };

// ----------------------------------------------------------------------------
// Anchor text parsing
// ----------------------------------------------------------------------------
struct AnchorTextOverrides
{
    std::string BaseLabel;
    bool HasUp = false;
    bool HasDown = false;
    bool UpIsX = false;
    bool DownIsX = false;
    int  Up = 0;
    int  Down = 0;
    bool ForceDetailed = false;  // If true, anchor always gets "nice" (detailed) rendering
    bool Locked = false;         // Stage 2: LOCK keyword
};

static inline AnchorTextOverrides ParseAnchorTextOverrides(const SCString& toolText)
{
    AnchorTextOverrides out;

    const char* raw = toolText.GetChars();
    if (raw == nullptr) return out;

    std::string s = TrimCopy(std::string(raw));
    if (s.empty()) return out;

    // Base label = first whitespace-delimited token
    size_t i = 0;
    while (i < s.size() && std::isspace((unsigned char)s[i])) ++i;

    size_t start = i;
    while (i < s.size() && !std::isspace((unsigned char)s[i])) ++i;

    out.BaseLabel = (start < s.size()) ? s.substr(start, i - start) : std::string();
    out.BaseLabel = TrimCopy(out.BaseLabel);

    // Check for trailing '*' which forces detailed ("nice") rendering
    if (!out.BaseLabel.empty() && out.BaseLabel.back() == '*')
    {
        out.ForceDetailed = true;
        out.BaseLabel.pop_back();
        out.BaseLabel = TrimCopy(out.BaseLabel);
    }

    std::string rem = (i < s.size()) ? TrimCopy(s.substr(i)) : std::string();
    if (rem.empty())
        return out;

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
            else
            {
                cur.push_back(ch);
            }
        }
        std::string t = TrimCopy(cur);
        if (!t.empty()) parts.push_back(t);
    }

    auto IsUnsignedTerm = [&](const std::string& term) -> bool
    {
        std::string t = TrimCopy(term);
        if (t.empty()) return false;
        return std::isdigit((unsigned char)t[0]) != 0;
    };

    auto ParseSignedTerm = [&](const std::string& term)
    {
        std::string t = ToUpperCopy(TrimCopy(term));
        if (t.empty()) return;

        // Stage 2: LOCK keyword
        if (t == "LOCK" || t == "FIXED")
        {
            out.Locked = true;
            return;
        }

        bool isX = false;
        if (!t.empty() && t.back() == 'X')
        {
            isX = true;
            t.pop_back();
            t = TrimCopy(t);
            if (t.empty()) return;
        }

        char sign = 0;
        std::string n;

        if (std::isdigit((unsigned char)t[0]))
        {
            sign = '+';
            n = t;
        }
        else if (t[0] == '+' || t[0] == '-')
        {
            sign = t[0];
            n = TrimCopy(t.substr(1));
        }
        else
        {
            return;
        }

        if (!IsDigits(n)) return;

        int v = std::atoi(n.c_str());
        if (v < 1) v = 1;
        if (v > 30) v = 30;

        if (sign == '+')
        {
            out.HasUp = true;
            out.Up = v;
            out.UpIsX = isX;
        }
        else
        {
            out.HasDown = true;
            out.Down = v;
            out.DownIsX = isX;
        }
    };

    if (parts.size() == 1)
    {
        ParseSignedTerm(parts[0]);
    }
    else
    {
        const bool p0Unsigned = IsUnsignedTerm(parts[0]);
        const bool p1Unsigned = IsUnsignedTerm(parts[1]);

        // If both unsigned (e.g. "4x,2x"), infer "+4x,-2x"
        if (p0Unsigned && p1Unsigned)
        {
            ParseSignedTerm(std::string("+") + parts[0]);
            ParseSignedTerm(std::string("-") + parts[1]);
        }
        else
        {
            ParseSignedTerm(parts[0]);
            ParseSignedTerm(parts[1]);
        }
    }

    return out;
}

// ----------------------------------------------------------------------------
// Styling helpers (settings-based, no subgraph colors)
// ----------------------------------------------------------------------------
static inline int ClampInt(int v, int lo, int hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }

static inline COLORREF GetZoneFillColor(const SCStudyInterfaceRef& sc, ZoneSide side, int group /*0..3*/)
{
    group = ClampInt(group, 0, 3);

    if (side == ZoneSide::Up)
    {
        switch (group)
        {
            case 0: return sc.Input[IN_UP_FILL_12].GetColor();
            case 1: return sc.Input[IN_UP_FILL_36].GetColor();
            case 2: return sc.Input[IN_UP_FILL_714].GetColor();
            default:return sc.Input[IN_UP_FILL_1530].GetColor();
        }
    }
    else
    {
        switch (group)
        {
            case 0: return sc.Input[IN_DN_FILL_12].GetColor();
            case 1: return sc.Input[IN_DN_FILL_36].GetColor();
            case 2: return sc.Input[IN_DN_FILL_714].GetColor();
            default:return sc.Input[IN_DN_FILL_1530].GetColor();
        }
    }
}

static inline int GetZoneTransparency(const SCStudyInterfaceRef& sc, ZoneSide side, int group /*0..3*/)
{
    group = ClampInt(group, 0, 3);

    if (side == ZoneSide::Up)
    {
        switch (group)
        {
            case 0: return ClampInt(sc.Input[IN_UP_TRANS_12].GetInt(), 0, 100);
            case 1: return ClampInt(sc.Input[IN_UP_TRANS_36].GetInt(), 0, 100);
            case 2: return ClampInt(sc.Input[IN_UP_TRANS_714].GetInt(), 0, 100);
            default:return ClampInt(sc.Input[IN_UP_TRANS_1530].GetInt(), 0, 100);
        }
    }
    else
    {
        switch (group)
        {
            case 0: return ClampInt(sc.Input[IN_DN_TRANS_12].GetInt(), 0, 100);
            case 1: return ClampInt(sc.Input[IN_DN_TRANS_36].GetInt(), 0, 100);
            case 2: return ClampInt(sc.Input[IN_DN_TRANS_714].GetInt(), 0, 100);
            default:return ClampInt(sc.Input[IN_DN_TRANS_1530].GetInt(), 0, 100);
        }
    }
}

static inline COLORREF ResolveBorderColor(const SCStudyInterfaceRef& sc, COLORREF fill)
{
    return (sc.Input[IN_BORDER_COLOR_MODE].GetInt() == 1)
        ? sc.Input[IN_CUSTOM_BORDER_COLOR].GetColor()
        : fill;
}

static inline COLORREF ResolveMidlineColor(const SCStudyInterfaceRef& sc, COLORREF fill)
{
    return (sc.Input[IN_MIDLINE_COLOR_MODE].GetInt() == 1)
        ? sc.Input[IN_CUSTOM_MIDLINE_COLOR].GetColor()
        : fill;
}

static inline bool WantBorderForAnchor(const SCStudyInterfaceRef& sc, bool isNiceMode)
{
    if (isNiceMode) return true;
    return sc.Input[IN_BASIC_DRAW_BORDER].GetYesNo() != 0;
}

// -----------------------------
// Global Structs
// -----------------------------
struct AnchorData
{
    int LineNumber = 0;
    int Region = 0;
    int BeginIndex = 0;
    int EndIndex = 0;
    double BeginValue = 0.0;
    double EndValue = 0.0;
    COLORREF Color = 0;
    COLORREF SecondaryColor = 0;
    int TransparencyLevel = 0;
    int DrawMidline = 0;    // Whether the anchor rectangle has native midline enabled
    int LineWidth = 1;      // Anchor outline/midline width
    SCDateTime BeginDateTime;
    SCDateTime EndDateTime;
    SCString Text;
};

struct AnchorCache
{
    std::vector<AnchorData> Anchors;
};

// ----------------------------------------------------------------------------
// Main study
// ----------------------------------------------------------------------------
SCSFExport scsf_BalanceZoneProjector(SCStudyInterfaceRef sc)
{
    // ---- Named Constants ----
    const unsigned LINE_NUMBER_MULTIPLIER = 10000u;
    const int LINE_NUMBER_MIN_BASE = 10000;
    
    const int UP_FILL_OFFSET      = 1000;
    const int UP_BORDER_OFFSET    = 2000;
    const int UP_MIDLINE_OFFSET   = 3000;
    const int UP_MIDTEXT_OFFSET   = 3500;
    const int UP_TOPTEXT_OFFSET   = 4000;
    const int UP_BOTTEXT_OFFSET   = 4500;
    
    const int DN_FILL_OFFSET      = 5000;
    const int DN_BORDER_OFFSET    = 6000;
    const int DN_MIDLINE_OFFSET   = 7000;
    const int DN_MIDTEXT_OFFSET   = 7500;
    const int DN_TOPTEXT_OFFSET   = 8000;
    const int DN_BOTTEXT_OFFSET   = 8500;
    
    const double FLOAT_EPSILON = 1e-9;
    const double PRICE_SCALE = 100000000.0; // for stable hashing
    sc.GraphName = "Balance Zone Engine";

    struct BZLabelCache
    {
        SCString inputState;
        std::vector<std::string> parsed;
    };

    struct AnchorIDTracker {
        // Key: Anchor LineNumber
        // Value: Vector of active projection LineNumbers (Zones, Midlines, Labels)
        std::unordered_map<int, std::vector<int>> activeMap;
    };

    // NOTE: BZZoneHashCache was removed - zone hash skip optimization caused drawing issues and was disabled.

    if (sc.SetDefaults)
    {
        sc.GraphRegion  = 0;
        sc.AutoLoop     = 0;
        sc.UpdateAlways = 1;
        sc.ReceiveChartDrawingEvents = 0;

        // Keep subgraphs only for alerts (hidden). Everything else ignored.
        for (int i = 0; i < 32; ++i)
        {
            SCSubgraphRef SG = sc.Subgraph[i];
            SG.Name.Clear();
            SG.DrawStyle    = DRAWSTYLE_IGNORE;
            SG.LineWidth    = 1;
            SG.LineStyle    = LINESTYLE_SOLID;
            SG.PrimaryColor = COLOR_BLACK;
        }

        {
            SCSubgraphRef Z = sc.Subgraph[24];
            Z.Name      = "Alert: Zone Index";
            Z.DrawStyle = DRAWSTYLE_HIDDEN;

            SCSubgraphRef C = sc.Subgraph[25];
            C.Name      = "Alert: Zone Changed";
            C.DrawStyle = DRAWSTYLE_HIDDEN;

            SCSubgraphRef E = sc.Subgraph[26];
            E.Name      = "Alert: Zone Entered";
            E.DrawStyle = DRAWSTYLE_HIDDEN;

            SCSubgraphRef D = sc.Subgraph[27];
            D.Name      = "Alert: Distance To Nearest Zone (ticks)";
            D.DrawStyle = DRAWSTYLE_HIDDEN;
        }

        SCString desc;
        desc.Format(
            "Balance Zone Engine v%s (%s)\n"
            "Projects balance zones above/below anchor rectangles with per-anchor overrides.\n"
            "Docs: https://tinyurl.com/BalanceZoneEngine",
            BZE_VERSION,
            BZE_BUILD_STR
        );
        sc.StudyDescription = desc;

        // ---- Inputs ----
        int dispOrder = 1;

        SCString helpName;
        helpName.Format("Balance Zone Engine Doco (v%s – %s)", BZE_VERSION, BZE_BUILD_STR);
        sc.Input[IN_HELP_URL].Name = helpName;
        sc.Input[IN_HELP_URL].SetString("https://tinyurl.com/BalanceZoneEngine");
        sc.Input[IN_HELP_URL].DisplayOrder = dispOrder++;

        sc.Input[IN_HDR_CORE].Name = "===== CORE CONTROL =====";
        sc.Input[IN_HDR_CORE].SetInt(0);
        sc.Input[IN_HDR_CORE].SetIntLimits(0, 0);
        sc.Input[IN_HDR_CORE].DisplayOrder = dispOrder++;

        sc.Input[IN_ANCHOR_LABELS].Name = "Anchor Rectangle Label Text(s) (comma-separated base labels, exact match)";
        sc.Input[IN_ANCHOR_LABELS].SetString("BZ");
        sc.Input[IN_ANCHOR_LABELS].DisplayOrder = dispOrder++;

        sc.Input[IN_ONLY_MOST_RECENT].Name = "Only Use Most Recent Anchor";
        sc.Input[IN_ONLY_MOST_RECENT].SetYesNo(0);
        sc.Input[IN_ONLY_MOST_RECENT].DisplayOrder = dispOrder++;

        sc.Input[IN_SHOW_RECT_PRICES].Name = "Show Price on Top and Bottom of Rectangles";
        sc.Input[IN_SHOW_RECT_PRICES].SetYesNo(1);
        sc.Input[IN_SHOW_RECT_PRICES].DisplayOrder = dispOrder++;

        sc.Input[IN_LABEL_DECIMALS].Name = "Label Decimal Places (default 2)";
        sc.Input[IN_LABEL_DECIMALS].SetInt(2);
        sc.Input[IN_LABEL_DECIMALS].SetIntLimits(0, 8);
        sc.Input[IN_LABEL_DECIMALS].DisplayOrder = dispOrder++;

        sc.Input[IN_LABEL_BAR_OFFSET].Name = "Label Bar Offset (+/- from RIGHT edge)";
        sc.Input[IN_LABEL_BAR_OFFSET].SetInt(2);
        sc.Input[IN_LABEL_BAR_OFFSET].SetIntLimits(-5000, 5000);
        sc.Input[IN_LABEL_BAR_OFFSET].DisplayOrder = dispOrder++;

        sc.Input[IN_LABEL_TEXT_COLOR].Name = "Label Color";
        sc.Input[IN_LABEL_TEXT_COLOR].SetColor(RGB(255, 255, 255));
        sc.Input[IN_LABEL_TEXT_COLOR].DisplayOrder = dispOrder++;

        sc.Input[IN_HDR_RANGE].Name = "===== PROJECTION RANGE =====";
        sc.Input[IN_HDR_RANGE].SetInt(0);
        sc.Input[IN_HDR_RANGE].SetIntLimits(0, 0);
        sc.Input[IN_HDR_RANGE].DisplayOrder = dispOrder++;
        
         // Moved Auto-Extend inputs here visually and physically
        sc.Input[IN_AUTO_EXTEND_MODE].Name = "Auto-Extend Latest Anchor Mode";
        sc.Input[IN_AUTO_EXTEND_MODE].SetCustomInputStrings("Disabled;Always Enabled;Control Bar Button");
        sc.Input[IN_AUTO_EXTEND_MODE].SetCustomInputIndex(0); // Default Disabled
        sc.Input[IN_AUTO_EXTEND_MODE].DisplayOrder = dispOrder++;

        sc.Input[IN_AUTO_EXTEND_BUTTON_ID].Name = "Control Bar Button ID (For Button Mode)";
        sc.Input[IN_AUTO_EXTEND_BUTTON_ID].SetInt(0);
        sc.Input[IN_AUTO_EXTEND_BUTTON_ID].DisplayOrder = dispOrder++;

        sc.Input[IN_ENABLE_UP].Name = "Enable Up Projections";
        sc.Input[IN_ENABLE_UP].SetYesNo(1);
        sc.Input[IN_ENABLE_UP].DisplayOrder = dispOrder++;

        sc.Input[IN_ENABLE_DOWN].Name = "Enable Down Projections";
        sc.Input[IN_ENABLE_DOWN].SetYesNo(1);
        sc.Input[IN_ENABLE_DOWN].DisplayOrder = dispOrder++;

        sc.Input[IN_MAX_UP_GROUPS].Name = "Max Up Zone Groups (0-4)";
        sc.Input[IN_MAX_UP_GROUPS].SetInt(4);
        sc.Input[IN_MAX_UP_GROUPS].SetIntLimits(0, 4);
        sc.Input[IN_MAX_UP_GROUPS].DisplayOrder = dispOrder++;

        sc.Input[IN_MAX_DOWN_GROUPS].Name = "Max Down Zone Groups (0-4)";
        sc.Input[IN_MAX_DOWN_GROUPS].SetInt(4);
        sc.Input[IN_MAX_DOWN_GROUPS].SetIntLimits(0, 4);
        sc.Input[IN_MAX_DOWN_GROUPS].DisplayOrder = dispOrder++;

        sc.Input[IN_HDR_FMT].Name = "===== BALANCE ZONE FORMATTING =====";
        sc.Input[IN_HDR_FMT].SetInt(0);
        sc.Input[IN_HDR_FMT].SetIntLimits(0, 0);
        sc.Input[IN_HDR_FMT].DisplayOrder = dispOrder++;

        sc.Input[IN_BASIC_DRAW_BORDER].Name = "Basic Mode: Draw Border (Y/N)";
        sc.Input[IN_BASIC_DRAW_BORDER].SetYesNo(0);
        sc.Input[IN_BASIC_DRAW_BORDER].DisplayOrder = dispOrder++;

        sc.Input[IN_BORDER_COLOR_MODE].Name = "Border Color";
        sc.Input[IN_BORDER_COLOR_MODE].SetCustomInputStrings("Fill;Custom");
        sc.Input[IN_BORDER_COLOR_MODE].SetCustomInputIndex(1);
        sc.Input[IN_BORDER_COLOR_MODE].SetIntLimits(0, 1);
        sc.Input[IN_BORDER_COLOR_MODE].DisplayOrder = dispOrder++;

        sc.Input[IN_CUSTOM_BORDER_COLOR].Name = "Custom Border Color";
        sc.Input[IN_CUSTOM_BORDER_COLOR].SetColor(RGB(255, 255, 255));
        sc.Input[IN_CUSTOM_BORDER_COLOR].DisplayOrder = dispOrder++;

        // ---- Zone Name Labels (zone bands; latest anchor only) ----
        sc.Input[IN_ZONE_NAME_LABEL_MODE].Name = "Zone Name Labels";
        sc.Input[IN_ZONE_NAME_LABEL_MODE].SetCustomInputStrings("Off;Nice Anchors Only;All Anchors");
        sc.Input[IN_ZONE_NAME_LABEL_MODE].SetCustomInputIndex(1);
        sc.Input[IN_ZONE_NAME_LABEL_MODE].SetIntLimits(0, 2);
        sc.Input[IN_ZONE_NAME_LABEL_MODE].DisplayOrder = dispOrder++;

        sc.Input[IN_ZONE_LABEL_FONT_SIZE].Name = "Zone Name Label Font Size";
        sc.Input[IN_ZONE_LABEL_FONT_SIZE].SetInt(12);
        sc.Input[IN_ZONE_LABEL_FONT_SIZE].SetIntLimits(6, 36);
        sc.Input[IN_ZONE_LABEL_FONT_SIZE].DisplayOrder = dispOrder++;

        sc.Input[IN_ZONE_LABEL_FONT_COLOR].Name = "Zone Name Label Font Color";
        sc.Input[IN_ZONE_LABEL_FONT_COLOR].SetColor(RGB(255, 255, 255));
        sc.Input[IN_ZONE_LABEL_FONT_COLOR].DisplayOrder = dispOrder++;

        sc.Input[IN_ZONE_LABEL_LEFT_OFFSET].Name = "Zone Name Label Left Offset (Bars)";
        sc.Input[IN_ZONE_LABEL_LEFT_OFFSET].SetInt(2);
        sc.Input[IN_ZONE_LABEL_LEFT_OFFSET].SetIntLimits(-5000, 5000);
        sc.Input[IN_ZONE_LABEL_LEFT_OFFSET].DisplayOrder = dispOrder++;

        sc.Input[IN_COST_BASIS_ZONE_LABEL].Name = "Cost Basis Zone Label";
        sc.Input[IN_COST_BASIS_ZONE_LABEL].SetString("Cost Basis");
        sc.Input[IN_COST_BASIS_ZONE_LABEL].DisplayOrder = dispOrder++;

        sc.Input[IN_UP_ZONE_LABEL_TEMPLATE].Name = "Up Zone Label Template";
        sc.Input[IN_UP_ZONE_LABEL_TEMPLATE].SetString("Balance x{X}");
        sc.Input[IN_UP_ZONE_LABEL_TEMPLATE].DisplayOrder = dispOrder++;

        sc.Input[IN_DOWN_ZONE_LABEL_TEMPLATE].Name = "Down Zone Label Template";
        sc.Input[IN_DOWN_ZONE_LABEL_TEMPLATE].SetString("Discount x{X}");
        sc.Input[IN_DOWN_ZONE_LABEL_TEMPLATE].DisplayOrder = dispOrder++;


        sc.Input[IN_HDR_UP].Name = "===== UP ZONES =====";
        sc.Input[IN_HDR_UP].SetInt(0);
        sc.Input[IN_HDR_UP].SetIntLimits(0, 0);
        sc.Input[IN_HDR_UP].DisplayOrder = dispOrder++;

        sc.Input[IN_UP_TRANS_12].Name = "Up Zones Transparency 1-2 (0=opaque..100=invisible)";
        sc.Input[IN_UP_TRANS_12].SetInt(75);
        sc.Input[IN_UP_TRANS_12].SetIntLimits(0, 100);
        sc.Input[IN_UP_TRANS_12].DisplayOrder = dispOrder++;

        sc.Input[IN_UP_FILL_12].Name = "Up Zones Fill Color 1-2";
        sc.Input[IN_UP_FILL_12].SetColor(RGB(0, 255, 0));
        sc.Input[IN_UP_FILL_12].DisplayOrder = dispOrder++;

        sc.Input[IN_UP_TRANS_36].Name = "Up Zones Transparency 3-6";
        sc.Input[IN_UP_TRANS_36].SetInt(80);
        sc.Input[IN_UP_TRANS_36].SetIntLimits(0, 100);
        sc.Input[IN_UP_TRANS_36].DisplayOrder = dispOrder++;

        sc.Input[IN_UP_FILL_36].Name = "Up Zones Fill Color 3-6";
        sc.Input[IN_UP_FILL_36].SetColor(RGB(255, 0, 255));
        sc.Input[IN_UP_FILL_36].DisplayOrder = dispOrder++;

        sc.Input[IN_UP_TRANS_714].Name = "Up Zones Transparency 7-14";
        sc.Input[IN_UP_TRANS_714].SetInt(85);
        sc.Input[IN_UP_TRANS_714].SetIntLimits(0, 100);
        sc.Input[IN_UP_TRANS_714].DisplayOrder = dispOrder++;

        sc.Input[IN_UP_FILL_714].Name = "Up Zones Fill Color 7-14";
        sc.Input[IN_UP_FILL_714].SetColor(RGB(0, 255, 64));
        sc.Input[IN_UP_FILL_714].DisplayOrder = dispOrder++;

        sc.Input[IN_UP_TRANS_1530].Name = "Up Zones Transparency 15-30";
        sc.Input[IN_UP_TRANS_1530].SetInt(70);
        sc.Input[IN_UP_TRANS_1530].SetIntLimits(0, 100);
        sc.Input[IN_UP_TRANS_1530].DisplayOrder = dispOrder++;

        sc.Input[IN_UP_FILL_1530].Name = "Up Zones Fill Color 15-30";
        sc.Input[IN_UP_FILL_1530].SetColor(RGB(255, 255, 255));
        sc.Input[IN_UP_FILL_1530].DisplayOrder = dispOrder++;

        sc.Input[IN_HDR_DOWN].Name = "===== DOWN ZONES =====";
        sc.Input[IN_HDR_DOWN].SetInt(0);
        sc.Input[IN_HDR_DOWN].SetIntLimits(0, 0);
        sc.Input[IN_HDR_DOWN].DisplayOrder = dispOrder++;

        sc.Input[IN_DN_TRANS_12].Name = "Down Zones Transparency 1-2 (0=opaque..100=invisible)";
        sc.Input[IN_DN_TRANS_12].SetInt(75);
        sc.Input[IN_DN_TRANS_12].SetIntLimits(0, 100);
        sc.Input[IN_DN_TRANS_12].DisplayOrder = dispOrder++;

        sc.Input[IN_DN_FILL_12].Name = "Down Zones Fill Color 1-2";
        sc.Input[IN_DN_FILL_12].SetColor(RGB(255, 0, 0));
        sc.Input[IN_DN_FILL_12].DisplayOrder = dispOrder++;

        sc.Input[IN_DN_TRANS_36].Name = "Down Zones Transparency 3-6";
        sc.Input[IN_DN_TRANS_36].SetInt(80);
        sc.Input[IN_DN_TRANS_36].SetIntLimits(0, 100);
        sc.Input[IN_DN_TRANS_36].DisplayOrder = dispOrder++;

        sc.Input[IN_DN_FILL_36].Name = "Down Zones Fill Color 3-6";
        sc.Input[IN_DN_FILL_36].SetColor(RGB(0, 255, 255));
        sc.Input[IN_DN_FILL_36].DisplayOrder = dispOrder++;

        sc.Input[IN_DN_TRANS_714].Name = "Down Zones Transparency 7-14";
        sc.Input[IN_DN_TRANS_714].SetInt(85);
        sc.Input[IN_DN_TRANS_714].SetIntLimits(0, 100);
        sc.Input[IN_DN_TRANS_714].DisplayOrder = dispOrder++;

        sc.Input[IN_DN_FILL_714].Name = "Down Zones Fill Color 7-14";
        sc.Input[IN_DN_FILL_714].SetColor(RGB(255, 0, 0));
        sc.Input[IN_DN_FILL_714].DisplayOrder = dispOrder++;

        sc.Input[IN_DN_TRANS_1530].Name = "Down Zones Transparency 15-30";
        sc.Input[IN_DN_TRANS_1530].SetInt(70);
        sc.Input[IN_DN_TRANS_1530].SetIntLimits(0, 100);
        sc.Input[IN_DN_TRANS_1530].DisplayOrder = dispOrder++;

        sc.Input[IN_DN_FILL_1530].Name = "Down Zones Fill Color 15-30";
        sc.Input[IN_DN_FILL_1530].SetColor(RGB(255, 255, 255));
        sc.Input[IN_DN_FILL_1530].DisplayOrder = dispOrder++;

        sc.Input[IN_HDR_MIDLINE].Name = "===== MIDLINE APPEARANCE =====";
        sc.Input[IN_HDR_MIDLINE].SetInt(0);
        sc.Input[IN_HDR_MIDLINE].SetIntLimits(0, 0);
        sc.Input[IN_HDR_MIDLINE].DisplayOrder = dispOrder++;

        sc.Input[IN_DRAW_MIDLINE].Name = "Draw Midline For Each Projection";
        sc.Input[IN_DRAW_MIDLINE].SetYesNo(0);
        sc.Input[IN_DRAW_MIDLINE].DisplayOrder = dispOrder++;

        sc.Input[IN_MIDLINE_STYLE].Name = "Midline Style";
        sc.Input[IN_MIDLINE_STYLE].SetCustomInputStrings("None;Solid;Dash;Dot");
        sc.Input[IN_MIDLINE_STYLE].SetCustomInputIndex(1);
        sc.Input[IN_MIDLINE_STYLE].SetIntLimits(0, 3);
        sc.Input[IN_MIDLINE_STYLE].DisplayOrder = dispOrder++;

        sc.Input[IN_MIDLINE_COLOR_MODE].Name = "Midline Color";
        sc.Input[IN_MIDLINE_COLOR_MODE].SetCustomInputStrings("Fill;Custom");
        sc.Input[IN_MIDLINE_COLOR_MODE].SetCustomInputIndex(0);
        sc.Input[IN_MIDLINE_COLOR_MODE].SetIntLimits(0, 1);
        sc.Input[IN_MIDLINE_COLOR_MODE].DisplayOrder = dispOrder++;

        sc.Input[IN_CUSTOM_MIDLINE_COLOR].Name = "Custom Midline Color";
        sc.Input[IN_CUSTOM_MIDLINE_COLOR].SetColor(RGB(255, 255, 255));
        sc.Input[IN_CUSTOM_MIDLINE_COLOR].DisplayOrder = dispOrder++;

        sc.Input[IN_SHOW_MIDLINE_PRICE].Name = "Show Price Text On Midline";
        sc.Input[IN_SHOW_MIDLINE_PRICE].SetYesNo(0);
        sc.Input[IN_SHOW_MIDLINE_PRICE].DisplayOrder = dispOrder++;

        sc.Input[IN_MIDLINE_WIDTH].Name = "Midline Line Width";
        sc.Input[IN_MIDLINE_WIDTH].SetInt(1);
        sc.Input[IN_MIDLINE_WIDTH].SetIntLimits(1, 6);
        sc.Input[IN_MIDLINE_WIDTH].DisplayOrder = dispOrder++;

        sc.Input[IN_HDR_PERF].Name = "===== REDRAW & PERFORMANCE =====";
        sc.Input[IN_HDR_PERF].SetInt(0);
        sc.Input[IN_HDR_PERF].SetIntLimits(0, 0);
        sc.Input[IN_HDR_PERF].DisplayOrder = dispOrder++;

        sc.Input[IN_AUTO_REDRAW].Name = "Auto Redraw Enabled";
        sc.Input[IN_AUTO_REDRAW].SetYesNo(1);
        sc.Input[IN_AUTO_REDRAW].DisplayOrder = dispOrder++;

        sc.Input[IN_FORCE_REDRAW_NOW].Name = "Force Redraw Now (Manual Trigger)";
        sc.Input[IN_FORCE_REDRAW_NOW].SetYesNo(0);
        sc.Input[IN_FORCE_REDRAW_NOW].DisplayOrder = dispOrder++;

        sc.Input[IN_CLEAR_OLD_PROJECTIONS].Name = "Clear Old Projections Each Update";
        sc.Input[IN_CLEAR_OLD_PROJECTIONS].SetYesNo(1);
        sc.Input[IN_CLEAR_OLD_PROJECTIONS].DisplayOrder = dispOrder++;

        sc.Input[IN_HDR_SCOPE].Name = "===== SCOPE & QUALITY =====";
        sc.Input[IN_HDR_SCOPE].SetInt(0);
        sc.Input[IN_HDR_SCOPE].SetIntLimits(0, 0);
        sc.Input[IN_HDR_SCOPE].DisplayOrder = dispOrder++;

        sc.Input[IN_PROCESS_VISIBLE_ONLY].Name = "Process Only Anchors Overlapping Visible Bars (Performance)";
        sc.Input[IN_PROCESS_VISIBLE_ONLY].SetYesNo(1);
        sc.Input[IN_PROCESS_VISIBLE_ONLY].DisplayOrder = dispOrder++;

        sc.Input[IN_NICE_ANCHORS_MODE].Name = "Nice Anchors Mode";
        sc.Input[IN_NICE_ANCHORS_MODE].SetCustomInputStrings("Latest N (by Right Edge);All Anchors");
        sc.Input[IN_NICE_ANCHORS_MODE].SetCustomInputIndex(0);
        sc.Input[IN_NICE_ANCHORS_MODE].SetIntLimits(0, 1);
        sc.Input[IN_NICE_ANCHORS_MODE].DisplayOrder = dispOrder++;

        sc.Input[IN_NICE_ANCHORS_COUNT].Name = "Nice Anchors: Latest N";
        sc.Input[IN_NICE_ANCHORS_COUNT].SetInt(1);
        sc.Input[IN_NICE_ANCHORS_COUNT].SetIntLimits(1, 5000);
        sc.Input[IN_NICE_ANCHORS_COUNT].DisplayOrder = dispOrder++;

        sc.Input[IN_REFRESH_BUTTON_ID].Name = "Refresh Button ID (Force Recalc - e.g., after hiding drawings)";
        sc.Input[IN_REFRESH_BUTTON_ID].SetInt(0);
        sc.Input[IN_REFRESH_BUTTON_ID].DisplayOrder = dispOrder++;

        sc.Input[IN_HDR_ALERTS].Name = "===== ALERTS & GAP DETECTION =====";
        sc.Input[IN_HDR_ALERTS].SetInt(0);
        sc.Input[IN_HDR_ALERTS].SetIntLimits(0, 0);
        sc.Input[IN_HDR_ALERTS].DisplayOrder = dispOrder++;

        sc.Input[IN_ALERT_PRICE_SOURCE].Name = "Alert Price Source";
        sc.Input[IN_ALERT_PRICE_SOURCE].SetCustomInputStrings("Last Trade;Bar Close");
        sc.Input[IN_ALERT_PRICE_SOURCE].SetCustomInputIndex(0);
        sc.Input[IN_ALERT_PRICE_SOURCE].SetIntLimits(0, 1);
        sc.Input[IN_ALERT_PRICE_SOURCE].DisplayOrder = dispOrder++;

        sc.Input[IN_ALERT_USE_GAP_DETECTION].Name = "Alert: Use Gap Detection (Tick-by-Tick)";
        sc.Input[IN_ALERT_USE_GAP_DETECTION].SetYesNo(1);
        sc.Input[IN_ALERT_USE_GAP_DETECTION].DisplayOrder = dispOrder++;

        sc.Input[IN_ALERT_INCLUDE_BAR_HL].Name = "Alert: Include Bar High/Low (Historical)";
        sc.Input[IN_ALERT_INCLUDE_BAR_HL].SetYesNo(1);
        sc.Input[IN_ALERT_INCLUDE_BAR_HL].DisplayOrder = dispOrder++;

        return;
    }

    // ---- Cleanup on unload ----
    if (sc.LastCallToFunction)
    {
        auto* prevAnchors = static_cast<std::vector<int>*>(sc.GetPersistentPointer(101));
        if (prevAnchors != nullptr)
        {
            delete prevAnchors;
            sc.SetPersistentPointer(101, nullptr);
        }

        auto* pCache = static_cast<BZLabelCache*>(sc.GetPersistentPointer(102));
        if (pCache != nullptr)
        {
            delete pCache;
            sc.SetPersistentPointer(102, nullptr);
        }
        
        // Clear the previous zone label owner tracking
        sc.SetPersistentPointer(103, nullptr);
        
        // Stage 1: Persistent Tracker Cleanup
        auto* pTracker = static_cast<AnchorIDTracker*>(sc.GetPersistentPointer(104));
        if (pTracker != nullptr)
        {
            delete pTracker;
            sc.SetPersistentPointer(104, nullptr);
        }

        // NOTE: Zone hash cache (persistent pointer 4) cleanup removed - optimization disabled.
        return;
    }

    // Hidden subgraphs for alerts
    SCSubgraphRef SG_ZoneIndex   = sc.Subgraph[24];
    SCSubgraphRef SG_ZoneChange  = sc.Subgraph[25];
    SCSubgraphRef SG_ZoneEntered = sc.Subgraph[26];
    SCSubgraphRef SG_ZoneDist    = sc.Subgraph[27];

    // ---- Runtime config ----
    auto* pCache = static_cast<BZLabelCache*>(sc.GetPersistentPointer(102));
    if (!pCache)
    {
        pCache = new BZLabelCache;
        sc.SetPersistentPointer(102, pCache);
    }

    
    // NOTE: Zone hash cache (persistent pointer 4) was removed - optimization disabled.

    const SCString currentLabelInput = sc.Input[IN_ANCHOR_LABELS].GetString();
    if (pCache->inputState != currentLabelInput)
    {
        pCache->inputState = currentLabelInput;
        pCache->parsed = SplitAndTrimCSV(currentLabelInput.GetChars());
    }
    const std::vector<std::string>& anchorLabels = pCache->parsed;

    auto MatchesAnchorLabel = [&](const SCString& toolText) -> bool
    {
        if (anchorLabels.empty()) return false;
        
        const char* txtPtr = toolText.GetChars();
        if (!txtPtr || !*txtPtr) return false;

        // Fast zero-allocation match (avoids string copies in loop)
        while (*txtPtr && std::isspace((unsigned char)*txtPtr)) ++txtPtr;
        if (!*txtPtr) return false;

        for (const auto& lbl : anchorLabels)
        {
            // Check prefix
            const size_t len = lbl.length();
            if (strncmp(txtPtr, lbl.c_str(), len) == 0)
            {
                // Check boundary (next char must be space, comma, '*', or null)
                const char next = txtPtr[len];
                if (next == 0 || next == ',' || next == '*' || std::isspace((unsigned char)next))
                    return true;
            }
        }

        return false;
    };

    auto GroupCountToMaxMultiplier = [](int groups) -> int
    {
        if (groups <= 0) return 0;
        if (groups == 1) return 2;
        if (groups == 2) return 6;
        if (groups == 3) return 14;
        return 30;
    };

    const int maxUpGroups   = sc.Input[IN_MAX_UP_GROUPS].GetInt();
    const int maxDownGroups = sc.Input[IN_MAX_DOWN_GROUPS].GetInt();

    const int maxUp   = GroupCountToMaxMultiplier(maxUpGroups);
    const int maxDown = GroupCountToMaxMultiplier(maxDownGroups);

    const bool enableUp   = (sc.Input[IN_ENABLE_UP].GetYesNo() != 0)   && maxUp > 0;
    const bool enableDown = (sc.Input[IN_ENABLE_DOWN].GetYesNo() != 0) && maxDown > 0;

    const bool onlyMostRecent = sc.Input[IN_ONLY_MOST_RECENT].GetYesNo() != 0;

    const bool drawMidline      = sc.Input[IN_DRAW_MIDLINE].GetYesNo() != 0;
    const bool showMidlinePrice = sc.Input[IN_SHOW_MIDLINE_PRICE].GetYesNo() != 0;
    const bool clearOld         = sc.Input[IN_CLEAR_OLD_PROJECTIONS].GetYesNo() != 0;
    const bool autoRedraw       = sc.Input[IN_AUTO_REDRAW].GetYesNo() != 0;
    const bool forceRedrawNow   = sc.Input[IN_FORCE_REDRAW_NOW].GetYesNo() != 0;
    const bool showRectPrices   = sc.Input[IN_SHOW_RECT_PRICES].GetYesNo() != 0;

    const COLORREF labelTextColor = sc.Input[IN_LABEL_TEXT_COLOR].GetColor();
    const bool hideStudy        = sc.HideStudy != 0;

    const int alertPriceSource = sc.Input[IN_ALERT_PRICE_SOURCE].GetInt(); // 0=LastTrade, 1=Close

    const int midlineStyleSetting = sc.Input[IN_MIDLINE_STYLE].GetInt(); // 0..3
    const int midlineWidth        = ClampInt(sc.Input[IN_MIDLINE_WIDTH].GetInt(), 1, 6);

    const bool midlineEnabledStudySetting = drawMidline && midlineStyleSetting != 0;

    // Labels
    const int labelBarOff = sc.Input[IN_LABEL_BAR_OFFSET].GetInt();

    // Zone-name labels (latest anchor only)
    const int zoneNameLabelMode = sc.Input[IN_ZONE_NAME_LABEL_MODE].GetInt(); // 0=Off,1=Nice Only,2=All
    const int zoneLabelBarOff   = sc.Input[IN_ZONE_LABEL_LEFT_OFFSET].GetInt();
    const int zoneLabelFontSize = ClampInt(sc.Input[IN_ZONE_LABEL_FONT_SIZE].GetInt(), 6, 36);
    const COLORREF zoneLabelFontColor = sc.Input[IN_ZONE_LABEL_FONT_COLOR].GetColor();

    const SCString zoneLabelCostBasis = sc.Input[IN_COST_BASIS_ZONE_LABEL].GetString();
    const SCString zoneLabelUpTpl     = sc.Input[IN_UP_ZONE_LABEL_TEMPLATE].GetString();
    const SCString zoneLabelDownTpl   = sc.Input[IN_DOWN_ZONE_LABEL_TEMPLATE].GetString();

    auto HashStr32 = [&](const SCString& s) -> int
    {
        const char* c = s.GetChars();
        if (!c) return 0;
        unsigned h = 2166136261u;
        for (; *c; ++c)
        {
            h ^= (unsigned char)(*c);
            h *= 16777619u;
        }
        return (int)h;
    };

    const int zoneLblHashCost = HashStr32(zoneLabelCostBasis);
    const int zoneLblHashUp   = HashStr32(zoneLabelUpTpl);
    const int zoneLblHashDn   = HashStr32(zoneLabelDownTpl);

    int userDecimals = sc.Input[IN_LABEL_DECIMALS].GetInt();
    userDecimals = ClampInt(userDecimals, 0, 8);

    // scope/quality
    const bool processVisibleOnlySetting = (sc.Input[IN_PROCESS_VISIBLE_ONLY].GetYesNo() != 0);
        const int niceMode = sc.Input[IN_NICE_ANCHORS_MODE].GetInt();
    int niceN = sc.Input[IN_NICE_ANCHORS_COUNT].GetInt();
    if (niceMode == 1)
        niceN = 0; // All Anchors
    else
        niceN = ClampInt(niceN, 1, 5000);





    // Resolve midline style enum
    SubgraphLineStyles midLineStyleEnum = LINESTYLE_DASH;
    if (midlineStyleSetting == 1) midLineStyleEnum = LINESTYLE_SOLID;
    else if (midlineStyleSetting == 2) midLineStyleEnum = LINESTYLE_DASH;
    else if (midlineStyleSetting == 3) midLineStyleEnum = LINESTYLE_DOT;

    // --------- Price formatting / snapping ---------
auto Pow10 = [&](int n) -> double
{
    double p = 1.0;
    for (int i = 0; i < n; ++i) p *= 10.0;
    return p;
};

const double baseIncrementManual = (userDecimals <= 0) ? 1.0 : (1.0 / Pow10(userDecimals));

auto DecimalsFromTick = [&](double tick) -> int
{
    if (tick <= 0.0) return userDecimals;

    for (int d = 0; d <= 8; ++d)
    {
        const double p = Pow10(d);
        const double x = tick * p;
        const double r = floor(x + 0.5);
        const double eps = 1e-8 * (fabs(x) + 1.0);
        if (fabs(x - r) <= eps)
            return d;
    }
    return 8;
};

const double rawTick = sc.TickSize;

// Manual decimals increment (used ONLY for label text formatting and as a fallback when TickSize is unavailable).
const bool tickLooksSane =
    (rawTick > 0.0) &&
    (rawTick >= baseIncrementManual * 0.5) &&
    (rawTick <= baseIncrementManual * 100000.0);

double snapIncManual = baseIncrementManual;
if (tickLooksSane)
{
    const int td = DecimalsFromTick(rawTick);
    const double p = Pow10(td);
    const double rounded = floor(rawTick * p + 0.5) / p;
    snapIncManual = (rounded > 0.0) ? rounded : baseIncrementManual;
}

// IMPORTANT:
//  - Draw geometry (extensions/midlines/alerts) must snap to the chart tick size, independent of Label Decimal Places.
//  - Label Decimal Places only controls how we DISPLAY the text.
const double snapIncDraw = (rawTick > 0.0) ? rawTick : snapIncManual;

auto SnapToIncrement = [&](double price) -> double
{
    const double inc = snapIncDraw;
    if (inc <= 0.0) return price;

    const long long ticks = llround(price / inc);
    double snapped = (double)ticks * inc;

    // Clean up floating residue without changing the tick-aligned value.
    const int cleanDecimals = (rawTick > 0.0) ? DecimalsFromTick(rawTick) : userDecimals;
    const double p = Pow10(cleanDecimals);
    if (p > 0.0)
        snapped = llround(snapped * p) / p;

    return snapped;
};

// Pre-compute format string once (avoids repeated format parsing)
char priceFmtStr[16];
sprintf(priceFmtStr, "%%.%df", userDecimals);  // e.g., "%.2f"

auto FormatPrice = [&](double price) -> SCString
{
    const double snapped = SnapToIncrement(price);
    SCString out;
    out.Format(priceFmtStr, snapped);
    return out;
};

const double labelOffsetPrice = (snapIncDraw > 0.0) ? (snapIncDraw * 0.5) : 0.0;
// --------------------------------------------
    // Redraw gating
    // --------------------------------------------
    int& PrevForceRedrawState = sc.GetPersistentInt(0);
    const bool prevForce = (PrevForceRedrawState != 0);

    // ---- Visible range (used for anchor filtering) ----
    int visibleLeftIndex = 0;
    int visibleRightIndex = (sc.ArraySize > 0) ? (sc.ArraySize - 1) : 0;

    if (sc.IndexOfFirstVisibleBar >= 0) visibleLeftIndex = sc.IndexOfFirstVisibleBar;
    if (sc.IndexOfLastVisibleBar >= 0)  visibleRightIndex = sc.IndexOfLastVisibleBar;

    if (visibleLeftIndex < 0) visibleLeftIndex = 0;
    if (visibleRightIndex < 0) visibleRightIndex = 0;
    if (visibleRightIndex < visibleLeftIndex) std::swap(visibleRightIndex, visibleLeftIndex);

    auto AnchorOverlapsVisible = [&](int leftIndex, int rightIndex) -> bool
    {
        if (rightIndex < visibleLeftIndex) return false;
        if (leftIndex  > visibleRightIndex) return false;
        return true;
    };

    // ---- Find anchor rectangles (using lightweight AnchorData instead of full s_UseTool) ----
    std::vector<AnchorData> anchors;
    anchors.reserve(64);

    s_UseTool Tool;
    Tool.Clear();
    int drawIndex = 0;

    while (sc.GetUserDrawnChartDrawing(sc.ChartNumber, DRAWING_RECTANGLEHIGHLIGHT, Tool, drawIndex) != 0)
    {
        ++drawIndex;

        if (Tool.Text.GetLength() > 0 && MatchesAnchorLabel(Tool.Text))
        {
            // Extract only the fields we need into lightweight struct
            AnchorData ad;
            ad.LineNumber = Tool.LineNumber;
            ad.Region = Tool.Region;
            ad.BeginIndex = Tool.BeginIndex;
            ad.EndIndex = Tool.EndIndex;
            ad.BeginValue = Tool.BeginValue;
            ad.EndValue = Tool.EndValue;
            ad.Color = Tool.Color;
            ad.SecondaryColor = Tool.SecondaryColor;
            ad.TransparencyLevel = Tool.TransparencyLevel;
            ad.DrawMidline = Tool.DrawMidline;
            ad.LineWidth = Tool.LineWidth;
            ad.BeginDateTime = Tool.BeginDateTime;
            ad.EndDateTime = Tool.EndDateTime;
            ad.Text = Tool.Text;
            anchors.push_back(ad);
        }

        Tool.Clear();
    }

    std::vector<int> currentFoundAnchorLines;
    currentFoundAnchorLines.reserve(anchors.size());
    for (const auto& a : anchors)
        currentFoundAnchorLines.push_back(a.LineNumber);

    // anchor geometry hash (includes text)
    auto HashMix64 = [&](uint64_t& h, uint64_t v)
    {
        v += 0x9e3779b97f4a7c15ULL;
        v = (v ^ (v >> 30)) * 0xbf58476d1ce4e5b9ULL;
        v = (v ^ (v >> 27)) * 0x94d049bb133111ebULL;
        v = v ^ (v >> 31);
        h ^= v;
        h *= 0x9ddfea08eb382d69ULL;
    };

    auto DblToKey = [&](double x) -> int64_t
    {
        const double s = SnapToIncrement(x);
        return (int64_t)llround(s * PRICE_SCALE);
    };

    auto TextKey = [&](const SCString& txt) -> std::string
    {
        const char* t = txt.GetChars();
        if (!t) return std::string();
        return TrimCopy(std::string(t));
    };

    // Pick latest anchor deterministically (same ordering you used)
    auto PickLatestAnchorByRightEdge = [&](const std::vector<AnchorData>& list) -> const AnchorData*
    {
        if (list.empty()) return nullptr;

        const AnchorData* best = &list[0];

        auto RightIndexOf = [&](const AnchorData& a) -> int
        {
            return (a.BeginIndex < a.EndIndex) ? a.EndIndex : a.BeginIndex;
        };
        auto BeginIdxOf = [&](const AnchorData& a) -> int { return a.BeginIndex; };

        int bestRight = RightIndexOf(*best);
        int bestBegin = BeginIdxOf(*best);
        int bestLine  = best->LineNumber;

        for (size_t k = 1; k < list.size(); ++k)
        {
            const AnchorData& a = list[k];
            const int r  = RightIndexOf(a);
            const int b  = BeginIdxOf(a);
            const int ln = a.LineNumber;

            if (r > bestRight || (r == bestRight && b > bestBegin) || (r == bestRight && b == bestBegin && ln > bestLine))
            {
                best = &a;
                bestRight = r;
                bestBegin = b;
                bestLine  = ln;
            }
        }
        return best;
    };

    uint64_t curAnchorGeomHash = 0xcbf29ce484222325ULL;

    if (!anchors.empty())
    {
        if (onlyMostRecent)
        {
            const AnchorData* pLatest = PickLatestAnchorByRightEdge(anchors);
            const AnchorData& a = (pLatest != nullptr) ? *pLatest : anchors.back();

            HashMix64(curAnchorGeomHash, (uint64_t)(unsigned)a.LineNumber);
            HashMix64(curAnchorGeomHash, (uint64_t)(unsigned)a.Region);
            HashMix64(curAnchorGeomHash, (uint64_t)(unsigned)a.BeginIndex);
            HashMix64(curAnchorGeomHash, (uint64_t)(unsigned)a.EndIndex);
            HashMix64(curAnchorGeomHash, (uint64_t)(uint64_t)DblToKey(a.BeginValue));
            HashMix64(curAnchorGeomHash, (uint64_t)(uint64_t)DblToKey(a.EndValue));
            HashString64(curAnchorGeomHash, TextKey(a.Text));
        }
        else
        {
            for (const auto& a : anchors)
            {
                HashMix64(curAnchorGeomHash, (uint64_t)(unsigned)a.LineNumber);
                HashMix64(curAnchorGeomHash, (uint64_t)(unsigned)a.Region);
                HashMix64(curAnchorGeomHash, (uint64_t)(unsigned)a.BeginIndex);
                HashMix64(curAnchorGeomHash, (uint64_t)(unsigned)a.EndIndex);
                HashMix64(curAnchorGeomHash, (uint64_t)(uint64_t)DblToKey(a.BeginValue));
                HashMix64(curAnchorGeomHash, (uint64_t)(uint64_t)DblToKey(a.EndValue));
                HashString64(curAnchorGeomHash, TextKey(a.Text));
            }
        }
    }

    // Persisted anchor hash (2x int)
    int& P_LastAnchorGeomHashLo = sc.GetPersistentInt(60);
    int& P_LastAnchorGeomHashHi = sc.GetPersistentInt(61);

    const uint64_t lastAnchorGeomHash =
        ((uint64_t)(uint32_t)P_LastAnchorGeomHashLo) |
        ((uint64_t)(uint32_t)P_LastAnchorGeomHashHi << 32);

    const bool anchorGeometryChanged = (curAnchorGeomHash != lastAnchorGeomHash);

    // Input-change tracking (store key values only)
    int& P_LastAnchorCount = sc.GetPersistentInt(3);
    int& P_LastMaxUpGroups = sc.GetPersistentInt(4);
    int& P_LastMaxDnGroups = sc.GetPersistentInt(5);
    int& P_LastOnlyRecent  = sc.GetPersistentInt(6);
    int& P_LastShowPrices  = sc.GetPersistentInt(7);
    int& P_LastMidStyle    = sc.GetPersistentInt(8);
    int& P_LastLblDec      = sc.GetPersistentInt(84);
    int& P_LastLblOff      = sc.GetPersistentInt(85);

    int& P_LastZoneLblMode = sc.GetPersistentInt(12);
    int& P_LastZoneLblHashCost = sc.GetPersistentInt(13);
    int& P_LastZoneLblHashUp   = sc.GetPersistentInt(14);
    int& P_LastZoneLblHashDn   = sc.GetPersistentInt(15);

    int& P_LastProcVisible = sc.GetPersistentInt(20);
    int& P_LastNiceN       = sc.GetPersistentInt(21);

    int& P_LastBasicBorder = sc.GetPersistentInt(22);
    int& P_LastBorderMode  = sc.GetPersistentInt(23);
    int& P_LastMidColorMode= sc.GetPersistentInt(24);
    int& P_LastMidWidth    = sc.GetPersistentInt(25);

    // Colors (cast to int)
    int& P_LastCustomBorderColor = sc.GetPersistentInt(26);
    int& P_LastCustomMidColor    = sc.GetPersistentInt(27);

    int& P_LastUpFill12   = sc.GetPersistentInt(28);
    int& P_LastUpFill36   = sc.GetPersistentInt(29);
    int& P_LastUpFill714  = sc.GetPersistentInt(30);
    int& P_LastUpFill1530 = sc.GetPersistentInt(31);

    int& P_LastDnFill12   = sc.GetPersistentInt(32);
    int& P_LastDnFill36   = sc.GetPersistentInt(33);
    int& P_LastDnFill714  = sc.GetPersistentInt(34);
    int& P_LastDnFill1530 = sc.GetPersistentInt(35);

    int& P_LastUpT12      = sc.GetPersistentInt(36);
    int& P_LastUpT36      = sc.GetPersistentInt(37);
    int& P_LastUpT714     = sc.GetPersistentInt(38);
    int& P_LastUpT1530    = sc.GetPersistentInt(39);

    int& P_LastDnT12      = sc.GetPersistentInt(40);
    int& P_LastDnT36      = sc.GetPersistentInt(41);
    int& P_LastDnT714     = sc.GetPersistentInt(42);
    int& P_LastDnT1530    = sc.GetPersistentInt(43);
    
    // Track auto-extend setting
    int& P_LastAutoExtend = sc.GetPersistentInt(46);
    
    // Fix for label redraw on scroll: Track visible index
    int& P_LastVisibleIndex = sc.GetPersistentInt(44);
    
    // Track ArraySize to trigger redraw when new bars arrive (for auto-extend)
    int& P_LastArraySize = sc.GetPersistentInt(45);
    
    // Track ID of the anchor currently being auto-extended (to ensure cleanup when it stops being latest)
    int& P_LastExtendedAnchor = sc.GetPersistentInt(56);

    int& P_JustLoaded = sc.GetPersistentInt(50);
    const bool isFirstRunAfterLoad = (P_JustLoaded == 0);
    if (isFirstRunAfterLoad) P_JustLoaded = 1;

    // Auto-Extend Logic (Mode: Disabled, Enabled, or Button)
    const int mode  = sc.Input[IN_AUTO_EXTEND_MODE].GetIndex();
    const int btnID = sc.Input[IN_AUTO_EXTEND_BUTTON_ID].GetInt();
    bool autoExtendLatest = false;

    // Persistent state for button toggle: 1=ON, 0=OFF, -1=Uninitialized
    int& P_AutoExtendState = sc.GetPersistentInt(55);

    switch (mode)
    {
        case 0: // Disabled
            autoExtendLatest = false;
            P_AutoExtendState = 0;
            if (btnID > 0)
            {
                 sc.SetCustomStudyControlBarButtonText(btnID, "BZ Ext: Disabled");
            }
            break;

        case 1: // Always Enabled
            autoExtendLatest = true;
            P_AutoExtendState = 1;
            if (btnID > 0)
            {
                 sc.SetCustomStudyControlBarButtonText(btnID, "BZ Ext: Always On");
            }
            break;

        case 2: // Control Bar Button
            if (btnID > 0)
            {
                // Init
                if (P_AutoExtendState == -1 || isFirstRunAfterLoad)
                {
                    P_AutoExtendState = 1; // Default ON
                    sc.SetCustomStudyControlBarButtonText(btnID, P_AutoExtendState ? "BZ Ext: ON" : "BZ Ext: OFF");
                    sc.SetCustomStudyControlBarButtonEnable(btnID, 1);
                }

                // Handle Click
                if (sc.MenuEventID == btnID)
                {
                    P_AutoExtendState = !P_AutoExtendState;
                    sc.SetCustomStudyControlBarButtonText(btnID, P_AutoExtendState ? "BZ Ext: ON" : "BZ Ext: OFF");
                    // Inputs change detection below will trigger redraw automatically
                }
                
                autoExtendLatest = (P_AutoExtendState == 1);
            }
            else
            {
                // Mode is Button but No Button ID -> Default to Disabled
                autoExtendLatest = false; 
            }
            break;
    }

    // Refresh Button: Force recalc when clicked (useful after hiding/showing drawings)
    const int refreshBtnID = sc.Input[IN_REFRESH_BUTTON_ID].GetInt();
    if (refreshBtnID > 0)
    {
        // Initialize button
        if (isFirstRunAfterLoad)
        {
            sc.SetCustomStudyControlBarButtonText(refreshBtnID, "BZ Refresh");
            sc.SetCustomStudyControlBarButtonEnable(refreshBtnID, 1);
        }

        // Handle click: Force redraw by setting the Force Redraw Now flag
        if (sc.MenuEventID == refreshBtnID)
        {
            sc.Input[IN_FORCE_REDRAW_NOW].SetYesNo(1); // Trigger recalc
        }
    }

    const int curAnchorCount = (int)currentFoundAnchorLines.size();

    const int curBasicBorder = sc.Input[IN_BASIC_DRAW_BORDER].GetYesNo() ? 1 : 0;
    const int curBorderMode  = sc.Input[IN_BORDER_COLOR_MODE].GetInt();
    const int curMidMode     = sc.Input[IN_MIDLINE_COLOR_MODE].GetInt();

    const int curCustomBorder = (int)sc.Input[IN_CUSTOM_BORDER_COLOR].GetColor();
    const int curCustomMid    = (int)sc.Input[IN_CUSTOM_MIDLINE_COLOR].GetColor();

    const int upFill12   = (int)sc.Input[IN_UP_FILL_12].GetColor();
    const int upFill36   = (int)sc.Input[IN_UP_FILL_36].GetColor();
    const int upFill714  = (int)sc.Input[IN_UP_FILL_714].GetColor();
    const int upFill1530 = (int)sc.Input[IN_UP_FILL_1530].GetColor();

    const int dnFill12   = (int)sc.Input[IN_DN_FILL_12].GetColor();
    const int dnFill36   = (int)sc.Input[IN_DN_FILL_36].GetColor();
    const int dnFill714  = (int)sc.Input[IN_DN_FILL_714].GetColor();
    const int dnFill1530 = (int)sc.Input[IN_DN_FILL_1530].GetColor();

    const int upT12   = ClampInt(sc.Input[IN_UP_TRANS_12].GetInt(), 0, 100);
    const int upT36   = ClampInt(sc.Input[IN_UP_TRANS_36].GetInt(), 0, 100);
    const int upT714  = ClampInt(sc.Input[IN_UP_TRANS_714].GetInt(), 0, 100);
    const int upT1530 = ClampInt(sc.Input[IN_UP_TRANS_1530].GetInt(), 0, 100);

    const int dnT12   = ClampInt(sc.Input[IN_DN_TRANS_12].GetInt(), 0, 100);
    const int dnT36   = ClampInt(sc.Input[IN_DN_TRANS_36].GetInt(), 0, 100);
    const int dnT714  = ClampInt(sc.Input[IN_DN_TRANS_714].GetInt(), 0, 100);
    const int dnT1530 = ClampInt(sc.Input[IN_DN_TRANS_1530].GetInt(), 0, 100);

    const bool inputsChanged =
        (P_LastMaxUpGroups != maxUpGroups) ||
        (P_LastMaxDnGroups != maxDownGroups) ||
        (P_LastOnlyRecent  != (onlyMostRecent ? 1 : 0)) ||
        (P_LastShowPrices  != (showRectPrices ? 1 : 0)) ||
        (P_LastMidStyle    != midlineStyleSetting) ||
        (P_LastMidWidth    != midlineWidth) ||
        (P_LastMidColorMode!= curMidMode) ||
        (P_LastBorderMode  != curBorderMode) ||
        (P_LastBasicBorder != curBasicBorder) ||
        (P_LastCustomBorderColor != curCustomBorder) ||
        (P_LastCustomMidColor    != curCustomMid) ||
        (P_LastLblDec      != userDecimals) ||
        (P_LastLblOff      != labelBarOff) ||
        (P_LastZoneLblMode != zoneNameLabelMode) ||
        (P_LastZoneLblHashCost != zoneLblHashCost) ||
        (P_LastZoneLblHashUp   != zoneLblHashUp) ||
        (P_LastZoneLblHashDn   != zoneLblHashDn) ||
        (P_LastProcVisible != (processVisibleOnlySetting ? 1 : 0)) ||
        (P_LastNiceN       != niceN) ||
        (P_LastUpFill12    != upFill12) ||
        (P_LastUpFill36    != upFill36) ||
        (P_LastUpFill714   != upFill714) ||
        (P_LastUpFill1530  != upFill1530) ||
        (P_LastDnFill12    != dnFill12) ||
        (P_LastDnFill36    != dnFill36) ||
        (P_LastDnFill714   != dnFill714) ||
        (P_LastDnFill1530  != dnFill1530) ||
        (P_LastUpT12       != upT12) ||
        (P_LastUpT36       != upT36) ||
        (P_LastUpT714      != upT714) ||
        (P_LastUpT1530     != upT1530) ||
        (P_LastDnT12       != dnT12) ||
        (P_LastDnT36       != dnT36) ||
        (P_LastDnT714      != dnT714) ||
        (P_LastDnT1530     != dnT1530) ||
        (P_LastAutoExtend  != (autoExtendLatest ? 1 : 0));  // Track auto-extend changes

    // NOTE: Zone hash cache optimization was disabled due to causing drawing issues.
    // The cache clear code is removed but inputsChanged is still used for redraw logic.

    const bool anchorsChanged = (P_LastAnchorCount != curAnchorCount);
    const bool isFullRecalc   = (sc.IsFullRecalculation != 0);

    // Check for scroll/zoom changes
    const int curVisibleIndex = sc.IndexOfFirstVisibleBar;
    const bool visibleChanged = (P_LastVisibleIndex != curVisibleIndex);
    P_LastVisibleIndex = curVisibleIndex;
    
    // Detect if new bars have arrived (important for auto-extend feature)
    const int currentArraySize = sc.ArraySize;
    const bool arraySizeChanged = (P_LastArraySize != currentArraySize);
    P_LastArraySize = currentArraySize;

    bool shouldRedraw = false;

    if (isFirstRunAfterLoad) shouldRedraw = true;
    if (isFullRecalc)        shouldRedraw = true;

    if (!shouldRedraw)
    {
        if (autoRedraw)
        {
            if (forceRedrawNow || inputsChanged || anchorsChanged || anchorGeometryChanged)
                shouldRedraw = true;
            
            // Fix: If we are scrolling and using features that depend on visibility
            // (Zone Name Labels or Process Visible Only), we MUST redraw.
            if (!shouldRedraw && visibleChanged)
            {
                const bool usingZoneLabels = (zoneNameLabelMode != 0); // 1=Nice, 2=All
                
                // If using Process Visible Only, we must redraw when view changes to find new anchors.
                // If using Zone Labels, we must redraw to update label positions relative to left edge.
                if (usingZoneLabels || processVisibleOnlySetting)
                {
                    shouldRedraw = true;
                }
            }
            
            // Fix for auto-extend: If ArraySize changed (new bars arrived) and auto-extend is enabled,
            // we must redraw to extend the zones to the new bars
            if (!shouldRedraw && arraySizeChanged && autoExtendLatest)
            {
                shouldRedraw = true;
            }
        }
        else
        {
            if (forceRedrawNow && !prevForce)
                shouldRedraw = true;
        }
    }

    PrevForceRedrawState = forceRedrawNow ? 1 : 0;
    
    // Update persistent state tracking for auto-extend
    P_LastAutoExtend = autoExtendLatest ? 1 : 0;

    // If projections disappeared after rebuild, force redraw
    // (Legacy logic removed)

    // Unified MakeBase function (used throughout)
    auto MakeBase = [&](int anchorLine) -> int
    {
        // Stable base derived from anchor LineNumber to avoid duplicates across DLL rebuilds
        // and to allow UTAM_ADD_OR_ADJUST to update existing projections.
        const unsigned u = (unsigned)anchorLine;
        const unsigned masked = (u & 0x00FFFFFFu);
        // Use uint64_t to prevent overflow
        const uint64_t product = (uint64_t)masked * (uint64_t)LINE_NUMBER_MULTIPLIER;
        int base = (int)(product & 0x7FFFFFFFu); // keep positive
        if (base < LINE_NUMBER_MIN_BASE) base += LINE_NUMBER_MIN_BASE;
        return base;
    };

    const int ZN_TEXT_OFFSET = 9000;
    const int ANCHOR_MID_EXT_OFFSET = 9500;  // Anchor native midline extension

    auto Ln = [&](int base, int block, int m) -> int { return base + block + m; };

    // ------------------------------------------------------------------
    // Removed redundant projectionsMissing logic to prevent infinite loop.
    // ------------------------------------------------------------------

    // Force redraw once after code upgrade
    int& P_CodeVersion = sc.GetPersistentInt(98);

    if (P_CodeVersion != BZE_BUILD)
    {
        shouldRedraw = true;
        P_CodeVersion = BZE_BUILD;
    }

    if (!shouldRedraw)
        return;

    // Persist state
    P_LastAnchorCount = curAnchorCount;
    P_LastMaxUpGroups = maxUpGroups;
    P_LastMaxDnGroups = maxDownGroups;
    P_LastOnlyRecent  = onlyMostRecent ? 1 : 0;
    P_LastShowPrices  = showRectPrices ? 1 : 0;
    P_LastMidStyle    = midlineStyleSetting;
    P_LastMidWidth    = midlineWidth;
    P_LastBorderMode  = curBorderMode;
    P_LastBasicBorder = curBasicBorder;
    P_LastMidColorMode= curMidMode;

    P_LastCustomBorderColor = curCustomBorder;
    P_LastCustomMidColor    = curCustomMid;

    P_LastLblDec      = userDecimals;
    P_LastLblOff      = labelBarOff;

    P_LastZoneLblMode = zoneNameLabelMode;
    P_LastZoneLblHashCost = zoneLblHashCost;
    P_LastZoneLblHashUp   = zoneLblHashUp;
    P_LastZoneLblHashDn   = zoneLblHashDn;

    P_LastProcVisible = processVisibleOnlySetting ? 1 : 0;
    P_LastNiceN       = niceN;

    P_LastUpFill12    = upFill12;
    P_LastUpFill36    = upFill36;
    P_LastUpFill714   = upFill714;
    P_LastUpFill1530  = upFill1530;

    P_LastDnFill12    = dnFill12;
    P_LastDnFill36    = dnFill36;
    P_LastDnFill714   = dnFill714;
    P_LastDnFill1530  = dnFill1530;

    P_LastUpT12       = upT12;
    P_LastUpT36       = upT36;
    P_LastUpT714      = upT714;
    P_LastUpT1530     = upT1530;

    P_LastDnT12       = dnT12;
    P_LastDnT36       = dnT36;
    P_LastDnT714      = dnT714;

    P_LastDnT1530     = dnT1530;

    P_LastVisibleIndex = sc.IndexOfFirstVisibleBar;

    // Persist anchor hash
    P_LastAnchorGeomHashLo = (int)(uint32_t)(curAnchorGeomHash & 0xFFFFFFFFULL);
    P_LastAnchorGeomHashHi = (int)(uint32_t)((curAnchorGeomHash >> 32) & 0xFFFFFFFFULL);

    // Auto-reset manual trigger (only in manual mode)
    if (!autoRedraw && forceRedrawNow)
        sc.Input[IN_FORCE_REDRAW_NOW].SetYesNo(0);

    // ------------------------------------------------------------------
    // DROP-IN PATCH: If projections disappeared (common after DLL rebuild),
    // force redraw even if inputs didn't change.
    // ------------------------------------------------------------------

    auto* pTracker = static_cast<AnchorIDTracker*>(sc.GetPersistentPointer(104));
    if (!pTracker) { pTracker = new AnchorIDTracker; sc.SetPersistentPointer(104, pTracker); }

    auto RegisterDrawingID = [&](int anchorLine, int drawingID) { pTracker->activeMap[anchorLine].push_back(drawingID); };
    auto ExecuteSmartCleanup = [&](int anchorLine) {
        auto it = pTracker->activeMap.find(anchorLine);
        if (it != pTracker->activeMap.end()) {
            for (int id : it->second) {
                sc.DeleteACSChartDrawing(sc.ChartNumber, TOOL_DELETE_CHARTDRAWING, id);
            }
            it->second.clear();
        }
    };

    auto DeleteLn = [&](int lineNumber) { /* Legacy cleanup wrapper, safely ignored by smart cleanup */ };
    auto DeleteAllForAnchor = [&](int anchorLine) { ExecuteSmartCleanup(anchorLine); };

    // Persist previous anchor list for cleanup
    auto* prevAnchors = static_cast<std::vector<int>*>(sc.GetPersistentPointer(101));
    if (prevAnchors == nullptr)
    {
        prevAnchors = new std::vector<int>();
        sc.SetPersistentPointer(101, prevAnchors);
    }

    auto ClearAlertSubgraphs = [&]()
    {
        int li = sc.ArraySize > 0 ? sc.ArraySize - 1 : 0;
        SG_ZoneIndex[li]   = 0.0f;
        SG_ZoneChange[li]  = 0.0f;
        SG_ZoneEntered[li] = 0.0f;
        SG_ZoneDist[li]    = 0.0f;
    };

    if (hideStudy)
    {
        // When the study is hidden (sc.HideStudy != 0), always remove
        // all projections/extensions created by this study, regardless
        // of the "Clear Old Projections" setting.
        for (int prevLine : *prevAnchors)
            DeleteAllForAnchor(prevLine);

        // Delete HUD labels (Stage 3)
        for (int i = 0; i < 3; ++i) DeleteLn(2000000 + i);

        prevAnchors->clear();
        ClearAlertSubgraphs();
        return;
    }

    if (anchors.empty())
    {
        if (clearOld)
        {
            for (int prevLine : *prevAnchors) DeleteAllForAnchor(prevLine);
            prevAnchors->clear();
        }

        // Delete HUD labels (Stage 3)
        for (int i = 0; i < 3; ++i) DeleteLn(2000000 + i);

        ClearAlertSubgraphs();
        return;
    }

    // Delete projections for anchors removed
    if (clearOld)
    {
        for (int prevLine : *prevAnchors)
        {
            if (std::find(currentFoundAnchorLines.begin(), currentFoundAnchorLines.end(), prevLine) == currentFoundAnchorLines.end())
                DeleteAllForAnchor(prevLine);
        }
    }
    *prevAnchors = currentFoundAnchorLines;

    auto ResolveLabelIndexFromRightEdge = [&](int rightIndex) -> int
    {
        int idx = rightIndex + labelBarOff;
        if (idx < 0) idx = 0;
        // Clamp to reasonable upper bound to prevent out-of-bounds placement
        const int maxIndex = sc.ArraySize > 0 ? sc.ArraySize + 5000 : rightIndex + 5000;
        if (idx > maxIndex) idx = maxIndex;
        return idx;
    };

    // --------- Determine nice anchors globally (by rightIndex, tie by beginIndex) ----------
    struct AnchorMeta
    {
        int line = 0;
        int left = 0;
        int right = 0;
        int beginIdx = 0;
        int region = 0;
    };

    std::vector<AnchorMeta> metas;
    metas.reserve(anchors.size());
    for (const auto& a : anchors)
    {
        const int leftIndex  = (a.BeginIndex < a.EndIndex) ? a.BeginIndex : a.EndIndex;
        const int rightIndex = (a.BeginIndex < a.EndIndex) ? a.EndIndex   : a.BeginIndex;

        AnchorMeta m;
        m.line = a.LineNumber;
        m.left = leftIndex;
        m.right = rightIndex;
        m.beginIdx = a.BeginIndex;
        m.region = a.Region;
        metas.push_back(m);
    }

    std::sort(metas.begin(), metas.end(), [](const AnchorMeta& A, const AnchorMeta& B)
    {
        if (A.right != B.right) return A.right > B.right;
        if (A.beginIdx != B.beginIdx) return A.beginIdx > B.beginIdx;
        return A.line > B.line;
    });

    std::unordered_set<int> niceLines;
    if (niceN > 0 && !metas.empty())
    {
        int ranksTaken = 0;
        int lastRight = 0;
        int lastBegin = 0;
        bool haveLast = false;

        for (const auto& m : metas)
        {
            const bool isNewRank = (!haveLast) || (m.right != lastRight) || (m.beginIdx != lastBegin);

            if (isNewRank)
            {
                if (ranksTaken >= niceN)
                    break;

                ranksTaken++;
                lastRight = m.right;
                lastBegin = m.beginIdx;
                haveLast = true;
            }

            niceLines.insert(m.line);
        }
    }

    // Latest anchor for Only Most Recent + Alerts
    const AnchorData* latestAnchorPtr = nullptr;
    {
        if (!metas.empty())
        {
            const int latestLine = metas[0].line;
            for (const auto& a : anchors)
            {
                if (a.LineNumber == latestLine)
                {
                    latestAnchorPtr = &a;
                    break;
                }
            }
        }
        if (latestAnchorPtr == nullptr)
            latestAnchorPtr = &anchors.back();
    }
    

    // Determine which anchors we will process
    const bool processVisibleOnly = processVisibleOnlySetting && !onlyMostRecent && !forceRedrawNow;

    std::vector<const AnchorData*> anchorsToProcess;
    anchorsToProcess.reserve(anchors.size());

    if (onlyMostRecent)
    {
        anchorsToProcess.push_back(latestAnchorPtr);

        if (clearOld)
        {
            for (const auto& a : anchors)
                if (a.LineNumber != latestAnchorPtr->LineNumber)
                    DeleteAllForAnchor(a.LineNumber);
        }
    }
    else if (processVisibleOnly)
    {
        for (const auto& a : anchors)
        {
            const int leftIndex  = (a.BeginIndex < a.EndIndex) ? a.BeginIndex : a.EndIndex;
            const int rightIndex = (a.BeginIndex < a.EndIndex) ? a.EndIndex   : a.BeginIndex;

            // Fix: ALWAYS process the latest anchor even if it's off-screen.
            // 1. If auto-extend is ON, we need to draw the extension.
            // 2. If auto-extend is OFF, we need to process it to DELETE any existing extension.
            const bool isLatest = (latestAnchorPtr != nullptr && a.LineNumber == latestAnchorPtr->LineNumber);
            
            // Fix: ALWAYS process:
            // 1. The latest anchor (to draw extension)
            // 2. The *previously* extended anchor (to delete extension if it's no longer latest)
            if (AnchorOverlapsVisible(leftIndex, rightIndex) || isLatest || a.LineNumber == P_LastExtendedAnchor)
                anchorsToProcess.push_back(&a);
        }
    }
    else
    {
        for (const auto& a : anchors)
            anchorsToProcess.push_back(&a);
    }

    if (anchorsToProcess.empty())
    {
        ClearAlertSubgraphs();
        return;
    }

    // --- Stage 3: Range for Smart Stacking ---
    double latestAnchorTop = 0, latestAnchorBot = 0;
    if (latestAnchorPtr) {
        latestAnchorTop = std::max(latestAnchorPtr->BeginValue, latestAnchorPtr->EndValue);
        latestAnchorBot = std::min(latestAnchorPtr->BeginValue, latestAnchorPtr->EndValue);
    }

    auto ApplyOverrideToMax = [&](bool has, bool isX, int v, int fallbackMax) -> int
    {
        if (!has) return fallbackMax;
        v = ClampInt(v, 0, 30);
        if (!isX) return v;
        return MapXToMaxMultiplier(v);
    };

    const bool allNice = (niceN == 0);

    // Accept pre-parsed overrides to avoid 3x parsing per anchor
    auto ProjectFromAnchor = [&](const AnchorData& src, bool isNice, bool isLatest, const AnchorTextOverrides& ov)
    {
        // Defensive validation: Sierra Chart can sometimes return stale/corrupted indices
        // for user drawings, especially after chart changes or when drawings extend beyond
        // loaded bars. Clamp to valid bounds to prevent unexpected behavior.
        const int maxValidIndex = std::max(0, sc.ArraySize - 1);
        
        int rawLeftIndex  = (src.BeginIndex < src.EndIndex) ? src.BeginIndex : src.EndIndex;
        int rawRightIndex = (src.BeginIndex < src.EndIndex) ? src.EndIndex   : src.BeginIndex;
        
#ifdef BZE_DEBUG_LOGGING
        // Log if we detect suspicious indices (for debugging rare edge cases)
        static std::unordered_set<int> s_loggedBadIndexAnchors;
        const bool suspiciousIndices = (rawLeftIndex < -1000 || rawRightIndex < -1000 || 
                                        rawLeftIndex > maxValidIndex + 10000);
        if (suspiciousIndices && s_loggedBadIndexAnchors.find(src.LineNumber) == s_loggedBadIndexAnchors.end())
        {
            s_loggedBadIndexAnchors.insert(src.LineNumber);
            SCString msg;
            msg.Format("BZ: Detected suspicious anchor indices. AnchorLine=%d, BeginIndex=%d, EndIndex=%d, ArraySize=%d. "
                       "Try deleting and re-adding the anchor if zones display incorrectly.",
                       src.LineNumber, src.BeginIndex, src.EndIndex, sc.ArraySize);
            sc.AddMessageToLog(msg, 1);
        }
#endif
        
        // Clamp indices to prevent extreme values from causing issues
        const int leftIndex  = std::max(-1000, rawLeftIndex);  // Allow some negative for future bars
        const int rightIndex = std::max(leftIndex, rawRightIndex);

		// Chart-history safety:
		// If the anchor's left edge has aged out of the chart's loaded bars, Sierra may still enumerate
		// the drawing but the aged-out edge can cause derived rectangles to silently fail to draw.
		// We clamp the derived drawings' left edge to the earliest loaded bar.
		//
		// Right-edge alignment:
		// For most anchors, align projections with the anchor's right edge.
		// For the latest anchor with auto-extend enabled, extend to current bar + 5 for visual breathing room.
		const int earliestLoadedIndex = 0;
		const int latestLoadedIndex   = std::max(0, sc.ArraySize - 1);

		int drawLeftIndex  = leftIndex;
		int drawRightIndex = rightIndex;  // Default: use anchor's right edge

		// Auto-extend: if enabled and this is the latest anchor, extend projections to current bar + 5
		if (autoExtendLatest && isLatest)
		{
			const int extendedRight = latestLoadedIndex + 5;  // 5 bars ahead for breathing room
			drawRightIndex = std::max(rightIndex, extendedRight);
		}


		bool isTrimmedByChartHistory = false;
		const SCDateTime earliestLoadedDT = sc.BaseDateTimeIn[earliestLoadedIndex];
		const SCDateTime aLeftDT = (src.BeginDateTime < src.EndDateTime) ? src.BeginDateTime : src.EndDateTime;
		// Fix: Only compare datetimes if both are valid (non-zero). When aLeftDT is 0/uninitialized
		// (can happen for rightmost anchors extending beyond loaded bars), skip the datetime check
		// to avoid incorrectly resetting drawLeftIndex to 0.
		const bool bothDateTimesValid = (aLeftDT.IsDateSet() && earliestLoadedDT.IsDateSet());
		if (drawLeftIndex < earliestLoadedIndex || (bothDateTimesValid && aLeftDT < earliestLoadedDT))
		{
			isTrimmedByChartHistory = true;
			drawLeftIndex = earliestLoadedIndex;
		}

		// Clamp left edge only (right edge always matches anchor)
		if (drawRightIndex < earliestLoadedIndex)
			return;

		// Replay safety: if anchor's left edge is in the future (beyond current bar),
		// don't draw zones yet - the anchor hasn't "started" in replay time.
		if (leftIndex > latestLoadedIndex)
			return;

		drawLeftIndex = ClampInt(drawLeftIndex, earliestLoadedIndex, drawRightIndex);

#ifdef BZE_DEBUG_LOGGING
		// Debug crumb (rate-limited per anchor line number).
		static std::unordered_set<int> s_loggedTrimmedAnchors;
		if (isTrimmedByChartHistory && s_loggedTrimmedAnchors.find(src.LineNumber) == s_loggedTrimmedAnchors.end())
		{
			s_loggedTrimmedAnchors.insert(src.LineNumber);
			SCString msg;
			msg.Format("BZ: Anchor left edge is before earliest loaded bar; trimming derived rectangles to chart start. AnchorLine=%d", src.LineNumber);
			sc.AddMessageToLog(msg, 0);
		}
#endif

        const int anchorLine = src.LineNumber;
        const int base       = MakeBase(anchorLine);

        // Use pre-parsed overrides (passed in from caller)

        int localMaxUp   = ApplyOverrideToMax(ov.HasUp,   ov.UpIsX,   ov.Up,   maxUp);
        int localMaxDown = ApplyOverrideToMax(ov.HasDown, ov.DownIsX, ov.Down, maxDown);

        localMaxUp   = ClampInt(localMaxUp,   0, 30);
        localMaxDown = ClampInt(localMaxDown, 0, 30);

        const bool localEnableUp   = enableUp   && localMaxUp > 0;
        const bool localEnableDown = enableDown && localMaxDown > 0;

        const double topRaw = std::max(src.BeginValue, src.EndValue);
        const double botRaw = std::min(src.BeginValue, src.EndValue);

        const double snappedTop = SnapToIncrement(topRaw);
        const double snappedBot = SnapToIncrement(botRaw);
        const double h = snappedTop - snappedBot;
        if (h <= 0.0) return;

        const bool midlineEnabledLocal   = isNice && midlineEnabledStudySetting;
        const bool showMidlinePriceLocal = isNice && showMidlinePrice;
        const bool showRectPricesLocal   = isNice && showRectPrices;

        auto DrawOne = [&](ZoneSide side, int m, double projTop, double projBot)
        {
            // For m=0 (anchor zone), use the anchor's own colors
            // For all other zones, use the standard zone color logic
            COLORREF fill;
            int transparency;
            COLORREF border;
            
            if (m == 0)
            {
                // Use anchor's colors for the extended anchor zone
                fill = src.SecondaryColor;  // Anchor fill color
                border = src.Color;          // Anchor border color
                transparency = src.TransparencyLevel;
                
                // If colors are unset, use a reasonable default
                if (fill == 0 && border == 0)
                {
                    const int group = 0;
                    fill = GetZoneFillColor(sc, side, group);
                    transparency = GetZoneTransparency(sc, side, group);
                    const bool wantBorder = WantBorderForAnchor(sc, isNice);
                    border = wantBorder ? ResolveBorderColor(sc, fill) : fill;
                }
            }
            else
            {
                // Standard zone color logic for m >= 1
                const int group = GroupFromMultiplier(m);
                fill = GetZoneFillColor(sc, side, group);
                transparency = GetZoneTransparency(sc, side, group);
                const bool wantBorder = WantBorderForAnchor(sc, isNice);
                border = wantBorder ? ResolveBorderColor(sc, fill) : fill;
            }

            // Stage 2: LOCK keyword visual boost (Relative boost)
            if (ov.Locked)
            {
                // Add 50% of the remaining transparency range to avoid full invisibility
                transparency = transparency + (int)((100 - transparency) * 0.50);
                transparency = ClampInt(transparency, 0, 100);
            }

            // Stage 3: Smart Stacking: +30% transparency penalty for historical zones overlapping latest anchor
            if (!isLatest && latestAnchorPtr != nullptr)
            {
                if (projBot < latestAnchorTop && projTop > latestAnchorBot)
                {
                    transparency = std::min(100, transparency + 30);
                }
            }

            const bool isUp = (side == ZoneSide::Up);

            int lnFill   = isUp ? Ln(base, UP_FILL_OFFSET, m)   : Ln(base, DN_FILL_OFFSET, m);
            int lnBorder = isUp ? Ln(base, UP_BORDER_OFFSET, m) : Ln(base, DN_BORDER_OFFSET, m);

            // NOTE: Zone hash skip optimization was DISABLED - caused missing zone draws.
            // Always call UseTool to ensure reliable drawing.
            
            // Fill rectangle (outline+fill in one tool)
            s_UseTool tFill;
            tFill.Clear();
            tFill.ChartNumber       = sc.ChartNumber;
            tFill.DrawingType       = DRAWING_RECTANGLEHIGHLIGHT;
            tFill.Region            = src.Region;
            // For m=0 (anchor zone extension), start at right edge to avoid overlap with user's anchor
            tFill.BeginIndex        = (m == 0) ? rightIndex : drawLeftIndex;
            tFill.EndIndex          = drawRightIndex;
            tFill.BeginValue        = projBot;
            tFill.EndValue          = projTop;
            tFill.LineWidth         = 1;
            tFill.LineStyle         = ov.Locked ? LINESTYLE_DOT : LINESTYLE_SOLID;

            // RECTANGLEHIGHLIGHT: Color = outline, SecondaryColor = fill
            tFill.Color             = border;
            tFill.SecondaryColor    = fill;

            tFill.TransparencyLevel = transparency;
            tFill.DrawOutlineOnly   = 0;
            tFill.LineNumber        = lnFill;
            tFill.AddMethod         = UTAM_ADD_OR_ADJUST;
            tFill.Text.Clear();
            if (sc.UseTool(tFill)) RegisterDrawingID(anchorLine, lnFill);

            // Delete any old separate border tool (legacy)
            DeleteLn(lnBorder);

            // Midline
            int lnMid   = isUp ? Ln(base, UP_MIDLINE_OFFSET, m) : Ln(base, DN_MIDLINE_OFFSET, m);
            int lnMidTx = isUp ? Ln(base, UP_MIDTEXT_OFFSET, m) : Ln(base, DN_MIDTEXT_OFFSET, m);

            if (midlineEnabledLocal)
            {
                const double mid = SnapToIncrement(0.5 * (projTop + projBot));
                const COLORREF midCol = ResolveMidlineColor(sc, fill);

                s_UseTool midLine;
                midLine.Clear();
                midLine.ChartNumber = sc.ChartNumber;
                midLine.DrawingType = DRAWING_LINE;
                midLine.Region      = src.Region;
                midLine.BeginIndex  = drawLeftIndex;
                midLine.EndIndex    = drawRightIndex;
                midLine.BeginValue  = mid;
                midLine.EndValue    = mid;
                midLine.LineWidth   = midlineWidth;
                midLine.LineStyle   = midLineStyleEnum;
                midLine.Color       = midCol;
                midLine.LineNumber  = lnMid;
                midLine.AddMethod   = UTAM_ADD_OR_ADJUST;
                midLine.Text.Clear();
                if (sc.UseTool(midLine)) RegisterDrawingID(anchorLine, lnMid);

                if (showMidlinePriceLocal && m != 0)  // Skip midline labels for m=0 (anchor zone)
                {
                    const int labelIndex = ResolveLabelIndexFromRightEdge(drawRightIndex);

                    DeleteLn(lnMidTx);

                    s_UseTool label;
                    label.Clear();
                    label.ChartNumber  = sc.ChartNumber;
                    label.DrawingType  = DRAWING_TEXT;
                    label.Region       = src.Region;
                    label.BeginIndex   = labelIndex;
                    label.BeginValue   = mid + labelOffsetPrice;
                    label.Color        = labelTextColor;
                    label.LineNumber   = lnMidTx;
                    label.AddMethod    = UTAM_ADD_OR_ADJUST;
                    label.Text         = FormatPrice(mid);
                    label.TransparentLabelBackground = 0;
                    label.FontBackColor = sc.ChartBackgroundColor;
                    if (sc.UseTool(label)) RegisterDrawingID(anchorLine, lnMidTx);
                }
                else
                {
                    DeleteLn(lnMidTx);
                }
            }
            else
            {
                DeleteLn(lnMid);
                DeleteLn(lnMidTx);
            }

            // Rectangle price labels (nice only)
            // Skip for m=0 since the user's anchor already has labels
            int lnTop = isUp ? Ln(base, UP_TOPTEXT_OFFSET, m) : Ln(base, DN_TOPTEXT_OFFSET, m);
            int lnBot = isUp ? Ln(base, UP_BOTTEXT_OFFSET, m) : Ln(base, DN_BOTTEXT_OFFSET, m);

            if (showRectPricesLocal && m != 0)  // Skip labels for m=0 (anchor zone)
            {
                const int labelIndex = ResolveLabelIndexFromRightEdge(drawRightIndex);

                DeleteLn(lnTop);
                DeleteLn(lnBot);

                // For up: always show top; show bottom only for m==1
                // For down: always show bottom; show top only for m==1
                if (side == ZoneSide::Up)
                {
                    s_UseTool topLabel;
                    topLabel.Clear();
                    topLabel.ChartNumber = sc.ChartNumber;
                    topLabel.DrawingType = DRAWING_TEXT;
                    topLabel.Region      = src.Region;
                    topLabel.BeginIndex  = labelIndex;
                    topLabel.BeginValue  = projTop + labelOffsetPrice;
                    topLabel.Color       = labelTextColor;
                    topLabel.LineNumber  = lnTop;
                    topLabel.AddMethod   = UTAM_ADD_OR_ADJUST;
                    topLabel.Text        = FormatPrice(projTop);
                    topLabel.TransparentLabelBackground = 0;
                    topLabel.FontBackColor = sc.ChartBackgroundColor;
                    if (sc.UseTool(topLabel)) RegisterDrawingID(anchorLine, lnTop);

                    if (m == 1)
                    {
                        s_UseTool botLabel;
                        botLabel.Clear();
                        botLabel.ChartNumber = sc.ChartNumber;
                        botLabel.DrawingType = DRAWING_TEXT;
                        botLabel.Region      = src.Region;
                        botLabel.BeginIndex  = labelIndex;
                        botLabel.BeginValue  = projBot - labelOffsetPrice;
                        botLabel.Color       = labelTextColor;
                        botLabel.LineNumber  = lnBot;
                        botLabel.AddMethod   = UTAM_ADD_OR_ADJUST;
                        botLabel.Text        = FormatPrice(projBot);
                        botLabel.TransparentLabelBackground = 0;
                        botLabel.FontBackColor = sc.ChartBackgroundColor;
                        if (sc.UseTool(botLabel)) RegisterDrawingID(anchorLine, lnBot);
                    }
                }
                else
                {
                    s_UseTool botLabel;
                    botLabel.Clear();
                    botLabel.ChartNumber = sc.ChartNumber;
                    botLabel.DrawingType = DRAWING_TEXT;
                    botLabel.Region      = src.Region;
                    botLabel.BeginIndex  = labelIndex;
                    botLabel.BeginValue  = projBot - labelOffsetPrice;
                    botLabel.Color       = labelTextColor;
                    botLabel.LineNumber  = lnBot;
                    botLabel.AddMethod   = UTAM_ADD_OR_ADJUST;
                    botLabel.Text        = FormatPrice(projBot);
                    botLabel.TransparentLabelBackground = 0;
                    botLabel.FontBackColor = sc.ChartBackgroundColor;
                    if (sc.UseTool(botLabel)) RegisterDrawingID(anchorLine, lnBot);

                    if (m == 1)
                    {
                        s_UseTool topLabel;
                        topLabel.Clear();
                        topLabel.ChartNumber = sc.ChartNumber;
                        topLabel.DrawingType = DRAWING_TEXT;
                        topLabel.Region      = src.Region;
                        topLabel.BeginIndex  = labelIndex;
                        topLabel.BeginValue  = projTop + labelOffsetPrice;
                        topLabel.Color       = labelTextColor;
                        topLabel.LineNumber  = lnTop;
                        topLabel.AddMethod   = UTAM_ADD_OR_ADJUST;
                        topLabel.Text        = FormatPrice(projTop);
                        topLabel.TransparentLabelBackground = 0;
                        topLabel.FontBackColor = sc.ChartBackgroundColor;
                        if (sc.UseTool(topLabel)) RegisterDrawingID(anchorLine, lnTop);
                    }
                }
            }
            else
            {
                DeleteLn(lnTop);
                DeleteLn(lnBot);
            }
        };

        // Draw the anchor zone itself (Cost Basis / m=0) when auto-extend is enabled
        // This allows the anchor zone to extend visually just like the other projections
        if (autoExtendLatest && isLatest)
        {
            // Draw the anchor zone using the same DrawOne pattern
            // Use m=0 for the anchor zone itself (Cost Basis)
            DrawOne(ZoneSide::Up, 0, snappedTop, snappedBot);

            // Extend the anchor's native midline if the user has it enabled on their rectangle
            const int lnAnchorMidExt = Ln(base, ANCHOR_MID_EXT_OFFSET, 0);
            if (src.DrawMidline != 0)
            {
                // Use raw anchor values (not snapped) to match Sierra Chart's native midline position
                const double anchorMid = 0.5 * (src.BeginValue + src.EndValue);

                s_UseTool midExt;
                midExt.Clear();
                midExt.ChartNumber = sc.ChartNumber;
                midExt.DrawingType = DRAWING_LINE;
                midExt.Region      = src.Region;
                midExt.BeginIndex  = rightIndex;
                midExt.EndIndex    = drawRightIndex;
                midExt.BeginValue  = anchorMid;
                midExt.EndValue    = anchorMid;
                midExt.LineWidth   = src.LineWidth;
                midExt.LineStyle   = LINESTYLE_SOLID;
                midExt.Color       = src.Color;  // Anchor outline color (same as native midline)
                midExt.LineNumber  = lnAnchorMidExt;
                midExt.AddMethod   = UTAM_ADD_OR_ADJUST;
                midExt.Text.Clear();
                if (sc.UseTool(midExt)) RegisterDrawingID(anchorLine, lnAnchorMidExt);
            }
            else
            {
                DeleteLn(lnAnchorMidExt);
            }
        }
        else
        {
            // Delete anchor zone drawing for non-latest or when auto-extend is off
            DeleteLn(Ln(base, UP_FILL_OFFSET, 0));
            DeleteLn(Ln(base, UP_BORDER_OFFSET, 0));
            DeleteLn(Ln(base, UP_MIDLINE_OFFSET, 0));
            DeleteLn(Ln(base, UP_MIDTEXT_OFFSET, 0));
            DeleteLn(Ln(base, UP_TOPTEXT_OFFSET, 0));
            DeleteLn(Ln(base, UP_BOTTEXT_OFFSET, 0));
            DeleteLn(Ln(base, ANCHOR_MID_EXT_OFFSET, 0));
        }

        // Up projections - Optimization #9: Only loop to actual localMaxUp instead of hardcoded 30
        const int upLoopMax = localEnableUp ? localMaxUp : 0;
        for (int m = 1; m <= upLoopMax; ++m)
        {
            const double offset  = h * (double)m;
            const double projTop = SnapToIncrement(snappedTop + offset);
            const double projBot = SnapToIncrement(snappedBot + offset);
            DrawOne(ZoneSide::Up, m, projTop, projBot);
        }
        
        // Clean up unused up zones (from localMaxUp+1 to 30)
        // Only do this if we previously had more zones (optimization: skip if upLoopMax is already at max)
        if (upLoopMax < 30)
        {
            for (int m = upLoopMax + 1; m <= 30; ++m)
            {
                DeleteLn(Ln(base, UP_FILL_OFFSET,    m));
                DeleteLn(Ln(base, UP_BORDER_OFFSET,  m));
                DeleteLn(Ln(base, UP_MIDLINE_OFFSET, m));
                DeleteLn(Ln(base, UP_MIDTEXT_OFFSET, m));
                DeleteLn(Ln(base, UP_TOPTEXT_OFFSET, m));
                DeleteLn(Ln(base, UP_BOTTEXT_OFFSET, m));
            }
        }

        // Down projections - Optimization #9: Only loop to actual localMaxDown instead of hardcoded 30
        const int downLoopMax = localEnableDown ? localMaxDown : 0;
        for (int m = 1; m <= downLoopMax; ++m)
        {
            const double offset  = h * (double)m;
            const double projTop = SnapToIncrement(snappedTop - offset);
            const double projBot = SnapToIncrement(snappedBot - offset);
            DrawOne(ZoneSide::Down, m, projTop, projBot);
        }
        
        // Clean up unused down zones (from localMaxDown+1 to 30)
        if (downLoopMax < 30)
        {
            for (int m = downLoopMax + 1; m <= 30; ++m)
            {
                DeleteLn(Ln(base, DN_FILL_OFFSET,    m));
                DeleteLn(Ln(base, DN_BORDER_OFFSET,  m));
                DeleteLn(Ln(base, DN_MIDLINE_OFFSET, m));
                DeleteLn(Ln(base, DN_MIDTEXT_OFFSET, m));
                DeleteLn(Ln(base, DN_TOPTEXT_OFFSET, m));
                DeleteLn(Ln(base, DN_BOTTEXT_OFFSET, m));
            }
        }
    };

    for (const AnchorData* pA : anchorsToProcess)
    {
        // NOTE: Per-anchor skip optimization (#8) was DISABLED due to causing inconsistent redraws.
        // The zone-level hashing inside DrawOne still provides optimization by skipping unchanged UseTool calls.
        // This ensures every anchor is always processed when a redraw is triggered.
        
        // Parse once per anchor (was previously parsed 3x: here, in ProjectFromAnchor, and in DrawZoneNameLabels)
        const AnchorTextOverrides ov = ParseAnchorTextOverrides(pA->Text);
        const bool isNice = allNice || ov.ForceDetailed || (niceLines.find(pA->LineNumber) != niceLines.end());
        const bool isLatest = (pA->LineNumber == latestAnchorPtr->LineNumber);
        ProjectFromAnchor(*pA, isNice, isLatest, ov);  // Pass pre-parsed overrides
    }

    // --------------------
    // Zone-name labels (latest anchor only)
    // --------------------
    auto ReplaceAll = [&](std::string s, const std::string& from, const std::string& to) -> std::string
    {
        if (from.empty()) return s;
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos)
        {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
        return s;
    };

    auto MakeZoneLabelText = [&](const SCString& tpl, int X) -> SCString
    {
        const char* c = tpl.GetChars();
        std::string s = c ? std::string(c) : std::string();
        s = ReplaceAll(s, "{X}", std::to_string(X));
        SCString out;
        out = s.c_str();
        return out;
    };

    auto DrawZoneNameLabelsForAnchor = [&](const AnchorData& src, bool isNice, const AnchorTextOverrides& ov) -> void
    {
        // Mode gate
        if (zoneNameLabelMode == 0)
            return;
        if (zoneNameLabelMode == 1 && !isNice)
            return;

        const int anchorLine = src.LineNumber;
        const int base       = MakeBase(anchorLine);

        // Optimized O(1) cleanup: Only delete labels for the previous owner, not all anchors
        // This uses persistent int to track who owned zone labels last frame
        int& P_PrevZoneLabelOwner = sc.GetPersistentInt(99);
        if (P_PrevZoneLabelOwner != 0 && P_PrevZoneLabelOwner != anchorLine)
        {
            const int prevBase = MakeBase(P_PrevZoneLabelOwner);
            for (int k = 1; k <= 9; ++k)
                DeleteLn(Ln(prevBase, ZN_TEXT_OFFSET, k));
        }
        P_PrevZoneLabelOwner = anchorLine;

        // Use pre-parsed overrides (passed in from caller)
        int localMaxUp   = ApplyOverrideToMax(ov.HasUp,   ov.UpIsX,   ov.Up,   maxUp);
        int localMaxDown = ApplyOverrideToMax(ov.HasDown, ov.DownIsX, ov.Down, maxDown);
        localMaxUp   = ClampInt(localMaxUp,   0, 30);
        localMaxDown = ClampInt(localMaxDown, 0, 30);

        const bool localEnableUp   = enableUp   && localMaxUp > 0;
        const bool localEnableDown = enableDown && localMaxDown > 0;

        const double topRaw = std::max(src.BeginValue, src.EndValue);
        const double botRaw = std::min(src.BeginValue, src.EndValue);
        const double snappedTop = SnapToIncrement(topRaw);
        const double snappedBot = SnapToIncrement(botRaw);
        const double h = snappedTop - snappedBot;
        // Use epsilon comparison for robustness
        if (h < FLOAT_EPSILON)
        {
            // nothing to label
            for (int k = 1; k <= 9; ++k) DeleteLn(Ln(base, ZN_TEXT_OFFSET, k));
            return;
        }

        // Left-margin list style:
        // - Stick to the far-left of the *visible* screen, plus "Label Bar Offset"
        // - But NEVER place labels earlier than the left-most edge of the anchor itself
        int visibleLeft = sc.IndexOfFirstVisibleBar;
        if (visibleLeft < 0) visibleLeft = 0;

        int anchorLeft = src.BeginIndex;
        if (src.EndIndex >= 0)
            anchorLeft = std::min(src.BeginIndex, src.EndIndex);
        if (anchorLeft < 0) anchorLeft = 0;

        // Replay safety: if anchor's left edge is in the future (beyond current bar),
        // don't show zone name labels yet - the anchor hasn't "started" in replay time.
        const int currentBarIndex = sc.ArraySize - 1;
        if (anchorLeft > currentBarIndex)
        {
            // Anchor is in the future; delete any existing labels and return
            for (int k = 1; k <= 9; ++k) DeleteLn(Ln(base, ZN_TEXT_OFFSET, k));
            return;
        }

        // Detect chart-history trimming (anchor left edge earlier than earliest loaded bar).
        // Used only for a small visual hint on the Cost Basis label.
        const SCDateTime earliestLoadedDT = sc.BaseDateTimeIn[0];
        const SCDateTime aLeftDT = (src.BeginDateTime < src.EndDateTime) ? src.BeginDateTime : src.EndDateTime;
        const bool isTrimmedByChartHistory = (aLeftDT < earliestLoadedDT);

        int xIndex = visibleLeft + zoneLabelBarOff;
        const int minAllowedX = anchorLeft + zoneLabelBarOff;
        if (xIndex < minAllowedX) xIndex = minAllowedX;
        if (xIndex < 0) xIndex = 0;
        if (xIndex > sc.ArraySize - 1) xIndex = sc.ArraySize - 1;

        auto DrawText = [&](int code, bool up, int X, const SCString& text) -> void
        {
            // Stage 2: LOCK keyword color shift
            const COLORREF labelColor = ov.Locked ? RGB(160, 160, 160) : zoneLabelFontColor;

            // Place label at the midpoint of a representative *rectangle* within the *zone band*.
            // Band definitions (by rectangle indices above/below the anchor):
            //   2x  = rects 1-2
            //   4x  = rects 3-6
            //   8x  = rects 7-14
            //   16x = rects 15-30
            // Representative rectangle is the middle of the band:
            //   2x  -> 1st rect in band
            //   4x  -> 2nd rect in band
            //   8x  -> 4th rect in band
            //   16x -> 8th rect in band
            // IMPORTANT: this is *relative to the start of the band*, not from the anchor.
            double mid = 0.0;
            if (code == 1)
            {
                mid = 0.5 * (snappedTop + snappedBot);
            }
            else
            {
                // Start rect for band X is (X - 1). (2->1, 4->3, 8->7, 16->15)
                // Middle rect within band is (X/2), 1-based.
                const int startRect = X - 1;
                const int rectInBand = X / 2;

                // 1-based absolute rectangle index from the anchor.
                // NOTE: for *upside* labels (Balance x2/x4/x8/x16), we intentionally shift the
                // representative rectangle up by +1 to keep the text sitting a bit higher inside
                // the band while still using the bottom-of-rectangle text behavior.
                int repRect = startRect + rectInBand - 1;
                if (up)
                {
                    // codes 2..5 are the Balance labels
                    const bool isBalanceLabel = (code >= 2 && code <= 5);
                    if (isBalanceLabel)
                    {
                        const int bandEndRect = 2 * X - 2; // 2->2, 4->6, 8->14, 16->30
                        repRect = std::min(repRect + 1, bandEndRect);
                    }
                }

                if (up)
                {
                    const double rLow  = snappedTop + h * (double)(repRect - 1);
                    const double rHigh = snappedTop + h * (double)(repRect);
                    mid = 0.5 * (rLow + rHigh);
                }
                else
                {
                    const double rHigh = snappedBot - h * (double)(repRect - 1);
                    const double rLow  = snappedBot - h * (double)(repRect);
                    mid = 0.5 * (rLow + rHigh);
                }
            }

            mid = SnapToIncrement(mid);

            s_UseTool t;
            t.Clear();
            t.ChartNumber = sc.ChartNumber;
            t.DrawingType = DRAWING_TEXT;
            t.Region      = src.Region;
            t.BeginIndex  = xIndex;
            t.BeginValue  = mid;
            // Stage 3: Color shift for locked labels
            t.Color       = ov.Locked ? RGB(160, 160, 160) : labelColor;
            t.FontSize    = zoneLabelFontSize;
            t.LineNumber  = Ln(base, ZN_TEXT_OFFSET, code);
            t.AddMethod   = UTAM_ADD_OR_ADJUST;
            t.Text        = text;
            t.TransparentLabelBackground = 1;
            if (sc.UseTool(t)) RegisterDrawingID(anchorLine, t.LineNumber);
        };

        // Cost Basis
        {
            SCString txt = zoneLabelCostBasis;
            if (ov.Locked)
            {
                SCString lockPrefix = "(!) LOCK ";
                lockPrefix += zoneLabelCostBasis;
                txt = lockPrefix;
            }

            if (isTrimmedByChartHistory && txt.GetLength() > 0)
                txt.Append(" (trimmed)");
            if (txt.GetLength() > 0)
                DrawText(1, true, 0, txt);
            else
                DeleteLn(Ln(base, ZN_TEXT_OFFSET, 1));
        }

        // Fixed labels: 2,4,8,16.
        struct Band { int code; int X; bool up; };
        const Band bands[] = {
            {2,  2, true},
            {3,  4, true},
            {4,  8, true},
            {5, 16, true},
            {6,  2, false},
            {7,  4, false},
            {8,  8, false},
            {9, 16, false},
        };

        for (const auto& b : bands)
        {
            if (b.up)
            {
                // To display the X label, the anchor must actually project through the end of the X band.
                // End rect for band X is (2X - 2): 2->2, 4->6, 8->14, 16->30.
                const int bandEndRect = 2 * b.X - 2;
                if (!localEnableUp || localMaxUp < bandEndRect)
                {
                    DeleteLn(Ln(base, ZN_TEXT_OFFSET, b.code));
                    continue;
                }
                const SCString txt = MakeZoneLabelText(zoneLabelUpTpl, b.X);
                if (txt.GetLength() > 0)
                    DrawText(b.code, true, b.X, txt);
                else
                    DeleteLn(Ln(base, ZN_TEXT_OFFSET, b.code));
            }
            else
            {
                const int bandEndRect = 2 * b.X - 2;
                if (!localEnableDown || localMaxDown < bandEndRect)
                {
                    DeleteLn(Ln(base, ZN_TEXT_OFFSET, b.code));
                    continue;
                }
                const SCString txt = MakeZoneLabelText(zoneLabelDownTpl, b.X);
                if (txt.GetLength() > 0)
                    DrawText(b.code, false, b.X, txt);
                else
                    DeleteLn(Ln(base, ZN_TEXT_OFFSET, b.code));
            }
        }
    };

    // Owner = latest anchor by right edge (same as alerts)
    {
        const int ownerLine = latestAnchorPtr ? latestAnchorPtr->LineNumber : 0;

        if (zoneNameLabelMode == 0)
        {
            // Delete any lingering zone-name labels for all anchors
            for (const auto& a : anchors)
            {
                const int b = MakeBase(a.LineNumber);
                for (int k = 1; k <= 9; ++k)
                    DeleteLn(Ln(b, ZN_TEXT_OFFSET, k));
            }
        }
        else if (latestAnchorPtr != nullptr)
        {
            // Reuse pre-parsed overrides for latest anchor when possible
            // (owner is always the latest anchor, so we parse once here)
            const AnchorTextOverrides ownerOv = ParseAnchorTextOverrides(latestAnchorPtr->Text);
            const bool ownerNice = allNice || ownerOv.ForceDetailed || (niceLines.find(ownerLine) != niceLines.end());
            DrawZoneNameLabelsForAnchor(*latestAnchorPtr, ownerNice, ownerOv);  // Pass pre-parsed overrides
        }
    }

    // --------------------
    // Alert subgraphs: Zone transitions (ALWAYS based on the most recent anchor)
    // --------------------
    
    // Validation: ensure we have valid array data
    if (sc.ArraySize <= 0)
    {
        ClearAlertSubgraphs();
        return;
    }
    
    int lastIndex = sc.ArraySize - 1;
    if (lastIndex < 0) lastIndex = 0;

    double price = 0.0;
    if (alertPriceSource == 1)
        price = sc.Close[lastIndex];
    else
    {
        price = sc.LastTradePrice;
        if (price <= 0.0)
            price = sc.Close[lastIndex];
    }

    const double snappedPrice = SnapToIncrement(price);
    const AnchorData& srcAnchor = *latestAnchorPtr;

    AnchorTextOverrides aov = ParseAnchorTextOverrides(srcAnchor.Text);

    int alertMaxUp   = ApplyOverrideToMax(aov.HasUp,   aov.UpIsX,   aov.Up,   maxUp);
    int alertMaxDown = ApplyOverrideToMax(aov.HasDown, aov.DownIsX, aov.Down, maxDown);

    alertMaxUp   = ClampInt(alertMaxUp,   0, 30);
    alertMaxDown = ClampInt(alertMaxDown, 0, 30);

    const bool alertEnableUp   = enableUp   && alertMaxUp > 0;
    const bool alertEnableDown = enableDown && alertMaxDown > 0;

    auto ComputeZoneAndDistance = [&](const AnchorData& a,
                                      int maxUpLocal, int maxDownLocal,
                                      bool enUpLocal, bool enDownLocal,
                                      int& outZone, double& outNearestBoundaryPrice) -> void
    {
        outZone = 0;
        outNearestBoundaryPrice = 0.0;

        const double topRaw = std::max(a.BeginValue, a.EndValue);
        const double botRaw = std::min(a.BeginValue, a.EndValue);

        const double snappedTop = SnapToIncrement(topRaw);
        const double snappedBot = SnapToIncrement(botRaw);
        const double h = snappedTop - snappedBot;
        if (h <= FLOAT_EPSILON) return;

        double bestAbsDiff = 1e100;
        double bestBoundary = 0.0;

        auto ConsiderBoundary = [&](double boundary)
        {
            const double diff = snappedPrice - boundary;
            const double ad = fabs(diff);
            if (ad < bestAbsDiff)
            {
                bestAbsDiff = ad;
                bestBoundary = boundary;
            }
        };

        // First check if price is in the anchor zone itself (zone 0)
        if (snappedPrice >= snappedBot && snappedPrice <= snappedTop)
        {
            outZone = 0; // Inside anchor zone
            ConsiderBoundary(snappedBot);
            ConsiderBoundary(snappedTop);
        }

        if (enUpLocal)
        {
            for (int m = 1; m <= maxUpLocal && m <= 30; ++m)
            {
                const double offset = h * (double)m;
                const double zb = SnapToIncrement(snappedBot + offset);
                const double zt = SnapToIncrement(snappedTop + offset);

                ConsiderBoundary(zb);
                ConsiderBoundary(zt);

                // Use half-open interval to avoid boundary overlap: [zb, zt)
                // Only override if we haven't found a zone yet
                if (outZone == 0 && snappedPrice >= zb && snappedPrice < zt)
                    outZone = +m;
            }
        }

        if (enDownLocal)
        {
            for (int m = 1; m <= maxDownLocal && m <= 30; ++m)
            {
                const double offset = h * (double)m;
                const double zb = SnapToIncrement(snappedBot - offset);
                const double zt = SnapToIncrement(snappedTop - offset);

                ConsiderBoundary(zb);
                ConsiderBoundary(zt);

                // Use half-open interval to avoid boundary overlap: [zb, zt)
                // Only override if we haven't found a zone yet
                if (outZone == 0 && snappedPrice >= zb && snappedPrice < zt)
                    outZone = -m;
            }
        }

        outNearestBoundaryPrice = (bestAbsDiff < 1e99) ? bestBoundary : 0.0;
    };

    int currentZone = 0;
    double nearestBoundary = 0.0;
    ComputeZoneAndDistance(srcAnchor, alertMaxUp, alertMaxDown, alertEnableUp, alertEnableDown,
                           currentZone, nearestBoundary);

    int& PrevZone = sc.GetPersistentInt(82);
    const int prevZone = PrevZone;

    // --- Stage 4: Alert System Robustness (Gap Detection) ---
    const bool useGapDetection = (sc.Input[IN_ALERT_USE_GAP_DETECTION].GetYesNo() != 0);
    const bool includeBarHL    = (sc.Input[IN_ALERT_INCLUDE_BAR_HL].GetYesNo() != 0);
    double& LastProcessedPrice = sc.GetPersistentDouble(80);
    int& LastProcessedBarIndex = sc.GetPersistentInt(81);

    bool boundaryCrossedInPath = false;
    if (useGapDetection && LastProcessedPrice > 0.0)
    {
        // Reset if we've scrolled back significantly
        if (lastIndex < LastProcessedBarIndex)
        {
            LastProcessedPrice = snappedPrice;
            LastProcessedBarIndex = lastIndex;
        }

        double pathLow, pathHigh;
        if (lastIndex > LastProcessedBarIndex)
        {
            // If we've skipped bars or moved to a new bar, the path includes the previous price, 
            // the current bar's H/L (if enabled), and the current price.
            pathLow  = (std::min)(snappedPrice, LastProcessedPrice);
            pathHigh = (std::max)(snappedPrice, LastProcessedPrice);

            if (includeBarHL)
            {
                pathLow  = (std::min)(pathLow,  (double)sc.Low[lastIndex]);
                pathHigh = (std::max)(pathHigh, (double)sc.High[lastIndex]);
            }
        }
        else
        {
            pathLow = (std::min)(snappedPrice, LastProcessedPrice);
            pathHigh = (std::max)(snappedPrice, LastProcessedPrice);
        }

        const double topRaw = (std::max)(srcAnchor.BeginValue, srcAnchor.EndValue);
        const double botRaw = (std::min)(srcAnchor.BeginValue, srcAnchor.EndValue);
        const double sTop = SnapToIncrement(topRaw);
        const double sBot = SnapToIncrement(botRaw);
        const double hVal = sTop - sBot;

        if (hVal > FLOAT_EPSILON)
        {
            // Check if any zone boundary is within the path [pathLow, pathHigh]
            for (int m = -alertMaxDown; m <= alertMaxUp + 1; ++m)
            {
                double b = SnapToIncrement(sBot + (double)m * hVal);
                
                if (b >= pathLow && b <= pathHigh) 
                { 
                    boundaryCrossedInPath = true; 
                    break; 
                }
            }
        }
    }

    SG_ZoneIndex[lastIndex]   = (float)currentZone;
    SG_ZoneChange[lastIndex]  = (prevZone != currentZone || boundaryCrossedInPath) ? 1.0f : 0.0f;
    SG_ZoneEntered[lastIndex] = (boundaryCrossedInPath || (prevZone == 0 && currentZone != 0)) ? 1.0f : 0.0f;

    double distPoints = 0.0;
    if (nearestBoundary != 0.0)
        distPoints = snappedPrice - nearestBoundary;

    double distTicks = 0.0;
    const double snapInc = snapIncDraw;
    if (snapInc > FLOAT_EPSILON)
        distTicks = distPoints / snapInc;
    else
        distTicks = 0.0;

    SG_ZoneDist[lastIndex] = (float)distTicks;
    
    PrevZone = currentZone;
    LastProcessedPrice = snappedPrice;
    LastProcessedBarIndex = lastIndex;

    // Update tracking for the next frame
    // This allows us to process the \"outgoing\" extended anchor even if it's off-screen
    if (autoExtendLatest && latestAnchorPtr != nullptr)
        P_LastExtendedAnchor = latestAnchorPtr->LineNumber;
    else
        P_LastExtendedAnchor = 0;
}
