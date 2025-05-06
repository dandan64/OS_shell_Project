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
#include "helper.h"
#include "CommandLittleHelprs.cpp"
#include "executeHelper.cpp"
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>


using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

// TODO : NEED TO ADD SPECIAL COMMANDS AS WELL - LATER
//const std::vector<std::string> RESERVED_COMMANDS = {"chprompt", "showpid", "pwd", "cd", "jobs", "fg",
//                                            "quit", "kill", "alias", "unalias", "unsetenv", "watchproc"};


#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

// Useful 'side' function



// TODO: Add your implementation for classes in Commands.h
//Command::Command(const char *cmd_line) {
//};
//SmallShell::SmallShell() {
//    // TODO: add your implementation
//    //m_promptName = "smash";
//}

SmallShell::~SmallShell() {
// TODO: add your implementation
}



std::unique_ptr<Command> SmallShell::createCommandHandler(string args_cmd[], const char* og_cmd, int numOfArgs, bool is_bg_cmd, int is_pipe_cmd) {
    if (is_pipe_cmd){
        bool is_err = is_pipe_cmd != 1;
        int pipe_place = findPipePlacement(args_cmd, numOfArgs);
        return make_unique<PipeCommand>(args_cmd, numOfArgs, *this, pipe_place, is_err);
    }
    else if(args_cmd[0] == "chprompt"){
        return make_unique<Chprompt>(args_cmd, numOfArgs, *this);
    }
    else if(args_cmd[0] == "showpid"){
        return make_unique<Showpid>(args_cmd, numOfArgs, *this);
    }
    else if(args_cmd[0] == "cd"){
        return make_unique<Cd>(args_cmd, numOfArgs, *this);
    }
    else if(args_cmd[0] == "pwd"){
        return make_unique<Pwd>(args_cmd, numOfArgs, *this);
    }
    else if(args_cmd[0] == "alias"){
        return build_alias(args_cmd,numOfArgs);
    }
    else if(args_cmd[0] == "unalias"){
        return make_unique<UnAliasCommand>(args_cmd, numOfArgs, *this);
    }
    else if (args_cmd[0] == "unsetenv") {
        return make_unique<UnSetEnvCommand>(args_cmd, numOfArgs, *this);
    }
    else if (args_cmd[0] == "watchproc") {
        return make_unique<WatchProcCommand>(args_cmd, numOfArgs, *this);
    }
    else if (args_cmd[0] == "jobs") {
        return make_unique<JobsCommand>(args_cmd, numOfArgs, *this);
    }
    else if (args_cmd[0] == "fg") {
        return make_unique<ForegroundCommand>(args_cmd, numOfArgs, *this);
    }
    else if (args_cmd[0] == "quit") {
        return make_unique<QuitCommand>(args_cmd, numOfArgs, *this);
    }
    else if (args_cmd[0] == "kill") {
        return make_unique<KillCommand>(args_cmd, numOfArgs, *this);
    }
    else if (args_cmd[0] == "du") {
        return make_unique<DiskUsageCommand>(args_cmd, numOfArgs, *this);
    }
    else if (args_cmd[0] == "whoami") {
        return make_unique<WhoAmICommand>(args_cmd, numOfArgs, *this);
    }
    else if (args_cmd[0] == "netinfo"){
        return make_unique<NetInfo>(args_cmd, numOfArgs, *this);
    }
    else {
        return make_unique<ExternalCommand>(args_cmd, og_cmd, numOfArgs, *this, is_bg_cmd);
    }
    return nullptr;
}


