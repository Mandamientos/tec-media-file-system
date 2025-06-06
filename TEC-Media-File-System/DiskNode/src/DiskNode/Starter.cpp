#include "Starter.h"
#include "DiskNodeService.h"

#include <memory>
#include <iostream>

void Starter::startServer(diskNodeConfig& config) {
	DiskNodeService service(config);

	std::string fAddress = config.nodeAddress + ":" + std::to_string(config.nodePort);

	grpc::ServerBuilder builder;
	builder.AddListeningPort(fAddress, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);

	std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
	std::cout << "[SV] Disk node listening to: " << fAddress << std::endl;
	server->Wait();
}