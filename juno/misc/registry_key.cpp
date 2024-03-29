// Copyright (c) 2013 dacci.org

#include "misc/registry_key.h"

#include <string>

RegistryKey::RegistryKey() : key_() {
}

RegistryKey::RegistryKey(HKEY key) : key_() {
  Attach(key);
}

RegistryKey::RegistryKey(RegistryKey&& other) : key_() {
  *this = std::move(other);
}

RegistryKey::~RegistryKey() {
  Close();
}

void RegistryKey::Attach(HKEY key) {
  Close();
  key_ = key;
}

HKEY RegistryKey::Detach() {
  HKEY detached = key_;
  key_ = NULL;
  return detached;
}

bool RegistryKey::Create(HKEY parent, const char* name) {
  HKEY new_key = NULL;
  LSTATUS result = ::RegCreateKeyExA(parent, name, 0, nullptr, 0,
                                     KEY_ALL_ACCESS, nullptr, &new_key,
                                     nullptr);
  if (result != ERROR_SUCCESS)
    return false;

  Close();
  key_ = new_key;
  return true;
}

bool RegistryKey::Create(HKEY parent, const wchar_t* name) {
  HKEY new_key = NULL;
  LSTATUS result = ::RegCreateKeyExW(parent, name, 0, nullptr, 0,
                                     KEY_ALL_ACCESS, nullptr, &new_key,
                                     nullptr);
  if (result != ERROR_SUCCESS)
    return false;

  Close();
  key_ = new_key;
  return true;
}

bool RegistryKey::Open(HKEY parent, const char* name) {
  HKEY new_key = NULL;
  LSTATUS result = ::RegOpenKeyExA(parent, name, 0, KEY_ALL_ACCESS, &new_key);
  if (result != ERROR_SUCCESS)
    return false;

  Close();
  key_ = new_key;
  return true;
}

bool RegistryKey::Open(HKEY parent, const wchar_t* name) {
  HKEY new_key = NULL;
  LSTATUS result = ::RegOpenKeyExW(parent, name, 0, KEY_ALL_ACCESS, &new_key);
  if (result != ERROR_SUCCESS)
    return false;

  Close();
  key_ = new_key;
  return true;
}

void RegistryKey::Close() {
  if (key_ != NULL) {
    ::RegCloseKey(key_);
    key_ = NULL;
  }
}

bool RegistryKey::QueryString(const char* name, std::string* value) const {
  DWORD type, size = 0;
  LSTATUS result = ::RegQueryValueExA(key_, name, nullptr, &type, nullptr,
                                      &size);
  if (result != ERROR_SUCCESS || type != REG_SZ)
    return false;

  char* buffer = new char[size];
  if (buffer == nullptr)
    return false;

  result = ::RegQueryValueExA(key_, name, nullptr, nullptr,
                              reinterpret_cast<BYTE*>(buffer), &size);
  if (result == ERROR_SUCCESS) {
    char* end = buffer + size - 1;
    while (end > buffer) {
      if (*end)
        break;
      --end;
    }

    value->assign(buffer, end + 1);
  }

  delete[] buffer;

  return true;
}

bool RegistryKey::QueryString(const wchar_t* name, std::wstring* value) const {
  DWORD type, size = 0;
  LSTATUS result = ::RegQueryValueExW(key_, name, nullptr, &type, nullptr,
                                      &size);
  if (result != ERROR_SUCCESS || type != REG_SZ)
    return false;

  wchar_t* buffer = new wchar_t[size / 2];
  if (buffer == nullptr)
    return false;

  result = ::RegQueryValueExW(key_, name, nullptr, nullptr,
                              reinterpret_cast<BYTE*>(buffer), &size);
  if (result == ERROR_SUCCESS) {
    wchar_t* end = buffer + size - 1;
    while (end > buffer) {
      if (*end)
        break;
      --end;
    }

    value->assign(buffer, end + 1);
  }

  delete[] buffer;

  return true;
}

bool RegistryKey::SetString(const char* name, const std::string& value) {
  return ::RegSetValueExA(key_, name, 0, REG_SZ,
                          reinterpret_cast<const BYTE*>(value.c_str()),
                          value.size() + 1) == ERROR_SUCCESS;
}

bool RegistryKey::SetString(const wchar_t* name, const std::wstring& value) {
  return ::RegSetValueExW(key_, name, 0, REG_SZ,
                          reinterpret_cast<const BYTE*>(value.c_str()),
                          value.size() + 1) == ERROR_SUCCESS;
}

