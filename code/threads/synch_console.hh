/// Data structures to export a synchronous interface to the raw disk device.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.

#ifndef NACHOS_THREADS_SYNCHCONSOLE__HH
#define NACHOS_THREADS_SYNCHCONSOLE__HH


#include "machine/console.hh"
#include "lock.hh"
#include "semaphore.hh"

class Semaphore;
class Lock;
class SynchConsole {
public:

    SynchConsole(const char *name);

    ~SynchConsole();

    void Read(char* c);
    void Write(char c);

    void fReadAvail();
    void fWriteDone();
private:
    Console *console;  
    Semaphore *readAvail;
    Semaphore *writeDone;
    Lock *lockRead;  
    Lock *lockWrite;
    const char *name;
};


#endif
