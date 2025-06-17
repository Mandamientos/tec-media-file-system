#include <iostream>
#include "DiskNodeService.h"

DiskNodeService::DiskNodeService(diskNodeConfig& config) : blockStorageManager(config.storagePath) {
	this->config = config;
}

grpc::Status DiskNodeService::StoreBlock(grpc::ServerContext* context, const tec_mfs::BlockData* request, tec_mfs::StatusResponse* response) {
    std::cout << "[DiskNode] Recibiendo bloque: " << request->block_id()
        << " (archivo: " << request->file_response()
        << ", paridad: " << (request->is_parity() ? "sí" : "no") << ")\n";

    bool success = blockStorageManager.saveBlock(*request);

    if (success) {
        response->set_success(true);
        response->set_message("Bloque almacenado exitosamente");
    }
    else {
        response->set_success(false);
        response->set_message("Error al guardar el bloque");
    }

    return grpc::Status::OK;
}

grpc::Status DiskNodeService::RetrieveBlock(grpc::ServerContext* context, const tec_mfs::BlockRequest* request, tec_mfs::BlockData* response) {
    std::cout << "[DiskNode] Solicitud de lectura para el bloque: " << request->block_id() << std::endl;

    if (!blockStorageManager.loadBlock(request->block_id(), *response)) {
        std::cerr << "[ERROR] No se pudo recuperar el bloque: " << request->block_id() << std::endl;
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Bloque no encontrado");
    }

    std::cout << "[DiskNode] Bloque enviado exitosamente: " << request->block_id() << std::endl;
    return grpc::Status::OK;
}