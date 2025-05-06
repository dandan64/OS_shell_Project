// Ver: 10-4-2025
#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <map>
#include <unistd.h>
#include <cstring>
#include <regex.h>
#include <memory>
#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class SmallShell;

class Command {
private:
    std::string m_cmd_args[COMMAND_MAX_ARGS];
    int m_num_of_args;
public:
    Command(std::string cmd_args[COMMAND_MAX_ARGS], int num_of_args){
        for(int i = 0; i < COMMAND_MAX_ARGS; ++i){
            this->m_cmd_args[i] = cmd_args[i];
        }
        this->m_num_of_args = num_of_args;
    };

    std::string* getCmdArgs(){
        return this->m_cmd_args;
    }

    int getNumOfArgs() const{
        return this->m_num_of_args;
    }

    virtual ~Command() = default;

    virtual void execute() = 0;
};

class BuiltInCommand : public Command {
protected:
    SmallShell& m_small_shell;
public:
    BuiltInCommand(std::string cmd_args[], int num_of_args, SmallShell& other) : Command(cmd_args, num_of_args), m_small_shell(other){};
    virtual void execute() = 0;
    virtual ~BuiltInCommand() {}
};

class Chprompt : public BuiltInCommand{
    std::string m_name = "chprompt";

public:
    Chprompt(std::string cmd_args[], int num_of_args, SmallShell& other) : BuiltInCommand(cmd_args, num_of_args, other){};
    virtual void execute();
};


class Showpid : public BuiltInCommand{

public:
    Showpid(std::string cmd_args[], int num_of_args, SmallShell& other) : BuiltInCommand(cmd_args, num_of_args, other){};
    virtual void execute();
};

class Pwd : public BuiltInCommand{

public:
    Pwd(std::string cmd_args[], int num_of_args, SmallShell& other) : BuiltInCommand(cmd_args, num_of_args, other){};
    virtual void execute();
};

class Cd : public BuiltInCommand{

public:
    Cd(std::string cmd_args[], int num_of_args, SmallShell& other) : BuiltInCommand(cmd_args, num_of_args, other){};
    virtual void execute();
};


class ExternalCommand : public Command {
    bool m_is_bg_cmd;
    SmallShell& m_shell;
    const char* m_og_cmd;
public:
    ExternalCommand(std::string cmd_args[], const char* og_cmd, int num_of_args, SmallShell& shell, bool is_bg_cmd) :
                                                                    Command(cmd_args, num_of_args), m_shell(shell) {
        m_is_bg_cmd = is_bg_cmd;
        m_og_cmd = og_cmd;
    };

    virtual ~ExternalCommand() {
    }

    bool getIsBgCmd() const {
        return m_is_bg_cmd;
    }

    void execute() override;
};


class RedirectionCommand : public Command {
    // TODO: Add your data members
    SmallShell& m_small_shell;
    std::string m_path;
    std::string m_mode;


public:
    explicit RedirectionCommand(std::string cmd_args[], size_t numOfArgs, std::string& mode, std::string& path, SmallShell& other)
                                                    : Command(cmd_args, numOfArgs), m_small_shell(other) {
        m_path = path;
        m_mode = mode;
    };

    virtual ~RedirectionCommand() {
    }

    std::string getPath() const{
        return this->m_path;
    }

    std::string getMode() const {
        return this->m_mode;
    }


    void execute() override;

};

class PipeCommand : public Command {
    // TODO: Add your data members
    SmallShell& m_small_shell;
    int m_pipe_place;
    bool m_is_err;
public:
    PipeCommand(std::string cmd_args[], int numOfArgs, SmallShell& other, int pipe_place, bool is_err) :
                        Command(cmd_args, numOfArgs), m_small_shell(other), m_pipe_place(pipe_place), m_is_err(is_err) {};

    virtual ~PipeCommand() {

    }
    bool getIsErr() const{
        return m_is_err;
    }
    int getPipePlace() {
        return this->m_pipe_place;
    }

    void execute() override;

