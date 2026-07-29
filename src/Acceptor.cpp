#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddr.h"

Acceptor::Acceptor(EventLoop *loop, const InetAddr &listenAddr, bool reuseport)
    : loop_(loop),
      acceptSocket_(Socket::createNoblockingOrDie(listenAddr.family())),
      acceptChannel_(loop, acceptSocket_.fd()), listening_(false) {}