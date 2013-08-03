// Copyright (c) 2013 dacci.org

#ifndef JUNO_NET_HTTP_HTTP_PROXY_H_
#define JUNO_NET_HTTP_HTTP_PROXY_H_

#include <list>
#include <string>
#include <vector>

#include "net/async_server_socket.h"
#include "net/http/http_proxy_session.h"

class HttpProxy : public AsyncServerSocket::Listener {
 public:
  HttpProxy(const char* address, const char* port);
  ~HttpProxy();

  bool Start();
  void Stop();
  void EndSession(HttpProxySession* session);

  void OnAccepted(AsyncServerSocket* server, AsyncSocket* client, DWORD error);

 private:
  std::string address_;
  std::string port_;
  madoka::net::AddressInfo resolver_;
  std::vector<AsyncServerSocket*> servers_;

  HANDLE empty_event_;
  CRITICAL_SECTION critical_section_;
  std::list<HttpProxySession*> sessions_;
};

#endif  // JUNO_NET_HTTP_HTTP_PROXY_H_
