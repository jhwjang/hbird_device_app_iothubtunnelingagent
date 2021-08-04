#include <stdio.h>

#include "mqtt_manager_for_broker.h"

MQTTManager_for_broker::MQTTManager_for_broker(std::string address,
							std::string id, std::string pw)
	: gMQTTServerAddress(address),
	 gDeviceID(id),
	 gDevicePW(pw)	 
{
	printf("Create MQTTManager , ID : %s , PW : %s\n ", gDeviceID.c_str(), gDevicePW.c_str());
	printf("gMQTTServerAddress : %s\n", gMQTTServerAddress.c_str());

	MQTTHandler_ = nullptr;
	observerForHbirdManager = nullptr;
}

MQTTManager_for_broker::~MQTTManager_for_broker()
{
	if(MQTTHandler_)
		delete MQTTHandler_;
}

void MQTTManager_for_broker::init(const std::string& path)
{
	MQTTHandler_ = new HummingbirdMqttInterface_for_broker(gMQTTServerAddress, gDeviceID, gDevicePW);
	if (MQTTHandler_ != nullptr)
	{
		MQTTHandler_->RegisterMQTTManagerInterface(this);
		MQTTHandler_->MQTT_Init(path); // 2018.10.29 hwanjang - add version
	}
}

void MQTTManager_for_broker::start()
{
	// MQTT Start
	if (MQTTHandler_ != nullptr)
	{
		MQTTHandler_->MQTT_Start();
	}
}

time_t MQTTManager_for_broker::getLastConnection_Time()
{
	if (MQTTHandler_ != nullptr)
	{
		return MQTTHandler_->MQTT_lastConnection_Time();
	}

	return 0;
}

void MQTTManager_for_broker::SendMessageToClient(std::string topic, std::string message, int type)
{
	if (MQTTHandler_ != nullptr)
	{
		MQTTHandler_->SendToMQTT(topic, message, type);
	}
}

void MQTTManager_for_broker::OnResponseCommandMessage(std::string topic, std::string message)
{
	printf("MQTTManager::OnResponseCommandMessage() : topic : %s\n", topic.c_str());
	printf("MQTTManager::OnResponseCommandMessage() : message : %s\n", message.c_str());
	if (MQTTHandler_ != nullptr)
	{
		MQTTHandler_->OnResponseCommandMessage(topic, message);
	}
}

void MQTTManager_for_broker::OnResponseCommandMessage(std::string topic, const void* payload, int size)
{
	printf("MQTTManager::OnResponseCommandMessage() : topic : %s\n", topic.c_str());

	if (MQTTHandler_ != nullptr)
	{
		MQTTHandler_->OnResponseCommandMessage(topic, payload, size);
	}
}

// callback to PeerManager

void MQTTManager_for_broker::ReceiveMessageFromPeer(mqtt::const_message_ptr mqttMsg)
{
	std::string topic = mqttMsg->get_topic();
	std::string message = mqttMsg->to_string();
	printf("MQTTManager_for_broker::ReceiveMessageFromPeer() : topic : %s\n", topic.c_str());
	printf("MQTTManager_for_broker::ReceiveMessageFromPeer() : message : %s\n", message.c_str());

	if (observerForHbirdManager != nullptr)
	{
		observerForHbirdManager->ReceiveMessageFromPeer(mqttMsg);
	}
}

#if 0
void MQTTManager::ReceiveMessageFromPeer(int type, const std::string& topic, const std::string& deviceid, const std::string& user, const std::string& message)
{
	printf("MQTTManager::ReceiveMessageFromPeer() : topic : %s\n", topic.c_str());
	printf("MQTTManager::ReceiveMessageFromPeer() : message : %s\n", message.c_str());

	if (observerForHbirdManager != nullptr)
	{
		observerForHbirdManager->ReceiveMessageFromPeer(type, topic, deviceid, user, message);
	}
}
#endif

void MQTTManager_for_broker::OnMQTTServerConnectionSuccess(void)
{
	if (observerForHbirdManager != nullptr)
	{
		observerForHbirdManager->OnMQTTServerConnectionSuccess();
	}
}
