#include "../userprog/syscall.h"

int
main(int argc, char **argv) {
    char *filename = argv[1];
    Remove(filename);
    return 0;
}
