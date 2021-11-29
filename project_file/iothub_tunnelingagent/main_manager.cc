#include <iostream>

#include "main_manager.h"

#include "jansson.h"
#include "print_log.h"

// time
//#include "win_time.h"

#define CA_FILE_INFO "config/ca-file_info.cfg"
#define CA_FILE_PATH "{\"path\": \"config/ca-certificates.crt\"}"

MainManager::MainManager() {
	bridge_handler_ = nullptr;
	broker_handler_ = nullptr;
}

MainManager::~MainManager() {

	if (bridge_handler_)
		delete bridge_handler_;

	if (broker_handler_)
		delete broker_handler_;
}

#ifndef USE_ARGV_JSON
void MainManager::StartMainManager(int mode, std::string strDeviceID, std::string strDeviceKey, int nWebPort)
{
	// temp -  broker ID
	// ipc/groups/$gid/did/$dest_appid/sid/$src_appid/command
	std::string broker_group_id = "7801e3a5-ef27-4ddc-8134-7097d7c794b3";
	std::string broker_agent_id = "mainAgent";  // cloud agent ID
	std::string broker_agent_key = broker_agent_id;
	broker_agent_key.append("_key");

#if 1
	std::string strGatewayIP, strGatewayPW;
	bool result = GetDeviceIP_PW(&strGatewayIP, &strGatewayPW);

	if (result != true)
	{
		strGatewayIP = "127.0.0.1";
		strGatewayPW = "admin:000ppp[[[";
	}
#endif

	switch (mode)
	{
		case 0:  // all mode
		{
			// bridge mode
			bridge_handler_ = new BridgeManager();
			ThreadStartForbridgeManager(strDeviceID, strDeviceKey, nWebPort, strGatewayIP, strGatewayPW);

			// broker mode
			broker_handler_ = new BrokerManager(broker_group_id, broker_agent_id, broker_agent_key);
			ThreadStartForbrokerManager();

			break;
		}
		case 1:  // bridge mode
		{
			bridge_handler_ = new BridgeManager();
			ThreadStartForbridgeManager(strDeviceID, strDeviceKey, nWebPort, strGatewayIP, strGatewayPW);
			break;
		}
		case 2:  // broker mode
		{
			broker_handler_ = new BrokerManager(broker_group_id, broker_agent_id, broker_agent_key);
			ThreadStartForbrokerManager();
			break;
		}
		default:
			break;
	}
}

int MainManager::ThreadStartForbridgeManager(std::string strDeviceID, std::string strDeviceKey, int nWebPort, std::string strGatewayIP, std::string strGatewayPW)
{
	std::thread thread_function_for_bridge([=] { thread_function_for_bridgeManager(strDeviceID, strDeviceKey, nWebPort, strGatewayIP, strGatewayPW); });
	thread_function_for_bridge.detach();

	return 0;
}

void MainManager::thread_function_for_bridgeManager(std::string strDeviceID, std::string strDeviceKey, int nWebPort, std::string strGatewayIP, std::string strGatewayPW)
{
	bridge_handler_->StartBridgeManager(strDeviceID, strDeviceKey, nWebPort, strGatewayIP, strGatewayPW);
}


int MainManager::GetDeviceIP_PW(std::string* strIP, std::string* strPW)
{
	bool result = false;

	std::string sunapi_info_file = "config/camera.cfg";

	json_error_t j_error;
	json_t* json_out = json_load_file(sunapi_info_file.c_str(), 0, &j_error);
	if (j_error.line != -1)
	{
		printf("[hwanjang] Error !! get camera info !!!\n");
		printf("json_load_file returned an invalid line number -- > CFG file check !!\n");
	}
	else
	{
		char* SUNAPIServerIP, * devicePW;
		result = json_unpack(json_out, "{s:s, s:s}", "ip", &SUNAPIServerIP, "pw", &devicePW);

		if (result || !SUNAPIServerIP || !devicePW)
		{
			printf("[hwanjang] Error !! camera cfg is wrong -> retur false !!!\n");
		}
		else
		{
			*strIP = SUNAPIServerIP;
			*strPW = devicePW;

			result = true;
		}
	}
	json_decref(json_out);

	return result;
}

#else

