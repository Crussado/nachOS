#include "channel.hh"
#include "condition.hh"
#include "lock.hh"
#include <stdio.h>
#include <queue>

Channel::Channel(const char *debugName)
{
    name = debugName;

    listenerLock = new Lock("listenerLock");
    writerLock = new Lock("writerLock");
    messagesLock = new Lock("messageLock");

    Lock *lock1 = new Lock("Lock1");
    Lock *lock2 = new Lock("Lock2");
    conditionListener = new Condition("CondicionalListener", lock1);
    conditionWriter = new Condition("CondicionalWriter", lock2);
    
    listener = 0;
    writer = 0;

    messages = new List<int>;
}

Channel::~Channel()
{
    delete conditionListener;
    delete conditionWriter;
    delete messages;
    return;
}

const char *
Channel::GetName() const
{
    return name;
}

void
Channel::Send(int message)
{
    writerLock->Acquire();
    writer ++;
    DEBUG('t', "Hay %i writers.\n", writer);
    writerLock->Release();

    listenerLock->Acquire();
    DEBUG('t', "Hay %i listeners.\n", listener);
    if(!listener) {
        listenerLock->Release();
        DEBUG('t', "Se espera condition writer.\n");
        conditionWriter->Wait();
        listenerLock->Acquire();
        listener --;
        listenerLock->Release();
    }
    else {
        listener--;
        listenerLock->Release();
    }

    DEBUG('t', "Se encola mensaje.\n");
    messagesLock->Acquire();
    messages->Append(message);
    messagesLock->Release();

    DEBUG('t', "Se hace signal de listener.\n");
    conditionListener->Signal();
}

void
Channel::Receive(int *message)
{    
    listenerLock->Acquire();
    listener ++;
    listenerLock->Release();

    writerLock->Acquire();
    DEBUG('t', "Hay %i writers.\n", writer);
    if(writer) {
        writerLock->Release();
        DEBUG('t', "Se despierta un writer.\n");
        conditionWriter->Signal();
    }
    else {
        writerLock->Release();
    }

    DEBUG('t', "Se espera listener.\n");
    conditionListener->Wait();
    writerLock->Acquire();
    writer --;
    writerLock->Release();

    DEBUG('t', "Se obtiene mensaje.\n");
    messagesLock->Acquire();
    *message = messages->Pop();
    messagesLock->Release();
}
