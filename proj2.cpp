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
#include <sys/stat.h>
#include <sys/mman.h>

int BUFSIZE = 1024;

main(int argc, char *argv[])
{
    char buf[BUFSIZE];
    int fdIn, cnt, i;
    char *pchFile;
    struct stat sb;

    if (argc < 2) {
        fdIn = 0;  /* just read from stdin */
    }
    else if ((fdIn = open(argv[1], O_RDONLY)) < 0) {
        cerr << "file open\n";
        exit(1);
    }

    char target;
    if(argc >= 3){
        target = *argv[2];
    } else {
        exit(0);
    }

    bool usingMmap = false;

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
            cout << totalProcesses << "\n";
        } else {
            BUFSIZE = atoi(argv[3]);
        }
    }

    cout << target << "\n";

    int count = 0;
    int fileSize = 0;
    if(!usingMmap) {
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

        if ((pchFile = (char *) mmap (NULL, sb.st_size, PROT_READ, MAP_SHARED, fdIn, 0))
            == (char *) -1)	{
            perror("Could not mmap file");
            exit (1);
        }
        if(totalProcesses == -1) {
            for (int i = 0; i < sb.st_size; i++) {
                fileSize++;
                if (pchFile[i] == target) {
                    count++;
                }
            }
        } else {
            pid_t pids[totalProcesses];

            int processSplit = sb.st_size / totalProcesses;

            for (int i = 0; i < totalProcesses; i++) {
                if ((pids[i] = fork()) < 0) {
                    perror("fork");
                    abort();
                } else if (pids[i] == 0) {
                    cout << getpid() << " " << getppid() << " " << i << "\n";
                    cout.flush();

                    int count = 0;
                    for (int j = i * processSplit; j < (i+1) * provessSplit; j++) {
                        if (pchFile[i] == target) {
                            count++;
                        }
                    }
                    cout << getpid() << " " << count;
                    exit(0);
                }
            }
        }

        if(munmap(pchFile, sb.st_size) < 0){
            perror("Could not unmap memory");
            exit(1);
        }
    }
    cout << "File size: " << fileSize << " bytes.\n";
    cout << "Occurrences of the character " << target << ": "<< count << "\n";
    cout.flush();

    if (fdIn > 0)
        close(fdIn);
}