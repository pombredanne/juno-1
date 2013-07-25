// Copyright (c) 2013 dacci.org

#include "net/http/http_response.h"

#include <picohttpparser/picohttpparser.h>

HttpResponse::HttpResponse() {
}

HttpResponse::~HttpResponse() {
}

int HttpResponse::Parse(const char* data, size_t length) {
  int minor_version;
  int status;
  const char* message;
  size_t message_length;
  phr_header headers[kMaxHeaders];
  size_t header_count = kMaxHeaders;

  int result = ::phr_parse_response(data, length, &minor_version, &status,
                                    &message, &message_length,
                                    headers, &header_count, 0);
  if (result > 0) {
    minor_version_ = minor_version;
    status_ = status;
    message_.assign(message, message_length);

    AddHeader(headers, header_count);
  }

  return result;
}
