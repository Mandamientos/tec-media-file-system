#include "NodeController.h"
#include <iostream>
#include <cstring>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

void NodeController::registerNodeDisk(const tec_mfs::NodeInfo& nodeInfo) {
	diskNodes[nodeInfo.node_id()] = nodeInfo;

	std::string fullAddress = nodeInfo.node_address() + ":" + std::to_string(nodeInfo.node_port());
	auto stub = tec_mfs::FileSytem::NewStub(grpc::CreateChannel(fullAddress, grpc::InsecureChannelCredentials()));

	diskStubs[nodeInfo.node_id()] = std::move(stub);
}

bool NodeController::nodeExists(const std::string& nodeId) const {
	return diskNodes.find(nodeId) != diskNodes.end();
}

bool NodeController::addDocument(const tec_mfs::FileRequest& request) {

	std::cout << "[addDocument] Iniciado\n";

	// Se obtienen los id de los nodos disponibles, se verifique que hayan al menos 4
	//Se extrae infromacion del archivo, verificando que no esté vacío 
	std::vector<std::string> nodeIds = getDiskNodeIds();
	std::cout << "[addDocument] Nodos disponibles: " << nodeIds.size() << std::endl;

	if (nodeIds.size() < 4) {
		std::cerr << "[ERROR] RAID 5 necesita al menos 4 nodos." << std::endl;
		return false;
	}

	const std::string& fileName = request.filename();
	const std::string& content = request.content();
	std::cout << "[addDocument] Archivo: " << fileName << ", tamaño contenido: " << content.size() << std::endl;

	if (content.empty()) {
		std::cerr << "[ERROR] El contenido del archivo está vacío.\n";
		return false;
	}

	// Acá se puede modificar el tamaño de los bloques
	const size_t blockSize = 512;

	size_t totalBlocks = (content.size() + blockSize - 1) / blockSize;
	std::cout << "[addDocument] Total de bloques: " << totalBlocks << std::endl;

	FileMetadata metadata;
	metadata.filename = fileName;

	// Se procesan los archivos en los bloques 
	for (size_t i = 0; i < totalBlocks; i += (nodeIds.size() - 1)) {
		std::vector<tec_mfs::BlockData> dataBlocks;
		std::vector<std::string> blockIds;

		for (size_t j = 0; j < nodeIds.size() - 1 && (i + j) < totalBlocks; ++j) {
			std::string blockId = generateBlockId();
			blockIds.push_back(blockId);

			std::string chunk = content.substr((i + j) * blockSize, blockSize);
			std::cout << "[addDocument] Generando bloque de datos ID: " << blockId << ", tamaño: " << chunk.size() << std::endl;

			tec_mfs::BlockData block;
			block.set_block_id(blockId);
			block.set_data(chunk);
			block.set_is_parity(false);
			block.set_file_response(fileName);

			dataBlocks.push_back(block);
		}

		//Se trabajan aspectos de paridad
		std::string parityData(blockSize, 0);
		for (const auto& block : dataBlocks) {
			for (size_t b = 0; b < block.data().size(); ++b) {
				parityData[b] ^= block.data()[b];
			}
		}

		std::string parityId = generateBlockId();
		tec_mfs::BlockData parityBlock;
		parityBlock.set_block_id(parityId);
		parityBlock.set_data(parityData);
		parityBlock.set_is_parity(true);
		parityBlock.set_file_response(fileName);

		// Se envian los bloques de datos y de paridad a los nodos
		for (size_t j = 0; j < dataBlocks.size(); ++j) {
			std::string nodeId = nodeIds[j];
			std::cout << "[addDocument] Enviando bloque " << dataBlocks[j].block_id() << " al nodo " << nodeId << std::endl;
			if (!storeBlock(dataBlocks[j], nodeId)) {
				std::cerr << "[ERROR] Error al enviar bloque de datos al nodo " << nodeId << std::endl;
				return false;
			}

			BlockLocation loc{ nodeId, dataBlocks[j].block_id(), false };
			metadata.blocks.push_back(loc);
		}

		std::string parityNodeId = nodeIds[dataBlocks.size() % nodeIds.size()];
		std::cout << "[addDocument] Enviando bloque de paridad " << parityId << " al nodo " << parityNodeId << std::endl;
		if (!storeBlock(parityBlock, parityNodeId)) {
			std::cerr << "[ERROR] Error al enviar bloque de paridad al nodo " << parityNodeId << std::endl;
			return false;
		}

		BlockLocation parityLoc{ parityNodeId, parityBlock.block_id(), true };
		metadata.blocks.push_back(parityLoc);
	}

	filesMetadata[fileName] = metadata;
	std::cout << "[+] Documento '" << fileName << "' almacenado exitosamente." << std::endl;
	return true;
}



bool NodeController::storeBlock(const tec_mfs::BlockData& block, const std::string& nodeId) {
	auto it = diskStubs.find(nodeId);
	if (it == diskStubs.end()) {
		std::cerr << "[ERROR] Stub no encontrado para nodo: " << nodeId << std::endl;
		return false;
	}

	grpc::ClientContext context;
	tec_mfs::StatusResponse response;
	grpc::Status status = it->second->StoreBlock(&context, block, &response);

	if (!status.ok() || !response.success()) {
		std::cerr << "[ERROR] Error al guardar bloque '" << block.block_id() << "' en " << nodeId << std::endl;
		return false;
	}

	return true;
}

bool NodeController::retrieveBlock(const std::string& blockId, tec_mfs::BlockData& block) {
	for (const auto& [nodeId, stub] : diskStubs) {
		grpc::ClientContext context;
		tec_mfs::BlockRequest request;
		request.set_block_id(blockId);
		tec_mfs::BlockData response;

		grpc::Status status = stub->RetrieveBlock(&context, request, &response);

		if (status.ok() && !response.block_id().empty()) {
			block = response;
			return true;
		}
	}
	return false;
}

bool NodeController::getDocument(const std::string& filename, tec_mfs::FileDataResponse& response) {
	if (filesMetadata.find(filename) == filesMetadata.end()) return false;

	const auto& metadata = filesMetadata[filename];
	std::string fileData;

	//Reconstruye el documento volviendo a juntar los bloques de datos 
	for (const auto& block : metadata.blocks) {
		tec_mfs::BlockData temp;
		if (retrieveBlock(block.blockId, temp)) {
			if (!block.isParity)
				fileData += temp.data();
		}
	}

	response.set_file_data(fileData);
	auto* meta = response.mutable_metadata();
	meta->set_file_size(fileData.size());
	
	return true;
}

void NodeController::listFiles(tec_mfs::FileListResponse& response) {
	for (const auto& [filename, metadata] : filesMetadata) {
		auto* ref = response.add_files();
		ref->set_file_id(filename); 
		ref->set_file_name(filename);
	}
}

void NodeController::getSystemStatus(tec_mfs::SystemStatusResponse& response) {
	for (const auto& [nodeId, info] : diskNodes) {
		auto* nodeStatus = response.add_nodes();
		nodeStatus->set_node_id(nodeId);
		nodeStatus->set_node_address(info.node_address());
		nodeStatus->set_node_port(info.node_port());
		nodeStatus->set_total_capacity(info.node_capacity());
		nodeStatus->set_used_capacity(0);  
		nodeStatus->set_free_capacity(info.node_capacity()); 
	}
}

std::vector<std::string> NodeController::getDiskNodeIds() const {
	std::vector<std::string> ids;
	for (const auto& pair : diskNodes) {
		ids.push_back(pair.first);
	}
	return ids;
}

std::string NodeController::generateBlockId() {
	return "block_" + std::to_string(blockCounter++);
}
