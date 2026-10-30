#define UNICODE
#define _UNICODE
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include "FileLogger.cpp"

namespace {
constexpr int START = 101, STOP = 102, CALENDAR = 103, PREVIOUS = 104,
              NEXT = 105, TODAY = 106;
enum class Phase { Idle, Prepare, Run, Recover };
HWND windowHandle, buttons[6];
FileLogger logger;
Phase phase = Phase::Idle;
bool calendarVisible = false, fullscreen = false;
int completed = 0, secondsLeft = 20, lastCue = -1;
int calendarYear, calendarMonth;
ULONGLONG deadline = 0;
time_t runStarted = 0;
WINDOWPLACEMENT placement = {};
std::map<int, int> minutes;
std::wstring calendarError;

int dateKey(const tm& date) {
    return (date.tm_year + 1900) * 10000 + (date.tm_mon + 1) * 100 + date.tm_mday;
}

void loadCalendar() {
    minutes.clear();
    calendarError.clear();
    WIN32_FIND_DATAW data;
    HANDLE search = FindFirstFileW(L"logs\\*.log", &data);
    if (search == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
            calendarError = L"Unable to read the logs folder.";
        return;
    }
    do {
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        // Logger filenames are ASCII (YYYY_M.log), as are legacy log names.
        std::wstring wideName(data.cFileName);
        std::string name(wideName.begin(), wideName.end());
        std::ifstream file("logs/" + name);
        if (!file) { calendarError = L"Some log files could not be read."; continue; }
        std::string line, previousEvent;
        time_t previousTime = 0;
        while (std::getline(file, line)) {
            std::istringstream entry(line);
            time_t timestamp;
            std::string event;
            if (!(entry >> timestamp >> event)) continue;
            if (previousEvent == "RUN" && event == "RELAX") {
                tm* local = localtime(&previousTime);
                if (local) minutes[dateKey(*local)] += 2;
            }
            previousEvent = event;
            previousTime = timestamp;
        }
        if (file.bad()) calendarError = L"Some log files could not be read completely.";
    } while (FindNextFileW(search, &data));
    FindClose(search);
}

void logError() {
    MessageBoxW(windowHandle,
        L"Could not save the training log. Check that the logs folder is writable. The session has stopped.",
        L"Logging error", MB_OK | MB_ICONERROR);
}

void beginPhase(Phase next, int duration) {
    phase = next;
    secondsLeft = duration;
    lastCue = -1;
    deadline = GetTickCount64() + static_cast<ULONGLONG>(duration) * 1000;
    if (next == Phase::Run) runStarted = time(nullptr);
}

void stopSession() {
    if (phase == Phase::Idle) return;
    phase = Phase::Idle;
    if (!logger.do_log("PROGRAM_END")) logError();
    loadCalendar();
}

void tick() {
    if (phase == Phase::Idle) return;
    ULONGLONG now = GetTickCount64();
    if (now >= deadline) {
        if (phase == Phase::Run) {
            if (!logger.log_completed_run(runStarted, time(nullptr))) {
                stopSession();
                logError();
                return;
            }
            ++completed;
            beginPhase(Phase::Recover, 60);
            loadCalendar();
        } else beginPhase(Phase::Run, 120);
    } else secondsLeft = static_cast<int>((deadline - now + 999) / 1000);
    if (secondsLeft <= 5 && secondsLeft != lastCue) {
        lastCue = secondsLeft;
        MessageBeep(MB_OK); // Asynchronous: sound never extends the countdown.
    }
}

void text(HDC dc, const std::wstring& value, RECT area, int size,
          COLORREF color = RGB(240, 244, 250), bool bold = false,
          UINT alignment = DT_CENTER | DT_VCENTER | DT_SINGLELINE) {
    HFONT font = CreateFontW(-size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HGDIOBJ old = SelectObject(dc, font);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, value.c_str(), -1, &area, alignment);
    SelectObject(dc, old);
    DeleteObject(font);
}

void fill(HDC dc, RECT area, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &area, brush);
    DeleteObject(brush);
}

