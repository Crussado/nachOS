#include "channel.hh"
#include "condition.hh"
#include "lock.hh"

Channel::Channel(const char *debugName)
{
    name = debugName;

    Lock *listenerLock = new Lock("listenerLock");
    Lock *writerLock = new Lock("writerLock");
    Lock *messageLock = new Lock("messageLock");

    Lock *lock1 = new Lock("Lock1");
    Lock *lock2 = new Lock("Lock2");
    conditionListener = new Condition("CondicionalListener", lock1);
    conditionWriter = new Condition("CondicionalWriter", lock2);
    
    listener = false;
    writer = false;
}

Channel::~Channel()
{
    delete conditionListener;
    delete conditionWriter;
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
    writer = true;
    if(!listener) {
        conditionWriter->Wait();
    }

    this->message = message;
    writer = false;
    conditionListener->Broadcast();
    // se envia el mensaje
}

void
Channel::Receive(int *message)
{
    listener = true;
    if(writer) {
        conditionWriter->Signal();
    }
    conditionListener->Wait();

    *message = this->message;
    listener = false;
}
