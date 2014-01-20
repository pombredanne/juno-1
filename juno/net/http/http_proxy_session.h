// Copyright (c) 2013 dacci.org

#ifndef JUNO_NET_HTTP_HTTP_PROXY_SESSION_H_
#define JUNO_NET_HTTP_HTTP_PROXY_SESSION_H_

#include <stdint.h>

#include <url/gurl.h>

#include <memory>

#include "net/async_socket.h"
#include "net/http/http_request.h"
#include "net/http/http_response.h"
#include "net/http/http_status.h"

class HttpProxy;
class HttpProxyConfig;

class HttpProxySession : public AsyncSocket::Listener {
 public:
  HttpProxySession(HttpProxy* proxy, HttpProxyConfig* config);
  virtual ~HttpProxySession();

  bool Start(AsyncSocket* client);
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

  static const size_t kBufferSize = 8 * 1024;   // 8 KiB
  static const int kTimeout = 15 * 1000;        // 15 sec

  static const std::string kConnection;
  static const std::string kContentEncoding;
  static const std::string kContentLength;
  static const std::string kExpect;
  static const std::string kKeepAlive;
  static const std::string kProxyAuthenticate;
  static const std::string kProxyAuthorization;
  static const std::string kProxyConnection;
  static const std::string kTransferEncoding;

  bool ReceiveAsync(const AsyncSocketPtr& socket, int flags);

  static int64_t ParseChunk(const std::string& buffer, int64_t* chunk_size);

  void ProcessRequestHeader();
  void ProcessResponseHeader();
  void ProcessMessageLength(HttpHeaders* headers);
  void ProcessHopByHopHeaders(HttpHeaders* headers);

  void SendError(HTTP::StatusCode status);
  void EndSession();

  bool FireReceived(const AsyncSocketPtr& socket, DWORD error, int length);
  static void CALLBACK FireEvent(PTP_CALLBACK_INSTANCE instance, void* param);

  void OnRequestReceived(DWORD error, int length);
  void OnRequestSent(DWORD error, int length);

  void OnRequestBodyReceived(DWORD error, int length);
  void OnRequestBodySent(DWORD error, int length);

  void OnResponseReceived(DWORD error, int length);
  void OnResponseSent(DWORD error, int length);

  void OnResponseBodyReceived(DWORD error, int length);
  void OnResponseBodySent(DWORD error, int length);

  static void CALLBACK OnTimeout(PTP_CALLBACK_INSTANCE instance, PVOID param,
                                 PTP_TIMER timer);
  static void CALLBACK DeleteThis(PTP_CALLBACK_INSTANCE instance, void* param);

  static FILETIME kTimerDueTime;

  HttpProxy* const proxy_;
  HttpProxyConfig* const config_;
  AsyncSocketPtr client_;
  std::string client_buffer_;
  HttpRequest request_;
  bool tunnel_;
  GURL url_;
  madoka::net::AddressInfo resolver_;

  AsyncSocketPtr remote_;
  std::string remote_buffer_;
  HttpResponse response_;

  std::unique_ptr<char[]> buffer_;
  Phase phase_;
  int64_t content_length_;
  bool chunked_;
  int64_t chunk_size_;
  bool close_client_;
  PTP_TIMER timer_;
  AsyncSocket* receiving_;
  bool continue_;
};

#endif  // JUNO_NET_HTTP_HTTP_PROXY_SESSION_H_
