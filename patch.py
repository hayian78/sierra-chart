import sys

filepath = '/home/ian/projects/sierra-chart/TimeBlockHighlighter.cpp'
with open(filepath, 'r') as f:
    content = f.read()

# 1. Update Input Name
old_input_name = 'TickIntervalMinutes.Name = "Tick Label Interval Minutes (0=auto: 120/60/30 based on bar period)";'
new_input_name = 'TickIntervalMinutes.Name = "Tick Label Interval Minutes (0=auto scale by zoom)";'
if old_input_name in content:
    content = content.replace(old_input_name, new_input_name)
else:
    print("Warning: Could not find old_input_name")

# 2. Update Hash Caching for Zoom Changes
old_hash = """        // Lazy Sticky Edge: REDRAW logic only executes on 30-bar quantized changes
        int quantizedFirstBar = sc.IndexOfFirstVisibleBar / 30;
        currentHash ^= quantizedFirstBar * 31;"""

new_hash = """        // Lazy Sticky Edge: REDRAW logic only executes on 30-bar quantized changes
        int quantizedFirstBar = sc.IndexOfFirstVisibleBar / 30;
        int quantizedLastBar = sc.IndexOfLastVisibleBar / 30;
        currentHash ^= quantizedFirstBar * 31;
        currentHash ^= quantizedLastBar * 37;"""
if old_hash in content:
    content = content.replace(old_hash, new_hash)
else:
    print("Warning: Could not find old_hash")

# 3. Update Tick Interval Logic
old_interval = """        // Choose interval
        if (tickEveryMin <= 0)
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
        }"""

new_interval = """        // Choose interval
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
        }"""
if old_interval in content:
    content = content.replace(old_interval, new_interval)
else:
    print("Warning: Could not find old_interval")

with open(filepath, 'w') as f:
    f.write(content)

print("Patch applied successfully!")
