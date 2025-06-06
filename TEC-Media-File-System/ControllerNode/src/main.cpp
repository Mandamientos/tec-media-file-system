#include "Starter.h"
#include <string>

int main() {
	std::string address = "0.0.0.0:25565";
	Starter::startServer(address);
	return 0;
}