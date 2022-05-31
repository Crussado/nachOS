/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "executable.hh"
#include "transfer.hh"
#include "threads/system.hh"

#include <string.h>
#include<cstdlib>
#include <stdio.h>

int PickVictim() {
    return rand() % NUM_PHYS_PAGES;
}

/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *executable_file, Thread *hilo, char **args)
{
    ASSERT(executable_file != nullptr);
    exe = new Executable (executable_file);
    ASSERT(exe->CheckMagic());

    // How big is address space?
    unsigned int size = exe->GetSize() + USER_STACK_SIZE;
      // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);

    // First, set up the translation.
    pageTable = new TranslationEntry[numPages];

    #ifndef DEMAND_LOADING
        lockCoremap->Acquire();
        if(usedPages->CountClear() < numPages){
          DEBUG('z', "No hay suficientes paginas fisicas disponibles.\n");
        }
    #endif
    for (unsigned i = 0; i < numPages; i++) {
        #ifndef DEMAND_LOADING
            pageTable[i].physicalPage = usedPages->Find(i, currentThread);
        #endif
        #ifdef DEMAND_LOADING
            pageTable[i].physicalPage = NOT_ALLOCATE_VALUE;
        #endif
        pageTable[i].virtualPage  = i;
          // For now, virtual page number = physical page number.
        pageTable[i].valid        = true;
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
          // If the code segment was entirely on a separate page, we could
          // set its pages to be read-only.
    }
    #ifdef SWAP
        char filename[100];
        sprintf(filename, "SWAP.%u", hilo);
        fileSystem->Create(filename, PAGE_SIZE * numPages);
        swap = fileSystem->Open(filename);
    #endif
    // #ifndef DEMAND_LOADING
    //     lockCoremap->Release();
    //     char *mainMemory = machine->GetMMU()->mainMemory;

    //     // Zero out the entire address space, to zero the unitialized data
    //     // segment and the stack segment.
    //     for(unsigned int i = 0; i < numPages; i++)
    //         memset(mainMemory + pageTable[i].physicalPage * PAGE_SIZE, 0, PAGE_SIZE);

    //     // Then, copy in the code and data segments into memory.
    //     uint32_t physicalAddr;
    //     uint32_t virtualAddr;
    //     unsigned int cantRead;
    //     unsigned int offset;
    //     uint32_t codeSize = exe->GetCodeSize();
    //     uint32_t initDataSize = exe->GetInitDataSize();
    //     if (codeSize > 0) {
    //         virtualAddr = exe->GetCodeAddr();
    //         cantRead = 0;
    //         offset = 0;
    //         for(;codeSize != 0;) {
    //             physicalAddr = TranslateAddr(virtualAddr);
    //             if(codeSize / PAGE_SIZE >= 1) {
    //                 cantRead = PAGE_SIZE;
    //             } else {
    //                 cantRead = codeSize;
    //             }
    //             DEBUG('z', "Initializing code segment, at 0x%X, size %u\n",
    //                   physicalAddr, cantRead);
    //             exe->ReadCodeBlock(&mainMemory[physicalAddr], cantRead, offset);
    //             codeSize -= cantRead;
    //             offset += cantRead;
    //             virtualAddr += cantRead;
    //         }
    //     }
    //     if (initDataSize > 0) {
    //         cantRead = 0;
    //         offset = 0;
    //         uint32_t nextPage;
    //         uint32_t space;
    //         for(; initDataSize != 0;) {
    //             physicalAddr = TranslateAddr(virtualAddr);
    //             nextPage = (GetPhyPage(virtualAddr) + 1) * PAGE_SIZE;
    //             space = nextPage - physicalAddr;
    //             if(space == PAGE_SIZE) {
    //                 if(initDataSize / PAGE_SIZE >= 1) {
    //                     cantRead = PAGE_SIZE;
    //                 } else {
    //                     cantRead = initDataSize;
    //                 }
    //             } else {
    //                 cantRead = space > initDataSize ? initDataSize : space; 
    //             cantRead = space > initDataSize ? initDataSize : space; 
    //                 cantRead = space > initDataSize ? initDataSize : space; 
    //             }
    //             DEBUG('z', "Initializing data segment, at 0x%u, size %u\n",
    //                   physicalAddr, cantRead);
    //             exe->ReadDataBlock(&mainMemory[physicalAddr], cantRead, offset);
    //             initDataSize -= cantRead;
    //             offset += cantRead;
    //             virtualAddr += cantRead;
    //         }
    //     }
    // #endif
    thread = hilo;
    initsArgs = args;
}

