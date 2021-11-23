#include <iostream>

#include "bridge_manager.h"

#include "jansson.h"

// time
//#include "win_time.h"

#define DeviceKeyFile "config/device_token.key"

#define CA_FILE_INFO "config/ca-file_info.cfg"
#define CA_FILE_PATH "{\"path\": \"config/ca-certificates.crt\"}"

BridgeManager::BridgeManager() {

	mMQTT_manager_ = nullptr;
	mAPI_manager_ = nullptr;
//	mConfig_manager_ = nullptr;

	//gMax_Channel = 0;

}

BridgeManager::~BridgeManager() {

#if 0
	if (mConfig_manager_)
		delete mConfig_manager_;

	if (mAPI_manager_)
		delete mAPI_manager_;

	if (mMQTT_manager_)
		delete mMQTT_manager_;
#endif

}

int BridgeManager::file_exit(std::string& filename)
{
	FILE* p_file = NULL;
//	if ((p_file = fopen(filename.c_str(), "r")))
	if ((0 == fopen_s(&p_file, filename.c_str(), "r")))
	{
		fclose(p_file);
		return 1;
	}
	return 0;
}

#ifndef USE_ARGV_JSON
void BridgeManager::StartBridgeManager(std::string strDeviceID, std::string strDeviceKey, int nWebPort, std::string strIP, std::string strPW)
{
	bool result = false;

	std::string device_id = strDeviceID;
	std::string	device_key = strDeviceKey;

	std::string strGatewayIP, strGatewayPW;

	strGatewayIP = strIP;
	strGatewayPW = strPW;

	//mAPI_manager_ = new APIManager();
	mAPI_manager_ = std::make_unique<APIManager>();
	mAPI_manager_->RegisterObserverForHbirdManager(this);

	mAPI_manager_->init(strGatewayIP, strGatewayPW);

#if 1  // for dashboard
	// using std::unique_ptr
	mSUNAPI_manager_ = std::make_unique<sunapi_manager>();
	mSUNAPI_manager_->RegisterObserverForHbirdManager(this);

	//gMax_Channel = mSUNAPI_manager_->GetMaxChannel();
	//printf("BridgeManager::StartBridgeManager() -> gMax_Channel : %d\n", gMax_Channel);

	result =  mSUNAPI_manager_->SunapiManagerInit(strGatewayIP, strGatewayPW, device_id, nWebPort);

	if (!result)
	{
		printf("BridgeManager::StartBridgeManager() -> failed to SunapiManagerInit ...  \n");
	}

#endif

	mMqtt_server_ = "tcp://localhost:2883";  // test for mosquitto bridge

	std::cout << "BridgeManager::StartBridgeManager() -> Connect to MQTT Server ... " << mMqtt_server_ << std::endl;

	if (!Init_MQTT(device_id, device_key))
	{
		printf("[hwanjang] * * *Error !!! BridgeManagerStart() ->  server-> MQTT_Init is failed !! -- > exit !!!\n");
		exit(1);
	}

	// MQTT connect
	Start_MQTT();
}
#else
void BridgeManager::StartBridgeManager(Setting_Infos* infos)
{
	Setting_Infos _setting_infos;

	memcpy(&_setting_infos, infos, sizeof(Setting_Infos));
	memcpy(&_setting_infos.System_info, &infos->System_info, sizeof(_setting_infos.System_info));

#ifdef HWANJANG_DEBUG
	printf("[hwanjang] StartBridgeManager() ->  %s , %d \n", _setting_infos.deviceId.c_str(), _setting_infos.System_info.https_port);
#endif

	bool result = false;

	std::string strGatewayIP, strGatewayPW;
	int gatewayPort;

	strGatewayIP = _setting_infos.System_info.gatewayIPAddress;
	strGatewayPW = _setting_infos.System_info.gatewayId;
	strGatewayPW.append(":");
	strGatewayPW.append(_setting_infos.System_info.gatewayPassword);
	gatewayPort = _setting_infos.System_info.https_port;


	//mAPI_manager_ = new APIManager();
	mAPI_manager_ = std::make_unique<APIManager>();
	mAPI_manager_->RegisterObserverForHbirdManager(this);

	mAPI_manager_->init(strGatewayIP, strGatewayPW, gatewayPort);

#if 1  // for dashboard
	// using std::unique_ptr
	mSUNAPI_manager_ = std::make_unique<sunapi_manager>();
	mSUNAPI_manager_->RegisterObserverForHbirdManager(this);

	//gMax_Channel = mSUNAPI_manager_->GetMaxChannel();
	//printf("BridgeManager::StartBridgeManager() -> gMax_Channel : %d\n", gMax_Channel);

	//result = mSUNAPI_manager_->SunapiManagerInit(strGatewayIP, strGatewayPW, device_id, nWebPort, strMac);

	mSUNAPI_manager_->SunapiManagerInit(&_setting_infos);
#endif

	mMqtt_server_ = "tcp://localhost:2883";  // test for mosquitto bridge

#ifdef HWANJANG_DEBUG
	std::cout << "[hwanjang] BridgeManager::StartBridgeManager() -> Connect to MQTT Server ... " << mMqtt_server_ << std::endl;
#endif

	std::string device_id = _setting_infos.deviceId;
	std::string	device_key = _setting_infos.deviceKey;

	if (!Init_MQTT(device_id, device_key))
	{
		printf("[hwanjang] * * *Error !!! BridgeManagerStart() ->  server-> MQTT_Init is failed !! -- > exit !!!\n");
		exit(1);
	}

	// MQTT connect
	Start_MQTT();
}
#endif

