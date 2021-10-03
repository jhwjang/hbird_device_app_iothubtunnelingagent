#ifndef BRIDGE_MANAGER_H_
#define BRIDGE_MANAGER_H_
#pragma once 

#include <stdio.h>
#include <string>

#include "api_manager.h"
#include "mqtt_manager.h"

#include "config_manager.h"
#include "sunapi_manager.h"

class BridgeManager : public IMQTTManagerObserver,
					public IAPIManagerObserver, public ISUNAPIManagerObserver {
public:
	BridgeManager();
	~BridgeManager();

	void StartBridgeManager(std::string strDeviceID, std::string strDeviceKey, int nWebPort);

	virtual void OnMQTTServerConnectionSuccess(void);

	virtual void ReceiveMessageFromPeer(mqtt::const_message_ptr mqttMsg);
	//virtual void ReceiveMessageFromPeer(int type, const std::string& topic, const std::string& deviceid, const std::string& user, const std::string& message);

	virtual void SendToPeer(const std::string& topic, const std::string& message, int type);

	// api_manager.h - IAPIManagerObserver
	virtual void SendResponseToPeer(const std::string& topic, const std::string& message);
	virtual void SendResponseToPeerForTunneling(const std::string& topic, const void* payload, int size);

	// sunapi_manager.h - ISUNAPIManagerObserver
	virtual void SendResponseForDashboard(const std::string& topic, const std::string& message);

private:
	// for MQTT 
	bool Init_MQTT(std::string deviceID, std::string devicePW);
	void Start_MQTT();

	int file_exit(std::string& filename);

	///////////////////////////////////////////////////////////////
	// thread for MQTTMsg
	int ThreadStartForMQTTMsg(mqtt::const_message_ptr mqttMsg);
	void thread_function_for_MQTTMsg(mqtt::const_message_ptr mqttMsg);
	void process_command(const std::string& strTopic, mqtt::const_message_ptr mqttMsg);
	void process_SUNAPITunneling(const std::string& strTopic, mqtt::const_message_ptr mqttMsg);
	void process_HttpTunneling(const std::string& strTopic, mqtt::const_message_ptr mqttMsg);


	///////////////////////////////////////////////////////////////
	//ConfigManager* mConfig_manager_;
	//APIManager* mAPI_manager_;

	std::unique_ptr<ConfigManager> mConfig_manager_;
	std::unique_ptr<APIManager> mAPI_manager_;

	std::unique_ptr<sunapi_manager> mSUNAPI_manager_;

	std::unique_ptr<MQTTManager> mMQTT_manager_;
	///////////////////////////////////////////////////////////////

	std::string mMqtt_server_;
	int gMax_Channel;

};

#endif // 