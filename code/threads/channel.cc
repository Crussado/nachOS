#include "channel.hh"
#include "condition.hh"
#include "lock.hh"
#include <queue>

Channel::Channel(const char *debugName)
{
    name = debugName;

    Lock *listenerLock = new Lock("listenerLock");
    Lock *writerLock = new Lock("writerLock");
    Lock *messagesLock = new Lock("messageLock");

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
    writerLock->Release();

    listenerLock->Acquire();
    if(!listener) {
        listenerLock->Release();
        conditionWriter->Wait();
    }
    else {
        listenerLock->Release();
    }

    messagesLock->Acquire();
    messages->Append(message);
    messagesLock->Release();

    writerLock->Acquire();
    writer --;
    writerLock->Release();

    conditionListener->Signal();
}

void
Channel::Receive(int *message)
{   
    listenerLock->Acquire();
    listener ++;
    listenerLock->Release();

    writerLock->Acquire();
    if(writer) {
        writerLock->Release();
        conditionWriter->Signal();
    }
    else {
        writerLock->Release();
    }

    conditionListener->Wait();
    
    messagesLock->Acquire();
    *message = messages->Pop();
    messagesLock->Release();

    listenerLock->Acquire();
    listener --;
    listenerLock->Release();
}
