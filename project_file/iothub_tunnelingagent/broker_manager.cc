#include <iostream>
#include <thread>
#include <random>

#include "broker_manager.h"

#include "jansson.h"

// time
//#include "win_time.h"

#define CA_FILE_INFO "config/ca-file_info.cfg"
#define CA_FILE_PATH "{\"path\": \"config/ca-certificates.crt\"}"

BrokerManager::BrokerManager() {

	mMQTT_manager_for_broker_ = nullptr;

	g_broker_group_id.clear();
	g_broker_agent_id.clear();
	g_broker_agent_key.clear();
}

BrokerManager::BrokerManager(std::string broker_group_id, std::string broker_agent_id, std::string broker_agent_key) {

#ifdef HWANJANG_DEBUG
	printf("[hwanjang] BrokerManager -> constructor ... \nbroker_group_id : %s, broker_agent_id : %s\nbroker_agent_key : %s\n", 
		broker_group_id.c_str(), broker_agent_id.c_str(), broker_agent_key.c_str());
#endif

	mMQTT_manager_for_broker_ = nullptr;

	g_broker_group_id = broker_group_id;
	g_broker_agent_id = broker_agent_id;
	g_broker_agent_key = broker_agent_key;

}

BrokerManager::~BrokerManager() {

	if (mMQTT_manager_for_broker_)
		delete mMQTT_manager_for_broker_;
}

void BrokerManager::StartBrokerManager()
{
#ifdef HWANJANG_DEBUG
	std::cout << "BrokerManager::StartBrokerManager() -> StartBrokerManager() ... " << std::endl;
#endif

	if (!Init_MQTT(g_broker_group_id, g_broker_agent_id, g_broker_agent_key))
	{
		printf("[hwanjang] * * *Error !!! StartBrokerManager() ->  server-> MQTT_Init is failed !! -- > exit !!!\n");
		exit(1);
	}

	// MQTT connect
	Start_MQTT();
}

bool BrokerManager::Init_MQTT(std::string broker_group_id, std::string agent_id, std::string agent_key)
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
	mMQTT_manager_for_broker_ = new MQTTManager_for_broker(mMqtt_server_, broker_group_id, agent_id, agent_key);
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
	std::string topic = mqttMsg->get_topic();

	if (topic.empty())
	{
		printf("Received message but topic is empty ... return !!!!\n");
		return;
	}

	if (topic.find("command") != std::string::npos)
	{
#ifdef HWANJANG_DEBUG
		printf("[hwanjang] BrokerManager::thread_function_for_MQTTMsg() -> Received command ... topic : %s\n", topic.c_str());
#endif
		process_command(topic, mqttMsg);
	}
	else if (topic.find("message") != std::string::npos)
	{
#ifdef HWANJANG_DEBUG
		printf("[hwanjang] BrokerManager::thread_function_for_MQTTMsg() -> Received message ... topic : %s\n", topic.c_str());
#endif
	}
	else
	{
		printf("[hwanjang] BrokerManager::thread_function_for_MQTTMsg() -> Received unknown topic ... %s \n", topic.c_str());
	}
}

void BrokerManager::process_command(const std::string& strTopic, mqtt::const_message_ptr mqttMsg)
{
	std::string strPayload = mqttMsg->get_payload_str().c_str();

#ifdef HWANJANG_DEBUG
	printf("BrokerManager::process_command() -> receive command : %s\n", strPayload.c_str());
#endif

	if (strPayload.find("CloudServiceStatus") != std::string::npos)
	{
		CommandRequestCloudServiceStatus(strTopic, strPayload);

#if 1 // for test
		std::this_thread::sleep_for(std::chrono::seconds(10));

		std::string strPayload = mqttMsg->get_payload_str().c_str();
		
		std::string target_app = "serviceTray";

		SendCloudServiceStatus(target_app);
#endif
	}
}

void BrokerManager::process_message(const std::string& strTopic, mqtt::const_message_ptr mqttMsg)
{
	std::string strPayload = mqttMsg->get_payload_str().c_str();

#ifdef HWANJANG_DEBUG
	printf("BrokerManager::process_message() -> receive message : %s\n", strPayload.c_str());
#endif

	if (strPayload.find("exit") != std::string::npos)
	{
		// exit !!!!!!!!!!!!!!!!
	}
}

