#ifndef MAIN_MANAGER_H_
#define MAIN_MANAGER_H_
#pragma once 

#include <stdio.h>
#include <string>

#include "bridge_manager.h"
#include "broker_manager.h"

#include "define_for_debug.h"

#if 0 // json - setting_ini
typedef struct _systemConfig_Info {	
	// common
	int web_port;
	std::string deviceModel; 
	//privqte
	std::string GatewayIPAddress;
	std::string GatewayId;		
	std::string GatewayPassword;
	std::string CloudAccountId;
	std::string CloudAccountPassword;
	std::string version;
} SystemConfig_Info;

typedef struct _configInformation {
	int wakeupInterval;
	int cache_control;
	std::string Mqtt_tls_server; 
} Configuration_Info;

typedef struct _setting_Info {
	std::string DeviceId;	
	std::string DeviceKey;		
	std::string MACAddress;
	SystemConfig_Info System_info;
	Configuration_Info Config_info;
} Setting_Infos;
#endif

class MainManager {
public:
	MainManager();
	~MainManager();

#ifndef USE_ARGV_JSON
	void StartMainManager(int mode, std::string strDeviceID, std::string strDeviceKey, int nWebPort);

private:

	// for bridge
	int ThreadStartForbridgeManager(std::string strDeviceID, std::string strDeviceKey, int nWebPort, std::string strGatewayIP, std::string strGatewayPW);
	void thread_function_for_bridgeManager(std::string strDeviceID, std::string strDeviceKey, int nWebPort, std::string strGatewayIP, std::string strGatewayPW);

	int GetDeviceIP_PW(std::string* strIP, std::string* strPW);

#else

	void StartMainManager(int mode, std::string strJSONData);

private:

	// for bridge
	int ThreadStartForbridgeManager(Setting_Infos* infos);
	void thread_function_for_bridgeManager(Setting_Infos* infos);
#endif

	// for broker
	int ThreadStartForbrokerManager();
	void thread_function_for_brokerManager();
	

	BridgeManager *bridge_handler_;
	BrokerManager *broker_handler_;

	Setting_Infos g_SettingInfo;

};

#endif // 