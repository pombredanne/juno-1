// Copyright (c) 2013 dacci.org

#include "net/scissors/scissors_udp_session.h"

#include <assert.h>

#include "net/scissors/scissors_config.h"

#define DELETE_THIS() \
  ::TrySubmitThreadpoolCallback(DeleteThis, this, NULL)

FILETIME ScissorsUdpSession::kTimerDueTime = {
  -kTimeout * 1000 * 10,   // in 100-nanoseconds
  -1
};

ScissorsUdpSession::ScissorsUdpSession(Scissors* service,
                                       Service::Datagram* datagram)
    : service_(service), datagram_(datagram), remote_(), buffer_(), timer_() {
  resolver_.ai_socktype = SOCK_DGRAM;
  timer_ = ::CreateThreadpoolTimer(OnTimeout, this, NULL);
}

ScissorsUdpSession::~ScissorsUdpSession() {
  if (datagram_ != NULL) {
    delete[] reinterpret_cast<char*>(datagram_);
    datagram_ = NULL;
  }

  if (timer_ != NULL) {
    ::SetThreadpoolTimer(timer_, NULL, 0, 0);
    ::WaitForThreadpoolTimerCallbacks(timer_, TRUE);
    ::CloseThreadpoolTimer(timer_);
    timer_ = NULL;
  }

  service_->EndSession(this);
}

bool ScissorsUdpSession::Start() {
  if (!resolver_.Resolve(service_->config_->remote_address().c_str(),
                         service_->config_->remote_port()))
    return false;

  remote_.reset(new AsyncDatagramSocket());
  if (remote_ == NULL)
    return false;

  buffer_.reset(new char[kBufferSize]);
  if (buffer_ == NULL)
    return false;

  if (!remote_->Connect(*resolver_))
    return false;

  if (!remote_->SendAsync(datagram_->data, datagram_->data_length, 0, this))
    return false;

  return true;
}

void ScissorsUdpSession::Stop() {
  remote_->Close();
}

void ScissorsUdpSession::OnReceived(AsyncDatagramSocket* socket, DWORD error,
                                    int length) {
  assert(timer_ != NULL);
  ::SetThreadpoolTimer(timer_, NULL, 0, 0);

  if (error == 0) {
    addrinfo end_point = {};
    end_point.ai_addrlen = datagram_->from_length;
    end_point.ai_addr = datagram_->from;

    if (datagram_->socket->SendToAsync(buffer_.get(), length, 0, &end_point,
                                       this))
      return;
  }

  DELETE_THIS();
}

void ScissorsUdpSession::OnReceivedFrom(AsyncDatagramSocket* socket,
                                        DWORD error, int length, sockaddr* from,
                                        int from_length) {
}

void ScissorsUdpSession::OnSent(AsyncDatagramSocket* socket, DWORD error,
                                int length) {
  if (error == 0) {
    do {
      assert(timer_ != NULL);
      ::SetThreadpoolTimer(timer_, &kTimerDueTime, 0, 0);

      if (!remote_->ReceiveAsync(buffer_.get(), kBufferSize, 0, this))
        break;

      return;
    } while (false);
  }

  DELETE_THIS();
}

void ScissorsUdpSession::OnSentTo(AsyncDatagramSocket* socket, DWORD error,
                                  int length, sockaddr* to, int to_length) {
  DELETE_THIS();
}

void CALLBACK ScissorsUdpSession::OnTimeout(PTP_CALLBACK_INSTANCE instance,
                                            PVOID param, PTP_TIMER timer) {
  ScissorsUdpSession* session = static_cast<ScissorsUdpSession*>(param);
  session->Stop();
}

void CALLBACK ScissorsUdpSession::DeleteThis(PTP_CALLBACK_INSTANCE instance,
                                             void* param) {
  delete static_cast<ScissorsUdpSession*>(param);
}
