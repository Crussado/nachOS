/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2007-2009 Universidad de Las Palmas de Gran Canaria.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "thread_test_prod_cons.hh"
#include "lock.hh"
#include "condition.hh"
#include "system.hh"
#include "../lib/debug.hh"
#include "../lib/assert.hh"
#include <stdio.h>

static const int MAX_PANES = 10;
static const unsigned CANT_PERS = 2;
static const unsigned CANT_ACCIONES = 50;
static int cant_panes = 0;
static Lock *lock = nullptr;
static Condition *conditionPanadero = nullptr;
static Condition *conditionCliente = nullptr;
static bool donePanaderos[CANT_PERS];
static bool doneClientes[CANT_PERS];

static void
Cliente(void *n_) {
    unsigned *n = (unsigned *) n_;
    bool consume = false;
    for (unsigned i = 0; i < CANT_ACCIONES; i++) {
        consume = false;
        while(!consume){
            lock->Acquire();
            if(cant_panes == 0) {
                lock->Release();
                conditionCliente->Wait();
            }
            else {
                cant_panes --;
                ASSERT(cant_panes >= 0 && cant_panes <= MAX_PANES);
                DEBUG('t',"Cliente %u comiendo. Quedan %i panes\n", *n, cant_panes);
                conditionPanadero->Signal();
                lock->Release();
                consume = true;
                currentThread->Yield();
            }
        }
    }
    printf("Clientes %u finished.\n", *n);
    doneClientes[*n] = true;
    delete n;
}

static void
Panadero(void *n_) {
    unsigned *n = (unsigned *) n_;
    bool producido = false;
    for (unsigned i = 0; i < CANT_ACCIONES; i++) {
        producido = false;
        while(!producido) {
            lock->Acquire();
            if(cant_panes == MAX_PANES) {
                lock->Release();
                conditionPanadero->Wait();
            }
            else {
                cant_panes ++;
                ASSERT(cant_panes >= 0 && cant_panes <= MAX_PANES);
                DEBUG('t', "Panadero %u produciendo. Quedan %i panes\n", *n, cant_panes);
                lock->Release();
                producido = true;
                currentThread->Yield();
            }
        }
    }
    printf("Panadero %u finished.\n", *n);
    donePanaderos[*n] = true;
    delete n;
}


void
ThreadTestProdCons()
{
    lock = new Lock("LockPanaderia");
    Lock *lock1 = new Lock("LockPanaderia1");
    Lock *lock2 = new Lock("LockPanaderia2");
    conditionCliente = new Condition("CondicionalCliente", lock1);
    conditionPanadero = new Condition("CondicionalPanadero", lock2);

    for (unsigned i = 0; i < CANT_PERS; i++) {
        printf("Launching panadero %u.\n", i);
        char *name = new char [16];
        sprintf(name, "panadero %u", i);
        unsigned *n = new unsigned;
        *n = i;
        Thread *t = new Thread(name);
        t->Fork(Panadero, (void *) n);
    }

    for (unsigned i = 0; i < CANT_PERS; i++) {
        printf("Launching cliente %u.\n", i);
        char *name = new char [16];
        sprintf(name, "cliente %u", i);
        unsigned *n = new unsigned;
        *n = i;
        Thread *t = new Thread(name);
        t->Fork(Cliente, (void *) n);
    }

    for (unsigned i = 0; i < CANT_PERS; i++) {
        while (!donePanaderos[i]) {
            currentThread->Yield();
        }
    }
    for (unsigned i = 0; i < CANT_PERS; i++) {
        while (!doneClientes[i]) {
            currentThread->Yield();
        }
    }
    delete conditionCliente;
    delete conditionPanadero;
    delete lock;
    printf("La panaderia cerro. Quedaron %u panes (should be 0).\n", cant_panes);
}
