//
// Created by alexander on 8/31/21.
//

/*
 Features Tested in script file
 ./doit <arg>
 - tested on ls
 - tested on ls -a
 - tested on wc foo.txt
 - tested on grep -c "hi" foo.txt
 - tested on date
 - test on date | grep -c Tue

 Basic Shell
 - Tested on ls
 - tested on ls -a
 - tested on wc foo.txt
 - tested on grep -c "hi" foo.txt
 - tested on date
 - tested on change prompt "UwUhello:"
 - tested on cd ..
 - tested on exit
 * all done in succession

 Background Tasks
 - tested sleep 5 & then jobs then ls
 - tested sleep 5 & then attempt to exit
 - tested multiple sleeps and jobs
 - test wc foo.txt & sleep 20 & ls -l & jobs & exit
 */

/*
My SHELL vs. Linux Shell
    When running these commands the majority of the functionality is the same. Obviously linux doesn't provide statistics for every command
    Even the | works, which is pretty cool. The difference mostly appears in the background tasks.
    In linux shell, the task does not state completion, which was a feature I had to implement. However, the output ina background process can still jumble
    with the current shell. Overall the shell that I made isn't perfect and polished like the linux one, but it functionally does the same thing.

 */

#include <iostream>
using namespace std;
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>

char* globalPrompt; // stores current prompt in use
int backgroundProcessCount = 0; // stores the number of background process created in total
vector <int> backgroundProcessIds; // stores the pids of all current background processes
vector <int> backgroundProcessNumbers; // stores the count of each current background process ([1], [2], [3]) for display
vector <string> commandNames; // stores the name of the command for each current background process
vector <timeval> backgroundStartTimes; // stores the start wallclock time for each current background process
vector <rusage> backgroundRUsages; // stores the rusage value for each current background process

extern char **environ;		/* environment info */

double timevalToMilliseconds(timeval t){ // Converts timeval struct to pure milliseconds
    return t.tv_sec * 1000 + t.tv_usec * 0.001;
}

double datetimeToWallclock(timeval start, timeval end){ // converts timeval struct into a wallclock time
    return (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) * 0.001;
}

void printStatistics(rusage usage, timeval startTime, timeval endTime){ // prints all statistics for the process execution of command
    struct timeval userTime = usage.ru_utime;
    struct timeval systemTime = usage.ru_stime;

    cout << "\nStatistics:\n";
    cout << " - System CPU Time Used: " << timevalToMilliseconds(systemTime)<< " milliseconds \n";
    cout << " - User CPU Time Used: " << timevalToMilliseconds(userTime) << " milliseconds \n";
    cout << " - Walk Clock Time Spent: " << datetimeToWallclock(startTime, endTime) << " milliseconds\n";
    cout << " - Involuntary Context Switches: " << usage.ru_nivcsw << "\n";
    cout << " - Voluntary Context Switches: " << usage.ru_nvcsw << "\n";
    cout << " - Major Page Faults: " << usage.ru_majflt << "\n";
    cout << " - Minor Page Faults: " << usage.ru_minflt << "\n";
    cout << " - Maximum Resident Set Size used: " << usage.ru_maxrss << "\n\n";
    cout.flush();
}

void checkBackgroundTaskState(){ // checks the current state of all background tasks
    for(int i = 0; i < backgroundProcessIds.size(); i++){
        if(waitpid(backgroundProcessIds[i], 0, WNOHANG) == backgroundProcessIds[i]){ // if a task has finished execution
            cout << "[" << backgroundProcessNumbers[i] << "] " << backgroundProcessIds[i] << " Completed" << "\n"; // Print Completion
            cout.flush();

            // delete the processes from the current processes
            backgroundProcessIds.erase(backgroundProcessIds.begin() + i);
            backgroundProcessNumbers.erase(backgroundProcessNumbers.begin() + i);
            commandNames.erase(commandNames.begin() + i);

            struct timeval endTime;
            gettimeofday(&endTime, NULL);

            // print statistics of that background process
            printStatistics(backgroundRUsages[i], backgroundStartTimes[i], endTime);
        }
    }
}

// creates a fork and executes a command
void forkAndRunCommand(char **argvNew, rusage *usage, timeval *startTime, timeval *endTime){
    int pid;
    gettimeofday(startTime, NULL);

    if ((pid = fork()) < 0) {
        cerr << "Fork error\n";
        exit(1);
    } else if (pid == 0) {
        /* child process */
        if (execvp(argvNew[0], argvNew) < 0) {
            cerr << "Execvp error\n";
            exit(1);
        }
        getrusage(RUSAGE_SELF, usage);
        cout.flush();

    } else {
        /* parent */
        cout.flush();
        wait(0);       /* wait for the child to finish */
        gettimeofday(endTime, NULL);
    }
}