bool RegistryKey::QueryInteger(const char* name, int* value) const {
  int stored;
  DWORD type, size = sizeof(stored);
  LSTATUS result = ::RegQueryValueExA(key_, name, nullptr, &type,
                                      reinterpret_cast<BYTE*>(&stored), &size);
  if (result != ERROR_SUCCESS || type != REG_DWORD)
    return false;

  *value = stored;

  return true;
}

bool RegistryKey::QueryInteger(const wchar_t* name, int* value) const {
  int stored;
  DWORD type, size = sizeof(stored);
  LSTATUS result = ::RegQueryValueExW(key_, name, nullptr, &type,
                                      reinterpret_cast<BYTE*>(&stored), &size);
  if (result != ERROR_SUCCESS || type != REG_DWORD)
    return false;

  *value = stored;

  return true;
}

bool RegistryKey::SetInteger(const char* name, int value) {
  return ::RegSetValueExA(key_, name, 0, REG_DWORD,
                          reinterpret_cast<BYTE*>(&value),
                          sizeof(value)) == ERROR_SUCCESS;
}

bool RegistryKey::SetInteger(const wchar_t* name, int value) {
  return ::RegSetValueExW(key_, name, 0, REG_DWORD,
                          reinterpret_cast<BYTE*>(&value),
                          sizeof(value)) == ERROR_SUCCESS;
}

bool RegistryKey::QueryBinary(const char* name, void* data, int* length) const {
  DWORD type, size = 0;
  LSTATUS status = ::RegQueryValueExA(key_, name, nullptr, &type, nullptr,
                                      &size);
  if (status != ERROR_SUCCESS || type != REG_BINARY)
    return false;

  if (data == nullptr) {
    if (length != nullptr)
      *length = size;
    return true;
  }

  if (*length < size)
    return false;

  status = ::RegQueryValueExA(key_, name, nullptr, nullptr,
                              static_cast<BYTE*>(data), &size);
  if (status != ERROR_SUCCESS)
    return false;

  *length = size;

  return true;
}

bool RegistryKey::QueryBinary(const wchar_t* name, void* data,
                              int* length) const {
  DWORD type, size = 0;
  LSTATUS status = ::RegQueryValueExW(key_, name, nullptr, &type, nullptr,
                                      &size);
  if (status != ERROR_SUCCESS || type != REG_BINARY)
    return false;

  if (data == nullptr) {
    if (length != nullptr)
      *length = size;
    return true;
  }

  if (*length < size)
    return false;

  status = ::RegQueryValueExW(key_, name, nullptr, nullptr,
                              static_cast<BYTE*>(data), &size);
  if (status != ERROR_SUCCESS)
    return false;

  *length = size;

  return true;
}

bool RegistryKey::SetBinary(const char* name, const void* data, int length) {
  return ::RegSetValueExA(key_, name, 0, REG_BINARY,
                          static_cast<const BYTE*>(data),
                          length) == ERROR_SUCCESS;
}

bool RegistryKey::SetBinary(const wchar_t* name, const void* data, int length) {
  return ::RegSetValueExW(key_, name, 0, REG_BINARY,
                          static_cast<const BYTE*>(data),
                          length) == ERROR_SUCCESS;
}

bool RegistryKey::EnumerateKey(int index, std::string* name) {
  char buffer[256];
  DWORD length = _countof(buffer);
  LSTATUS result = ::RegEnumKeyExA(key_, index, buffer, &length, nullptr,
                                   nullptr, nullptr, nullptr);
  if (result != ERROR_SUCCESS)
    return false;

  name->assign(buffer, length);

  return true;
}

bool RegistryKey::EnumerateKey(int index, std::wstring* name) {
  wchar_t buffer[256];
  DWORD length = _countof(buffer);
  LSTATUS result = ::RegEnumKeyExW(key_, index, buffer, &length, nullptr,
                                   nullptr, nullptr, nullptr);
  if (result != ERROR_SUCCESS)
    return false;

  name->assign(buffer, length);

  return true;
}

bool RegistryKey::DeleteKey(const char* sub_key) {
  return ::RegDeleteTreeA(key_, sub_key) == ERROR_SUCCESS;
}

bool RegistryKey::DeleteKey(const wchar_t* sub_key) {
  return ::RegDeleteTreeW(key_, sub_key) == ERROR_SUCCESS;
}

RegistryKey& RegistryKey::operator=(RegistryKey&& other) {
  if (&other != this) {
    Close();

    key_ = other.key_;
    other.key_ = NULL;
  }

  return *this;
}
