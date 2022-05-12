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
SynchConsole::Read(char *buffer, int size)
{
    ASSERT(buffer != nullptr);
    char ch;
    lockRead->Acquire();
    for(int i = 0; i < size; i++) {
        readAvail->P();
        ch = console->GetChar();
        buffer[i] = ch;
        if(ch == '\0') {
            break;
        }
    }
    lockRead->Release();
}

void
SynchConsole::Write(char *buffer)
{
    ASSERT(buffer != nullptr);
    lockWrite->Acquire();
    for(int i = 0; buffer[i] != '\0'; i++) {
        console->PutChar(buffer[i]);
        writeDone->P();
    }
    console->PutChar('\0');
    writeDone->P();
    lockWrite->Release();
}
