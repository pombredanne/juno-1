// Copyright (c) 2013 dacci.org

#ifndef JUNO_APP_SERVICE_H_
#define JUNO_APP_SERVICE_H_

#include <windows.h>

struct sockaddr;

class AsyncSocket;
class AsyncDatagramSocket;

class Service {
 public:
  struct Datagram {
    Service* service;
    AsyncDatagramSocket* socket;
    int data_length;
    void* data;
    int from_length;
    sockaddr* from;
  };

  virtual ~Service() {
  }

  virtual void Stop() = 0;

  virtual bool OnAccepted(AsyncSocket* client) = 0;
  virtual bool OnReceivedFrom(Datagram* datagram) = 0;
  virtual void OnError(DWORD error) = 0;
};

#endif  // JUNO_APP_SERVICE_H_