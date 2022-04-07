/// Routines for synchronizing threads.
///
/// The implementation for this primitive does not come with base Nachos.
/// It is left to the student.
///
/// When implementing this module, keep in mind that any implementation of a
/// synchronization routine needs some primitive atomic operation.  The
/// semaphore implementation, for example, disables interrupts in order to
/// achieve this; another way could be leveraging an already existing
/// primitive.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "condition.hh"
#include "semaphore.hh"
#include "lock.hh"

/// Dummy functions -- so we can compile our later assignments.
///
/// Note -- without a correct implementation of `Condition::Wait`, the test
/// case in the network assignment will not work!

Condition::Condition(const char *debugName, Lock *conditionLock)
{
    name = debugName;
    queue = new List<Semaphore *>;
    lock = conditionLock;
}

Condition::~Condition()
{
    delete queue;
    delete lock;
}

const char *
Condition::GetName() const
{
    return name;
}

void
Condition::Wait()
{
    Semaphore *semaphore = new Semaphore("semaphore", 0);

    lock->Acquire();
    queue->Append(semaphore);
    semaphore->V();
    lock->Release();

    delete semaphore;
}

void
Condition::Signal()
{
    Semaphore *semaphore = nullptr;

    lock->Acquire();
    semaphore = queue->Pop();
    lock->Release();

    if(semaphore) {
        semaphore->P();
    }
}

void
Condition::Broadcast()
{
    Semaphore *semaphore;
    lock->Acquire();
    while(queue->IsEmpty()) {
        semaphore = queue->Pop();
        semaphore->P();
    }
    lock->Release();
}
