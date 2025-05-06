
#include "helper.h"
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <cstring>
#include <regex>
#include <cstddef>
#include <thread>
#include <fcntl.h>
#include <cmath>
#include <experimental/filesystem>
#include <sys/stat.h>
#include <memory>

using namespace std;
const std::string WHITESPACE = " \n\r\t\f\v";

// TODO : NEED TO ADD SPECIAL COMMANDS AS WELL - LATER
//const std::vector<std::string> RESERVED_COMMANDS = {"chprompt", "showpid", "pwd", "cd", "jobs", "fg",
//                                                    "quit", "kill", "alias", "unalias", "unsetenv", "watchproc"};
//

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif


string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

std::size_t _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    std::size_t i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

void charPtrArrayToStringArray(char* const src[],
                               std::string dest[],
                               std::size_t n)
{
    for (std::size_t i = 0; i < n; ++i) {
        // if src[i] is nullptr, make dest[i] an empty string
        dest[i] = src[i] ? src[i] : "";
    }
}

std::vector<std::string> extract_env_var_names(const std::vector<char>& env_file) {
    std::vector<std::string> names;
    size_t i = 0;

    while (i < env_file.size()) {
        if (env_file[i] != '\0') {
            std::string entry(&env_file[i]); // Whole "NAME=VALUE" string
            size_t equal_pos = entry.find('=');
            if (equal_pos != std::string::npos && equal_pos > 0) {
                names.emplace_back(entry.substr(0, equal_pos)); // Only NAME part
            }
            i += entry.length() + 1; // Skip to next NUL-separated entry
        } else {
            ++i; // Skip any unexpected NULs
        }
    }
    return names;
}

std::vector<char> environ_file_to_vector() {

    std::string environ_path = "/proc/" + to_string(getpid()) + "/environ";
    std::ifstream environ_file(environ_path, std::ios::binary);
    std::vector<char> environ_vector((std::istreambuf_iterator<char>(environ_file)),
                                     (std::istreambuf_iterator<char>())); // vector is filled with the file content
    return environ_vector;
}

void remove_env_var(const std::string& varname) {
    size_t varlen = varname.length();
    for (char **env = __environ; *env != nullptr; ++env) {
        if (std::strncmp(*env, varname.c_str(), varlen) == 0 && (*env)[varlen] == '=') {
            // Found it
            char **next = env;
            do {
                next[0] = next[1]; // Shift all pointers left
                ++next;
            } while (next[-1] != nullptr); // Stop when NULL pointer shifted

            break; // Only remove the first occurrence
        }
    }
}

//TODO : !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
uint64_t get_total_cpu_time() {
    std::ifstream stat_file("/proc/stat");
    std::string line;
    std::getline(stat_file, line);

    std::istringstream iss(line);
    std::string cpu_label;
    uint64_t user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
    iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;

    return user + nice + system + idle + iowait + irq + softirq + steal + guest + guest_nice;
}

uint64_t get_process_cpu_time(pid_t pid) { // TODO: need to check if the pid exists - if the stat_file is not failing
    std::ifstream stat_file("/proc/" + std::to_string(pid) + "/stat");
    if (!stat_file) {
        throw;
    }
    std::string line;
    std::getline(stat_file, line);

    std::istringstream iss(line);
    std::string dummy;
    int field_count = 0;

    uint64_t utime = 0, stime = 0;

    while (iss >> dummy) {
        ++field_count;
        if (field_count == 14) { // utime is 14th field
            utime = std::stoull(dummy);
        } else if (field_count == 15) { // stime is 15th field
            stime = std::stoull(dummy);
            break;
        }
    }
    return utime + stime;
}

double get_mem_mb(pid_t pid){
    std::ifstream f("/proc/" + std::to_string(pid) + "/statm");
    size_t pages_resident = 0, size_pages = 0;
    f >> size_pages >> pages_resident; // We wanna get only the pages_resident, so we put the first arg in a 'dummy' var
    double bytes = pages_resident * sysconf(_SC_PAGESIZE);
    return bytes / (1024.0 * 1024.0);
}

