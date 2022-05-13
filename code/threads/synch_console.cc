#include "synch_console.hh"
#include "machine/console.hh"
#include <stdio.h>

void
SynchConsole::fReadAvail() {
    readAvail->V();
}

void
ReadAvail(void *arg) {
    SynchConsole *synchConsole = (SynchConsole *) arg;
    synchConsole->fReadAvail();
}


void
SynchConsole::fWriteDone() {
    writeDone->V();
}
void
WriteDone(void *arg) {
    SynchConsole *synchConsole = (SynchConsole *) arg;
    synchConsole->fWriteDone();
}

SynchConsole::SynchConsole(const char *debugName)
{
    name=debugName;
    lockRead = new Lock("synch console lock read");
    lockWrite = new Lock("synch console lock write");
    readAvail = new Semaphore("console read avail", 0);
    writeDone = new Semaphore("console write done", 0);
    console = new Console(nullptr, nullptr, ReadAvail, WriteDone, this);
}

SynchConsole::~SynchConsole()
{
    delete lockRead;
    delete lockWrite;
    delete console;
    delete readAvail;
    delete writeDone;
}

void
SynchConsole::Read(char *c)
{
    ASSERT(c != nullptr);
    lockRead->Acquire();
    readAvail->P();
    *c = console->GetChar();
    lockRead->Release();
}

void
SynchConsole::Write(char c)
{
    lockWrite->Acquire();
    console->PutChar(c);
    writeDone->P();
    lockWrite->Release();
}
