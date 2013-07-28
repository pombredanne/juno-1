// Copyright (c) 2013 dacci.org

#include "net/http/http_proxy_session.h"

HttpProxySession::HttpProxySession(AsyncSocket* client)
    : client_(client),
      remote_(),
      buffer_(new char[kBufferSize]),
      phase_(Request) {
}

HttpProxySession::~HttpProxySession() {
  if (client_ != NULL) {
    client_->Shutdown(SD_BOTH);
    delete client_;
  }

  if (remote_ != NULL) {
    remote_->Shutdown(SD_BOTH);
    delete remote_;
  }

  delete[] buffer_;
}

bool HttpProxySession::Start() {
  return client_->ReceiveAsync(buffer_, kBufferSize, 0, this);
}

void HttpProxySession::OnConnected(AsyncSocket* socket, DWORD error) {
  if (error != 0) {
    delete this;
    return;
  }

  std::string request_string;
  request_string += request_.method();
  request_string += ' ';
  request_string += url_.GetUrlPath();
  request_string += " HTTP/1.";
  request_string += '0' + request_.minor_version();
  request_string += "\x0D\x0A";
  request_.SerializeHeaders(&request_string);
  request_string += "\x0D\x0A";
  if (request_string.size() > kBufferSize) {
    delete this;
    return;
  }

  ::memmove(buffer_, request_string.data(), request_string.size());

  if (!remote_->SendAsync(buffer_, request_string.size(), 0, this)) {
    delete this;
    return;
  }
}

void HttpProxySession::OnReceived(AsyncSocket* socket, DWORD error,
                                  int length) {
  if (error != 0 || length == 0) {
    delete this;
    return;
  }

  if (phase_ == Request) {
    client_buffer_.append(buffer_, length);
    int result = request_.Parse(client_buffer_);
    if (result == HttpRequest::kPartial) {
      if (!client_->ReceiveAsync(buffer_, kBufferSize, 0, this))
        delete this;
      return;
    } else if (result <= 0) {
      delete this;
      return;
    } else {
      client_buffer_.erase(0, result);
    }

    content_length_ = 0;
    chunked_ = false;
    if (request_.HeaderExists("Content-Length")) {
      const std::string& value = request_.GetHeader("Content-Length");
      content_length_ = ::_strtoi64(value.c_str(), NULL, 10);
    } else if (request_.HeaderExists("Transfer-Encoding")) {
      chunked_ = true;
    }

    if (!url_.CrackUrl(request_.path().c_str())) {
      delete this;
      return;
    }

    char service[8];
    ::sprintf_s(service, "%d", url_.GetPortNumber());

    if (!resolver_.Resolve(url_.GetHostName(), service)) {
      delete this;
      return;
    }

    remote_ = new AsyncSocket();
    if (!remote_->ConnectAsync(*resolver_, this)) {
      delete this;
      return;
    }
  } else if (phase_ == RequestBody) {
    if (chunked_) {
      client_buffer_.append(buffer_, length);

      content_length_ = ParseChunk(client_buffer_, &chunk_size_);
      if (content_length_ == -2) {
        if (!client_->ReceiveAsync(buffer_, kBufferSize, 0, this))
          delete this;
        return;
      } else if (content_length_ <= 0) {
        delete this;
        return;
      } else {
        if (!remote_->SendAsync(client_buffer_.data(), content_length_, 0,
                                this))
          delete this;
        return;
      }
    } else {
      if (!remote_->SendAsync(buffer_, length, 0, this))
        delete this;
      return;
    }
  } else if (phase_ == Response) {
    remote_buffer_.append(buffer_, length);
    int result = response_.Parse(remote_buffer_);
    if (result == HttpResponse::kPartial) {
      if (!remote_->ReceiveAsync(buffer_, kBufferSize, 0, this))
        delete this;
      return;
    } else if (result <= 0) {
      delete this;
      return;
    } else {
      remote_buffer_.erase(0, result);
    }

    content_length_ = 0;
    chunked_ = false;
    if (response_.HeaderExists("Content-Length")) {
      const std::string& value = response_.GetHeader("Content-Length");
      content_length_ = ::_strtoi64(value.c_str(), NULL, 10);
    } else if (response_.HeaderExists("Transfer-Encoding")) {
      chunked_ = true;
    }

    std::string response_string;
    response_string += "HTTP/1.";
    response_string += '0' + response_.minor_version();
    response_string += ' ';
    ::sprintf_s(buffer_, kBufferSize, "%d ", response_.status());
    response_string += buffer_;
    response_string += response_.message();
    response_string += "\x0D\x0A";
    response_.SerializeHeaders(&response_string);
    response_string += "\x0D\x0A";
    if (response_string.size() > kBufferSize) {
      delete this;
      return;
    }

    ::memmove(buffer_, response_string.data(), response_string.size());

    if (!client_->SendAsync(buffer_, response_string.size(), 0, this)) {
      delete this;
      return;
    }
  } else if (phase_ == ResponseBody) {
    if (chunked_) {
      if (content_length_ == -2) {
        remote_buffer_.append(buffer_, length);

        content_length_ = ParseChunk(remote_buffer_, &chunk_size_);
        if (content_length_ == -2) {
          if (!remote_->ReceiveAsync(buffer_, kBufferSize, 0, this)) {
            delete this;
            return;
          }
        } else {
          if (!client_->SendAsync(remote_buffer_.data(), content_length_, 0,
                                  this)) {
            delete this;
            return;
          }
        }
      } else if (content_length_ <= 0) {
        delete this;
        return;
      } else {
        if (!client_->SendAsync(remote_buffer_.data(), content_length_, 0,
                                this)) {
            delete this;
            return;
        }
      }
    } else {
      content_length_ -= length;

      if (!client_->SendAsync(buffer_, length, 0, this)) {
        delete this;
        return;
      }
    }
  }
}