bool isExternalCommand(const std::string& command){
    if (command.find('/') != std::string::npos) {
        return access(command.c_str(), X_OK) == 0;
    }

    const char* pathEnv = std::getenv("PATH");
    if (!pathEnv) return false;

    std::stringstream ss(pathEnv);
    std::string dir;
    while (std::getline(ss, dir, ':')) {
        if (dir.empty()) {
            dir = ".";
        }
        std::string full = dir + "/" + command;
        if (access(full.c_str(), X_OK) == 0) {
            return true;
        }
    }
    return false;
}

bool isComplexCommand(std::string args_cmd[], const int numOfArgs){
    for(int i = 0; i < numOfArgs; i++){
        if(args_cmd[i].find_first_of("*?") != std::string::npos){
            return true;
        }
    }
    return false;
}

std::pair<std::string, std::size_t>* findSpecial(const char* cmdLine)
{
    if (!cmdLine) return nullptr;

    const char* p = cmdLine;
    std::size_t idx = 0;

    while (*p) {
        /* skip ordinary characters that are not | or > */
        if (*p != '|' && *p != '>') {
            ++p; ++idx;
            continue;
        }

        /* ---- candidate found ---- */
        if (*p == '|') {
            if (*(p + 1) == '&') {            //   |&
                return new std::pair<std::string,std::size_t>("|&", idx);
            } else {                          //   |
                return new std::pair<std::string,std::size_t>("|", idx);
            }
        }
        else if (*p == '>') {
            if (*(p + 1) == '>') {            //   >>
                return new std::pair<std::string,std::size_t>(">>", idx);
            } else {                          //   >
                return new std::pair<std::string,std::size_t>(">", idx);
            }
        }
    }
    return nullptr;   // nothing found
}

std::pair<std::string, std::size_t>* findAndSplitSpecial(std::string args[], std::size_t &numArgs) {
    static const char* ops[] = {"|&", ">>", "|", ">"};

    for (std::size_t i = 0; i < numArgs; ++i) {
        const std::string &token = args[i];
        for (const char* opC : ops) {
            std::string op(opC);
            std::size_t pos = token.find(op);
            if (pos != std::string::npos) {
                // Compute left and right substrings
                std::string left  = token.substr(0, pos);
                std::string right = token.substr(pos + op.size());

                // Determine how many new slots we need
                std::size_t insertCount = 1 + (left.empty() ? 0 : 1) + (right.empty() ? 0 : 1);

                // Shift existing elements to make room
                for (std::size_t j = numArgs; j-- > i + 1; ) {
                    args[j + insertCount - 1] = args[j];
                }

                // Insert left, op, right
                std::size_t idx = i;
                if (!left.empty()) {
                    args[idx++] = left;
                }
                args[idx++] = op;
                if (!right.empty()) {
                    args[idx++] = right;
                }

                // Update the count
                numArgs += insertCount - 1;

                // Compute the position of the operator
                std::size_t opIndex = i + (left.empty() ? 0 : 1);
                return new std::pair<std::string, std::size_t>(op, opIndex);
            }
        }
    }
    return nullptr;
}


int findPipePlacement(std::string args[], int numOfArgs) {
    for (int i = 0; i < numOfArgs; i++) {
        if (args[i] == "|") {
            return i;
        }
    }
    return 0;
}


std::vector<std::string> charArrayToVector(char* arr[], std::size_t length) {
    std::vector<std::string> result;
    result.reserve(length);
    for (std::size_t i = 0; i < length; ++i) {
        if (arr[i])                      // skip nullptrs if any
            result.emplace_back(arr[i]); // construct std::string from char*
    }
    return result;
}

std::string* vectorToStringArray(const std::vector<std::string>& vec) {
    std::size_t n = vec.size();
    std::string* arr = new std::string[n];
    for (std::size_t i = 0; i < n; ++i) {
        arr[i] = vec[i];  // copy each string
    }
    return arr;
}