#include "NodeController.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

void NodeController::registerNodeDisk(const tec_mfs::NodeInfo& nodeInfo) {
	diskNodes[nodeInfo.node_id()] = nodeInfo;
	std::string address = nodeInfo.node_address();
	if (address == "0.0.0.0") {
		std::cerr << "[WARN] Dirección 0.0.0.0 no es válida para conectar. Usando 127.0.0.1 por defecto.\n";
		address = "127.0.0.1";
	}

	std::string fullAddress = address + ":" + std::to_string(nodeInfo.node_port());
	auto stub = tec_mfs::FileSytem::NewStub(grpc::CreateChannel(fullAddress, grpc::InsecureChannelCredentials()));

	diskStubs[nodeInfo.node_id()] = std::move(stub);
}

bool NodeController::nodeExists(const std::string& nodeId) const {
	return diskNodes.find(nodeId) != diskNodes.end();
}

bool NodeController::addDocument(const tec_mfs::FileRequest& request) {
	std::cout << "[addDocument] Iniciado\n";

	// Obtener nodos disponibles
	std::vector<std::string> nodeIds = getDiskNodeIds();
	size_t nNodes = nodeIds.size();

	std::cout << "[addDocument] Nodos disponibles: " << nNodes << std::endl;
	if (nNodes < 4) {
		std::cerr << "[ERROR] RAID 5 requiere al menos 4 nodos.\n";
		return false;
	}

	const std::string& filename = request.filename();
	const std::string& content = request.content();

	if (content.empty()) {
		std::cerr << "[ERROR] Contenido vacío.\n";
		return false;
	}

	const size_t blockSize = 512;
	size_t totalBlocks = (content.size() + blockSize - 1) / blockSize;
	std::cout << "[addDocument] Total bloques: " << totalBlocks << std::endl;

	FileMetadata metadata;
	metadata.filename = filename;
	metadata.fileSize = content.size(); // Tamaño real

	// Iterar por cada "stripe" (grupo de bloques + 1 de paridad)
	for (size_t i = 0; i < totalBlocks; i += nNodes - 1) {
		std::vector<tec_mfs::BlockData> dataBlocks;
		std::vector<std::string> blockIds;

		for (size_t j = 0; j < nNodes - 1 && (i + j) < totalBlocks; ++j) {
			std::string blockId = generateBlockId();
			blockIds.push_back(blockId);

			std::string chunk = content.substr((i + j) * blockSize, blockSize);
			if (chunk.size() < blockSize) chunk.resize(blockSize, '\0');

			tec_mfs::BlockData block;
			block.set_block_id(blockId);
			block.set_data(chunk);
			block.set_is_parity(false);
			block.set_file_response(filename);

			dataBlocks.push_back(block);
		}

		// Calcular bloque de paridad XOR
		std::string parityData(blockSize, 0);
		for (const auto& blk : dataBlocks) {
			for (size_t k = 0; k < blockSize; ++k) {
				parityData[k] ^= blk.data()[k];
			}
		}

		std::string parityId = generateBlockId();
		tec_mfs::BlockData parityBlock;
		parityBlock.set_block_id(parityId);
		parityBlock.set_data(parityData);
		parityBlock.set_is_parity(true);
		parityBlock.set_file_response(filename);

		// Nodo de paridad rotativo
		size_t stripeIndex = i / (nNodes - 1);
		size_t parityNodeIndex = stripeIndex % nNodes;
		std::string parityNodeId = nodeIds[parityNodeIndex];

		// Enviar bloques de datos a nodos (excepto el nodo de paridad)
		size_t dataNodeIdx = 0;
		for (size_t nodeIdx = 0; nodeIdx < nNodes; ++nodeIdx) {
			std::string nodeId = nodeIds[nodeIdx];
			if (nodeIdx == parityNodeIndex) continue; // Nodo de paridad

			if (dataNodeIdx >= dataBlocks.size()) break;

			std::cout << "[addDocument] Enviando bloque " << dataBlocks[dataNodeIdx].block_id() << " al nodo " << nodeId << std::endl;

			if (!storeBlock(dataBlocks[dataNodeIdx], nodeId)) {
				std::cerr << "[ERROR] Falló enviar bloque " << dataBlocks[dataNodeIdx].block_id() << " al nodo " << nodeId << std::endl;
				return false;
			}

			BlockLocation loc{ nodeId, dataBlocks[dataNodeIdx].block_id(), false };
			metadata.blocks.push_back(loc);

			++dataNodeIdx;
		}

		// Enviar bloque de paridad
		std::cout << "[addDocument] Enviando bloque de paridad " << parityId << " al nodo " << parityNodeId << std::endl;
		if (!storeBlock(parityBlock, parityNodeId)) {
			std::cerr << "[ERROR] Falló enviar bloque de paridad al nodo " << parityNodeId << std::endl;
			return false;
		}
		BlockLocation parityLoc{ parityNodeId, parityBlock.block_id(), true };
		metadata.blocks.push_back(parityLoc);
	}

	// Guardar metadata del archivo
	filesMetadata[filename] = metadata;
	std::cout << "[+] Archivo '" << filename << "' almacenado correctamente." << std::endl;

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
		std::cerr << "[ERROR] gRPC status error: " << status.error_message() << std::endl;
		std::cerr << "[ERROR] Server message: " << response.message() << std::endl;
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

	const size_t nNodes = getDiskNodeIds().size();
	const size_t blockSize = 512;

	std::cout << "[getDocument] Iniciando recuperación del archivo: " << filename << std::endl;
	std::cout << "[getDocument] Total bloques en metadata: " << metadata.blocks.size() << std::endl;

	for (size_t stripeStart = 0; stripeStart < metadata.blocks.size(); stripeStart += nNodes) {
		std::vector<std::string> stripeData(nNodes);
		std::vector<bool> blockAvailable(nNodes, false);
		std::vector<bool> isParityBlock(nNodes, false);

		size_t stripeEnd = std::min(stripeStart + nNodes, metadata.blocks.size());

		std::cout << "[getDocument] Procesando stripe desde " << stripeStart << " hasta " << stripeEnd << std::endl;

		// Intentar recuperar todos los bloques del stripe
		for (size_t i = stripeStart; i < stripeEnd; ++i) {
			size_t relativeIndex = i - stripeStart;
			const auto& blockLoc = metadata.blocks[i];

			tec_mfs::BlockData blockData;
			if (retrieveBlock(blockLoc.blockId, blockData)) {
				stripeData[relativeIndex] = blockData.data();
				blockAvailable[relativeIndex] = true;
				isParityBlock[relativeIndex] = blockLoc.isParity;

				std::cout << "[getDocument] Bloque recuperado: " << blockLoc.blockId
					<< " (paridad: " << blockLoc.isParity << ")" << std::endl;
			}
			else {
				std::cout << "[getDocument] FALLO al recuperar bloque: " << blockLoc.blockId
					<< " del nodo " << blockLoc.nodeId << std::endl;
				isParityBlock[relativeIndex] = blockLoc.isParity;
			}
		}

		// Contar bloques perdidos
		int failedBlocks = 0;
		int failedIndex = -1;
		for (size_t i = 0; i < stripeEnd - stripeStart; ++i) {
			if (!blockAvailable[i]) {
				failedBlocks++;
				failedIndex = i;
			}
		}

		std::cout << "[getDocument] Bloques perdidos en stripe: " << failedBlocks << std::endl;

		if (failedBlocks > 1) {
			std::cerr << "[ERROR] RAID5 no puede recuperar más de 1 bloque perdido por stripe" << std::endl;
			return false;
		}

		if (failedBlocks == 1) {
			// Reconstruir bloque perdido con XOR
			std::string reconstructedData(blockSize, 0);

			for (size_t i = 0; i < stripeEnd - stripeStart; ++i) {
				if (i == failedIndex) continue; 

				const std::string& blockData = stripeData[i];
				for (size_t j = 0; j < blockSize; ++j) {
					reconstructedData[j] ^= blockData[j];
				}
			}

			stripeData[failedIndex] = reconstructedData;
			blockAvailable[failedIndex] = true;

			std::cout << "[getDocument] Bloque reconstruido en posición " << failedIndex
				<< " (era paridad: " << isParityBlock[failedIndex] << ")" << std::endl;
		}

		// Agregar solo los bloques de datos (no paridad) al resultado final
		for (size_t i = 0; i < stripeEnd - stripeStart; ++i) {
			if (!isParityBlock[i] && blockAvailable[i]) {
				fileData += stripeData[i];
				std::cout << "[getDocument] Agregando bloque de datos " << i << " al documento." << std::endl;
			}
		}

		std::cout << "[getDocument] Tamaño acumulado después del stripe: " << fileData.size() << " bytes." << std::endl;
	}

	if (fileData.size() > metadata.fileSize) {
		fileData.resize(metadata.fileSize);
	}

	response.set_file_data(fileData);
	auto* meta = response.mutable_metadata();
	meta->set_file_size(fileData.size());

	return true;
}