// creates the shell like behavior
void shellLine(char *prompt){

    char* cmd = new char[32];
    cout << prompt;
    cout.flush();

    checkBackgroundTaskState(); // check on background processes continually

    cin.getline(cmd, 128);
    if(cin.eof()){
        cout << "EOF Detected";
        exit(0);
    }

    checkBackgroundTaskState(); // check on background processes continually

    char *argvNew[32];
    int counter = 0;

    char* args = strtok(cmd, " ");

    // exit if prompted to do so
    if(strcmp(args, "exit") == 0){
        if(backgroundProcessIds.size() == 0){
            exit(0);
        } else {
            cout << "Wait for all background processes to end.\n";
            return;
        }
    }

    // separate into args
    while (args != NULL)
    {
        //printf ("%s\n",args);
        argvNew[counter] = args;
        counter++;
        args = strtok (NULL, " ");
    }

    // handling the set prompt keyword
    if(argvNew[0] != NULL && strcmp(argvNew[0], "set") == 0
        && argvNew[1] != NULL && strcmp(argvNew[1], "prompt") == 0
            && argvNew[2] != NULL && strcmp(argvNew[2], "=") == 0 ) {

        if(argvNew[3] != NULL){
            globalPrompt = argvNew[3];
            return;
        }
    }

    // handling the change directory keyword
    if(argvNew[0] != NULL && strcmp(argvNew[0], "cd") == 0) {
        if(argvNew[1] != NULL){
            char s[100];
            chdir(argvNew[1]);
            printf("Current Directory is Changed to %s.\n", getcwd(s, 100));
            return;
        }
    }

    for(int i = counter; i < 32; i++){
        argvNew[i] = NULL;
    }

    // Background Task
    if(strcmp(argvNew[counter-1], "&") == 0) {

        argvNew[counter-1] = NULL;
        int pid;
        backgroundProcessCount++;

        // structs for stats
        struct rusage backgroundUsage;
        getrusage(RUSAGE_SELF, &backgroundUsage);
        struct timeval backgroundStartTime;
        gettimeofday(&backgroundStartTime, NULL);

        backgroundStartTimes.push_back(backgroundStartTime);

        if ((pid = fork()) < 0) {
            cerr << "Fork error\n";
            exit(1);
        } else if (pid == 0) {
            /* child process */

            // move process group id, so the child becomes run in the background.
            setpgid(0, 0);
            cout.flush();

            if (execvp(argvNew[0], argvNew) < 0) {
                cerr << "Execvp error\n";
                exit(1);
            }

            getrusage(RUSAGE_SELF, &backgroundUsage);

        } else {
            // parent process pushes values for tracking the background tasks progression
            backgroundProcessIds.push_back(pid);
            backgroundProcessNumbers.push_back(backgroundProcessCount);
            commandNames.push_back(argvNew[0]);
            backgroundRUsages.push_back(backgroundUsage);

            checkBackgroundTaskState();
            cout << "[" << backgroundProcessCount << "] " << pid << " " << argvNew[0] << "\n";

        }
        return;
    }

    // jobs functionality
    if(strcmp(argvNew[0], "jobs") == 0){

        // update background tasks, then print all background processes
        checkBackgroundTaskState();
        for(int i = 0; i < backgroundProcessIds.size(); i++){
            cout << "[" << backgroundProcessNumbers[i] << "] " << backgroundProcessIds[i] << " " << commandNames[i] << "\n";
        }
        return;
    }

    // average functionality (no special keywords or background tasks)
    struct timeval startTime;
    struct timeval endTime;

    gettimeofday(&startTime, NULL);
    gettimeofday(&endTime, NULL);

    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    checkBackgroundTaskState();

    forkAndRunCommand(argvNew, &usage, &startTime, &endTime);

    printStatistics(usage, startTime, endTime);
}

main(int argc, char *argv[]) {
/* argc -- number of arguments */
/* argv -- an array of strings */
    // Shell behavior
    if(argc == 1){ // Shell / Part 2
        globalPrompt= "==>";
        while(true){
            shellLine(globalPrompt);
        }

    // one execute
    } else {
        struct timeval startTime;
        struct timeval endTime;

        gettimeofday(&startTime, NULL);
        gettimeofday(&endTime, NULL);

        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);

        char *argvNew[argc];
        for (int i = 1; i < argc; i++) {
            //cout << argv[i] << "\n";
            argvNew[i - 1] = argv[i];
        }
        argvNew[argc - 1] = NULL;

        forkAndRunCommand(argvNew, &usage, &startTime, &endTime);

        printStatistics(usage, startTime, endTime);
    }
}



