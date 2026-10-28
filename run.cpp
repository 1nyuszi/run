#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <windows.h>
#include "FileLogger.cpp"
using namespace std;

const int COLOR_RED = 1;
const int COLOR_YELLOW = 3;
const int COLOR_CYAN = 6;
const int COLOR_WHITE = 7;
const int COLOR_GREY = 8;

const int RUN_SECONDS = 120;
HANDLE stop_event;
FileLogger logger;

BOOL WINAPI handle_stop(DWORD event) {
    if (event != CTRL_C_EVENT && event != CTRL_BREAK_EVENT) return FALSE;
    SetEvent(stop_event);
    return TRUE;
}

void clear_screen();
void clear_line();
void move_cursor(int row, int col);
void set_color(int color);
bool count_back(int seconds, int color, string text);

int run_session() {
    int completed_rounds = 0;
    if (!logger.do_log("PROGRAM_START")) return EXIT_FAILURE;
    bool continuing = count_back(20, COLOR_GREY, "It's Interval Run 2.0 program. Prepare... ");
    while (continuing) {
        int round = completed_rounds + 1;
        time_t started = time(nullptr);
        if (!count_back(RUN_SECONDS, COLOR_YELLOW,
                "It's your " + to_string(round) + ". round. Let's run...")) break;
        ++completed_rounds;
        if (!logger.log_completed_run(started, time(nullptr))) return EXIT_FAILURE;
        continuing = count_back(60, COLOR_CYAN,
            "It was your " + to_string(round) + ". round. Let's relax a bit...");
    }
    set_color(COLOR_WHITE);
    cout << "\nSession stopped.\nCompleted rounds: " << completed_rounds
         << "\nRunning time: " << completed_rounds * RUN_SECONDS / 60 << " minutes\n";
    return logger.do_log("PROGRAM_END") ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main() {
    stop_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!stop_event) {
        cerr << "Error: cannot create stop event." << endl;
        return EXIT_FAILURE;
    }
    if (!SetConsoleCtrlHandler(handle_stop, TRUE)) {
        cerr << "Error: cannot register Ctrl+C handler." << endl;
        CloseHandle(stop_event);
        return EXIT_FAILURE;
    }
    int result = run_session();
    SetConsoleCtrlHandler(handle_stop, FALSE);
    CloseHandle(stop_event);
    return result;
}

void clear_screen() {
    cout << "\033[2J\033[H";
}

void clear_line() {
    cout << "\x1b[2K";
}

void move_cursor(int row, int col) {
    cout << "\033[" << row << ";" << col << "H";
}

void set_color(int color) {
    cout << "\033[1;" << (30 + color % 8) << "m";
}

bool count_back(int seconds, int color, string text) {
    if (WaitForSingleObject(stop_event, 0) == WAIT_OBJECT_0) return false;
    clear_screen();
    move_cursor(0, 0);
    set_color(color);
    cout << text << endl;
    set_color(COLOR_WHITE);
    for(;seconds>0;seconds--) {
        if (WaitForSingleObject(stop_event, 0) == WAIT_OBJECT_0) return false;
        move_cursor(2, 1);
        clear_line();
        if (seconds <= 5) {
            set_color(COLOR_RED);
            Beep(750, 300);
        }
        cout << seconds;
        cout << flush;
        if (WaitForSingleObject(stop_event, 1000) == WAIT_OBJECT_0) return false;
    }
    return true;
}