void paintTimer(HDC dc, int width, int height) {
    const wchar_t* label = phase == Phase::Prepare ? L"GET READY" :
        phase == Phase::Run ? L"RUN" : phase == Phase::Recover ? L"RECOVER" : L"READY TO RUN";
    COLORREF accent = phase == Phase::Run ? RGB(255, 210, 70) : RGB(90, 215, 235);
    if (phase != Phase::Idle && secondsLeft <= 5) accent = RGB(255, 90, 90);
    int top = 75, bottom = height - 105, available = bottom - top;
    text(dc, label, {20, top, width - 20, top + available / 5},
         std::min(64, available / 7), accent, true);
    int shown = phase == Phase::Idle ? 20 : secondsLeft;
    std::wstring timer = std::to_wstring(shown / 60) + L":" +
        (shown % 60 < 10 ? L"0" : L"") + std::to_wstring(shown % 60);
    text(dc, timer, {10, top + available / 5, width - 10, bottom - available / 5},
         std::min(width / 4, available / 2), accent, true);
    std::wstring summary = L"Round " + std::to_wstring(completed + (phase == Phase::Run ? 1 : 0)) +
        L"   |   Completed: " + std::to_wstring(completed) + L"   |   Running: " +
        std::to_wstring(completed * 2) + L" min";
    text(dc, summary, {10, bottom - available / 5, width - 10, bottom},
         std::min(32, width / 35));
    text(dc, L"20 sec preparation  /  2 min run  /  1 min recovery",
         {10, height - 100, width - 10, height - 60}, 22, RGB(160, 175, 195));
}

void paintCalendar(HDC dc, int width, int height) {
    tm first = {};
    first.tm_year = calendarYear - 1900;
    first.tm_mon = calendarMonth - 1;
    first.tm_mday = 1;
    first.tm_isdst = -1;
    mktime(&first);
    wchar_t title[80];
    wcsftime(title, 80, L"%B %Y", &first);
    text(dc, title, {10, 65, width - 10, 125}, 36, RGB(240,244,250), true);
    int total = 0, monthTotal = 0;
    for (const auto& day : minutes) {
        total += day.second;
        if (day.first / 100 == calendarYear * 100 + calendarMonth) monthTotal += day.second;
    }
    text(dc, L"This month: " + std::to_wstring(monthTotal) + L" min   |   All time: " +
         std::to_wstring(total) + L" min   |   Training days: " + std::to_wstring(minutes.size()),
         {10, 125, width - 10, 165}, std::min(24, width / 40));
    const wchar_t* weekdays[] = {L"Mon",L"Tue",L"Wed",L"Thu",L"Fri",L"Sat",L"Sun"};
    int cellWidth = (width - 40) / 7, cellHeight = (height - 280) / 6;
    for (int col = 0; col < 7; ++col)
        text(dc, weekdays[col], {20 + col * cellWidth, 170, 20 + (col+1)*cellWidth, 205}, 22);
    tm next = first;
    ++next.tm_mon;
    next.tm_mday = 0;
    mktime(&next);
    int offset = (first.tm_wday + 6) % 7;
    time_t now = time(nullptr);
    int today = dateKey(*localtime(&now));
    for (int day = 1; day <= next.tm_mday; ++day) {
        int slot = offset + day - 1, x = 20 + slot % 7 * cellWidth, y = 210 + slot / 7 * cellHeight;
        int key = calendarYear * 10000 + calendarMonth * 100 + day;
        auto found = minutes.find(key);
        int amount = found == minutes.end() ? 0 : found->second;
        RECT cell = {x + 2, y + 2, x + cellWidth - 2, y + cellHeight - 2};
        fill(dc, cell, amount ? RGB(30, 120, 80) : RGB(35, 45, 62));
        text(dc, std::to_wstring(day) + (key == today ? L" *" : L""),
             {cell.left, cell.top, cell.right, cell.top + cellHeight / 2}, std::min(26, cellHeight / 3),
             key == today ? RGB(255, 220, 90) : RGB(240,244,250), true);
        if (amount) text(dc, std::to_wstring(amount) + L" min",
            {cell.left, cell.top + cellHeight / 2, cell.right, cell.bottom}, std::min(24, cellHeight / 3));
    }
    text(dc, calendarError.empty() ? L"Completed runs only. * Today. Calendar refreshes when opened and after each run." : calendarError,
         {10, height - 65, width - 10, height - 25}, std::min(18, width / 55), RGB(180,195,210));
}

void layout() {
    RECT area;
    GetClientRect(windowHandle, &area);
    for (int i = 0; i < 6; ++i) {
        MoveWindow(buttons[i], 16 + i * 125, 16, 115, 36, TRUE);
        ShowWindow(buttons[i], i >= 3 && !calendarVisible ? SW_HIDE : SW_SHOW);
    }
    EnableWindow(buttons[0], phase == Phase::Idle);
    EnableWindow(buttons[1], phase != Phase::Idle);
    SetWindowTextW(buttons[2], calendarVisible ? L"Timer" : L"Calendar");
    InvalidateRect(windowHandle, nullptr, FALSE);
}