std::unique_ptr<Command> SmallShell::CreateCommand(const char *cmd_line) {

    std::size_t len = std::strlen(cmd_line); //get len of the cmd line
    char* copy_cmd_line = new char[len + 1]; //copy the cmd line for usege
    std::strcpy(copy_cmd_line, cmd_line);

    char* char_args_cmd[COMMAND_MAX_ARGS];
    string args_cmd[COMMAND_MAX_ARGS]; // Full command for non-alias command


    _removeBackgroundSign(copy_cmd_line);
    std::size_t numOfArgs = _parseCommandLine(copy_cmd_line, char_args_cmd); // Working without the & incase it's internal command

    charPtrArrayToStringArray(char_args_cmd, args_cmd, numOfArgs); // Working with strings - easier

    auto mapped_cmd = this->m_name_to_cmd.find(args_cmd[0]);
    if(mapped_cmd != this->m_name_to_cmd.end()){
        std::vector<std::string> alias_args_cmd_v = CreateCommandVectorAlias(args_cmd, numOfArgs);
        std::string alias_args_cmd[COMMAND_MAX_ARGS];
        std::string* tmp = vectorToStringArray(alias_args_cmd_v);
        for (size_t i = 0; i < COMMAND_MAX_ARGS && i < alias_args_cmd_v.size(); ++i){
            alias_args_cmd[i] = tmp[i];
        }
        delete[] tmp;
        int start = _parseCommandLine(cmd_line, char_args_cmd);
        return CreateCommandAndRedirect(alias_args_cmd, numOfArgs, cmd_line, start - 1);
    }
    else{
        return CreateCommandAndRedirect(args_cmd, numOfArgs,cmd_line, 0);
    }
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    std::unique_ptr<Command> cmd = CreateCommand((cmd_line));
    if (cmd != nullptr) {
        this->getJobsList().removeFinishedJobs();
        cmd->execute();
    }

    this->restoreStdOut();
//    this->restoreStdErr();
//    this->restoreStdIn();
}

// Implementing execute for all the commands:

void Chprompt::execute() {
    string cmd_args[COMMAND_MAX_ARGS];
    for(int i = 0; i < COMMAND_MAX_ARGS; ++i){
        cmd_args[i] = this->getCmdArgs()[i];
    }
    if(cmd_args[1].empty()){ //if it's nullptr
        m_small_shell.setPromptName("smash");
    }
    else {
        this->m_small_shell.setPromptName(cmd_args[1]);
    }
}

void Showpid::execute() {
    std::cout << "smash pid is " << getpid() << std::endl;
}

void Pwd::execute() {
    char* cwd = getcwd(nullptr, 0); // Dynamically allocates space for cwd
    std::cout << cwd << std::endl;
}

void Cd::execute() {
    std::string specialCommand = "-";
    int numOfArgs = this->getNumOfArgs();

    if(numOfArgs > 2){
        std::cerr << "smash error: cd: too many arguments" << std::endl;
    }
    else if(numOfArgs == 2){
        if(this->getCmdArgs()[1] == specialCommand){
            if(this->m_small_shell.getlastWorkingDir() == "\n"){
                std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
            }
            else {
                std::string currDir = getcwd(nullptr, 0); //dynamically allocate buffer for cwd
                if (chdir(this->m_small_shell.getlastWorkingDir().c_str()) >= 0){ // make sure the syscall did not fail
                    this->m_small_shell.setlastWorkingDir(currDir);
                }
                else {
                    std::perror("smash error: chdir failed");
                }
            }
        }
        else{
            std::string currDir = getcwd(nullptr, 0); //dynamically allocate buffer for cwd
            this->m_small_shell.setlastWorkingDir(currDir);
            if (chdir(this->getCmdArgs()[1].c_str()) < 0) {
                std::perror("smash error: chdir failed");
            }
        }
    }
}

void AliasCommand::execute() {
    if(this->getCmdArgs()[1].empty()){ // printing all the alias's the user created
        for (const string& alias : this->m_small_shell.getOrder()) {
            std::cout << alias << "='" << this->m_small_shell.getMap()[alias] << "'" << std::endl;
        }
        return;
    }
    this->m_small_shell.addAlias(this->getCmd(), this->getAlias());
}



void UnAliasCommand::execute(){
    int numOfArgs = this->getNumOfArgs();
    if (numOfArgs < 2) {
        std::cerr << "smash error: unalias: not enough arguments" << std::endl;
    }
    else {
        removeAllAlias(numOfArgs);
    }
}

void UnSetEnvCommand::execute() {
    std::vector<char> environ_vector = environ_file_to_vector();
    std::ifstream in("/proc/self/environ", std::ios::binary);


    std::vector<std::string> env_names = extract_env_var_names(environ_vector);
    // Now we have an array of the names of the vars

    if (this->getNumOfArgs() > 1) {
        for (int i = 1; i < this->getNumOfArgs(); i++) {
            bool exists = std::find(env_names.begin(), env_names.end(), this->getCmdArgs()[i]) != env_names.end();
            if (exists) {
                remove_env_var(this->getCmdArgs()[i]);
            }
            else {
                std::cerr << "smash error: unsetenv: " << this->getCmdArgs()[i] << " does not exist" << std::endl;
                break;
            }
        }
    }
    else {
        std::cerr << "smash error: unsetenv: not enough arguments" << std::endl;
    }
}


