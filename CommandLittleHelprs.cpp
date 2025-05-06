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
#include "CommandLittleHelprs.h"

const std::vector<std::string> RESERVED_COMMANDS = {"chprompt", "showpid", "pwd", "cd", "jobs", "fg",
                                                    "quit", "kill", "alias", "unalias", "unsetenv", "watchproc"};

using namespace std;


std::vector<std::string> SmallShell::CreateCommandVectorAlias(std::string args_cmd[], size_t numOfArgs){
    char* char_alias_args_cmd[COMMAND_MAX_ARGS];
    int start = _parseCommandLine(this->m_name_to_cmd[string(args_cmd[0])].c_str(), char_alias_args_cmd);
    std::vector<std::string> alias_args_cmd = charArrayToVector(char_alias_args_cmd, numOfArgs);
    alias_args_cmd.resize(COMMAND_MAX_ARGS);
    for(std::size_t i = 0; i < numOfArgs; ++i){ // Copy the arguments of the rest of the command to a string format
        alias_args_cmd[i + start] = args_cmd[i + 1];
    }
    return alias_args_cmd;
}

std::unique_ptr<Command> SmallShell::CreateCommandAndRedirect(std::string args_cmd[], size_t numOfArgs, const char* cmd_line, int start){
    int is_pipe_cmd = false;
    bool is_redirect = false;
    bool is_bg_cmd = _isBackgroundComamnd(cmd_line);
    std::pair<std::string, std::size_t>*  is_special = findAndSplitSpecial(args_cmd, numOfArgs);
    if(is_special){

        if(is_special->first == "|"){
            is_pipe_cmd = 1;
        }
        else if(is_special->first == "|&"){
            is_pipe_cmd = 2;
        }
        else if(is_special->first == ">" || is_special->first == ">>"){
            is_redirect = true;
        }
    }
    if (is_redirect) {
        auto redirect_cmd = make_unique<RedirectionCommand>(args_cmd, numOfArgs, args_cmd[numOfArgs - 2]
                , args_cmd[numOfArgs - 1], *this);
        redirect_cmd->execute();
        numOfArgs = numOfArgs - 2;
        return createCommandHandler(args_cmd, cmd_line, numOfArgs + start, is_bg_cmd, is_pipe_cmd);
    }
    return createCommandHandler(args_cmd, cmd_line, numOfArgs + start, is_bg_cmd, is_pipe_cmd);
}


std::unique_ptr<Command> SmallShell::build_alias(string args_cmd[], int numOfArgs){
    std::string full_alias_cmd; // Building the full command again :(
    for (int i = 0; i < numOfArgs - 1; i++) {
        full_alias_cmd += args_cmd[i] + " ";
    }
    full_alias_cmd += args_cmd[numOfArgs - 1];

    std::regex pat(R"(^alias ([A-Za-z0-9_]+)='([^']*)'$)");
    std::smatch result;
    if(numOfArgs == 1){
        return make_unique<AliasCommand>("", "", args_cmd, numOfArgs, *this);
    }
    if(std::regex_match(full_alias_cmd, result, pat)){
        std::string alias = result[1];
        std::string cmd = result[2];
        bool exists = std::find(RESERVED_COMMANDS.begin(), RESERVED_COMMANDS.end(), alias) != RESERVED_COMMANDS.end();
        if (this->m_name_to_cmd.find(alias) == this->m_name_to_cmd.end() && !exists) {
            return make_unique<AliasCommand>(cmd, alias, args_cmd, numOfArgs, *this);
        }
        else {
            std::cerr << "smash error: " << alias << " already exists or is a reserved word" << std::endl;
        }
    }
    else{
        std::cerr<< "smash error: alias: invalid alias format" << std::endl;
    }
    return nullptr;
}