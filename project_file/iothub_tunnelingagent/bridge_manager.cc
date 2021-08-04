#include <iostream>

#include "bridge_manager.h"

#include "jansson.h"

// time
//#include "win_time.h"

#define DeviceKeyFile "config/device_token.key"

#define CA_FILE_INFO "config/ca-file_info.cfg"
#define CA_FILE_PATH "{\"path\": \"config/ca-certificates.crt\"}"

#define USER_ACCESS_TOKEN "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VySWQiOiJodHd1LWI0MjAyMTUyLWJjZTYtNGI2MS1iNmYyLTBmOWE0YWQ1ZjcxMiIsImxvY2F0aW9uIjoidXMtd2VzdC0xIiwidXNlclN0YXR1cyI6InZlcmlmaWVkIiwidHNpIjoiMDk2MTc2ODAtN2I5Mi00MGExLTkxYWUtNTU2OTEwNDRmNTkzIiwic2NvcGUiOiJkZXZpY2UuaHR3ZDAwZDhjYjhhZjY1Mzc4Lm93bmVyIiwiaWF0IjoxNjIxMjQ1MTYyLCJleHAiOjE2MjEyNTIzNjEsImlzcyI6Imh0dzIuaGJpcmQtaW90LmNvbSIsImp0aSI6ImJkMzcyMjY1LWYxYzYtNDE4OS1iM2VmLTE4NmRlYTUxZTRhOSJ9.Z14Ot0SDGuSJVeiCFoEr-FrBk5JFHXyXnYFvnPYgwV3MxWBb8T9-KyKIYbtVMRJoEY9o4vyIQp7SGzcU7g18C0-B2Gmc3YmwDdDJWftXBtt7H-Pf6_bkNYGVUjfZBUd_hC-zneNEW_pexW3Gz6ZUJp3UnhjC0YMh591WM8alA7L5mwYIqMqrRbsRt3LFV4W5rCnKqUm7fRgoLkc9LBpTXfly3Q1JqYHGeCdNB_34h_y8oPVh-FZPBQyRxLTAt9uxwya_p9u-Tvs8FvLoDBZEDbz65p3wEKiyp7hp_gRHwoB8yBgIam6Qb9vwG4D42YVOVUGMp0pX-MkkiYcrorJdqQ"


BridgeManager::BridgeManager() {

	printf("[hwanjang] BridgeManager -> constructor !!!\n");

	mMQTT_manager_ = nullptr;
	mAPI_manager_ = nullptr;
	mConfig_manager_ = nullptr;
}

BridgeManager::~BridgeManager() {
	printf("[hwanjang] BridgeManager::~BridgeManager() -> Destructor !!!\n");

	if (mConfig_manager_)
		delete mConfig_manager_;

	if (mAPI_manager_)
		delete mAPI_manager_;

	if (mMQTT_manager_)
		delete mMQTT_manager_;

}

int BridgeManager::file_exit(std::string& filename)
{
	FILE* file;
	if ((file = fopen(filename.c_str(), "r")))
	{
		fclose(file);
		return 1;
	}
	return 0;
}

void BridgeManager::StartBridgeManager()
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

	//mMqtt_server_ = server_address;
	//mMqtt_server_ = "tcp://192.168.11.2:1883";
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

	int size = json_string_length(path_p);
	char* strPATH = new char[size];

	strcpy(strPATH, json_string_value(path_p));

	std::string strCAFilePath = strPATH;
#endif

	// Create MQTT Manager 
	int maxCh = 1; // Cloud Gateway
	mMQTT_manager_ = new MQTTManager(mMqtt_server_, deviceID, devicePW);
	mMQTT_manager_->RegisterObserverForMQTT(this);
	mMQTT_manager_->init(strCAFilePath);

	json_decref(json_out);

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
	// ...
	// tunneling
	std::string topic = mqttMsg->get_topic();

	if (topic.find("tunneling") != std::string::npos)
	{
		printf("Received tunneling command ...\n");
		process_tunneling(topic, mqttMsg);
	}
}

void BridgeManager::process_tunneling(const std::string& strTopic, mqtt::const_message_ptr mqttMsg)
{
	json_error_t error_check;
	json_t* json_strData = json_loads(mqttMsg->get_payload_str().c_str(), 0, &error_check);

	cout << "process_tunneling() -> " << json_dumps(json_strData, 0) << endl;

	int result;

	if(json_strData)
		result = mAPI_manager_->tunneling_command(strTopic, json_strData);
	else
	{
		// mqtt 로 받은 msg에 대한 파싱을 실패하면 해당 block 에서 에러 처리 해야 한다.
		printf("---> 1. Received unknown message !!!!\nmsg:\n%s\n return !!!!!!!!!!!!\n", mqttMsg->to_string().c_str());		
	}

	json_decref(json_strData);
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
	printf("** HummingbirdManager::SendResponseToPeer() -> Start !!\n");
	printf("--> topic : %s\n", topic.c_str());
#endif
	mMQTT_manager_->OnResponseCommandMessage(topic, payload, size);
}
