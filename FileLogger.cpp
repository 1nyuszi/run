#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <direct.h>
#include <cerrno>
#include <cstring>

using namespace std;

class FileLogger {
public:

    bool do_log(string type) {
        time_t now = chrono::system_clock::to_time_t(chrono::system_clock::now());
        return write_entries(get_file_name(now), to_string(now) + " " + type + "\n");
    }

    bool log_completed_run(time_t started, time_t finished) {
        // Keep the pair together, including when a run crosses a month boundary.
        return write_entries(get_file_name(started),
            to_string(started) + " RUN\n" + to_string(finished) + " RELAX\n");
    }

private:

    bool write_entries(const string& filename, const string& entries) {
        if (_mkdir("logs") != 0 && errno != EEXIST) {
            cerr << "Error: cannot create logs directory: " << strerror(errno) << endl;
            return false;
        }
        ofstream file(filename, ios::app);
        if (!file.is_open()) {
            cerr << "Error: cannot open log file '" << filename << "' for writing." << endl;
            return false;
        }
        file << entries;
        file.close();
        if (!file) {
            cerr << "Error: failed to write or close log file '" << filename << "'." << endl;
            return false;
        }
        return true;
    }

    string get_file_name(time_t timestamp) {
        struct tm local = *localtime(&timestamp);
        return "logs/" + to_string(local.tm_year + 1900) + "_"
            + to_string(local.tm_mon + 1) + ".log";
    }

};
