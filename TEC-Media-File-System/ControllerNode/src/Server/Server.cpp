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

grpc::Status FileSystemServiceImpl::AddDocument(grpc::ServerContext* context, const tec_mfs::FileRequest* request, tec_mfs::StatusResponse* response) {
	
	std::cout << "[AddDocument] Recibiendo archivo: " << request->filename() << std::endl;

	if (nodeController.addDocument(*request)) {
		response->set_success(true);
		response->set_message("Documento cargado exitosamente");
	}
	else {
		response->set_success(false);
		response->set_message("Error al cargar el documento");
	}
	return grpc::Status::OK;
}

grpc::Status FileSystemServiceImpl::GetDocument(grpc::ServerContext* context, const tec_mfs::FileRequest* request, tec_mfs::FileDataResponse* response) {
	if (nodeController.getDocument(request->filename(), *response)) {
		return grpc::Status::OK;
	}
	return grpc::Status(grpc::StatusCode::NOT_FOUND, "Documento no encontrado");
}

grpc::Status FileSystemServiceImpl::DeleteDocument(grpc::ServerContext* context, const tec_mfs::DeleteRequest* request, tec_mfs::StatusResponse* response) {
	if (nodeController.deleteDocument(request->filename())) {
		response->set_success(true);
		response->set_message("[delDocument] El archivo ha sido eliminado correctamente.");
	} else {
		response->set_success(false);
		response->set_message("[delDocument] Error al eliminar el documento");
	}
	return grpc::Status::OK;
}

grpc::Status FileSystemServiceImpl::GetDocumentList(grpc::ServerContext* context, const tec_mfs::Empty* request, tec_mfs::FileListResponse* response) {
	nodeController.listFiles(*response);
	return grpc::Status::OK;
}

grpc::Status FileSystemServiceImpl::GetSystemStatus(grpc::ServerContext* context, const tec_mfs::Empty* request, tec_mfs::SystemStatusResponse* response) {
	nodeController.getSystemStatus(*response);
	return grpc::Status::OK;
}