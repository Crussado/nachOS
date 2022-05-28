/// Copyright (c) 2019-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "transfer.hh"
#include "lib/utility.hh"
#include "threads/system.hh"
#include "lib/assert.hh"


void ReadBufferFromUser(int userAddress, char *outBuffer,
                        unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outBuffer != nullptr);
    ASSERT(byteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        bool success = false;
        for(unsigned i = 0; i < TRIES && !success; i++)
            success = machine->ReadMem(userAddress, 1, &temp);
        if(!success) {
            DEBUG('a', "Direccion de memoria invalida\n");
            ASSERT(false);
        }
        *outBuffer = (unsigned char) temp;
        outBuffer++;
        userAddress++;
    } while (count < byteCount);
}

bool ReadStringFromUser(int userAddress, char *outString,
                        unsigned maxByteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outString != nullptr);
    ASSERT(maxByteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        bool success = false;
        for(unsigned int i = 0; i < TRIES && !success; i++){
            success = machine->ReadMem(userAddress, 1, &temp);
            DEBUG('a', "ENTRADA NRO: %u\n", i);
        }
        if(!success) {
            DEBUG('a', "Direccion de memoria invalida\n");
            ASSERT(false);
        }
        *outString = (unsigned char) temp;
        userAddress++;
    } while (*outString++ != '\0' && count < maxByteCount);

    return *(outString - 1) == '\0';
}

void WriteBufferToUser(const char *buffer, int userAddress,
                       unsigned byteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(buffer != nullptr);
    ASSERT(byteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        bool success = false;
        for(unsigned i = 0; i < TRIES && !success; i++)
            success = machine->WriteMem(userAddress, 4, (int) *buffer);
        if(!success) {
            DEBUG('a', "Direccion de memoria invalida\n");
            ASSERT(false);
        }
        buffer ++;
        userAddress++;
    } while (count < byteCount);
}

void WriteStringToUser(const char *string, int userAddress)
{
    ASSERT(userAddress != 0);
    ASSERT(string != nullptr);

    unsigned count = 0;
    do {
        int temp;
        count++;
        bool success = false;
        for(unsigned i = 0; i < TRIES && !success; i++)
            success = machine->WriteMem(userAddress, 1, (int) *string);
        if(!success) {
            DEBUG('a', "Direccion de memoria invalida\n");
            ASSERT(false);
        }
        string ++;
        userAddress++;
    } while (*(string - 1) != '\0');
}
