#include "ConfigLoader.h"
#include <tinyxml2.h>
#include <iostream>

using namespace tinyxml2;

bool ConfigLoader::loadConfig(const std::string& filePath, diskNodeConfig& config) {
	XMLDocument doc;

	if (doc.LoadFile(filePath.c_str()) != XML_SUCCESS) {
		std::cerr << "Error loading XML file: " << filePath << std::endl;
		return false;
	}

	XMLElement* root = doc.FirstChildElement("DiskNodeConfig");
	if (!root) return false;

	config.nodeId = root->FirstChildElement("NodeId")->GetText();
	config.nodeAddress = root->FirstChildElement("NodeAddress")->GetText();
	config.nodePort = std::stoi(root->FirstChildElement("NodePort")->GetText());
	config.storagePath = root->FirstChildElement("StoragePath")->GetText();
	config.maxStorageSize = std::stoll(root->FirstChildElement("MaxStorageSize")->GetText());
	config.controllerAddress = root->FirstChildElement("ControllerAddress")->GetText();

	return true;
}