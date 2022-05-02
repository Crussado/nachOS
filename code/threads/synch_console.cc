#include "synch_console.hh"
#include "machine/console.hh"
#include <stdio.h>

void
SynchConsole::ReadAvail(void *arg) {
    readAvail->V();
}

void
SynchConsole::WriteDone(void *arg) {
    writeDone->V();
}

SynchConsole::SynchConsole(const char *debugName)
{
    name=debugName;
    lockRead = new Lock("synch console lock read");
    lockWrite = new Lock("synch console lock write");
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    console = new Console(stdin, stdout, ReadAvail, WriteDone, 0);
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