    void executePipeHandler(int fd);
};

class DiskUsageCommand : public Command {
    SmallShell& m_small_shell;
public:
    DiskUsageCommand(std::string cmd_args[], int numOfArgs, SmallShell& other): Command(cmd_args, numOfArgs), m_small_shell(other) {};

    virtual ~DiskUsageCommand() {
    }

    void execute() override;
};

class WhoAmICommand : public Command {
    SmallShell& m_small_shell;
public:
    WhoAmICommand(std::string cmd_args[], int numOfArgs, SmallShell& other): Command(cmd_args, numOfArgs), m_small_shell(other){};

    virtual ~WhoAmICommand() {
    }

    void execute() override;
};

class NetInfo : public Command {
    // TODO: Add your data members **BONUS: 10 Points**
    SmallShell& m_small_shell;
public:
    NetInfo(std::string cmd_args[], int numOfArgs, SmallShell& other): Command(cmd_args, numOfArgs), m_small_shell(other){};
    virtual ~NetInfo() {
    }

    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {

public:
    // TODO: Add your data members public:
    QuitCommand(std::string cmd_args[], int num_of_args, SmallShell& other)
                        : BuiltInCommand(cmd_args, num_of_args, other){};

    virtual ~QuitCommand() {
    }

    void execute() override;
};


class JobsList {
private:
    class JobEntry {
    private:
        int m_jobId;
        std::string m_command;
        pid_t m_pid;

    public:
        JobEntry(const int& jobId, pid_t pid, const char* command){
            m_jobId = jobId;
            m_command = command;
            m_pid = pid;
        };
        ~JobEntry() = default;

        int getJobId() const {
            return m_jobId;
        }

        int getPid() const {
            return m_pid;
        }

        std::string getCommand() const {
            return m_command;
        }
    };

    // TODO: Add your data members
    std::vector<JobEntry> m_jobs;
    int max_id = 0;

public:
    JobsList() = default;

    ~JobsList() = default;

    int getMaxId() const {
        return this->max_id;
    }

    void setMaxId(int max_id) {
        this->max_id = max_id;
    }

    std::vector<JobEntry>& getJobsList() {
        return this->m_jobs;
    }

    void addJob(const char* cmd, pid_t pid);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob();

    JobEntry *getLastStoppedJob(int *jobId);

    void updateMaxJobId();
    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(std::string cmd_line[], int num_of_args, SmallShell& other) : BuiltInCommand(cmd_line, num_of_args, other) {};

    virtual ~JobsCommand() {
    }

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
//    JobsList& m_job_list;
public:
    KillCommand(std::string cmd_line[], int num_of_args, SmallShell& other)
                        : BuiltInCommand(cmd_line, num_of_args, other) {};
    virtual ~KillCommand() {
    }

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members

public:
    ForegroundCommand(std::string cmd_line[], int num_of_args, SmallShell& other) : BuiltInCommand(cmd_line, num_of_args, other) {};

    virtual ~ForegroundCommand() {
    }

    void execute() override;
    void executeFgByID();
    void executeLastId();
};

class AliasCommand : public BuiltInCommand {
private:
    std::string m_alias;
    std::string m_cmd;

public:
    AliasCommand(std::string cmd, std::string alias, std::string cmd_args[], int num_of_args, SmallShell& other)
                                                            : BuiltInCommand(cmd_args, num_of_args, other){
        m_alias = alias;
        m_cmd= cmd;
    };

    virtual ~AliasCommand() {
    }
    std::string& getAlias(){
        return this->m_alias;
    }
    std::string& getCmd(){
        return m_cmd;
    }

    void execute() override;
};

class UnAliasCommand : public BuiltInCommand {
public:
    UnAliasCommand(std::string cmd_args[], int num_of_args, SmallShell& other)
                                    : BuiltInCommand(cmd_args, num_of_args, other){};

    virtual ~UnAliasCommand() {
    }

