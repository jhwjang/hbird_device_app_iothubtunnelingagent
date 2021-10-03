#include <iostream>

#include "main_manager.h"

#include "jansson.h"

// time
//#include "win_time.h"

#define CA_FILE_INFO "config/ca-file_info.cfg"
#define CA_FILE_PATH "{\"path\": \"config/ca-certificates.crt\"}"

#define USER_ACCESS_TOKEN "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VySWQiOiJodHd1LWI0MjAyMTUyLWJjZTYtNGI2MS1iNmYyLTBmOWE0YWQ1ZjcxMiIsImxvY2F0aW9uIjoidXMtd2VzdC0xIiwidXNlclN0YXR1cyI6InZlcmlmaWVkIiwidHNpIjoiMDk2MTc2ODAtN2I5Mi00MGExLTkxYWUtNTU2OTEwNDRmNTkzIiwic2NvcGUiOiJkZXZpY2UuaHR3ZDAwZDhjYjhhZjY1Mzc4Lm93bmVyIiwiaWF0IjoxNjIxMjQ1MTYyLCJleHAiOjE2MjEyNTIzNjEsImlzcyI6Imh0dzIuaGJpcmQtaW90LmNvbSIsImp0aSI6ImJkMzcyMjY1LWYxYzYtNDE4OS1iM2VmLTE4NmRlYTUxZTRhOSJ9.Z14Ot0SDGuSJVeiCFoEr-FrBk5JFHXyXnYFvnPYgwV3MxWBb8T9-KyKIYbtVMRJoEY9o4vyIQp7SGzcU7g18C0-B2Gmc3YmwDdDJWftXBtt7H-Pf6_bkNYGVUjfZBUd_hC-zneNEW_pexW3Gz6ZUJp3UnhjC0YMh591WM8alA7L5mwYIqMqrRbsRt3LFV4W5rCnKqUm7fRgoLkc9LBpTXfly3Q1JqYHGeCdNB_34h_y8oPVh-FZPBQyRxLTAt9uxwya_p9u-Tvs8FvLoDBZEDbz65p3wEKiyp7hp_gRHwoB8yBgIam6Qb9vwG4D42YVOVUGMp0pX-MkkiYcrorJdqQ"


MainManager::MainManager() {

	printf("[hwanjang] MainManager -> constructor !!!\n");

	bridge_handler_ = nullptr;
	broker_handler_ = nullptr;

}

MainManager::~MainManager() {
	printf("[hwanjang] MainManager::~MainManager() -> Destructor !!!\n");

	if (bridge_handler_)
		delete bridge_handler_;

	if (broker_handler_)
		delete broker_handler_;
}

void MainManager::StartMainManager(int mode, std::string strDeviceID, std::string strDeviceKey, int nWebPort)
{
	switch (mode)
	{
		case 0:  // all mode
		{
			// bridge mode
			bridge_handler_ = new BridgeManager();
			ThreadStartForbridgeManager(strDeviceID, strDeviceKey, nWebPort);
			
			// broker mode
			broker_handler_ = new BrokerManager();
			ThreadStartForbrokerManager();

			break;
		}
		case 1:  // bridge mode
		{
			bridge_handler_ = new BridgeManager();
			ThreadStartForbridgeManager(strDeviceID, strDeviceKey, nWebPort);
			break;
		}
		case 2:  // broker mode
		{
			broker_handler_ = new BrokerManager();
			ThreadStartForbrokerManager();			
			break;
		}
		default : 
			break;
	}
}

int MainManager::ThreadStartForbridgeManager(std::string strDeviceID, std::string strDeviceKey, int nWebPort)
{
	std::thread thread_function_for_bridge([=] { thread_function_for_bridgeManager(strDeviceID, strDeviceKey, nWebPort); });
	thread_function_for_bridge.detach();

	return 0;
}

void MainManager::thread_function_for_bridgeManager(std::string strDeviceID, std::string strDeviceKey, int nWebPort)
{
	bridge_handler_->StartBridgeManager(strDeviceID, strDeviceKey, nWebPort);
}


int MainManager::ThreadStartForbrokerManager()
{
	std::thread thread_function_for_broker([=] { thread_function_for_brokerManager(); });
	thread_function_for_broker.detach();

	return 0;
}

void MainManager::thread_function_for_brokerManager()
{
	broker_handler_->StartBrokerManager();
}


#if 0  // old -> move bridge_manager.cc
void MainManager::StartMainManager()
{
	bool result = false;

	//APIManager* mAPI_manager_;
	mAPI_manager_ = new APIManager();
	mAPI_manager_->RegisterObserverForHbirdManager(this);

	std::string user_token = USER_ACCESS_TOKEN;  // temp
	std::string address, device_id, device_key;

	mConfig_manager_ = new ConfigManager();
	result = mConfig_manager_->GetConfigModelDevice(user_token, &address, &device_id, &device_key);

	if (!result || address.empty())
	{
		printf("[hwanjang] *** Error !!! config server -> failed to get MQTT Server address !! --> exit !!!\n");
		exit(1);
	}

	// Start to connect to MQTT Server 
	std::string server_address = "ssl://";
	server_address.append(address);

	mMqtt_server_ = server_address;
	//mMqtt_server_ = "tcp://192.168.11.2:1883";

	if (!Init_MQTT(device_id, device_key))
	{
		printf("[hwanjang] * * *Error !!! MainManagerStart() ->  server-> MQTT_Init is failed !! -- > exit !!!\n");
		exit(1);
	}

	// MQTT connect
	Start_MQTT();
}
#endif