double WatchProcCommand::calculateCpuUsege(int pid){
    long hz = sysconf(_SC_CLK_TCK);
    uint64_t total_cpu_time_0 = get_total_cpu_time();
    uint64_t process_cpu_time_0;

    try {
        process_cpu_time_0 = get_process_cpu_time(pid);
    }
    catch (...) {
        std::cerr << "smash error: watchproc: pid " << pid << " does not exist" << std::endl;
        return -1;
    }

//    std::this_thread::sleep_for(std::chrono::seconds(1));
    struct timespec duration{};
    duration.tv_sec = 1;
    duration.tv_nsec = 0;
    if(nanosleep(&duration, nullptr) == -1){
        std::perror("smash error: nanosleep failed");
    }

    uint64_t total_cpu_time_1 = get_total_cpu_time();
    uint64_t process_cpu_time_1 = get_process_cpu_time(pid);

    uint64_t delta_total = total_cpu_time_1 - total_cpu_time_0;
    uint64_t delta_process = process_cpu_time_1 - process_cpu_time_0;

    return ((double(delta_process) / hz) / (double (delta_total) / hz)) * 100.0;
}

void WatchProcCommand::execute() {
    if (this->getNumOfArgs() == 2) {
        int pid;
        try {
            pid = atoi(this->getCmdArgs()[1].c_str());
        }
        catch(...){
            std::cerr << "smash error: watchproc: invalid arguments" << std::endl;
        }

        double cpu_usage = calculateCpuUsege(pid);
        if(cpu_usage == -1){
            return;
        }

        double mem_mb = get_mem_mb(pid);
        std::cout << "PID: " << pid
                << " | CPU Usage: " << fixed << std::setprecision(1)
                << cpu_usage << "% | Memory Usage: "
                << setprecision(1) << mem_mb <<" MB"
                << std::endl;
    }
    else {
        std::cerr << "smash error: watchproc: invalid arguments" << std::endl;
    }
}

void ExternalCommand::execute(){
    int num_of_args = this->getNumOfArgs();
    bool bg_cmd = this->getIsBgCmd();
    bool complex = isComplexCommand(this->getCmdArgs(), num_of_args);

    pid_t pid = fork();
    if(pid < 0){
        std::perror("smash error: fork failed");
        return;
    }

    // Child process
    if(pid == 0){
        setpgrp(); // Cannot fail
        if(!complex) {
            std::vector<char*> args;
            args.reserve(num_of_args + 1);
            for (int i = 0; i < num_of_args; ++i) {
                args.push_back(const_cast<char*>(this->getCmdArgs()[i].c_str()));
            }
            args.push_back(nullptr);
            execvp(args[0], args.data());
            std::perror("smash error: execvp failed");
        }
        else{
            std::string cmd_line;
            for(int i = 0; i < num_of_args - 1; i++){
                cmd_line += this->getCmdArgs()[i] + " ";
            }
            cmd_line += this->getCmdArgs()[num_of_args - 1];

            const char* bash = "/bin/bash";
            const char* args[] = {"bash", "-c", cmd_line.c_str(), nullptr};

            execvp(bash, const_cast<char* const*>((args)));
            std::perror("smash error: execvp failed");
        }
    }

    // Parent process
    if(pid > 0){
        this->m_shell.setFgProcPID(pid);
        if (!bg_cmd) {
            if(waitpid(pid, nullptr, 0 ) < 0){
                std::perror("smash error: waitpid failed");
            }
        }
        else{
            this->m_shell.getJobsList().removeFinishedJobs();
            this->m_shell.getJobsList().addJob(m_og_cmd, pid);
        }
        this->m_shell.setFgProcPID(0);
    }
}

// Jobs related methods

void JobsList::addJob(const char* cmd, pid_t pid) {
    int new_id = this->getMaxId() > 0 ? this->getMaxId() + 1 : 1;
    JobEntry new_job = JobEntry(new_id, pid, cmd);
    this->getJobsList().push_back(new_job);
    this->setMaxId(new_id);
}

