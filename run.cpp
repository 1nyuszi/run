#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <windows.h>
#include "FileLogger.cpp"
using namespace std;

const int COLOR_RED = 1;
const int COLOR_YELLOW = 3;
const int COLOR_CYAN = 6;
const int COLOR_WHITE = 7;
const int COLOR_GREY = 8;

int run_round = 0;
FileLogger logger;

void clear_screen();
void clear_line();
void move_cursor(int row, int col);
void set_color(int color);
void count_back(int seconds, int color, string text);

int main() {
    logger.do_log("PROGRAM_START");
    count_back(20, COLOR_GREY, "It's Interval Run 2.0 program. Prepare... ");
    while(true) {
        run_round++;
        logger.do_log("RUN");
        count_back(120, COLOR_YELLOW, "It's your " + to_string(run_round) + ". round. Let's run...");
        logger.do_log("RELAX");
        count_back(60, COLOR_CYAN, "It was your " + to_string(run_round) + ". round. Let's relax a bit...");
    }
    return 0;
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

void count_back(int seconds, int color, string text) {
    clear_screen();
    move_cursor(0, 0);
    set_color(color);
    cout << text << endl;
    set_color(COLOR_WHITE);
    for(;seconds>0;seconds--) {
        move_cursor(2, 1);
        clear_line();
        if (seconds <= 5) {
            set_color(COLOR_RED);
            Beep(750, 300);
        }
        cout << seconds;
        Sleep(1000);
    }    
}