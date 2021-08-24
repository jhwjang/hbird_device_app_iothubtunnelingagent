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

	gMax_Channel = 0;

}

BridgeManager::~BridgeManager() {
	printf("[hwanjang] BridgeManager::~BridgeManager() -> Destructor !!!\n");

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

	//mAPI_manager_ = new APIManager();
	mAPI_manager_ = std::make_unique<APIManager>();
	mAPI_manager_->RegisterObserverForHbirdManager(this);

	std::string user_token = USER_ACCESS_TOKEN;  // temp
	std::string address, device_id, device_key;

	user_token.clear(); // for test --> empty? -> read user_token.cfg

	//mConfig_manager_ = new ConfigManager();
	mConfig_manager_ = std::make_unique<ConfigManager>();
	result = mConfig_manager_->GetConfigModelDevice(user_token, &address, &device_id, &device_key);

	if (!result)
	{
		printf("[hwanjang] *** Error !!! config server -> failed to get MQTT Server address !! --> Set localhost ...\n");

		mMqtt_server_ = "tcp://localhost:2883";  // test for mosquitto bridge
	}
	else
	{
		if (address.empty())
		{
			mMqtt_server_ = "tcp://localhost:2883";  // test for mosquitto bridge
		}
		else	
		{
			if ((address.find("localhost") != std::string::npos) || (address.find("127.0.0.1") != std::string::npos))
			{
				mMqtt_server_ = "tcp://localhost:2883";  // test for mosquitto bridge
			}
			else
			{
				// Start to connect to MQTT Server 
				std::string server_address = "ssl://";
				server_address.append(address);

				mMqtt_server_ = server_address;
				//mMqtt_server_ = "tcp://192.168.11.2:1883";
			}
		}
	}

	std::cout << "BridgeManager::StartBridgeManager() -> Connect to MQTT Server ... " << mMqtt_server_ << std::endl;

	if (!Init_MQTT(device_id, device_key))
	{
		printf("[hwanjang] * * *Error !!! BridgeManagerStart() ->  server-> MQTT_Init is failed !! -- > exit !!!\n");
		exit(1);
	}

	// MQTT connect
	Start_MQTT();

#if 1  // for dashboard
	// using std::unique_ptr
	mSUNAPI_manager_ = std::make_unique<sunapi_manager>(device_id);
	mSUNAPI_manager_->RegisterObserverForHbirdManager(this);

	gMax_Channel = mSUNAPI_manager_->GetMaxChannel();

	printf("BridgeManager::StartBridgeManager() -> gMax_Channel : %d\n", gMax_Channel);

	mSUNAPI_manager_->SunapiManagerInit();

	// Test
	//mSUNAPI_manager_->TestDashboardView();
	//mSUNAPI_manager_->TestDeviceInfoView();
	//mSUNAPI_manager_->TestFirmwareVersionInfoView();
#endif

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
		printf("Received command ...\n");
		process_command(topic, mqttMsg);
	}
	else if (topic.find("sunapi") != std::string::npos)
	{
		printf("Received SUNAPI tunneling ...\n");
		process_SUNAPITunneling(topic, mqttMsg);
	}
	else if (topic.find("http") != std::string::npos)
	{
		printf("Received HTTP tunneling ...\n");
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

	//printf("process_command() -> receive command : %s\n", strPayload.c_str());

#if 1
	mSUNAPI_manager_->GetDataForDashboardAPI(strTopic, strPayload);
#else
	json_error_t error_check;
	json_t* json_strRoot = json_loads(mqttMsg->get_payload_str().c_str(), 0, &error_check);

	//cout << "process_command() -> " << json_dumps(json_strData, 0) << endl;

	int ret;
	char* charCommand, * charType;
	ret = json_unpack(json_strRoot, "{s:s, s:s}", "command", &charCommand, "type", &charType);

	if (ret)
	{
		printf("[hwanjang] Error !! process_command() -> json_unpack fail .. !!!\n");

		//printf("strPayload : %s\n", strPayload.c_str());
	}
	else
	{
		// Test
		//mSUNAPI_manager_->TestDashboardView();
		//mSUNAPI_manager_->TestDeviceInfoView();
		//mSUNAPI_manager_->TestFirmwareVersionInfoView();

		if (strncmp("dashboard", charCommand, 9) == 0)
		{
			mSUNAPI_manager_->GetDashboardView(strTopic, json_strRoot);
		}
		else if (strncmp("deviceinfo", charCommand, 10) == 0)
		{
			mSUNAPI_manager_->GetDeviceInfoView(strTopic, json_strRoot);
		}
		else if (strncmp("firmware", charCommand, 10) == 0)
		{
			if (strncmp("view", charType, 4) == 0)
			{
				mSUNAPI_manager_->GetFirmwareVersionInfoView(strTopic, json_strRoot);
			}
			else if (strncmp("update", charType, 4) == 0)
			{
				// not yet
			}
		}
	}

	json_decref(json_strRoot);
#endif
}

void BridgeManager::process_SUNAPITunneling(const std::string& strTopic, mqtt::const_message_ptr mqttMsg)
{
	json_error_t error_check;
	json_t* json_root = json_loads(mqttMsg->get_payload_str().c_str(), 0, &error_check);

	cout << "process_SUNAPITunneling() -> " << json_dumps(json_root, 0) << endl;

	bool result;

	if (json_root)
		result = mAPI_manager_->SUNAPITunnelingCommand(strTopic, json_root);
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

	cout << "process_tunneling() -> " << json_dumps(json_root, 0) << endl;

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
#if 1 // debug
	printf("** HummingbirdManager::SendResponseForDashboard() -> Start !!\n");
	printf("--> topic : %s\n", topic.c_str());
	printf("message : \n%s\n", message.c_str());
#endif

	mMQTT_manager_->OnResponseCommandMessage(topic, message);
}
