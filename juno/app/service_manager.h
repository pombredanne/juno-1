// Copyright (c) 2013 dacci.org

#ifndef JUNO_APP_SERVICE_MANAGER_H_
#define JUNO_APP_SERVICE_MANAGER_H_

#include <map>
#include <string>
#include <vector>

#include "app/server_config.h"
#include "misc/registry_key.h"
#include "service/service_provider.h"

class Server;

typedef std::map<std::string, ServiceProviderPtr> ProviderMap;
typedef std::map<std::string, ServiceConfigPtr> ServiceConfigMap;
typedef std::map<std::string, ServerConfigPtr> ServerConfigMap;

class ServiceManager {
 public:
  ServiceManager();
  virtual ~ServiceManager();

  static ServiceManager* GetInstance() {
    return instance_;
  }

  bool LoadServices();
  void StopServices();

  bool LoadServers();
  bool StartServers();
  void StopServers();

  ServiceProviderPtr GetProvider(const std::string& name) const;

  ServiceConfigPtr GetServiceConfig(const std::string& name) const;
  void CopyServiceConfigs(ServiceConfigMap* configs) const;

  void CopyServerConfigs(ServerConfigMap* configs) const;

  bool UpdateConfiguration(ServiceConfigMap&& service_configs,
                           ServerConfigMap&& server_configs);

  const ProviderMap& providers() const {
    return providers_;
  }

 private:
  typedef std::unique_ptr<Server> ServerPtr;
  typedef std::map<std::string, ServicePtr> ServiceMap;
  typedef std::map<std::string, ServerPtr> ServerMap;

  bool LoadService(const RegistryKey& parent, const std::string& key_name);
  bool SaveService(const RegistryKey& parent, const ServiceConfigPtr& config);
  bool CreateService(const std::string& name);

  bool LoadServer(const RegistryKey& parent, const std::string& key_name);
  bool SaveServer(const RegistryKey& parent, const ServerConfigPtr& config);
  bool CreateServer(const std::string& name);

  static ServiceManager* instance_;

  ProviderMap providers_;
  ServiceConfigMap service_configs_;
  ServerConfigMap server_configs_;
  ServiceMap services_;
  ServerMap servers_;
};

#endif  // JUNO_APP_SERVICE_MANAGER_H_
