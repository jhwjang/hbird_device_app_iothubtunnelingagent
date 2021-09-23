#include <iostream>

#include "broker_manager.h"

#include "jansson.h"

// time
//#include "win_time.h"

#define CA_FILE_INFO "config/ca-file_info.cfg"
#define CA_FILE_PATH "{\"path\": \"config/ca-certificates.crt\"}"

#define USER_ACCESS_TOKEN "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VySWQiOiJodHd1LWI0MjAyMTUyLWJjZTYtNGI2MS1iNmYyLTBmOWE0YWQ1ZjcxMiIsImxvY2F0aW9uIjoidXMtd2VzdC0xIiwidXNlclN0YXR1cyI6InZlcmlmaWVkIiwidHNpIjoiMDk2MTc2ODAtN2I5Mi00MGExLTkxYWUtNTU2OTEwNDRmNTkzIiwic2NvcGUiOiJkZXZpY2UuaHR3ZDAwZDhjYjhhZjY1Mzc4Lm93bmVyIiwiaWF0IjoxNjIxMjQ1MTYyLCJleHAiOjE2MjEyNTIzNjEsImlzcyI6Imh0dzIuaGJpcmQtaW90LmNvbSIsImp0aSI6ImJkMzcyMjY1LWYxYzYtNDE4OS1iM2VmLTE4NmRlYTUxZTRhOSJ9.Z14Ot0SDGuSJVeiCFoEr-FrBk5JFHXyXnYFvnPYgwV3MxWBb8T9-KyKIYbtVMRJoEY9o4vyIQp7SGzcU7g18C0-B2Gmc3YmwDdDJWftXBtt7H-Pf6_bkNYGVUjfZBUd_hC-zneNEW_pexW3Gz6ZUJp3UnhjC0YMh591WM8alA7L5mwYIqMqrRbsRt3LFV4W5rCnKqUm7fRgoLkc9LBpTXfly3Q1JqYHGeCdNB_34h_y8oPVh-FZPBQyRxLTAt9uxwya_p9u-Tvs8FvLoDBZEDbz65p3wEKiyp7hp_gRHwoB8yBgIam6Qb9vwG4D42YVOVUGMp0pX-MkkiYcrorJdqQ"


BrokerManager::BrokerManager() {

	printf("[hwanjang] MainManager -> constructor !!!\n");

	mMQTT_manager_for_broker_ = nullptr;
}

BrokerManager::~BrokerManager() {
	printf("[hwanjang] MainManager::~MainManager() -> Destructor !!!\n");

	if (mMQTT_manager_for_broker_)
		delete mMQTT_manager_for_broker_;
}

void BrokerManager::StartBrokerManager()
{
	// temp
	std::string agent_id = "app-mainagent";  // cloud agent ID
	std::string agent_key = "agentKey";

	if (!Init_MQTT(agent_id, agent_key))
	{
		printf("[hwanjang] * * *Error !!! StartBrokerManager() ->  server-> MQTT_Init is failed !! -- > exit !!!\n");
		exit(1);
	}

	// MQTT connect
	Start_MQTT();
}

bool BrokerManager::Init_MQTT(std::string agentID, std::string agentPW)
{

	// 2018.02.25 hwanjan - CA file check
	std::string filename = CA_FILE_INFO;
	json_error_t j_error;
	json_t *json_out = json_load_file(filename.c_str(), 0, &j_error);

	if (!json_out)
	{
		cout << "the error variable contains error information .... json_load_file() " << endl;
	}

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
		printf("[hwanjang] Error !! broker , ca-info cfg is wrong -> retur false !!!\n");

		json_decref(json_out);
		return false;
	}

	std::string strCAFilePath = strCAPath;
#else
	std::string strCAFilePath;
	json_t* path_p = NULL;
	path_p = json_object_get(json_out, "path");

	if (!path_p || !json_is_string(path_p))
	{
		json_decref(json_out);		
	}
	else
	{

		int size = json_string_length(path_p);

#if 0 // use std::unique_ptr
		std::unique_ptr<char[]> strPATH(new char[size]);

		strcpy(strPATH, json_string_value(path_p));

		strCAFilePath = strPATH;
#else
		const char* strPATH;

		strPATH = json_string_value(path_p);

		strCAFilePath = strPATH;
#endif


	}
#endif

    //mMqtt_server_ = server_address;
    //mMqtt_server_ = "tcp://192.168.11.2:1883";
    mMqtt_server_ = "tcp://localhost:1883";  // test for mosquitto broker

	// Create MQTT Manager 
	mMQTT_manager_for_broker_ = new MQTTManager_for_broker(mMqtt_server_, agentID, agentPW);
	mMQTT_manager_for_broker_->RegisterObserverForMQTT(this);
	mMQTT_manager_for_broker_->init(strCAFilePath);

	json_decref(json_out);

	return true;
}

void BrokerManager::Start_MQTT()
{
	mMQTT_manager_for_broker_->start();
}

void BrokerManager::OnMQTTServerConnectionSuccess(void)
{
	printf("MainManager::OnMQTTServerConnectionSuccess() --> do something ...\n");
}

///////////////////////////////////////////////////////////////////////////
/// <summary>
/// 
/// </summary>
/// <param name="mqttMsg"></param>
void BrokerManager::ReceiveMessageFromPeer(mqtt::const_message_ptr mqttMsg)
{
	ThreadStartForMQTTMsg(mqttMsg);
}

int BrokerManager::ThreadStartForMQTTMsg(mqtt::const_message_ptr mqttMsg)
{
	std::thread thread_function_th([=] { thread_function_for_MQTTMsg(mqttMsg); });
	thread_function_th.detach();

	return 0;
}

void BrokerManager::thread_function_for_MQTTMsg(mqtt::const_message_ptr mqttMsg)
{
	// ...

}

void BrokerManager::SendToPeer(const std::string& topic, const std::string& message, int type) {
	//printf("[hwanjang] HummingbirdManager::SendToPeer() -> message :\n%s\n", message.c_str());

	mMQTT_manager_for_broker_->SendMessageToClient(topic, message, type);
}

void BrokerManager::SendResponseToPeer(const std::string& topic, const std::string& message)
{
#if 0 // debug
	printf("** HummingbirdManager::SendResponseToPeer() -> Start !!\n");
	printf("--> topic : %s\n", topic.c_str());
#endif
	mMQTT_manager_for_broker_->OnResponseCommandMessage(topic, message);
}

void BrokerManager::SendResponseToPeerForTunneling(const std::string& topic, const void* payload, int size)
{
#if 0 // debug
	printf("** HummingbirdManager::SendResponseToPeer() -> Start !!\n");
	printf("--> topic : %s\n", topic.c_str());
#endif
	mMQTT_manager_for_broker_->OnResponseCommandMessage(topic, payload, size);
}
