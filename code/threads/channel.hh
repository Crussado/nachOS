#ifndef NACHOS_THREADS_CHANNEL__HH
#define NACHOS_THREADS_CHANNEL__HH

#include "condition.hh"
#include "lock.hh"

class Channel {
public:

    Channel(const char *debugName);

    ~Channel();

    const char *GetName() const;

    void Send (int message);
    
    void Receive (int *message);

private:

    const char *name;

    bool listener;
    bool writer;

    int message;

    Condition *conditionListener;
    Condition *conditionWriter;

    Lock *listenerLock;
    Lock *writerLock;
    Lock *messageLock;
    
};


#endif