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
    delete listenerLock;
    delete writerLock;
    delete messagesLock;
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
    listenerLock->Acquire();
    DEBUG('t', "Hay %i listeners.\n", listener);
    if(listener <=0) {
        writerLock->Acquire();
        writer ++;
        DEBUG('t', "Hay %i writers.\n", writer);
        writerLock->Release();
        listenerLock->Release();
        DEBUG('t', "Se espera condition writer.\n");
        conditionWriter->Wait();
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
    writerLock->Acquire();
    DEBUG('t', "Hay %i writers.\n", writer);
    if(writer > 0) {
        writer --;
        DEBUG('t', "Se despierta un writer.\n");
        conditionWriter->Signal();
        writerLock->Release();
    }
    else {
        listenerLock->Acquire();
        listener ++;
        listenerLock->Release();
        writerLock->Release();
    }

    DEBUG('t', "Se espera listener.\n");
    conditionListener->Wait();

    DEBUG('t', "Se obtiene mensaje.\n");
    messagesLock->Acquire();
    *message = messages->Pop();
    messagesLock->Release();
}
