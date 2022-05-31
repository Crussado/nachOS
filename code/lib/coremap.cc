/// Routines to manage a Coremap -- an array of bits each of which can be
/// either on or off.  Represented as an array of integers.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "coremap.hh"

#include <stdio.h>


/// Initialize a Coremap with `nitems` bits, so that every bit is clear.  It
/// can be added somewhere on a list.
///
/// * `nitems` is the number of bits in the Coremap.
Coremap::Coremap(unsigned nitems)
{
    ASSERT(nitems > 0);

    numEntrys  = nitems;
    // numEntrys = DivRoundUp(numEntrys, BITS_IN_WORD);
    map = new InfoCore [numEntrys];
    for (unsigned i = 0; i < numEntrys; i++) {
        Clear(i);
    }
}

/// De-allocate a Coremap.
Coremap::~Coremap()
{
    delete [] map;
}

/// Set the “nth” bit in a Coremap.
///
/// * `which` is the number of the bit to be set.
void
Coremap::Mark(unsigned which, int vpn, Thread *thread)
{
    ASSERT(which < numEntrys);
    map[which].busy = true;
    map[which].vpn = vpn;
    map[which].thread = thread;
}

/// Clear the “nth” bit in a Coremap.
///
/// * `which` is the number of the bit to be cleared.
void
Coremap::Clear(unsigned which)
{
    ASSERT(which < numEntrys);
    map[which].busy = false;
    map[which].thread = nullptr;
    map[which].vpn = -1;
}

/// Return true if the “nth” bit is set.
///
/// * `which` is the number of the bit to be tested.
bool
Coremap::Test(unsigned which) const
{
    ASSERT(which < numEntrys);
    return map[which].busy == true;
}

/// Return the number of the first bit which is clear.  As a side effect, set
/// the bit (mark it as in use).  (In other words, find and allocate a bit.)
///
/// If no bits are clear, return -1.
int
Coremap::Find(int vpn, Thread *thread)
{
    for (unsigned i = 0; i < numEntrys; i++) {
        if (!Test(i)) {
            Mark(i, vpn, thread);
            return i;
        }
    }
    return -1;
}

/// Return the number of clear bits in the Coremap.  (In other words, how many
/// bits are unallocated?)
unsigned
Coremap::CountClear() const
{
    unsigned count = 0;

    for (unsigned i = 0; i < numEntrys; i++) {
        if (!Test(i)) {
            count++;
        }
    }
    return count;
}

/// Print the contents of the Coremap, for debugging.
///
/// Could be done in a number of ways, but we just print the indexes of all
/// the bits that are set in the Coremap.
void
Coremap::Print() const
{
    printf("Coremap bits set:\n");
    for (unsigned i = 0; i < numEntrys; i++) {
        if (Test(i)) {
            printf("%u ", i);
        }
    }
    printf("\n");
}

Thread *
Coremap::GetThread(unsigned which) {
    return map[which].thread;
}

int
Coremap::GetVPN(unsigned which) {
    return map[which].vpn;
}