void BrokerManager::CommandRequestCloudServiceStatus(const std::string& strTopic, const std::string& strPayload)
{
	json_error_t error_check;
	json_t* json_strRoot = json_loads(strPayload.c_str(), 0, &error_check);

	if (!json_strRoot)
	{
		printf("BrokerManager::CommandRequestCloudServiceStatus() -> json_loads fail .. strPayload : \n%s\n", strPayload.c_str());

		return;
	}
		
	std::string strCommand, strType, strTid;

	int ret;
	char* charCommand, * charType, *charTid;
	ret = json_unpack(json_strRoot, "{s:s, s:s, s:s}", "command", &charCommand, "type", &charType, "tid", &charTid);

	if (ret)
	{
		printf("BrokerManager::CommandRequestCloudServiceStatus() -> json_unpack fail .. strPayload : \n%s\n", strPayload.c_str());
		return;
	}
	else
	{
		strCommand = charCommand;
		strType = charType;
		strTid = charTid;
	}

	json_t* main_ResponseMsg, * sub_Msg;

	main_ResponseMsg = json_object();
	sub_Msg = json_object();

	// main message
#if 0
	json_object_set(main_ResponseMsg, "command", json_string(strCommand.c_str()));
	json_object_set(main_ResponseMsg, "type", json_string(strType.c_str()));
	json_object_set(main_ResponseMsg, "tid", json_string(strTid.c_str()));
#else
	json_object_set(main_ResponseMsg, "command", json_string(charCommand));
	json_object_set(main_ResponseMsg, "type", json_string(charType));
	json_object_set(main_ResponseMsg, "tid", json_string(charTid));
#endif

	// sub message
	json_object_set(sub_Msg, "status", json_integer(1));

	json_object_set(main_ResponseMsg, "message", sub_Msg);

	std::string strMQTTMsg = json_dumps(main_ResponseMsg, 0);

#ifdef HWANJANG_DEBUG
	printf("[hwanjang] BrokerManager::CommandRequestCloudServiceStatus() response ---> size : %lu, send message : \n%s\n", strMQTTMsg.size(), strMQTTMsg.c_str());
#endif

	SendResponseToPeer(strTopic, strMQTTMsg);
}

void BrokerManager::SendToPeer(const std::string& target_app, const std::string& topic, const std::string& message) {
	//printf("[hwanjang] HummingbirdManager::SendToPeer() -> message :\n%s\n", message.c_str());

	mMQTT_manager_for_broker_->SendMessageToApp(target_app, topic, message);
}

void BrokerManager::SendResponseToPeer(const std::string& topic, const std::string& message)
{
#ifdef HWANJANG_DEBUG // debug
	printf("** HummingbirdManager::SendResponseToPeer() -> Start !!\n");
	printf("--> topic : %s\n", topic.c_str());
#endif
	mMQTT_manager_for_broker_->OnResponseCommandMessage(topic, message);
}

void BrokerManager::SendResponseToPeerForTunneling(const std::string& topic, const void* payload, int size)
{
#ifdef HWANJANG_DEBUG
	printf("** HummingbirdManager::SendResponseToPeer() -> Start !!\n");
	printf("--> topic : %s\n", topic.c_str());
#endif
	mMQTT_manager_for_broker_->OnResponseCommandMessage(topic, payload, size);
}

void BrokerManager::SendCloudServiceStatus(const std::string& target_app_id)
{
	json_t* mqtt_MainMsg = json_object();

	// json key - command
	json_object_set(mqtt_MainMsg, "command", json_string("CloudServiceStatus"));
	// json key - type
	json_object_set(mqtt_MainMsg, "type", json_string("check"));

	// json key - tid
	std::string strTid = "cmd_";

	std::random_device rd;  // seed 값을 얻기 위해 random_device 생성.
	//std::mt19937 gen(rd());   // 난수 생성 엔진 초기화.
	std::minstd_rand gen(rd()); // 난수 생성 엔진 초기화. (light version)
	std::uniform_int_distribution<int> dis(0, 20150101);    // 0~19840612 까지 난수열을 생성하기 위한 균등 분포.

	int tid = dis(gen) % 10000;
#if 0
	std::stringstream ss;
	ss << tid;
	std::string result = ss.str();
#else
	char result[8] = { 0, };
	sprintf_s(result, sizeof(result), "%d", tid);
#endif

	strTid.append(result);

	json_object_set(mqtt_MainMsg, "tid", json_string(strTid.c_str()));

	json_t* sub_Msg = json_object();

	// sub message
	json_object_set(sub_Msg, "status", json_integer(1));

	json_object_set(mqtt_MainMsg, "message", sub_Msg);

	std::string strMQTTMsg = json_dumps(mqtt_MainMsg, 0);

#ifdef HWANJANG_DEBUG
	printf("[hwanjang] BrokerManager::SendCloudServiceStatus() ---> size : %lu, send message : \n%s\n", strMQTTMsg.size(), strMQTTMsg.c_str());
#endif

	std::string topic_type = "message";	
	
	SendToPeer(target_app_id, topic_type, strMQTTMsg);
}
