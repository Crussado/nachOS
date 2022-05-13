#include "../userprog/syscall.h"
#include "lib.h"

int
main(int argc, char **argv) {
    char *filename = argv[1];
    OpenFileId fp = Open(filename);
    char info[1000];
    Read(info, 1000, fp);
    Close(fp);
    puts(info);
    return 0;
}