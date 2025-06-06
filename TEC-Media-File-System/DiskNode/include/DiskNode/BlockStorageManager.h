#pragma once

#include <string>
#include "tec_mfs.pb.h"

class BlockStorageManager {
public:
	BlockStorageManager(const std::string& storagePath);

	bool saveBlock(const tec_mfs::BlockData& blockData);
	bool loadBlock(const std::string& blockId, tec_mfs::BlockData& blockData);
private:
	std::string storagePath;
};