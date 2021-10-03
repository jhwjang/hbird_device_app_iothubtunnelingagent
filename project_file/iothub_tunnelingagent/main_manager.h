#ifndef MAIN_MANAGER_H_
#define MAIN_MANAGER_H_
#pragma once 

#include <stdio.h>
#include <string>

#include "bridge_manager.h"
#include "broker_manager.h"

class MainManager {
public:
	MainManager();
	~MainManager();

	void StartMainManager(int mode, std::string strDeviceID, std::string strDeviceKey, int nWebPort);

private:

	// for bridge
	int ThreadStartForbridgeManager(std::string strDeviceID, std::string strDeviceKey, int nWebPort);
	void thread_function_for_bridgeManager(std::string strDeviceID, std::string strDeviceKey, int nWebPort);

	// for broker
	int ThreadStartForbrokerManager();
	void thread_function_for_brokerManager();

	BridgeManager *bridge_handler_;
	BrokerManager *broker_handler_;



};

#endif // 