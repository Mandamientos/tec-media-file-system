#include "ConfigLoader.h"
#include "Starter.h"
#include "tec_mfs.grpc.pb.h"
#include <grpcpp/grpcpp.h>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

bool registerNode(diskNodeConfig& config) {
	tec_mfs::NodeInfo nodeInfo;
	nodeInfo.set_node_address(config.nodeAddress);
	nodeInfo.set_node_port(config.nodePort);
	nodeInfo.set_node_id(config.nodeId);
	nodeInfo.set_storage_path(config.storagePath);
	nodeInfo.set_node_capacity(config.maxStorageSize);

	std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(config.controllerAddress, grpc::InsecureChannelCredentials());
	std::shared_ptr<tec_mfs::FileSytem::Stub> stub = tec_mfs::FileSytem::NewStub(channel);
	tec_mfs::StatusResponse statusResponse;
	grpc::ClientContext context;

	grpc::Status status = stub->RegisterDiskNode(&context, nodeInfo, &statusResponse);

	if (status.ok()) {
		if (statusResponse.success()) {
			std::cout << "[+] DiskNode registered successfully." << std::endl;
			return true;
		}
		else {
			std::cerr << "[-] Failed to register DiskNode: " << statusResponse.message() << std::endl;
			return false;
		}
	}
	else {
		std::cerr << "[-] gRPC call failed: " << status.error_message() << std::endl;
		return false;
	}
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <config_file_path>" << std::endl;
		return 1;
	}

	std::string configFilePath = argv[1];
	diskNodeConfig config;

	if(!ConfigLoader::loadConfig(configFilePath, config)) {
		std::cerr << "Failed to load configuration from " << configFilePath << std::endl;
		return 1;
	}

	if(!registerNode(config)) {
		return 1;
	}

	if(!fs::exists(config.storagePath)) {
		fs::create_directories(config.storagePath);
		std::cout << "[+] Created storage directory: " << config.storagePath << std::endl;
	}

	Starter::startServer(config);

	return 0;
}