#ifndef BROKER_MANAGER_H_
#define BROKER_MANAGER_H_
#pragma once 

#include <stdio.h>
#include <string>

#include "mqtt_manager_for_broker.h"

class BrokerManager : public IMQTTManagerObserver_for_broker
{
public:
	BrokerManager();
	BrokerManager(std::string broker_agent_id, std::string broker_agent_key);

	~BrokerManager();

	void StartBrokerManager();

	virtual void OnMQTTServerConnectionSuccess(void);

	virtual void ReceiveMessageFromPeer(mqtt::const_message_ptr mqttMsg);
	//virtual void ReceiveMessageFromPeer(int type, const std::string& topic, const std::string& deviceid, const std::string& user, const std::string& message);

	void process_command(const std::string& strTopic, mqtt::const_message_ptr mqttMsg);

	void CommandRequestCloudServiceStatus(const std::string& strTopic, const std::string& strPayload);
	// for test
	void SendCloudServiceStatus(const std::string& target_app_id);

	virtual void SendToPeer(const std::string& target_app, const std::string& topic, const std::string& message);
	virtual void SendResponseToPeer(const std::string& topic, const std::string& message);
	virtual void SendResponseToPeerForTunneling(const std::string& topic, const void* payload, int size);

private:
	// for MQTT 
	bool Init_MQTT(std::string deviceID, std::string devicePW);
	void Start_MQTT();

	///////////////////////////////////////////////////////////////
	// thread for MQTTMsg
	int ThreadStartForMQTTMsg(mqtt::const_message_ptr mqttMsg);
	void thread_function_for_MQTTMsg(mqtt::const_message_ptr mqttMsg);

	///////////////////////////////////////////////////////////////
	MQTTManager_for_broker* mMQTT_manager_for_broker_;

	std::string mMqtt_server_;

	// temp
	std::string g_broker_agent_id;
	std::string g_broker_agent_key;
};

#endif // 