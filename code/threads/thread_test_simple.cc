/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_simple.hh"
#include "system.hh"
#include "semaphore.hh"

#include <stdio.h>
#include <string.h>

#ifdef SEMAPHORE_TEST
    static Semaphore *semaphore = nullptr;
#endif

/// Loop 10 times, yielding the CPU to another ready thread each iteration.
///
/// * `name` points to a string with a thread name, just for debugging
///   purposes.
void
SimpleThread(void *name_)
{
    // Reinterpret arg `name` as a string.
    char *name = (char *) name_;

    // If the lines dealing with interrupts are commented, the code will
    // behave incorrectly, because printf execution may cause race
    // conditions.
    for (unsigned num = 0; num < 10; num++) {
        #ifdef SEMAPHORE_TEST
            semaphore->P();
            DEBUG('t', "el hilo %s decrementa el semaforo\n", name);
        #endif

        printf("*** Thread `%s` is running: iteration %u\n", name, num);
        currentThread->Yield();

        #ifdef SEMAPHORE_TEST
            semaphore->V();
            DEBUG('t', "el hilo %s incrementa el semaforo\n", name);
        #endif
    }
    printf("!!! Thread `%s` has finished\n", name);
}

/// Set up a ping-pong between several threads.
///
/// Do it by launching one thread which calls `SimpleThread`, and finally
/// calling `SimpleThread` on the current thread.
void
ThreadTestSimple()
{   
    #ifdef SEMAPHORE_TEST
        semaphore = new Semaphore("semaphoreSimpleTest", 3);
    #endif

    unsigned cant_thread = 4;
    const char *name[cant_thread] = {"2nd", "3rd", "4th", "5th"};
    Thread *newThread[cant_thread];

    for(unsigned num = 0; num < cant_thread; num++) {
        newThread[num] = new Thread(name[num]);
        newThread[num]->Fork(SimpleThread, (void *) name[num]);
    }

    SimpleThread((void *) "1st");

    #ifdef SEMAPHORE_TEST
        delete semaphore;
    #endif
}
 