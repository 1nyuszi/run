# Interval Running Timer and Training Calendar

A small local tool for interval running: a Windows console timer alternates running and recovery periods, records events in monthly log files, and a Python script generates an HTML calendar of completed running minutes.

## How it works

The timer starts with a 20-second preparation countdown, then repeats:

1. Run for 120 countdown seconds.
2. Recover for 60 countdown seconds.

The console displays the round number and a colored countdown. During the last five seconds of each countdown, the display turns red and a beep sounds. Press **Ctrl+C** to stop cleanly, save a `PROGRAM_END` event, and display completed rounds and running minutes. Only fully completed running intervals count; stopping during recovery keeps the preceding completed run. Interrupted runs produce no interval log entries or partial minutes. There is no pause control or preset round limit.

The calendar highlights training days in green, shows running minutes for each day, and displays total training days and running minutes. It includes every month from the earliest logged event through the current month, with weeks starting on Monday.

## Requirements

- **Timer:** Windows and a `g++` compiler on `PATH`, such as a MinGW toolchain. The source uses Windows console control, event, and `Beep` functions.
- **Console:** A terminal that supports ANSI cursor movement and colors.
- **Calendar:** Python 3.6 or newer and a browser. All Python dependencies are in the standard library; no packages need to be installed.

## Build and run

Run these commands in PowerShell from the project root:

```powershell
# Compile the timer.
g++ run.cpp -o run.exe

# Start a session.
.\run.exe
```

Alternatively, run `.\run.bat` to compile. The batch file pauses after compilation and does not start the timer. It actually compiles `run.cpp`.

Start the timer from the project root: log paths are relative to the current working directory. The logger creates `logs/` automatically. If creating the directory, opening the log file, writing, or closing fails, the timer prints an error and exits with a nonzero status so a session cannot continue without saving history.

## Generate the training calendar

After recording a session, run:

```powershell
python calendar/training_calendar.py
```

The script reads all `.log` files in the project's `logs` directory, overwrites `calendar/calendar.html`, and opens it in the default browser. You can also open that HTML file manually. It is a standalone page with embedded CSS and requires no server or internet connection.

The script resolves its input and output paths relative to its own location, so it can also be launched from another directory. Regenerate the page whenever you want to include new sessions; it does not update automatically.

If the directory exists but contains no valid timestamped entries, the script prints `No log entries found.` and leaves any existing HTML unchanged. A missing `logs` directory causes an error.

## Log format and counting rules

Events are appended to `logs/YYYY_M.log`, using the local year and an unpadded month (for example, `2026_1.log`). Each line contains a Unix timestamp in seconds and an event name:

```text
1700000000 PROGRAM_START
1700000020 RUN
1700000142 RELAX
```

- `PROGRAM_START` marks the start of the program, before preparation.
- `RUN` records the start timestamp of a completed running interval.
- `RELAX` records its finish timestamp, at the beginning of recovery. The `RUN` / `RELAX` pair is saved together only after the run finishes, in the log file for the run's starting month.
- `PROGRAM_END` marks a clean stop with Ctrl+C (or Ctrl+Break), including during preparation.

The calendar processes each file in line order. Each adjacent `RUN` / `RELAX` pair contributes **exactly two running minutes**, assigned to the local date of the `RUN` timestamp. Preparation and recovery are excluded. Interrupted runs are not saved; older logs may contain unmatched `RUN` entries, which are not counted. Pairs split across files are not counted. Lines with fewer than two fields or a non-integer timestamp are skipped.

These totals represent completed intervals, not measured elapsed time. In particular, each final countdown beep blocks for 300 ms before the one-second sleep, making the actual countdown slightly longer than its nominal duration.

## Project layout

| Path | Purpose |
| --- | --- |
| `run.cpp` | Console countdown, interval loop, colors, and sound cues. |
| `FileLogger.cpp` | Appends timestamped events to monthly log files; included directly by `run.cpp`. |
| `run.bat` | Windows build helper using `g++`. |
| `logs/` | Local training logs; `.log` files are ignored by Git. |
| `calendar/training_calendar.py` | Parses logs, aggregates running minutes, and renders the calendar. |
| `calendar/calendar.html` | Generated calendar snapshot, tracked in the repository. |

Compiled `.exe` files and Python cache files are also ignored by Git. The checked-in calendar is a snapshot; regenerate it from your own logs for current results.

## Changing interval lengths

Preparation and recovery durations are hardcoded in the `count_back` calls in `run.cpp`; running duration uses `RUN_SECONDS`. Edit those values and rebuild to change the timer. If you change the running duration, also update the fixed `minutes_per_day[day] += 2` calculation in `parse_logs` in `calendar/training_calendar.py` so the calendar reflects the new interval length. Neither tool currently accepts command-line configuration.

Run the graceful stopping tests with `python tests/test_stopping.py`. They compile the timer with simulated waits and Ctrl+C events and verify the summary, saved events, and calendar totals without changing your training logs.
