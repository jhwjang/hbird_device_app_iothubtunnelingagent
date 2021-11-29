#pragma once

#define USE_ARGV_JSON

#if 1 // json - setting_ini
typedef struct _systemConfig_Info {
	// common
	int maxChannels;
	int https_port;
	std::string deviceModel;
	//privqte
	std::string gatewayLocalHostIPAddress;
	std::string gatewayId;
	std::string gatewayPassword;
	std::string cloudAccountId;
	std::string cloudAccountPassword;
	std::string version;
} SystemConfig_Info;

typedef struct _configInformation {
	int wakeupInterval;
	int cache_control;
	std::string mqtt_tls_server;
} Configuration_Info;

typedef struct _setting_Info {
	std::string deviceId;
	std::string deviceKey;
	std::string MACAddress;
	SystemConfig_Info System_info;
	Configuration_Info Config_info;
} Setting_Infos;
#endif