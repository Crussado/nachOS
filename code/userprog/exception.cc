/// Entry points into the Nachos kernel from user programs.
///
/// There are two kinds of things that can cause control to transfer back to
/// here from user code:
///
/// * System calls: the user code explicitly requests to call a procedure in
///   the Nachos kernel.  Right now, the only function we support is `Halt`.
///
/// * Exceptions: the user code does something that the CPU cannot handle.
///   For instance, accessing memory that does not exist, arithmetic errors,
///   etc.
///
/// Interrupts (which can also cause control to transfer from user code into
/// the Nachos kernel) are handled elsewhere.
///
/// For now, this only handles the `Halt` system call.  Everything else core-
/// dumps.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "transfer.hh"
#include "syscall.h"
#include "args.hh"
#include "address_space.hh"

#include "filesys/directory_entry.hh"
#include "filesys/open_file.hh"
#include "threads/system.hh"

#include <stdio.h>


static void
IncrementPC()
{
    unsigned pc;

    pc = machine->ReadRegister(PC_REG);
    machine->WriteRegister(PREV_PC_REG, pc);
    pc = machine->ReadRegister(NEXT_PC_REG);
    machine->WriteRegister(PC_REG, pc);
    pc += 4;
    machine->WriteRegister(NEXT_PC_REG, pc);
}

/// Do some default behavior for an unexpected exception.
///
/// NOTE: this function is meant specifically for unexpected exceptions.  If
/// you implement a new behavior for some exception, do not extend this
/// function: assign a new handler instead.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
static void
DefaultHandler(ExceptionType et)
{
    int exceptionArg = machine->ReadRegister(2);

    fprintf(stderr, "Unexpected user mode exception: %s, arg %d.\n",
            ExceptionTypeToString(et), exceptionArg);
    ASSERT(false);
}

void
StartProcess(void *addressSpace)
{
    ASSERT(addressSpace != nullptr);
    AddressSpace *space = (AddressSpace *) addressSpace;
    currentThread->space = space;

    space->InitRegisters();  // Set the initial register values.
    space->RestoreState();   // Load page table register.

    if(space->GetInitsArgs()) {
        unsigned argv = WriteArgs(space->GetInitsArgs());
        machine->WriteRegister(4, (int) argv);
        machine->WriteRegister(5, machine->ReadRegister(STACK_REG));
    }

    machine->Run();  // Jump to the user progam.
    ASSERT(false);   // `machine->Run` never returns; the address space
                     // exits by doing the system call `Exit`.
}