void MainManager::StartMainManager(int mode, std::string strJSONData)
{
	int result;
	json_error_t error_check;
	json_t* json_strRoot = json_loads(strJSONData.c_str(), 0, &error_check);

	if (!json_strRoot)
	{
		fprintf(stderr, "error : root\n");
		fprintf(stderr, "error : on line %d: %s\n", error_check.line, error_check.text);

		//printf("sunapi_manager::StartMainManager() -> strJSONData : \n%s\n", strJSONData.c_str());
		HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());

		return;
	}
	else
	{
		// deviceId & deviceKey
		char* charDeviceId, *charDeviceKey;
		result = json_unpack(json_strRoot, "{s:s, s:s}", "deviceId", &charDeviceId, "deviceKey", &charDeviceKey);

		if (result)
		{
			HWANJANG_LOG("json_unpack fail .. deviceId or deviceKey in json_strRoot !!!\n");
			HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());

			exit(1);
		}
		else
		{
			if ((strlen(charDeviceId) != 0) && (strlen(charDeviceKey) != 0))
			{
				g_SettingInfo.deviceId = charDeviceId;
				g_SettingInfo.deviceKey = charDeviceKey;
			}
			else
			{
				HWANJANG_LOG("json_unpack fail .. DeviceId or DeviceKey is empty() !!!\n");
				HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());

				exit(1);
			}
		}

		// MAC
		char* charMACAddress;
		result = json_unpack(json_strRoot, "{s:s}", "MAC", &charMACAddress);

		if (result)
		{
			HWANJANG_LOG("json_unpack fail .. MAC in json_strRoot !!!\n");
			HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());

			g_SettingInfo.MACAddress = "fail";
		}
		else
		{
			if (strlen(charMACAddress) != 0)
			{
				g_SettingInfo.MACAddress = charMACAddress;
			}
			else
			{
				HWANJANG_LOG("json_unpack fail .. MACAddress is empty() !!!\n");
				HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());

				g_SettingInfo.MACAddress = "fail";
			}
		}

		// systemConfig
		json_t* json_system = json_object_get(json_strRoot, "systemConfig");

		if (!json_system)
		{
			printf("cannot find systemConfig ... strJSONData : %s\n", strJSONData.c_str());
		}
		else
		{
			// version
			char* charVersion;
			result = json_unpack(json_system, "{s:s}", "version", &charVersion);
			if (result)
			{
				HWANJANG_LOG("json_unpack fail .. version in systemConfig !!!\n");
				HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());
			}
			else
			{
				//printf("GetStorageCheckOfSubdeivce() -> channel : %d, sdcard : %d , sdfail : %d\n",	channel, sdcard, sdfail);

				g_SettingInfo.System_info.version = charVersion;
			}

			// common
			json_t* json_sub_common = json_object_get(json_system, "common");

			if (!json_sub_common)
			{
				printf("cannot find systemConfig ... strJSONData : %s\n", strJSONData.c_str());
			}
			else
			{
				// web_port
				int nWebPort;
				result = json_unpack(json_sub_common, "{s:i}", "web_port", &nWebPort);

				if (result)
				{
					HWANJANG_LOG("json_unpack fail .. web_port in common !!!\n");
					HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());

					g_SettingInfo.System_info.https_port = 443;  // default
				}
				else
				{
					g_SettingInfo.System_info.https_port = nWebPort;
				}

				// deviceModel
				char* charDeviceModel;
				result = json_unpack(json_sub_common, "{s:s}", "deviceModel", &charDeviceModel);

				if (result)
				{
					HWANJANG_LOG("json_unpack fail .. deviceModel in common !!!\n");
					HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());

					g_SettingInfo.System_info.deviceModel = "cloudgateway";  // default
				}
				else
				{
					if(strlen(charDeviceModel) != 0)
						g_SettingInfo.System_info.deviceModel = charDeviceModel;
					else
						g_SettingInfo.System_info.deviceModel = "cloudgateway";  // default
				}

				// maxChannels
				int nMaxChannel;
				result = json_unpack(json_sub_common, "{s:i}", "maxChannels", &nMaxChannel);

				if (result)
				{
					HWANJANG_LOG("json_unpack fail .. web_port in common !!!\n");
					HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());

					g_SettingInfo.System_info.maxChannels = 128;  // default
				}
				else
				{
					g_SettingInfo.System_info.maxChannels = nMaxChannel;
				}
			}
			json_decref(json_sub_common);

			// private
			json_t* json_sub_private = json_object_get(json_system, "private");

			if (!json_sub_private)
			{
				printf("cannot find private ... strJSONData : \n%s\n", strJSONData.c_str());
			}
			else
			{
				// gateway ID
				char* charGatewayId;
				result = json_unpack(json_sub_private, "{s:s}", "cg_id", &charGatewayId);

				if (result)
				{
					HWANJANG_LOG("json_unpack fail .. cg_id in private !!!\n");
					HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());

					g_SettingInfo.System_info.gatewayId = "admin";  // default
				}
				else
				{
					if (strlen(charGatewayId) != 0)
						g_SettingInfo.System_info.gatewayId = charGatewayId;
					else
						g_SettingInfo.System_info.gatewayId = "admin";  // default
				}

				// gateway Password
				char* charGatewayPassword;
				result = json_unpack(json_sub_private, "{s:s}", "cg_password", &charGatewayPassword);

				if (result)
				{
					HWANJANG_LOG("json_unpack fail .. cg_password in private !!!\n");
					HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());

					g_SettingInfo.System_info.gatewayPassword = "1234";  // default
				}
				else
				{
					if (strlen(charGatewayPassword) != 0)
						g_SettingInfo.System_info.gatewayPassword = charGatewayPassword;
					else
						g_SettingInfo.System_info.gatewayPassword = "1234";  // default
				}

				// cloud account ID
				char* charCloudAccountId;
				result = json_unpack(json_sub_private, "{s:s}", "cloud_id", &charCloudAccountId);

				if (result)
				{
					HWANJANG_LOG("json_unpack fail .. cloud_id in private !!!\n");
					HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());

					g_SettingInfo.System_info.cloudAccountId = "default@wisenetlife.com";  // default
				}
				else
				{
					if (strlen(charCloudAccountId) != 0)
						g_SettingInfo.System_info.cloudAccountId = charCloudAccountId;
					else
						g_SettingInfo.System_info.cloudAccountId = "default@wisenetlife.com";  // default
				}

				// cloud account Password
				char* charCloudAccountPassword;
				result = json_unpack(json_sub_private, "{s:s}", "cloud_password", &charCloudAccountPassword);

				if (result)
				{
					HWANJANG_LOG("json_unpack fail .. cloud_password in private !!!\n");
					HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());

					g_SettingInfo.System_info.cloudAccountPassword = "pasword";  // default
				}
				else
				{
					if (strlen(charCloudAccountPassword) != 0)
						g_SettingInfo.System_info.cloudAccountPassword = charCloudAccountPassword;
					else
						g_SettingInfo.System_info.cloudAccountPassword = "pasword";  // default
				}
			}
			json_decref(json_sub_private);
		}
		json_decref(json_system);

