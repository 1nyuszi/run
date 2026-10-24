import os
import webbrowser
from datetime import datetime, date
from collections import defaultdict
from calendar import monthrange, month_name

LOGS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "../logs")
OUTPUT_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "calendar.html")


def parse_logs(logs_dir):
    minutes_per_day = defaultdict(int)
    all_timestamps = []

    for filename in sorted(os.listdir(logs_dir)):
        if not filename.endswith(".log"):
            continue

        with open(os.path.join(logs_dir, filename), "r") as f:
            entries = []
            for line in f:
                parts = line.strip().split()
                if len(parts) < 2:
                    continue
                try:
                    ts = int(parts[0])
                except ValueError:
                    continue
                entries.append((ts, parts[1]))

        for i in range(len(entries) - 1):
            ts, event = entries[i]
            _, next_event = entries[i + 1]
            if event == "RUN" and next_event == "RELAX":
                day = datetime.fromtimestamp(ts).date()
                minutes_per_day[day] += 2

        all_timestamps.extend(ts for ts, _ in entries)

    first_date = datetime.fromtimestamp(min(all_timestamps)).date() if all_timestamps else None
    return minutes_per_day, first_date


def iter_months(start, end):
    year, month = start.year, start.month
    while (year, month) <= (end.year, end.month):
        yield year, month
        if month == 12:
            year, month = year + 1, 1
        else:
            month += 1


def render_month(year, month, minutes_per_day):
    first_weekday, days_in_month = monthrange(year, month)

    cells = [None] * first_weekday + list(range(1, days_in_month + 1))
    while len(cells) % 7 != 0:
        cells.append(None)

    rows = []
    for w in range(0, len(cells), 7):
        tds = []
        for d in cells[w:w + 7]:
            if d is None:
                tds.append('<td class="empty"></td>')
            else:
                minutes = minutes_per_day.get(date(year, month, d), 0)
                cls = "trained" if minutes > 0 else "rest"
                inner = f'<div class="day-num">{d}</div>'
                if minutes > 0:
                    inner += f'<div class="minutes">{minutes} min</div>'
                tds.append(f'<td class="{cls}">{inner}</td>')
        rows.append("      <tr>" + "".join(tds) + "</tr>")

    weekdays = "".join(f"<th>{d}</th>" for d in ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"])
    return (
        '  <section class="month">\n'
        f'    <h2>{month_name[month]} {year}</h2>\n'
        '    <table>\n'
        f'      <thead><tr>{weekdays}</tr></thead>\n'
        '      <tbody>\n'
        + "\n".join(rows) + "\n"
        '      </tbody>\n'
        '    </table>\n'
        '  </section>'
    )


CSS = """
* { box-sizing: border-box; }
body {
  font-family: -apple-system, Segoe UI, Roboto, sans-serif;
  background: #f3f4f6;
  margin: 0;
  padding: 24px;
  color: #222;
}
h1 { text-align: center; margin: 0 0 24px; }
.totals { text-align: center; color: #555; margin-bottom: 24px; }
.month {
  background: #fff;
  border-radius: 8px;
  padding: 18px 22px 22px;
  margin: 0 auto 24px;
  max-width: 640px;
  box-shadow: 0 1px 3px rgba(0,0,0,0.08);
}
.month h2 { text-align: center; margin: 0 0 12px; font-weight: 600; }
table { width: 100%; border-collapse: collapse; table-layout: fixed; }
th {
  background: #eef0f3;
  padding: 8px 0;
  font-size: 0.85em;
  color: #555;
  font-weight: 600;
}
td {
  height: 68px;
  vertical-align: top;
  padding: 6px;
  border: 1px solid #e5e7eb;
  text-align: left;
  font-size: 0.9em;
}
td.empty { background: #fafafa; border: 1px solid #f0f0f0; }
td.rest { background: #fff; }
td.trained {
  background: #4caf50;
  color: #fff;
  border-color: #3e9c42;
}
.day-num { font-weight: 600; }
td.trained .day-num { color: #fff; }
.minutes {
  margin-top: 6px;
  font-size: 0.95em;
  font-weight: 600;
}
"""


def render_html(minutes_per_day, first_date, today):
    months_html = "\n".join(
        render_month(y, m, minutes_per_day) for y, m in iter_months(first_date, today)
    )

    total_minutes = sum(minutes_per_day.values())
    total_days = sum(1 for v in minutes_per_day.values() if v > 0)

    return (
        '<!DOCTYPE html>\n'
        '<html lang="en">\n'
        '<head>\n'
        '  <meta charset="utf-8">\n'
        '  <title>Training Calendar</title>\n'
        f'  <style>{CSS}</style>\n'
        '</head>\n'
        '<body>\n'
        '  <h1>Training Calendar</h1>\n'
        f'  <p class="totals">{total_days} training days &middot; {total_minutes} minutes total</p>\n'
        f'{months_html}\n'
        '</body>\n'
        '</html>\n'
    )


def main():
    minutes_per_day, first_date = parse_logs(LOGS_DIR)
    if first_date is None:
        print("No log entries found.")
        return

    today = date.today()
    html = render_html(minutes_per_day, first_date, today)

    with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
        f.write(html)

    print(f"Wrote {OUTPUT_FILE}")
    webbrowser.open("file:///" + OUTPUT_FILE.replace("\\", "/"))


if __name__ == "__main__":
    main()
