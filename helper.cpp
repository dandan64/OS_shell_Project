
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

std::vector<std::string> splitTokens(const std::string& s) {
    std::vector<std::string> toks;
    size_t i = 0, n = s.size();
    while (i < n) {
        // skip spaces
        while (i < n && isspace(static_cast<unsigned char>(s[i]))) i++;
        if (i >= n) break;
        size_t j = i;
        while (j < n && !isspace(static_cast<unsigned char>(s[j]))) j++;
        toks.emplace_back(s.substr(i, j - i));
        i = j;
    }
    return toks;
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
    // build the path to /proc/<pid>/environ
    const std::string path = "/proc/" + std::to_string(getpid()) + "/environ";

    // read the entire file into a string (throws on error)
    std::string raw;

    readFileContent(path, raw);

    // move its bytes into a vector<char>
    return std::vector<char>(raw.begin(), raw.end());
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


uint64_t get_total_cpu_time() {
    std::string content;
    try {
        readFileContent("/proc/stat", content);
    } catch (...) {
        return 0;
    }

    auto lines = splitLines(content);
    if (lines.empty()){
        return 0;
    }
    const std::string& first = lines[0];

    // Tokenize on whitespace
    auto toks = splitTokens(first);
    // toks[0] == "cpu", then user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice
    if (toks.size() < 11) return 0;

    // Sum fields 1..10
    uint64_t total = 0;
    for (size_t idx = 1; idx <= 10; ++idx) {
        total += std::stoull(toks[idx]);
    }
    return total;
}

uint64_t get_process_cpu_time(pid_t pid) {
    // build path to /proc/<pid>/stat
    const std::string path = "/proc/" + std::to_string(pid) + "/stat";

    std::string content;

    // read the entire file (throws on failure)
    try {
        readFileContent(path, content);
    }catch(...){
        return 0;
    }

    // split into lines and take the first one
    auto lines = splitLines(content);
    const std::string& firstLine = lines[0];

    // split the line on whitespace into tokens
    auto toks = splitTokens(firstLine);

    uint64_t utime = std::stoull(toks[13]);
    uint64_t stime = std::stoull(toks[14]);
    return utime + stime;
}



double get_mem_mb(pid_t pid) {
    const std::string path = "/proc/" + std::to_string(pid) + "/statm";
    std::string content;
    try{
        readFileContent(path, content);
    }catch(...){
        return 0;
    }

    auto toks = splitTokens(content);
    if (toks.size() < 2) {
        throw std::runtime_error("get_mem_mb: malformed statm file");
    }

    uint64_t pages_resident = std::stoull(toks[1]);

    long page_size = sysconf(_SC_PAGESIZE);
    double bytes = static_cast<double>(pages_resident) * page_size;
    return bytes / (1024.0 * 1024.0);
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

void readFileContent(const std::string& path, std::string& content) {
    // Open the file for reading only
    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        perror("smash error: open failed");
        return;
    }

    constexpr size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;

    // Repeatedly read until EOF
    while ((bytesRead = read(fd, buffer, BUFFER_SIZE)) > 0) {
        content.append(buffer, static_cast<size_t>(bytesRead));
    }
    if (bytesRead == -1) {
        close(fd);
        perror("smash error: read failed");
        return;
    }

    close(fd);
}

std::vector<std::string> splitLines(const std::string& s) {
    std::vector<std::string> lines;
    size_t start = 0;
    while (start < s.size()) {
        size_t pos = s.find('\n', start);
        if (pos == std::string::npos) pos = s.size();
        lines.emplace_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    return lines;
}
