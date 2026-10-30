# Interval Running Timer and Training Calendar

A local Windows tool for interval running with a large-number GUI, a console timer, and training calendars. Both timers record the same monthly logs, so the built-in GUI calendar and the existing Python HTML calendar share your training history.

## How it works

The timer starts with a 20-second preparation countdown, then repeats:

1. Run for 120 countdown seconds.
2. Recover for 60 countdown seconds.

Only fully completed running intervals count; stopping during recovery keeps the preceding completed run. Interrupted runs produce no interval log entries or partial minutes. There is no pause control or preset round limit.

Both calendars highlight training days in green and use weeks starting on Monday. The GUI displays a navigable month; the HTML calendar includes every month from the earliest logged event through the current month.

## Requirements

- **Build:** Windows and a `g++` compiler on `PATH`, such as a MinGW toolchain. The GUI uses native Windows controls and needs no additional GUI libraries. Its compiler runtimes are linked statically so double-clicking `runui.exe` does not require MinGW runtime DLLs on `PATH`.
- **Console:** A terminal that supports ANSI cursor movement and colors.
- **HTML calendar only:** Python 3.6 or newer and a browser. All dependencies are in the standard library. The GUI and its built-in calendar do not require Python.

## Build and run

Run these commands in PowerShell from the project root:

```powershell
# Compile both timers.
.\run.bat

# Open the GUI, then click Start.
.\runui.exe

# Or start a console session.
.\run.exe
```

The batch file builds both executables, pauses after compilation, and does not launch either timer. Close an executable before rebuilding it. Launch `runui.exe` to use the GUI; `runui.cpp` is its source code.

To compile manually:

```powershell
g++ run.cpp -o run.exe
g++ -std=c++11 -Wall -Wextra runui.cpp -o runui.exe -mwindows -static -lgdi32 -luser32
```

The GUI writes logs beside `runui.exe`, even when a shortcut uses a different working directory. Keep it in the project folder to share history with the Python calendar. Start the console timer from the project root: its log paths are relative to the current working directory.

The shared logger creates `logs/` automatically. If creating the directory, opening the log file, writing, or closing fails, the GUI stops the session and displays an error dialog; the console prints an error and exits with a nonzero status.

## GUI timer and built-in calendar

The GUI opens maximized with a large, high-contrast countdown that scales with the window. Press **F11** to toggle fullscreen. Click **Start** to begin preparation. The display shows the phase, round, completed rounds, and running minutes. The last five seconds turn red and use the Windows notification sound; audibility depends on Windows sound settings.

Click **Stop**, press **Esc**, or close the window to end the session cleanly. Starting again resets the on-screen session totals while preserving all logs.

Click **Calendar** to view monthly history. Use **Previous**, **Next**, and **Today** to navigate. Green cells show completed running minutes; an asterisk marks today. The header shows monthly minutes, all-time minutes, and training days. Empty history displays an empty calendar.

Click **Timer** to return to the countdown. An active workout continues while viewing history, and Stop and Esc remain available. History refreshes when opened, on Today, and after a completed run.

The GUI uses elapsed-time deadlines, and sound does not extend its countdown. Keep the computer awake during exercise: suspension or a blocked UI can delay phase transitions. Missed intervals are never counted as extra runs.

## Console timer

The console displays the round number and a colored countdown. During the last five seconds, the display turns red and a beep sounds. Press **Ctrl+C** or **Ctrl+Break** to stop cleanly, save a `PROGRAM_END` event, and display completed rounds and running minutes.

The console's final countdown beeps each block for 300 ms before a one-second sleep, making its countdown slightly longer than the nominal duration.

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
1700000200 PROGRAM_END
```

- `PROGRAM_START` marks the start of a session, before preparation: on GUI Start or console launch.
- `RUN` records the start timestamp of a completed running interval.
- `RELAX` records its finish timestamp, at the beginning of recovery. The `RUN` / `RELAX` pair is saved together only after the run finishes, in the log file for the run's starting month.
- `PROGRAM_END` marks a clean stop with GUI Stop, Esc, or normal window close, or console Ctrl+C / Ctrl+Break, including during preparation. Forced termination cannot save an end event.

Both calendars process each file in line order. Each adjacent `RUN` / `RELAX` pair contributes **exactly two running minutes**, assigned to the local date of the `RUN` timestamp. Preparation and recovery are excluded. Interrupted runs are not saved; older logs may contain unmatched `RUN` entries, which are not counted. Pairs split across files are not counted. Lines with fewer than two fields or a non-integer timestamp are skipped.

These totals represent completed intervals, not measured elapsed time. The GUI reuses `FileLogger.cpp` and preserves the existing log format, so the Python calendar works without changes. Avoid running multiple timers simultaneously against the same logs, as their session entries can interleave.

## Project layout

| Path | Purpose |
| --- | --- |
| `run.cpp` | Console countdown, interval loop, colors, and sound cues. |
| `runui.cpp` | Native Windows GUI with a large countdown and built-in calendar. |
| `FileLogger.cpp` | Shared monthly event logger, included by both timers. |
| `run.bat` | Builds both timers using `g++`. |
| `logs/` | Local training logs; `.log` files are ignored by Git. |
| `calendar/training_calendar.py` | Parses logs, aggregates running minutes, and renders the calendar. |
| `calendar/calendar.html` | Generated calendar snapshot, tracked in the repository. |

Compiled `.exe` files and Python cache files are also ignored by Git. The checked-in calendar is a snapshot; regenerate it from your own logs for current results.

## Changing interval lengths

Console preparation and recovery durations are hardcoded in the `count_back` calls in `run.cpp`; running duration uses `RUN_SECONDS`. GUI durations are set in `beginPhase` calls in `runui.cpp`. Edit the relevant values and rebuild to change a timer. If changing the two-minute running duration, also update the fixed two-minute counting and summary calculations in `runui.cpp` and `calendar/training_calendar.py`, along with the displayed duration labels. Neither timer currently accepts command-line configuration.
