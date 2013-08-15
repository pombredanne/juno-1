// Copyright (c) 2013 dacci.org

#ifndef JUNO_NET_HTTP_HTTP_PROXY_SESSION_H_
#define JUNO_NET_HTTP_HTTP_PROXY_SESSION_H_

#include <stdint.h>

#include <url/gurl.h>

#include "net/async_socket.h"
#include "net/http/http_request.h"
#include "net/http/http_response.h"
#include "net/http/http_status.h"

class HttpProxy;

class HttpProxySession : public AsyncSocket::Listener {
 public:
  HttpProxySession(HttpProxy* proxy, AsyncSocket* client);
  virtual ~HttpProxySession();

  bool Start();
  void Stop();

  void OnConnected(AsyncSocket* socket, DWORD error);
  void OnReceived(AsyncSocket* socket, DWORD error, int length);
  void OnSent(AsyncSocket* socket, DWORD error, int length);

 private:
  enum Phase {
    Request, RequestBody, Response, ResponseBody, Error,
  };

  enum Event {
    Connected, Received, Sent,
  };

  struct EventArgs {
    Event event;
    HttpProxySession* session;
    AsyncSocket* socket;
    DWORD error;
    int length;
  };

  static const size_t kBufferSize = 4096;
  static const DWORD kTimeout = 15000;

  static const std::string kConnection;
  static const std::string kContentEncoding;
  static const std::string kContentLength;
  static const std::string kExpect;
  static const std::string kKeepAlive;
  static const std::string kProxyAuthenticate;
  static const std::string kProxyAuthorization;
  static const std::string kProxyConnection;
  static const std::string kTransferEncoding;

  bool ReceiveAsync(AsyncSocket* socket, int flags);

  static int64_t ParseChunk(const std::string& buffer, int64_t* chunk_size);

  void ProcessRequestHeader();
  void ProcessResponseHeader();
  void ProcessMessageLength(HttpHeaders* headers);
  void ProcessHopByHopHeaders(HttpHeaders* headers);

  void SendError(HTTP::StatusCode status);
  void EndSession();

  bool FireReceived(AsyncSocket* socket, DWORD error, int length);
  static DWORD CALLBACK FireEvent(void* param);

  void OnRequestReceived(DWORD error, int length);
  void OnRequestSent(DWORD error, int length);

  void OnRequestBodyReceived(DWORD error, int length);
  void OnRequestBodySent(DWORD error, int length);

  void OnResponseReceived(DWORD error, int length);
  void OnResponseSent(DWORD error, int length);

  void OnResponseBodyReceived(DWORD error, int length);
  void OnResponseBodySent(DWORD error, int length);

  static void CALLBACK OnTimeout(void* param, BOOLEAN fired);

  HttpProxy* const proxy_;
  AsyncSocket* client_;
  std::string client_buffer_;
  HttpRequest request_;
  bool tunnel_;
  GURL url_;
  madoka::net::AddressInfo resolver_;

  AsyncSocket* remote_;
  std::string remote_buffer_;
  HttpResponse response_;

  char* buffer_;
  Phase phase_;
  int64_t content_length_;
  bool chunked_;
  int64_t chunk_size_;
  bool close_client_;

  HANDLE timer_;
  AsyncSocket* receiving_;
  bool continue_;
};

#endif  // JUNO_NET_HTTP_HTTP_PROXY_SESSION_H_
