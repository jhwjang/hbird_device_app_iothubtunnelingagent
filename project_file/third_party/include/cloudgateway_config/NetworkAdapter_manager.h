#ifndef NETWORK_ADAPTER_MANAGER_H_
#define NETWORK_ADAPTER_MANAGER_H_
#pragma once 

#include <stdio.h>
#include <string>

class NetworkApapterManager {
public:
	NetworkApapterManager();
	~NetworkApapterManager();

	std::string GetMacAddress();

private:

};

#endif // 