    void execute() override;
    void removeAllAlias(int numOfArgs);
};

class UnSetEnvCommand : public BuiltInCommand {
public:
    UnSetEnvCommand(std::string cmd_args[], int num_of_args, SmallShell& other)
                                        : BuiltInCommand(cmd_args, num_of_args, other){};

    virtual ~UnSetEnvCommand() {
    }

    void execute() override;
};

class WatchProcCommand : public BuiltInCommand {
public:
    WatchProcCommand(std::string cmd_args[], int num_of_args, SmallShell& other)
            : BuiltInCommand(cmd_args, num_of_args, other){};

    virtual ~WatchProcCommand() {
    }

    void execute() override;
    double calculateCpuUsege(int pid);
};


class SmallShell {
private:
    std::string m_promptName = "smash";
    std::string m_lastWorkingDir = "\n"; // Initialized
    std::map<std::string, std::string> m_name_to_cmd;
    std::vector<std::string> m_order;
    JobsList m_jobs_list;

    int saved_std_out;
    int saved_std_in;
    int saved_std_err;
    int redirected_out;
    int redirected_in;
    int redirected_err;

    pid_t m_fg_proc_pid = 0;

    SmallShell() {
        saved_std_out = dup(STDOUT_FILENO);
        if (saved_std_out < 0) {
            std::perror("smash error: dup failed");
        }

        saved_std_in = dup(STDIN_FILENO);
        if (saved_std_in < 0) {
            std::perror("smash error: dup failed");
        }

        saved_std_err = dup(STDERR_FILENO);
        if (saved_std_err < 0) {
            std::perror("smash error: dup failed");
        }
    };

public:
    std::string getPromptName() const{
        return m_promptName;
    }

    void setPromptName(const std::string& newPromptName){
        this->m_promptName = newPromptName;
    }

    std::unique_ptr<Command>CreateCommand(const char *cmd_line);

    std::unique_ptr<Command> createCommandHandler(std::string args_cmd[], const char* og_cmd, int numOfArgs, bool is_bg_cmd, int is_pipe_cmd);
    std::unique_ptr<Command> build_alias(std::string args_cmd[], int numOfArgs);
    std::vector<std::string> CreateCommandVectorAlias(std::string args_cmd[], size_t numOfArgs);
    std::unique_ptr<Command> CreateCommandAndRedirect(std::string args_cmd[], size_t numOfArgs, const char* cmd_line, int start);
    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();

    void executeCommand(const char *cmd_line);

    // TODO: add extra methods as needed

    std::string getlastWorkingDir(){
        return this->m_lastWorkingDir;
    }
    void setlastWorkingDir(const std::string& dir) {
        this->m_lastWorkingDir = dir;
    }

    void addAlias(std::string& cmd, std::string& alias){
        m_name_to_cmd.insert({alias, cmd});
        m_order.push_back(alias);
    }
    std::map<std::string, std::string>& getMap(){
        return this->m_name_to_cmd;
    }
    std::vector<std::string>& getOrder(){
        return this->m_order;
    }

    JobsList& getJobsList() {
        return this->m_jobs_list;
    }

    int getSavedStdOut() const {
        return saved_std_out;
    }

    int getSavedStdIn() const {
        return saved_std_in;
    }

    int getSavedStdErr() const {
        return saved_std_err;
    }

    void setRedirectionOut(int new_fd) {
        this->redirected_out = new_fd;
    }

    void setRedirectionIn(int new_fd) {
        this->redirected_in = new_fd;
    }

    void setRedirectionErr(int new_fd) {
        this->redirected_err = new_fd;
    }

    int getRedirectedOut() {
        return this->redirected_out;
    }

    int getRedirectedIn() {
        return this->redirected_in;
    }

    int getRedirectedErr() {
        return this->redirected_err;
    }

    void restoreStdOut();

    void restoreStdIn();

    void restoreStdErr();

    pid_t getFgProcPID() const {
        return m_fg_proc_pid;
    }

    void setFgProcPID(pid_t pid) {
        this->m_fg_proc_pid = pid;
    }
};

#endif //SMASH_COMMAND_H_
