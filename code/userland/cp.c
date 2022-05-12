#include "userprog/syscall.h"

int
main(int argc, char **argv) {
    char *filename = argv[1];
    char *copyFilename = argv[2];
    OpenFileId fp = Open(filename);
    char info[1000];
    int cant = Read(info, 1000, fp);
    Close(fp);
    Create(copyFilename);
    fp = Open(copyFilename);
    Write(info, cant, fp);
    Close(fp);
    return 0;
}
