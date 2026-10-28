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
        if (_mkdir("logs") != 0 && errno != EEXIST) {
            cerr << "Error: cannot create logs directory: " << strerror(errno) << endl;
            return false;
        }
        time_t now_c = chrono::system_clock::to_time_t(chrono::system_clock::now());
        string filename = this->get_file_name();
        ofstream file(filename, ios::app);
        if (!file.is_open()) {
            cerr << "Error: cannot open log file '" << filename << "' for writing." << endl;
            return false;
        }
        file << now_c << " " << type << endl;
        file.close();
        if (!file) {
            cerr << "Error: failed to write or close log file '" << filename << "'." << endl;
            return false;
        }
        return true;
    }

private:

    string get_file_name() {
        return "logs/" + this->get_current_year_month() + ".log";
    }

    string get_current_year_month() {
        string yearmonth;
        time_t now = time(nullptr);
        struct tm local = *localtime(&now);
        yearmonth = to_string(local.tm_year + 1900) + "_" + to_string(local.tm_mon + 1);
        return yearmonth;
    }

};