/// Handle a system call exception.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
///
/// The calling convention is the following:
///
/// * system call identifier in `r2`;
/// * 1st argument in `r4`;
/// * 2nd argument in `r5`;
/// * 3rd argument in `r6`;
/// * 4th argument in `r7`;
/// * the result of the system call, if any, must be put back into `r2`.
///
/// And do not forget to increment the program counter before returning. (Or
/// else you will loop making the same system call forever!)
static void
SyscallHandler(ExceptionType _et)
{
    int scid = machine->ReadRegister(2);

    switch (scid) {

        case SC_HALT: {
            DEBUG('e', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
            break;
        }

        case SC_CREATE: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
            }

            DEBUG('e', "`Create` requested for file `%s`.\n", filename);
            if(fileSystem->Create(filename, FILE_NAME_MAX_LEN)) {
                machine->WriteRegister(2, (int) 0);
                DEBUG('e', "Create success.\n");
            }
            else {
                machine->WriteRegister(2, (int) 1);
                DEBUG('e', "Create failed.\n");
            }
            break;
        }

        case SC_REMOVE: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
            }

            DEBUG('e', "`Remove` requested for file `%s`.\n", filename);
            if(fileSystem->Remove(filename)) {
                machine->WriteRegister(2, (int) 0);
                DEBUG('e', "Remove success.\n");
            }
            else {
                machine->WriteRegister(2, (int) 1);
                DEBUG('e', "Remove failed.\n");
            }
            break;
        }

        case SC_EXIT: {
            int status = machine->ReadRegister(4);

            currentThread->Finish(status);
            break;
        }

        case SC_OPEN: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
            }

            DEBUG('e', "`Open` requested for file `%s`.\n", filename);
            OpenFile *fileAddr = fileSystem->Open(filename);
            if(fileAddr == nullptr) {
                machine->WriteRegister(2, (int) -1);
                DEBUG('e', "Error: file couldn't be opened.\n");
            }
            else {
                int key = currentThread->Open(fileAddr);
                machine->WriteRegister(2, key);
                DEBUG('e', "Success: file opened.\n");
            }
            break;
        }

        case SC_CLOSE: {
            int key = machine->ReadRegister(4);
            DEBUG('e', "`Close` requested for id %u.\n", key);
            currentThread->Close(key);
            DEBUG('e', "`Close` requested success.\n");
            break;
        }

        case SC_WRITE: {
            int buff = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            int fid = machine->ReadRegister(6);
            DEBUG('e', "`Write` requested for id %u.\n", fid);
            char content[size + 1];
            if (!ReadStringFromUser(buff,
                                    content, size)) {
            }

            if(fid == CONSOLE_OUTPUT) {
                for(int i = 0; i < size; i++) {
                    synchConsole->Write(content[i]);
                }
                machine->WriteRegister(2, size);
                DEBUG('e', "`Write` Success.\n");
            }
            else {
                
                OpenFile *openFile = currentThread->GetFile(fid);
                int result = openFile->Write(content, size);
                if(result == size) {
                    DEBUG('e', "`Write` Success.\n");
                }
                else {
                    DEBUG('e', "`Write` Error.\n");
                }
                machine->WriteRegister(2, result);
            }

            break;
        }

        case SC_READ: {
            int buff = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            int fid = machine->ReadRegister(6);
            DEBUG('e', "`Read` requested for id %u.\n", fid);
            char buffRead[size];

            if(fid == CONSOLE_INPUT) {
                for(int i = 0; i < size; i++) {
                    synchConsole->Read(&buffRead[i]);
                }
                WriteStringToUser(buffRead, buff);
                machine->WriteRegister(2, size);
            }
            else {
                OpenFile *openFile = currentThread->GetFile(fid);
                int result = openFile->Read(buffRead, size);
                if(result == size) {
                    WriteStringToUser(buffRead, buff);
                    DEBUG('e', "`Read` Success.\n");
                }
                else {
                    DEBUG('e', "`Read` Error.\n");
                }
                machine->WriteRegister(2, result);
            }

            break;
        }

        case SC_EXEC: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
            }

            int addressArgs = machine->ReadRegister(5);
            char **args;
            if (addressArgs == 0) {
                args = nullptr;
            }
            else {
                args = SaveArgs(addressArgs);
            }

            char filename[50 + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
            }

            DEBUG('e', "`Exec` requested for file `%s`.\n", filename);
            OpenFile *fileAddr = fileSystem->Open(filename);
            Thread *son = new Thread(filename, true);
            int key = currentThread->AddSon(son);
            AddressSpace *addressSpace = new AddressSpace(fileAddr, son, args);
            delete fileAddr;
            son->Fork(StartProcess, (void *) addressSpace);

            machine->WriteRegister(2, key);
            DEBUG('e', "Exec success.\n");

            break;
        }

        case SC_JOIN: {
            int spaceInt = machine->ReadRegister(4);
            Thread *son = currentThread->GetSon(spaceInt);
            if(son == nullptr) {
                DEBUG('e', "Join error: invalid space.\n");
            }
            int result = son->Join();
            machine->WriteRegister(2, result);
            DEBUG('e', "Join success.\n");
            break;
        }

        default:
            fprintf(stderr, "Unexpected system call: id %d.\n", scid);
            ASSERT(false);

    }

    IncrementPC();
}


/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void
SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION,            &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION,       &SyscallHandler);
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &DefaultHandler);
    machine->SetHandler(READ_ONLY_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION,      &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}
