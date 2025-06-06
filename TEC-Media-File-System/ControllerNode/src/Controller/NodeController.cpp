#include "NodeController.h"

void NodeController::registerNodeDisk(const tec_mfs::NodeInfo& nodeInfo) {
	diskNodes[nodeInfo.node_id()] = nodeInfo;
}

bool NodeController::nodeExists(const std::string& nodeId) const {
	return diskNodes.find(nodeId) != diskNodes.end();
}