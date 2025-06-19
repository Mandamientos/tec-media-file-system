#pragma once

#include <string>
#include <unordered_map>
#include "tec_mfs.pb.h"
#include <vector>
#include <memory>
#include "tec_mfs.grpc.pb.h"

struct DiskNode {
	std::string id;
	std::string address;
	int port;
	std::shared_ptr<tec_mfs::FileSytem::Stub> stub;
};

struct BlockLocation {
	std::string nodeId;
	std::string blockId;
	bool isParity;
};

struct FileMetadata {
	std::string filename;
	std::int64_t fileSize;
	std::vector<BlockLocation> blocks;
};

class NodeController {
public:
	void registerNodeDisk(const tec_mfs::NodeInfo& nodeInfo);
	bool nodeExists(const std::string& nodeId) const;
	bool addDocument(const tec_mfs::FileRequest& request);
	bool storeBlock(const tec_mfs::BlockData& block, const std::string& nodeId);
	bool retrieveBlock(const std::string& blockId, tec_mfs::BlockData& block);
	bool getDocument(const std::string& filename, tec_mfs::FileDataResponse& response);
	bool deleteDocument(const std::string& filename);
	void listFiles(tec_mfs::FileListResponse& response);
	void getSystemStatus(tec_mfs::SystemStatusResponse& response);

private:
	std::unordered_map<std::string, tec_mfs::NodeInfo> diskNodes;
	std::unordered_map<std::string, FileMetadata> filesMetadata;
	std::unordered_map<std::string, std::shared_ptr<tec_mfs::FileSytem::Stub>> diskStubs;


	std::vector<std::string> getDiskNodeIds() const;
	std::string generateBlockId();

	int blockCounter = 0; 
};
