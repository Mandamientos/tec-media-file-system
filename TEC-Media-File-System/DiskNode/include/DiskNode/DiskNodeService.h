#pragma once

#include <grpcpp/grpcpp.h>
#include "tec_mfs.grpc.pb.h"
#include "BlockStorageManager.h"
#include "ConfigLoader.h"

class DiskNodeService final : public tec_mfs::FileSytem::Service {
public:
	DiskNodeService(diskNodeConfig& config);
	
	grpc::Status StoreBlock(grpc::ServerContext* context, const tec_mfs::BlockData* request, tec_mfs::StatusResponse* response) override;
	grpc::Status RetrieveBlock(grpc::ServerContext* context, const tec_mfs::BlockRequest* request, tec_mfs::BlockData* response) override;
	grpc::Status DeleteBlock(grpc::ServerContext* context, const tec_mfs::BlockRequest* request, tec_mfs::StatusResponse* response) override;
private:
	BlockStorageManager blockStorageManager;
	diskNodeConfig config;
};