bool NodeController::deleteDocument(const std::string& filename) {
	auto it = filesMetadata.find(filename);
	if (it == filesMetadata.end()) {
		std::cerr << "[ERROR] Archivo no encontrado: " << filename << std::endl;
		return false;
	}

	const FileMetadata& metadata = it->second;
	for (const auto& block : metadata.blocks) {
		if(diskStubs.find(block.nodeId) == diskStubs.end()) {
			std::cerr << "[ERROR] Nodo no encontrado: " << block.nodeId << std::endl;
			continue;
		}

		auto stub = diskStubs[block.nodeId];
		grpc::ClientContext context;
		tec_mfs::BlockRequest request;
		request.set_block_id(block.blockId);
		tec_mfs::StatusResponse response;

		grpc::Status status = stub->DeleteBlock(&context, request, &response);
		if(!status.ok() || !response.success()) {
			std::cerr << "[ERROR] Fallo al eliminar bloque " << block.blockId 
				<< " del nodo " << block.nodeId << ": " << response.message() << std::endl;
			return false;
		}
		else {
			std::cout << "[-] Bloque " << block.blockId 
				<< " eliminado correctamente del nodo " << block.nodeId << std::endl;
		}
	}
	filesMetadata.erase(it);
	std::cout << "[-] Archivo '" << filename << "' eliminado correctamente." << std::endl;
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
