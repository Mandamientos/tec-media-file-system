#pragma once

#include <grpcpp/grpcpp.h>
#include "NodeController.h"
#include "tec_mfs.grpc.pb.h"

class FileSystemServiceImpl final : public tec_mfs::FileSytem::Service {
public:
	FileSystemServiceImpl();

	grpc::Status RegisterDiskNode(grpc::ServerContext* context, const tec_mfs::NodeInfo* request, tec_mfs::StatusResponse* reponse) override;
	grpc::Status StoreBlock(grpc::ServerContext* context, const tec_mfs::BlockData* request, tec_mfs::StatusResponse* response) override;
	grpc::Status RetrieveBlock(grpc::ServerContext* context, const tec_mfs::BlockRequest* request, tec_mfs::BlockData* response) override;
	grpc::Status AddDocument(grpc::ServerContext* context, const tec_mfs::FileRequest* request, tec_mfs::StatusResponse* response) override;
	grpc::Status GetDocument(grpc::ServerContext* context, const tec_mfs::FileRequest* request, tec_mfs::FileDataResponse* response) override;
	grpc::Status GetDocumentList(grpc::ServerContext* context, const tec_mfs::Empty* request, tec_mfs::FileListResponse* response) override;
	grpc::Status GetSystemStatus(grpc::ServerContext* context, const tec_mfs::Empty* request, tec_mfs::SystemStatusResponse* response) override;
private:
	NodeController nodeController;
};

