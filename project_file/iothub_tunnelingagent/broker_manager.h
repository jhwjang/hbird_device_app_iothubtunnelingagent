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
	~BrokerManager();

	void StartBrokerManager();

	virtual void OnMQTTServerConnectionSuccess(void);

	virtual void ReceiveMessageFromPeer(mqtt::const_message_ptr mqttMsg);
	//virtual void ReceiveMessageFromPeer(int type, const std::string& topic, const std::string& deviceid, const std::string& user, const std::string& message);

	virtual void SendToPeer(const std::string& topic, const std::string& message, int type);
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
};

#endif // 