#pragma once
#include <string>

struct diskNodeConfig
{
	std::string nodeId;
	std::string nodeAddress;
	int nodePort;
	std::string storagePath;
	int64_t maxStorageSize;
	std::string controllerAddress;
};

class ConfigLoader
{
public:
	static bool loadConfig(const std::string& configFilePath, diskNodeConfig& config);
};