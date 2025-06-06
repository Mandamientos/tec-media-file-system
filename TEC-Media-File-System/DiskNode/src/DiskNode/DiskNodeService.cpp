#include "DiskNodeService.h"

DiskNodeService::DiskNodeService(diskNodeConfig& config) : blockStorageManager(config.storagePath) {
	this->config = config;
}

grpc::Status DiskNodeService::StoreBlock(grpc::ServerContext* context, const tec_mfs::BlockData* request, tec_mfs::StatusResponse* response) {
	return grpc::Status::OK; // Placeholder for actual implementation
}

grpc::Status DiskNodeService::RetrieveBlock(grpc::ServerContext* context, const tec_mfs::BlockRequest* request, tec_mfs::BlockData* response) {
	return grpc::Status::OK; // Placeholder for actual implementation
}