bool BridgeManager::Init_MQTT(std::string deviceID, std::string devicePW)
{

	// 2018.02.25 hwanjan - CA file check
	std::string filename = CA_FILE_INFO;
	json_error_t j_error;
	json_t *json_out = json_load_file(filename.c_str(), 0, &j_error);

	if (j_error.line != -1)
	{
		printf("[hwanjang] Error !! CA_FILE_INFO !!!\n");
		printf("json_load_file returned an invalid line number -- > CFG file check !!\n");

		//json_out = json_string(CA_FILE_PATH);
		// char* -> json_t *string -> get json object -> get value from key
		json_out = json_loads(CA_FILE_PATH, 0, &j_error);
	}

#if 0
	char* strCAPath;
	int result = json_unpack(json_out, "{s:s}", "path", &strCAPath);

	if (result || !strCAPath)
	{
		printf("[hwanjang] Error !! hbirdURL cfg is wrong -> retur false !!!\n");

		json_decref(json_out);
		return false;
	}

	std::string strCAFilePath = strCAPath;
#else
	json_t* path_p = NULL;
	path_p = json_object_get(json_out, "path");

	if (!path_p || !json_is_string(path_p))
	{
		json_decref(json_out);
		return false;
	}
	const char* strPATH;

	strPATH = json_string_value(path_p);

	std::string strCAFilePath = strPATH;
#endif

	// Create MQTT Manager
	int maxCh = 1; // Cloud Gateway

	//mMQTT_manager_ = new MQTTManager(mMqtt_server_, deviceID, devicePW);
	mMQTT_manager_ = std::make_unique<MQTTManager>(mMqtt_server_, deviceID, devicePW);
	mMQTT_manager_->RegisterObserverForMQTT(this);
	mMQTT_manager_->init(strCAFilePath);

	json_decref(json_out);

#if 0
// using std::unique_ptr
std::unique_ptr<sunapi_manager> sunapi_handler(new sunapi_manager());
gMax_Channel = sunapi_handler->get_maxChannel();

printf("BridgeManager::Init_MQTT() -> gMax_Channel : %d\n", gMax_Channel);
#endif

	return true;
}

void BridgeManager::Start_MQTT()
{
	mMQTT_manager_->start();
}

void BridgeManager::OnMQTTServerConnectionSuccess(void)
{
	printf("BridgeManager::OnMQTTServerConnectionSuccess() --> do something ...\n");
}

///////////////////////////////////////////////////////////////////////////
/// <summary>
///
/// </summary>
/// <param name="mqttMsg"></param>
void BridgeManager::ReceiveMessageFromPeer(mqtt::const_message_ptr mqttMsg)
{
	ThreadStartForMQTTMsg(mqttMsg);
}

int BridgeManager::ThreadStartForMQTTMsg(mqtt::const_message_ptr mqttMsg)
{
	std::thread thread_function_th([=] { thread_function_for_MQTTMsg(mqttMsg); });
	thread_function_th.detach();

	return 0;
}

void BridgeManager::thread_function_for_MQTTMsg(mqtt::const_message_ptr mqttMsg)
{
	std::string topic = mqttMsg->get_topic();

	if (topic.empty())
	{
		printf("Received message but topic is empty ... return !!!!\n");
		return;
	}

	if (topic.find("command") != std::string::npos)
	{
#ifdef HWANJANG_DEBUG
		printf("Received command ...\n");
#endif
		process_command(topic, mqttMsg);
	}
	else if (topic.find("sunapi") != std::string::npos)
	{
#ifdef HWANJANG_DEBUG
		printf("Received SUNAPI tunneling ...\n");
#endif
		process_SUNAPITunneling(topic, mqttMsg);
	}
	else if (topic.find("http") != std::string::npos)
	{
#ifdef HWANJANG_DEBUG
		printf("Received HTTP tunneling ...\n");
#endif
		process_HttpTunneling(topic, mqttMsg);
	}
	else
	{
		printf("Received unknown topic ... %s \n", topic.c_str());
	}
}

