#pragma once

#include <string>
#include <unordered_map>
#include "tec_mfs.pb.h"

class NodeController {
public:
	void registerNodeDisk(const tec_mfs::NodeInfo& nodeInfo);
	bool nodeExists(const std::string& nodeId) const;

private:
	std::unordered_map<std::string, tec_mfs::NodeInfo> diskNodes;
};