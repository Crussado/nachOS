#include "thread_test_prod_cons.hh"
#include "system.hh"
#include "../lib/debug.hh"
#include "../lib/assert.hh"
#include <stdio.h>


static const int CANT_HIJOS = 10;
static const int TIEMPO = 100;


static void
Hijo(void *n_) {
    unsigned *n = (unsigned *) n_;
    for (unsigned i = 0; i < TIEMPO; i++) {
        currentThread->Yield();
    }
    printf("Hijo %u finished.\n", *n);
}

void
ThreadTestJoin()
{
    Thread *t[CANT_HIJOS];
    for (unsigned i = 0; i < CANT_HIJOS; i++) {
        printf("Launching hijo %u.\n", i);
        char *name = new char [16];
        sprintf(name, "hijo %u", i);
        unsigned *n = new unsigned;
        *n = i;
        t[i] = new Thread(name, true);
        t[i]->Fork(Hijo, (void *) n);
    }

    for (unsigned i = 0; i < CANT_HIJOS; i++) {
        t[i]->Join();
    }

    printf("El padre se va a dormir.\n");
}
