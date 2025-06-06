#include "Server.h"

FileSystemServiceImpl::FileSystemServiceImpl() {}

grpc::Status FileSystemServiceImpl::RegisterDiskNode(grpc::ServerContext* context, const tec_mfs::NodeInfo* request, tec_mfs::StatusResponse* response) {
	std::cout << "[DiskNodes] Registering node: " << request->node_id() << " with address " << request->node_address() << ":" << request->node_port() << std::endl;
	if (nodeController.nodeExists(request->node_id())) {
		response->set_success(false);
		response->set_message("Node already exists.");
	}
	else {
		nodeController.registerNodeDisk(*request);
		response->set_success(true);
		response->set_message("Node registered successfully.");
	}
	return grpc::Status::OK;
}

grpc::Status FileSystemServiceImpl::StoreBlock(grpc::ServerContext* context, const tec_mfs::BlockData* request, tec_mfs::StatusResponse* response) {
	// Implementation for storing a block
	return grpc::Status::OK;
}

grpc::Status FileSystemServiceImpl::RetrieveBlock(grpc::ServerContext* context, const tec_mfs::BlockRequest* request, tec_mfs::BlockData* response) {
	// Implementation for retrieving a block
	return grpc::Status::OK;
}

grpc::Status FileSystemServiceImpl::AddDocument(grpc::ServerContext* context, const tec_mfs::FileRequest* request, tec_mfs::StatusResponse* response) {
	// Implementation for adding a document
	return grpc::Status::OK;
}

grpc::Status FileSystemServiceImpl::GetDocument(grpc::ServerContext* context, const tec_mfs::FileRequest* request, tec_mfs::FileDataResponse* response) {
	// Implementation for getting a document
	return grpc::Status::OK;
}

grpc::Status FileSystemServiceImpl::GetDocumentList(grpc::ServerContext* context, const tec_mfs::Empty* request, tec_mfs::FileListResponse* response) {
	// Implementation for getting the document list
	return grpc::Status::OK;
}

grpc::Status FileSystemServiceImpl::GetSystemStatus(grpc::ServerContext* context, const tec_mfs::Empty* request, tec_mfs::SystemStatusResponse* response) {
	// Implementation for getting the system status
	return grpc::Status::OK;
}