#ifndef NACHOS_LIB_COREMAP__HH
#define NACHOS_LIB_COREMAP__HH


#include "utility.hh"

class Thread;
class InfoCore {
    public:
        bool busy;
        int vpn;
        Thread *thread;
};

class Coremap {
public:

    /// Initialize a bitmap with `nitems` bits; all bits are cleared.
    ///
    /// * `nitems` is the number of items in the bitmap.
    Coremap(unsigned nitems);

    /// Uninitialize a Coremap.
    ~Coremap();

    /// Set the “nth” bit.
    void Mark(unsigned which, int vpn, Thread *thread);

    /// Clear the “nth” bit.
    void Clear(unsigned which);

    /// Is the “nth” bit set?
    bool Test(unsigned which) const;

    /// Return the index of a clear bit, and as a side effect, set the bit.
    ///
    /// If no bits are clear, return -1.
    int Find(int vpn, Thread *thread);

    /// Return the number of clear bits.
    unsigned CountClear() const;

    /// Print contents of Coremap.
    void Print() const;

    Thread *GetThread(unsigned which);
    int GetVPN(unsigned which);

private:

    /// Number of bits in the Coremap.
    unsigned numEntrys;
    /// Bit storage.
    InfoCore *map;

};

#endif
