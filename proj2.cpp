//
// Created by alexander on 9/9/21.
//

#include <iostream>
using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/mman.h>

int BUFSIZE = 1024;

main(int argc, char *argv[])
{
    char buf[BUFSIZE];
    int fdIn, cnt, i;
    char *pchFile;
    struct stat sb;

    if (argc < 2) {
        fdIn = 0; 

    } else if ((fdIn = open(argv[1], O_RDONLY)) < 0) {
        cerr << "file open\n";
        exit(1);
    }

    // Set target character
    char target;
    if(argc >= 3){
        target = *argv[2];
    } else {
        exit(0);
    }

    bool usingMmap = false;

    // Check for mmap, parallel processes, or specified buffer read side
    int totalProcesses = -1;
    if(argc >= 4){
        if(strcmp(argv[3], "mmap") == 0){
            usingMmap = true;

        } else if(argv[3][0] == 'p' ) {
            usingMmap = true;
            totalProcesses = atoi(&argv[3][1]);
            if(totalProcesses > 16){
                cout << "Too many processes required." << "\n";
                exit(0);
            }
            //cout << totalProcesses << "\n";

        } else {
            BUFSIZE = atoi(argv[3]);
        }
    }

    //cout << target << "\n";

    int count = 0;
    int fileSize = 0;
    if(!usingMmap) {

        // Loop through file using read
        while ((cnt = read(fdIn, buf, BUFSIZE)) > 0) {
            fileSize += cnt;
            for (int i = 0; i < cnt; i++) {
                if (buf[i] == target) {
                    count++;
                }
            }
        }
    } else {

        if(fstat(fdIn, &sb) < 0){
            perror("Could not stat file to obtain its size");
            exit(1);
        }

        // Map to memory
        if ((pchFile = (char *) mmap (NULL, sb.st_size, PROT_READ, MAP_SHARED, fdIn, 0))
            == (char *) -1)	{
            perror("Could not mmap file");
            exit (1);
        }

        // Single MMAP
        if(totalProcesses == -1) {
            for (int i = 0; i < sb.st_size; i++) {
                fileSize++;
                if (pchFile[i] == target) {
                    count++;
                }
            }

        // Parallel processes
        } else {
            pid_t pids[totalProcesses];

            int processSplit = sb.st_size / totalProcesses;
            //cout << "size"<< processSplit << "\n";

            // Fork off processes
            for (int i = 0; i < totalProcesses; i++) {
                if ((pids[i] = fork()) < 0) {
                    perror("fork");
                    abort();

                // In child
                } else if (pids[i] == 0) {

                    int count = 0;
                    int end = (i+1) * processSplit;

                    if(i == totalProcesses-1){
                        end = sb.st_size;
                    }

                    // loop through portion of the file
                    for (int j = i * processSplit; j < end; j++) {
                        if (pchFile[j] == target) {
                            count++;
                        }
                    }

                    // print occurrences found by this process
                    cout << "Process Id: " << getpid() << " found " << count << " occurrences of " << "'" << target << "'" << " from bytes " << i * processSplit << " to " << end-1 << "\n\n";
                    cout.flush();
                    exit(0);
                }
            }
        }

        // Unmap memory
        if(munmap(pchFile, sb.st_size) < 0){
            perror("Could not unmap memory");
            exit(1);
        }
    }

    // Wait for all Processes to finish
    while(totalProcesses > 0){
        int pid = wait(0);
        //cout << pid << "is done \n";
        totalProcesses--;
    }

    // Output occurences if not doing parallel
    if(totalProcesses == -1){
        cout << "File size: " << fileSize << " bytes.\n";
        cout << "Occurrences of the character " << "'" <<target << "'" <<  ": "<< count << "\n";
        cout.flush();
    } else {
        cout << "File size: " << sb.st_size << " bytes.\n";
        cout.flush();
    }

    // Close File
    if (fdIn > 0)
        close(fdIn);
}