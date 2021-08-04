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

	void StartMainManager(int mode);

private:

	// for bridge
	int ThreadStartForbridgeManager();
	void thread_function_for_bridgeManager();

	// for broker
	int ThreadStartForbrokerManager();
	void thread_function_for_brokerManager();

	BridgeManager *bridge_handler_;
	BrokerManager *broker_handler_;



};

#endif // 