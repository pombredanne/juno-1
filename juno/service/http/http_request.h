// Copyright (c) 2013 dacci.org

#ifndef JUNO_SERVICE_HTTP_HTTP_REQUEST_H_
#define JUNO_SERVICE_HTTP_HTTP_REQUEST_H_

#include <string>

#include "service/http/http_headers.h"

class HttpRequest : public HttpHeaders {
 public:
  static const int kPartial = -2;
  static const int kError = -1;

  HttpRequest();
  virtual ~HttpRequest();

  int Parse(const char* data, size_t length);
  int Parse(const std::string& data) {
    return Parse(data.data(), data.size());
  }

  void Clear();

  void Serialize(std::string* output);

  const std::string& method() const {
    return method_;
  }

  void set_method(const char* method) {
    method_ = method;
  }

  void set_method(const std::string& method) {
    method_ = method;
  }

  const std::string& path() const {
    return path_;
  }

  void set_path(const char* path) {
    path_ = path;
  }

  void set_path(const std::string& path) {
    path_ = path;
  }

  int minor_version() const {
    return minor_version_;
  }

  void set_minor_version(int minor_version) {
    minor_version_ = minor_version;
  }

 private:
  static const size_t kMaxHeaders = 128;

  std::string method_;
  std::string path_;
  int minor_version_;
};

#endif  // JUNO_SERVICE_HTTP_HTTP_REQUEST_H_
