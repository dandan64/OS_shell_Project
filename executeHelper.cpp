#include "executeHelper.h"
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
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




void ForegroundCommand::executeFgByID(){
    std::string job_id_s = this->getCmdArgs()[1];
    try {
        int job_id = stoi(job_id_s);
        if (this->m_small_shell.getJobsList().getJobById(job_id) == nullptr) {
            std::cerr << "smash error: fg: job-id " << job_id << " does not exist" << std::endl;
            return;
        }
        auto job = this->m_small_shell.getJobsList().getJobById(job_id);
        std::cout << job->getCommand() << " " << job->getPid() << std::endl;
        this->m_small_shell.setFgProcPID(job->getPid());

        waitpid(job->getPid(), nullptr, 0);

        this->m_small_shell.getJobsList().removeJobById(job->getJobId());
        if(job_id == this->m_small_shell.getJobsList().getMaxId()){
            this->m_small_shell.getJobsList().updateMaxJobId();
        }
        this->m_small_shell.setFgProcPID(0);
    }
    catch(...){
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        return;
    }
}
void ForegroundCommand::executeLastId(){
    if(this->m_small_shell.getJobsList().getJobsList().empty()){
        std::cerr << "smash error: fg: jobs list is empty" << std::endl;
        return;
    }
    auto job = this->m_small_shell.getJobsList().getLastJob();
    this->m_small_shell.setFgProcPID(job->getPid());

    std::cout << job->getCommand() << " " << job->getPid() << std::endl;
    waitpid(job->getPid(), nullptr,0);
    this->m_small_shell.getJobsList().removeJobById(job->getJobId());
    this->m_small_shell.getJobsList().updateMaxJobId();

    this->m_small_shell.setFgProcPID(0);
}



void UnAliasCommand::removeAllAlias(int numOfArgs){
    std::map<string,string>& alias_map = this->m_small_shell.getMap();
    std::vector<std::string>& ordered_vector = this->m_small_shell.getOrder();

    for(int i = 1; i < numOfArgs; ++i){
        bool exists = alias_map.find(this->getCmdArgs()[i]) != alias_map.end();
        string alias_to_remove = this->getCmdArgs()[i];
        if(exists){
            alias_map.erase(alias_to_remove);
            auto alias_to_remove_ord = std::find(ordered_vector.begin(),
                                                 ordered_vector.end(), this->getCmdArgs()[i]);
            ordered_vector.erase(alias_to_remove_ord);
        }
        else{
            std::cerr << "smash error: " + alias_to_remove + " alias does not exist" << std::endl;
            break;
        }
    }
}

void JobsList::updateMaxJobId(){
    this->setMaxId(this->getLastJob()->getJobId());
}
