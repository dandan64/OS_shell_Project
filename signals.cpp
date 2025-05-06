#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    std::cout << "smash: got ctrl-C" << std::endl;
    pid_t pid = SmallShell::getInstance().getFgProcPID();
    if (pid){
        if(kill(pid, SIGKILL) == 0){;
            std::cout << "smash: process " << pid << " was killed" << std::endl;
            return;
        }
        else {
            std::cout << "smash error: kill failed" << std::endl;
        }
    }
}
