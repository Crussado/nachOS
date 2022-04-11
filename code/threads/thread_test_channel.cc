/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_channel.hh"
#include "channel.hh"
#include "system.hh"
#include "../lib/debug.hh"
#include "../lib/assert.hh"
#include <stdio.h>

static const int CANT_PERS = 2;
static const int CANT_MSG = 3;
Channel *channel;

static void
Receiver(void *n_) {
    unsigned *n = (unsigned *) n_;
    int msg;
    for (unsigned i = 0; i < CANT_MSG; i++) {
        DEBUG('t', "Receiver %u a punto de recibir.\n", *n);
        channel->Receive(&msg);
        printf("Receiver %u recibio %i.\n", *n, msg);
        currentThread->Yield();
    }
    printf("Receiver %u finished.\n", *n);
    delete n;
}

static void
Writer(void *n_) {
    unsigned *n = (unsigned *) n_;
    for (int i = 0; i < CANT_MSG; i++) {
        DEBUG('t', "Writer %u a punto de escribir.\n", *n);
        channel->Send(i);
        printf("Writer %u envio %i.\n", *n, i);
        currentThread->Yield();
    }
    printf("Writer %u finished.\n", *n);
    delete n;
}


void
ThreadTestChannel()
{
    channel = new Channel("channelTest");
    Thread *writerers[CANT_PERS];
    Thread *receivers[CANT_PERS];

    for (unsigned i = 0; i < CANT_PERS; i++) {
        printf("Launching writer %u.\n", i);
        char *name = new char [16];
        sprintf(name, "writer %u", i);
        unsigned *n = new unsigned;
        *n = i;
        writerers[i] = new Thread(name, true);
        writerers[i]->Fork(Writer, (void *) n);
    }
    for (unsigned i = 0; i < CANT_PERS; i++) {
        printf("Launching listener %u.\n", i);
        char *name = new char [16];
        sprintf(name, "listener %u", i);
        unsigned *n = new unsigned;
        *n = i;
        receivers[i] = new Thread(name, true);
        receivers[i]->Fork(Receiver, (void *) n);
    }

    for (unsigned i = 0; i < CANT_PERS; i++) {
        receivers[i]->Join();
        writerers[i]->Join();
    }

    delete channel;
    printf("La mensajeria cerro.\n");
}