#if 0 // no need
		// cloudServerConfig config Information
		json_t* json_configInfo = json_object_get(json_strRoot, "cloudServerConfig");

		if (!json_configInfo)
		{
			printf("cannot find systemConfig ... strJSONData : %s\n", strJSONData.c_str());
		}
		else
		{
			// wakeupInterval
			int nWakeupInterval;
			result = json_unpack(json_configInfo, "{s:i}", "wakeupInterval", &nWakeupInterval);

			if (result)
			{

				HWANJANG_LOG("json_unpack fail .. wakeupInterval in configInformation !!!\n");
				HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());

				g_SettingInfo.Config_info.wakeupInterval = 86400;
			}
			else
			{
				g_SettingInfo.Config_info.wakeupInterval = nWakeupInterval;
			}

			// cache-control
			json_t* json_cache_array = json_object_get(json_configInfo, "cache-control");

			if (!json_cache_array)
			{
				HWANJANG_LOG("NG1, json_object_get fail ... cache-control in configInformation !!\n");
				HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());

				g_SettingInfo.Config_info.cache_control = 300;
			}
			else
			{
				int ret;
				int nMaxAge;
				size_t index;
				json_t* obj;

				json_array_foreach(json_cache_array, index, obj)
				{
					//  - Type(<enum> DAS, NAS) , Enable , Satus
					ret = json_unpack(obj, "{s:i}", "max-age", &nMaxAge);

					if (ret)
					{
						HWANJANG_LOG("json_unpack fail .. max-age in cache-control !!!\n");
						HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());

						g_SettingInfo.Config_info.cache_control = 300;
					}
					else
					{
						g_SettingInfo.Config_info.cache_control = nMaxAge;
					}
				}
			}

			// mqtt
			json_t* json_sub_mqtt = json_object_get(json_configInfo, "mqtt");

			if (!json_sub_mqtt)
			{
				HWANJANG_LOG("cannot find mqtt ... strJSONData : \n%s\n", strJSONData.c_str());

				g_SettingInfo.Config_info.Mqtt_tls_server = "mqtt.prod.wisenetcloud.com:5000";  // default
			}
			else
			{
				// tls
				char* charMqttTls;
				result = json_unpack(json_sub_mqtt, "{s:s}", "tls", &charMqttTls);

				if (result)
				{
					HWANJANG_LOG("json_unpack fail .. tls in MQTT !!!\n");
					HWANJANG_LOG("strJSONData : \n%s\n", strJSONData.c_str());

					g_SettingInfo.Config_info.Mqtt_tls_server = "mqtt.prod.wisenetcloud.com:5000";  // default
				}
				else
				{
					if (strlen(charMqttTls) != 0)
						g_SettingInfo.Config_info.Mqtt_tls_server = charMqttTls;
					else
						g_SettingInfo.Config_info.Mqtt_tls_server = "mqtt.prod.wisenetcloud.com:5000";  // default
				}
			}
		}