void JobsList::printJobsList() {
    this->removeFinishedJobs();
    for (const JobEntry& job : this->getJobsList()) {
        std::cout <<"[" << job.getJobId() << "] " << job.getCommand() << std::endl;
    }
}

void JobsCommand::execute() {
    this->m_small_shell.getJobsList().printJobsList();
}

void JobsList::removeFinishedJobs() {
    std::vector<JobEntry>& jobs_list = this->getJobsList();
    for (auto job_it = jobs_list.begin(); job_it < jobs_list.end();) {
        int ret = waitpid(job_it->getPid(), nullptr, WNOHANG);
        if (ret > 0) { // TODO: CHECK IF IT CAN FAIL
            jobs_list.erase(job_it);
            if(job_it->getJobId() == this->getMaxId()){
                updateMaxJobId();
            }
        }
        else {
            job_it++;
        }
    }
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for (JobEntry& job : this->getJobsList()) {
        if (job.getJobId() == jobId) {
            return &job;
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    for (auto it = this->getJobsList().begin(); it < this->getJobsList().end(); ++it) {
        if (it->getJobId() == jobId) {
            this->getJobsList().erase(it);
        }
    }
}

JobsList::JobEntry* JobsList::getLastJob() {
    return &*(this->getJobsList().end() - 1);
}
void ForegroundCommand::execute() {
    if(this->getNumOfArgs() > 2){
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        return;
    }
    else if (this->getNumOfArgs() < 2) {
        executeLastId();
    }
    else {
        executeFgByID();
        return;
    }
}


void QuitCommand::execute() {
    if(this->getCmdArgs()[1] == "kill") {
        std::cout << "smash: sending SIGKILL signal to " << this->m_small_shell.getJobsList().getJobsList().size()
            << " jobs:" << std::endl;
        for(auto job : this->m_small_shell.getJobsList().getJobsList()){
            std::cout << job.getPid() <<": " << job.getCommand() << std::endl;
            if(kill(job.getPid(), SIGKILL)){
                std::perror("smash error: kill failed");
            }
        }
    }
    exit(SIGKILL);
}

void KillCommand::execute() {
    if(this->getNumOfArgs() != 3){
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
    }

    if(this->getCmdArgs()[1][0] == '-'){
        try{
            int signum = atoi(const_cast<char*>(&this->getCmdArgs()[1][1]));
            int jobId = stoi(this->getCmdArgs()[2]);
            auto jobEntry = this->m_small_shell.getJobsList().getJobById(jobId);
            if(jobEntry == nullptr){
                std::cout << "smash error: kill: job-id " << jobId << " does not exist" << std::endl;
                return;
            }
            int jobPid = jobEntry->getPid();
            if(!kill(jobPid, signum)){
                std::cout << "signal number " << signum << " was sent to pid " << jobPid << std::endl;
                return;
            }
            else{
                perror("smash error: kill failed");
            }

        } catch(...){
            std::cerr << "smash error: kill: invalid arguments" << std::endl;
            return;
        }
    }
    std::cerr << "smash error: kill: invalid arguments" << std::endl;
}

void RedirectionCommand::execute() {
    int flags = O_CREAT | O_WRONLY | (this->getMode() == ">>" ? O_APPEND : O_TRUNC);
    int new_fd = open(this->getPath().c_str(), flags, 0666);

    if (new_fd < 0) {
        std::perror("smash error: open failed");
    }

    if (dup2(new_fd, STDOUT_FILENO) < 0) {
        std::perror("smash error: open failed");
        close(new_fd);
    }
}

void PipeRedirectionOut() {

}

void SmallShell::restoreStdOut() {
    fflush(stdout);
    if (dup2(this->getSavedStdOut(), STDOUT_FILENO) < 0) {
        std::perror("smash error: dup failed");
    }
}

void SmallShell::restoreStdIn() {
    fflush(stdin);
    if (dup2(this->getSavedStdIn(), STDIN_FILENO) < 0) {
        std::perror("smash error: dup failed");
    }
}

void SmallShell::restoreStdErr() {
    fflush((stderr));
    if (dup2(this->getSavedStdErr(), STDERR_FILENO) < 0) {
        std::perror("smash error: dup failed");
    }
}


uintmax_t   diskUsageWrapper(const std::experimental::filesystem::path& path){
    struct stat sb;

    // Checks for unreadable folder
    if (lstat(path.c_str(), &sb) != 0) {
        return 0;
    }

    // Checks if the path is a symlink - if so, ignore
    if (S_ISLNK(sb.st_mode)) {
        return 0;
    }

    uintmax_t disk_usage = sb.st_blocks;

//    for (const auto& entry : std::experimental::filesystem::directory_iterator(path)) {
//        if(is_directory(entry.path())){
//            disk_usage += diskUsageWrapper(entry.path());
//        }
//    }
    if (S_ISDIR(sb.st_mode)) {
        for (auto const& entry : std::experimental::filesystem::directory_iterator(path)) {
            disk_usage += diskUsageWrapper(entry.path());
        }
    }
    return disk_usage;
}


void DiskUsageCommand::execute() {
    std::string path;
    struct stat sb;

    if(this->getNumOfArgs() == 1){
        path = getcwd(nullptr, 0);
    }
    else {
        path = this->getCmdArgs()[1];
    }
    if (stat(path.c_str(), &sb) != 0){
        std::cerr << "smash error: du: directory " << path << " does not exist" << std::endl;
        return;
    }
    if(this->getNumOfArgs() > 2){
        std::cerr << "smash error: du: too many arguments" << std::endl;
        return;
    }
    std::cout << "Total disk usage: " << std::ceil(diskUsageWrapper(path) / 2) <<" KB" << std::endl;
}

void WhoAmICommand::execute() {
    uid_t user_id = getuid();

    std::ifstream iss("/etc/passwd");
    std::string line;
    std::string user, uid, path;

    while (std::getline(iss, line)) {
        std::istringstream line_s(line);
        std::getline(line_s, user, ':');
        std::getline(line_s, uid, ':');
        std::getline(line_s, uid, ':');
        if (user_id == uid_t(stoi(uid))) {
            for (int i = 0; i < 3; i++) {
                std::getline(line_s,path, ':');
            }
            break;
        }
    }
    std::cout << user << " " << path << std::endl;
}

void PipeCommand::execute() {
    if (!this->getIsErr()) {
        this->executePipeHandler(STDOUT_FILENO);
    }
    else {
        this->executePipeHandler(STDERR_FILENO);
    }
}

void PipeCommand::executePipeHandler(int fd){
    int my_pipe[2];
    pipe(my_pipe);

    pid_t pid = fork();
    if (pid == 0) {
        close(my_pipe[0]);
        std::string cmd_1_args;
        for (int i = 0; i < getPipePlace(); i++) {
            cmd_1_args += this->getCmdArgs()[i] + " ";
        }

        if (dup2(my_pipe[1], fd) < 0) {
            std::perror("smash error: dup failed");
        }
        std::unique_ptr<Command> cmd_1 = this->m_small_shell.CreateCommand(cmd_1_args.c_str());
        if (cmd_1 != nullptr) {
            cmd_1->execute();
        }

        this->m_small_shell.restoreStdOut();
        this->m_small_shell.restoreStdErr();
        _exit(1);
    }
    else {
        if (waitpid(pid, nullptr, 0 ) < 0) {
            perror("smash error: waitpid failed");
        }
        close(my_pipe[1]);
        std::string cmd_2_s;
        for (int i = this->getPipePlace() + 1; i < this->getNumOfArgs(); i++) {
            cmd_2_s += this->getCmdArgs()[i] + " ";
        }
        if (dup2(my_pipe[0], STDIN_FILENO) < 0) {
            perror("smash error: dup failed");
        }

        std::unique_ptr<Command> cmd_2 = this->m_small_shell.CreateCommand(cmd_2_s.c_str());
        cmd_2->execute();

        this->m_small_shell.restoreStdIn();
    }
}


void NetInfo::execute() {
    if (this->getNumOfArgs() < 2) {
        std::cerr << "smash error: netinfo: interface not specified" << std:: endl;
        return;
    }

    // Creates a dummy socket
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("smash error: socket failed");
        return;
    }

    // Struct ifreq will hold the information needed
    struct ifreq ifr{};
    strncpy(ifr.ifr_name, this->getCmdArgs()[1].c_str(), IFNAMSIZ);

    // Get IP address
    if (ioctl(fd, SIOCGIFADDR, &ifr) >= 0) {
        struct sockaddr_in* ip = (struct sockaddr_in*)&ifr.ifr_addr;
        std::cout << "IP Address: " << inet_ntoa(ip->sin_addr) << std::endl;
    }
    else {
        perror("smash error: ioctl failed");
        return;
    }

    // Get subnet mask
    if (ioctl(fd, SIOCGIFNETMASK, &ifr) == 0) {
        struct sockaddr_in* ip = (struct sockaddr_in*)&ifr.ifr_netmask;
        std::cout << "Subnet Mask: " << inet_ntoa(ip->sin_addr) << std::endl;
    }
    else {
        perror("smash error: ioctl failed");
        return;
    }

    close(fd);

    // TODO: Get default gateway

    std::string gateway;                            // result buffer

// 3) Default Gateway via /proc/net/route using syscalls only
    int fd_gateway = open("/proc/net/route", O_RDONLY);
    if (fd_gateway < 0) {
        perror("smash error: open /proc/net/route");
    } else {
        // read the entire file into a stack buffer (it's small)
        char buf[8192];
        ssize_t tot = 0, n;
        while ((n = read(fd_gateway, buf + tot, sizeof(buf) - tot - 1)) > 0) {
            tot += n;
            if (tot >= (ssize_t)sizeof(buf) - 1) break;
        }
        if (n < 0) perror("smash error: read /proc/net/route");
        buf[tot] = '\0';
        close(fd);

        // walk line by line
        char *line = buf, *next;
        // skip header
        if ((next = strchr(line, '\n'))) line = next + 1;
        while (line && *line) {
            // split out the first three whitespace-separated fields:
            //  iface   destHex   gateHex   …
            char *p = line;
            // 1) iface
            char *f_iface = p;
            while (*p && *p!=' ' && *p!='\t') ++p;
            bool hasIfaceField = (*p==' '||*p=='\t');
            if (hasIfaceField) *p++ = '\0';

            // 2) destHex
            while (*p==' '||*p=='\t') ++p;
            char *f_dest = p;
            while (*p && *p!=' ' && *p!='\t') ++p;
            bool hasDestField = (*p==' '||*p=='\t');
            if (hasDestField) *p++ = '\0';

            // 3) gateHex
            while (*p==' '||*p=='\t') ++p;
            char *f_gate = p;
            while (*p && *p!=' ' && *p!='\t' && *p!='\n') ++p;
            char saved = *p;
            *p = '\0';

            // check for default route on our interface
            if (hasIfaceField
                && strcmp(f_iface, this->getCmdArgs()[1].c_str()) == 0
                && strcmp(f_dest, "00000000") == 0)
            {
                // parse f_gate as little-endian hex
                unsigned long gw = 0;
                for (char* h = f_gate; *h; ++h) {
                    char c = *h;
                    unsigned digit = (c>='0'&&c<='9' ? c-'0'
                                                     : (c>='A'&&c<='F' ? c-'A'+10
                                                                       : (c>='a'&&c<='f' ? c-'a'+10 : 0)));
                    gw = (gw << 4) | digit;
                }
                // extract bytes
                unsigned b0 = (gw      ) & 0xFF;
                unsigned b1 = (gw >>  8) & 0xFF;
                unsigned b2 = (gw >> 16) & 0xFF;
                unsigned b3 = (gw >> 24) & 0xFF;

                // format "d.d.d.d" manually
                char tmp[16];
                int len = snprintf(tmp, sizeof(tmp), "%u.%u.%u.%u",
                                   b0, b1, b2, b3);
                gateway.assign(tmp, len);
                break;
            }

            // restore and advance to next line
            *p = saved;
            line = strchr(line, '\n');
            if (line) line++;
        }
    }

    std::cout << "Default Gateway: " << gateway <<std::endl;

    // Get DNS servers
    std::ifstream resolv("/etc/resolv.conf");
    std::string line;
    std::vector<std::string> dns;

    while (std::getline(resolv, line)) {
        if (line.rfind("nameserver", 0) == 0) {
            std::istringstream iss(line);
            std::string keyword, ip;
            iss >> keyword >> ip;
            dns.push_back(ip);
        }
    }
    for (int i = 0; i < dns.size() - 1; i++) {
        std::cout << "DNS Servers: " << dns[i] << ", ";
    }
    std::cout << "DNS Servers: " << dns[dns.size() - 1] << std::endl;
}