void HttpProxySession::OnSent(AsyncSocket* socket, DWORD error, int length) {
  if (error != 0 || length == 0) {
    delete this;
    return;
  }

  if (phase_ == Request) {
    phase_ = RequestBody;

    if (chunked_) {
      content_length_ = ParseChunk(client_buffer_, &chunk_size_);
      if (content_length_ == -2) {
        if (!client_->ReceiveAsync(buffer_, kBufferSize, 0, this))
          delete this;
        return;
      } else if (content_length_ <= 0) {
        delete this;
        return;
      } else {
        if (!remote_->SendAsync(client_buffer_.data(), content_length_, 0,
                                this))
          delete this;
        return;
      }
    } else if (content_length_ > 0) {
      size_t length = min(client_buffer_.size(), content_length_);
      if (length > 0) {
        ::memmove(buffer_, client_buffer_.data(), length);
        client_buffer_.erase(0, length);

        if (!remote_->SendAsync(buffer_, length, 0, this)) {
          delete this;
          return;
        }
      } else {
        if (!client_->ReceiveAsync(buffer_, kBufferSize, 0, this)) {
          delete this;
          return;
        }
      }
    } else {
      phase_ = Response;
      if (!remote_->ReceiveAsync(buffer_, kBufferSize, 0, this)) {
        delete this;
        return;
      }
    }
  } else if (phase_ == RequestBody) {
    if (chunked_) {
      // XXX(dacci): what if length < content_length_
      client_buffer_.erase(0, content_length_);

      if (chunk_size_ == 0) {
        phase_ = Response;
        if (!remote_->ReceiveAsync(buffer_, kBufferSize, 0, this))
          delete this;
        return;
      }

      content_length_ = ParseChunk(client_buffer_, &chunk_size_);
      if (content_length_ == -2) {
        if (!client_->ReceiveAsync(buffer_, kBufferSize, 0, this))
          delete this;
        return;
      } else if (content_length_ <= 0) {
        delete this;
        return;
      } else {
        if (!remote_->SendAsync(client_buffer_.data(), content_length_, 0,
                                this))
          delete this;
        return;
      }
    } else {
      content_length_ -= length;
      if (content_length_ > 0) {
        if (!client_->ReceiveAsync(buffer_, kBufferSize, 0, this)) {
          delete this;
          return;
        }
      } else {
        phase_ = Response;
        if (!remote_->ReceiveAsync(buffer_, kBufferSize, 0, this)) {
          delete this;
          return;
        }
      }
    }
  } else if (phase_ == Response) {
    phase_ = ResponseBody;

    if (chunked_) {
      content_length_ = ParseChunk(remote_buffer_, &chunk_size_);

      if (content_length_ == -2) {
        if (!remote_->ReceiveAsync(buffer_, kBufferSize, 0, this))
          delete this;
        return;
      } else if (content_length_ < 0) {
        delete this;
        return;
      }

      if (!client_->SendAsync(remote_buffer_.data(), content_length_, 0,
                              this))
        delete this;
      return;
    } else if (content_length_ > 0) {
      if (remote_buffer_.empty()) {
        if (!remote_->ReceiveAsync(buffer_, kBufferSize, 0, this)) {
          delete this;
          return;
        }
      } else {
        content_length_ -= remote_buffer_.size();
        if (!client_->SendAsync(remote_buffer_.data(), remote_buffer_.size(),
                                0, this)) {
          delete this;
          return;
        }
      }
    } else {
      delete this;
      return;
    }
  } else if (phase_ == ResponseBody) {
    if (chunked_) {
      // XXX(dacci): what if length < content_length_
      remote_buffer_.erase(0, content_length_);

      if (chunk_size_ == 0) {
        delete this;
        return;
      }

      content_length_ = ParseChunk(remote_buffer_, &chunk_size_);
      if (content_length_ == -2) {
        if (!remote_->ReceiveAsync(buffer_, kBufferSize, 0, this)) {
          delete this;
          return;
        }
      } else if (content_length_ <= 0) {
        delete this;
        return;
      } else {
        if (!client_->SendAsync(remote_buffer_.data(), content_length_, 0,
                                this)) {
          delete this;
          return;
        }
      }
    } else if (content_length_ > 0) {
      if (!remote_->ReceiveAsync(buffer_, kBufferSize, 0, this)) {
        delete this;
        return;
      }
    } else {
      delete this;
      return;
    }
  }
}

int64_t HttpProxySession::ParseChunk(const std::string& buffer,
                                     int64_t* chunk_size) {
  char* start = const_cast<char*>(buffer.c_str());
  char* end = start;
  *chunk_size = ::_strtoi64(buffer.c_str(), &end, 16);

  while (*end) {
    if (*end != '\x0D' && *end != '\x0A')
      return -1;

    if (*end == '\x0A')
      break;

    ++end;
  }

  if (*end == '\0')
    return -2;

  ++end;

  int64_t length = *chunk_size + (end - start);
  if (buffer.size() < length)
    return -2;

  end = start + length;

  while (*end) {
    if (*end != '\x0D' && *end != '\x0A')
      return -1;

    if (*end == '\x0A')
      break;

    ++end;
  }

  if (*end == '\0')
    return -2;

  ++end;

  return end - start;
}