#endif

	}
	json_decref(json_strRoot);


#if 1
	std::string strDeviceID = g_SettingInfo.deviceId;
	std::string strDeviceKey = g_SettingInfo.deviceKey;
	std::string strMac = g_SettingInfo.MACAddress;
	int nWebPort = g_SettingInfo.System_info.https_port;

#ifdef HWANJANG_DEBUG
	printf("StartMainManager() -> deviceId : %s\nMACAddress : %s\n",
		strDeviceID.c_str(), strMac.c_str());
#endif

	std::string strGatewayIP = "127.0.0.1";  // local
	g_SettingInfo.System_info.gatewayLocalHostIPAddress = "127.0.0.1";  // local
	std::string strGatewayPW = g_SettingInfo.System_info.gatewayId;
	strGatewayPW.append(":");
	strGatewayPW.append(g_SettingInfo.System_info.gatewayPassword);

	// temp -  broker ID
	// ipc/groups/$gid/did/$dest_appid/sid/$src_appid/command
	std::string broker_group_id = "7801e3a5-ef27-4ddc-8134-7097d7c794b3";
	std::string broker_agent_id = "mainAgent";  // cloud agent ID
	std::string broker_agent_key = broker_agent_id;
	broker_agent_key.append("_key");

	switch (mode)
	{
		case 0:  // all mode
		{
			// broker mode
			broker_handler_ = new BrokerManager(broker_group_id, broker_agent_id, broker_agent_key);
			ThreadStartForbrokerManager();

			// bridge mode
			bridge_handler_ = new BridgeManager();
			//ThreadStartForbridgeManager(strDeviceID, strDeviceKey, nWebPort, strGatewayIP, strGatewayPW, strMac);
			ThreadStartForbridgeManager(&g_SettingInfo);

			break;
		}
		case 1:  // bridge mode
		{
			bridge_handler_ = new BridgeManager();
			//ThreadStartForbridgeManager(strDeviceID, strDeviceKey, nWebPort, strGatewayIP, strGatewayPW, strMac);
			ThreadStartForbridgeManager(&g_SettingInfo);

			break;
		}

		case 2:  // broker mode
		{
			broker_handler_ = new BrokerManager(broker_group_id, broker_agent_id, broker_agent_key);
			ThreadStartForbrokerManager();
			break;
		}

		default:
			break;
	}
#endif

}

int MainManager::ThreadStartForbridgeManager(Setting_Infos* infos)
{
	std::thread thread_function_for_bridge([=] { thread_function_for_bridgeManager(infos); });
	thread_function_for_bridge.detach();

	return 0;
}

void MainManager::thread_function_for_bridgeManager(Setting_Infos* infos)
{
	bridge_handler_->StartBridgeManager(infos);
}
#endif

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