void BridgeManager::process_command(const std::string& strTopic, mqtt::const_message_ptr mqttMsg)
{
	std::string strPayload = mqttMsg->get_payload_str().c_str();

#ifdef HWANJANG_DEBUG
	printf("process_command() -> receive command : %s\n", strPayload.c_str());
#endif

	if (strPayload.find("checkPassword") != std::string::npos)
		//if (strncmp("checkPassword", charCommand, 9) == 0)
	{
		mSUNAPI_manager_->CommandCheckPassword(strTopic, strPayload);
	}
#if 1
	else
	{
		mSUNAPI_manager_->GetDataForDashboardAPI(strTopic, strPayload);
	}
#else
	else if (strncmp("dashboard", charCommand, 9) == 0)
	{
		mSUNAPI_manager_->GetDashboardView(strTopic, json_strRoot);
	}
	else if (strncmp("deviceinfo", charCommand, 10) == 0)
	{
		mSUNAPI_manager_->GetDeviceInfoView(strTopic, json_strRoot);
	}
	else if (strncmp("firmware", charCommand, 8) == 0)
	{
		if (strncmp("view", charType, 4) == 0)
		{
			mSUNAPI_manager_->GetFirmwareVersionInfoView(strTopic, json_strRoot);
		}
		else if (strncmp("update", charType, 6) == 0)
		{
			// not yet
		}
	}
#endif

}

void BridgeManager::process_SUNAPITunneling(const std::string& strTopic, mqtt::const_message_ptr mqttMsg)
{
	json_error_t error_check;
	json_t* json_root = json_loads(mqttMsg->get_payload_str().c_str(), 0, &error_check);

#ifdef HWANJANG_DEBUG
	cout << "process_SUNAPITunneling() -> " << json_dumps(json_root, 0) << endl;
#endif

	bool result;

	if (json_root)
		//result = mAPI_manager_->SUNAPITunnelingCommand(strTopic, json_root);		// use api_manager
		result = mSUNAPI_manager_->SUNAPITunnelingCommand(strTopic, json_root);		// use sunapi_manager
	else
	{
		// mqtt 로 받은 msg에 대한 파싱을 실패하면 해당 block 에서 에러 처리 해야 한다.
		printf("---> 1. Received unknown message !!!!\nmsg:\n%s\n return !!!!!!!!!!!!\n", mqttMsg->to_string().c_str());
	}

	json_decref(json_root);
}

void BridgeManager::process_HttpTunneling(const std::string& strTopic, mqtt::const_message_ptr mqttMsg)
{
	json_error_t error_check;
	json_t* json_root = json_loads(mqttMsg->get_payload_str().c_str(), 0, &error_check);

#ifdef HWANJANG_DEBUG
	cout << "process_tunneling() -> " << json_dumps(json_root, 0) << endl;
#endif
	int result;

	if(json_root)
		result = mAPI_manager_->HttpTunnelingCommand(strTopic, json_root);
	else
	{
		// mqtt 로 받은 msg에 대한 파싱을 실패하면 해당 block 에서 에러 처리 해야 한다.
		printf("---> 1. Received unknown message !!!!\nmsg:\n%s\n return !!!!!!!!!!!!\n", mqttMsg->to_string().c_str());
	}

	json_decref(json_root);
}

void BridgeManager::SendToPeer(const std::string& topic, const std::string& message, int type) {
	//printf("[hwanjang] HummingbirdManager::SendToPeer() -> message :\n%s\n", message.c_str());

	mMQTT_manager_->SendMessageToClient(topic, message, type);
}

void BridgeManager::SendResponseToPeer(const std::string& topic, const std::string& message)
{
#if 0 // debug
	printf("** HummingbirdManager::SendResponseToPeer() -> Start !!\n");
	printf("--> topic : %s\n", topic.c_str());
#endif
	mMQTT_manager_->OnResponseCommandMessage(topic, message);
}

void BridgeManager::SendResponseToPeerForTunneling(const std::string& topic, const void* payload, int size)
{
#if 0 // debug
	printf("** HummingbirdManager::SendResponseToPeerForTunneling() -> Start !!\n");
	printf("--> topic : %s\n", topic.c_str());
#endif

	mMQTT_manager_->OnResponseCommandMessage(topic, payload, size);
}

void BridgeManager::SendResponseForDashboard(const std::string& topic, const std::string& message)
{
#if 0 // debug
	printf("** HummingbirdManager::SendResponseForDashboard() -> Start !!\n");
	printf("--> topic : %s\n", topic.c_str());
	printf("message : \n%s\n", message.c_str());
#endif

	mMQTT_manager_->OnResponseCommandMessage(topic, message);
}