void toggleFullscreen() {
    fullscreen = !fullscreen;
    if (fullscreen) {
        placement.length = sizeof(placement);
        GetWindowPlacement(windowHandle, &placement);
        MONITORINFO monitor = {};
        monitor.cbSize = sizeof(monitor);
        GetMonitorInfoW(MonitorFromWindow(windowHandle, MONITOR_DEFAULTTONEAREST), &monitor);
        SetWindowLongPtrW(windowHandle, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(windowHandle, HWND_TOP, monitor.rcMonitor.left, monitor.rcMonitor.top,
            monitor.rcMonitor.right-monitor.rcMonitor.left, monitor.rcMonitor.bottom-monitor.rcMonitor.top,
            SWP_FRAMECHANGED);
    } else {
        SetWindowLongPtrW(windowHandle, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        SetWindowPlacement(windowHandle, &placement);
        SetWindowPos(windowHandle, nullptr, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
    }
}

LRESULT CALLBACK procedure(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
    switch (message) {
    case WM_CREATE: {
        windowHandle = hwnd;
        const wchar_t* labels[] = {L"Start",L"Stop",L"Calendar",L"< Previous",L"Next >",L"Today"};
        for (int i = 0; i < 6; ++i)
            buttons[i] = CreateWindowW(L"BUTTON", labels[i], WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                0,0,0,0, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(START+i)), nullptr, nullptr);
        if (!SetTimer(hwnd, 1, 50, nullptr)) return -1;
        return 0;
    }
    case WM_GETMINMAXINFO:
        reinterpret_cast<MINMAXINFO*>(lp)->ptMinTrackSize = {800, 600};
        return 0;
    case WM_SIZE: layout(); return 0;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case START:
            if (phase == Phase::Idle) {
                if (!logger.do_log("PROGRAM_START")) { logError(); break; }
                completed = 0;
                calendarVisible = false;
                beginPhase(Phase::Prepare, 20);
            }
            break;
        case STOP: tick(); stopSession(); break;
        case CALENDAR: calendarVisible = !calendarVisible; if (calendarVisible) loadCalendar(); break;
        case PREVIOUS: if (--calendarMonth == 0) { calendarMonth = 12; --calendarYear; } break;
        case NEXT: if (++calendarMonth == 13) { calendarMonth = 1; ++calendarYear; } break;
        case TODAY: {
            time_t now = time(nullptr); tm local = *localtime(&now);
            calendarYear = local.tm_year + 1900; calendarMonth = local.tm_mon + 1;
            loadCalendar(); break;
        }
        }
        layout(); return 0;
    case WM_TIMER: tick(); layout(); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT area; GetClientRect(hwnd, &area);
        HDC buffer = CreateCompatibleDC(dc);
        HBITMAP bitmap = CreateCompatibleBitmap(dc, std::max(1L,area.right), std::max(1L,area.bottom));
        HGDIOBJ old = SelectObject(buffer, bitmap);
        fill(buffer, area, RGB(15, 23, 38));
        if (calendarVisible) paintCalendar(buffer, area.right, area.bottom);
        else paintTimer(buffer, area.right, area.bottom);
        BitBlt(dc,0,0,area.right,area.bottom,buffer,0,0,SRCCOPY);
        SelectObject(buffer, old); DeleteObject(bitmap); DeleteDC(buffer);
        EndPaint(hwnd, &ps); return 0;
    }
    case WM_CLOSE: tick(); stopSession(); DestroyWindow(hwnd); return 0;
    case WM_DESTROY: KillTimer(hwnd,1); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd,message,wp,lp);
}
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
    SetProcessDPIAware();
    // Keep GUI logs beside the executable, regardless of shortcut working directory.
    wchar_t path[32768];
    DWORD length = GetModuleFileNameW(nullptr,path,32768);
    if (!length || length >= 32768) return 1;
    std::wstring directory(path);
    directory = directory.substr(0,directory.find_last_of(L"\\/"));
    if (!SetCurrentDirectoryW(directory.c_str())) return 1;
    time_t now = time(nullptr); tm local = *localtime(&now);
    calendarYear = local.tm_year + 1900; calendarMonth = local.tm_mon + 1;
    loadCalendar();
    WNDCLASSW wc = {};
    wc.lpfnWndProc = procedure; wc.hInstance = instance;
    wc.lpszClassName = L"IntervalRunGUI";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    if (!RegisterClassW(&wc)) return 1;
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Interval Run | F11 fullscreen | Esc stop",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT,CW_USEDEFAULT,1100,800,
        nullptr,nullptr,instance,nullptr);
    if (!hwnd) return 1;
    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    MSG message;
    BOOL result;
    while ((result = GetMessageW(&message,nullptr,0,0)) > 0) {
        // Handle shortcuts even when a button has keyboard focus.
        if (message.message == WM_KEYDOWN && message.wParam == VK_F11) { toggleFullscreen(); continue; }
        if (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE) {
            tick(); stopSession(); layout(); continue;
        }
        TranslateMessage(&message); DispatchMessageW(&message);
    }
    return result == -1 ? 1 : 0;
}
