#include "Starter.h"
#include "Server.h"

#include <memory>
#include <iostream>

void Starter::startServer(const std::string& serverAddress) {
	FileSystemServiceImpl service;

	grpc::ServerBuilder builder;
	builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);

	std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
	std::cout << "Server started at " << serverAddress << std::endl;
	server->Wait();
}