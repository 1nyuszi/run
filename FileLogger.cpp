#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

using namespace std;

class FileLogger {
public: 

    void do_log(string type) {
        time_t now_c = chrono::system_clock::to_time_t(chrono::system_clock::now());
        string filename = this->get_file_name();
        ofstream file(filename, ios::app);
        if (file.is_open()) {
            file << now_c << " " << type << endl;
            file.close();
        }
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