/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
    for (unsigned i = 0; i < numPages; i++) {
        usedPages->Clear(pageTable[i].physicalPage);
    }
    #ifdef SWAP
        delete swap;
        char filename[100];
        sprintf(filename, "SWAP.%u", this);
        fileSystem->Remove(filename);
    #endif
    delete exe;
    delete initsArgs;
    delete [] pageTable;
}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void
AddressSpace::InitRegisters()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++) {
        machine->WriteRegister(i, 0);
    }

    // Initial program counter -- must be location of `Start`.
    machine->WriteRegister(PC_REG, 0);

    // Need to also tell MIPS where next instruction is, because of branch
    // delay possibility.
    machine->WriteRegister(NEXT_PC_REG, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we do not
    // accidentally reference off the end!
    machine->WriteRegister(STACK_REG, (numPages - 1) * PAGE_SIZE - 16);
    DEBUG('z', "Initializing stack register to 0x%X\n",
          numPages * PAGE_SIZE - 16);
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void
AddressSpace::SaveState()
{
    TranslationEntry *tlb = machine->GetMMU()->tlb;
    unsigned int vpn;
    for(unsigned int i = 0; i < TLB_SIZE; i++){
        vpn = tlb[i].virtualPage;
        if(tlb[i].valid) {
            pageTable[vpn].dirty = tlb[i].dirty;
            pageTable[vpn].use = tlb[i].use;
            pageTable[vpn].physicalPage = tlb[i].physicalPage;
            pageTable[vpn].valid = tlb[i].valid;
            pageTable[vpn].readOnly = tlb[i].readOnly;
        }
    }
}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void
AddressSpace::RestoreState()
{
    // machine->GetMMU()->pageTable     = pageTable;
    // machine->GetMMU()->pageTableSize = numPages;
    TranslationEntry *tlb = machine->GetMMU()->tlb;
    for(unsigned int i = 0; i < TLB_SIZE; i++)
        tlb[i].valid = false;
}

Thread *
AddressSpace::GetThread() {
  return thread;
}

char **
AddressSpace::GetInitsArgs() {
  return initsArgs;
}

uint32_t
AddressSpace::TranslateAddr(uint32_t virtualAddr) {
    uint32_t page = GetPhyPage(virtualAddr);
    uint32_t offset = GetOffset(virtualAddr);

    return (page * PAGE_SIZE) + offset;
}

uint32_t
AddressSpace::GetPhyPage(uint32_t virtualAddr) {
    return pageTable[virtualAddr / PAGE_SIZE].physicalPage;
}

uint32_t
AddressSpace::GetOffset(uint32_t virtualAddr) {
    return virtualAddr % PAGE_SIZE;
}

TranslationEntry *
AddressSpace::GetTranslate(unsigned int vpn) {
    return &pageTable[vpn];
}

void
AddressSpace::MarkSwap(unsigned int vpn) {
    pageTable[vpn].physicalPage = SWAP_VALUE;
}

void
AddressSpace::ApplySwap() {
    int victim = PickVictim();
    Thread *t = usedPages->GetThread(victim);
    int vpn = usedPages->GetVPN(victim);
    usedPages->Clear(victim);
    OpenFile *swapFile = t->space->swap;
    char *mainMemory = machine->GetMMU()->mainMemory;
    DEBUG('z', "Swaping virtual page %d from 0x%X\n", vpn, t);
    swapFile->WriteAt(&mainMemory[victim * PAGE_SIZE], PAGE_SIZE, PAGE_SIZE * vpn);
    t->space->MarkSwap(vpn);
    if(t == currentThread) {
        for(unsigned int i = 0; i < TLB_SIZE; i++) {
            if(machine->GetMMU()->tlb[i].virtualPage == vpn && machine->GetMMU()->tlb[i].valid) {
                machine->GetMMU()->tlb[i].valid = false;
                pageTable[vpn].use = machine->GetMMU()->tlb[i].use;
                pageTable[vpn].dirty = machine->GetMMU()->tlb[i].dirty;
                pageTable[vpn].readOnly = machine->GetMMU()->tlb[i].readOnly;
            }
        }
    }
}

void
AddressSpace::ReturnSwap(unsigned int vpn) {
    lockCoremap->Acquire();
    if(!(usedPages->CountClear() >= 1)) {
        DEBUG('z', "No hay paginas fisicas disponibles\n");
        ApplySwap();
    }
    pageTable[vpn].physicalPage = usedPages->Find(vpn, currentThread);
    lockCoremap->Release();
    char *mainMemory = machine->GetMMU()->mainMemory;
    DEBUG('z', "Restoring virtual page %d\n", vpn);
    swap->ReadAt(&mainMemory[pageTable[vpn].physicalPage * PAGE_SIZE], PAGE_SIZE, PAGE_SIZE * vpn);
}

void
AddressSpace::AllocatePage(unsigned int vpn) {
    lockCoremap->Acquire();
    if(usedPages->CountClear() == 0) {
        DEBUG('z', "No hay paginas fisicas disponibles\n");
        #ifdef SWAP
            ApplySwap();
        #else
            ASSERT(false);
        #endif
    }
    DEBUG('z', "ALLOCATE\n");
    pageTable[vpn].physicalPage = usedPages->Find(vpn, currentThread);
    lockCoremap->Release();
    unsigned int toAllocate = PAGE_SIZE;
    unsigned int cantRead;
    unsigned int virtualAddr = vpn * PAGE_SIZE;
    unsigned int phyAddr = TranslateAddr(virtualAddr);
    char *mainMemory = machine->GetMMU()->mainMemory;
    uint32_t codeSize = exe->GetCodeSize();
    uint32_t dataSize = exe->GetInitDataSize();
    uint32_t dataAddr = exe->GetInitDataAddr();

    memset(&mainMemory[phyAddr], 0, PAGE_SIZE);

    if (virtualAddr < codeSize) {
        unsigned int space = codeSize - virtualAddr;
        cantRead = space < PAGE_SIZE ? space : PAGE_SIZE;
        DEBUG('z', "Initializing code segment, at 0x%X, size %u\n",
                phyAddr, cantRead);
        DEBUG('z', "codesize %u, vpn %u\n", codeSize, vpn);
        exe->ReadCodeBlock(&mainMemory[phyAddr], cantRead, virtualAddr);
        toAllocate -= cantRead;
        virtualAddr += cantRead;
    }
    unsigned int endData = codeSize + dataSize;
    if (toAllocate > 0 && dataSize > 0 && virtualAddr < endData) {
        phyAddr = TranslateAddr(virtualAddr);
        unsigned int nextPage = (pageTable[vpn].physicalPage + 1) * PAGE_SIZE;
        unsigned int space = nextPage - phyAddr;
        if(space == PAGE_SIZE) {
            if(toAllocate == PAGE_SIZE) {
                cantRead = PAGE_SIZE;
            } else {
                cantRead = toAllocate;
            }
        } else {
            cantRead = space > toAllocate ? toAllocate : space; 
        }
        DEBUG('z', "Initializing data segment, at 0x%u, size %u\n",
                phyAddr, cantRead);
        exe->ReadDataBlock(&mainMemory[phyAddr], cantRead, virtualAddr - dataAddr);
    }
}
