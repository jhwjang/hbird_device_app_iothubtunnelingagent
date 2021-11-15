#include <stdio.h>

#include "mqtt_manager.h"

MQTTManager::MQTTManager(std::string address,
							std::string id, std::string pw)
	: gMQTTServerAddress(address),
	 gDeviceID(id),
	 gDevicePW(pw)	 
{
#if 0 // for debug
	printf("MQTTManager() Create MQTTManager , ID : %s , PW : %s\n ", gDeviceID.c_str(), gDevicePW.c_str());
	printf("MQTTManager() gMQTTServerAddress : %s\n", gMQTTServerAddress.c_str());
#endif

	g_MQTT_interface_Handler = nullptr;
	observerForHbirdManager = nullptr;
}

MQTTManager::~MQTTManager()
{
	if(g_MQTT_interface_Handler)
		delete g_MQTT_interface_Handler;
}

void MQTTManager::init(const std::string& path)
{
	g_MQTT_interface_Handler = new HummingbirdMqttInterface(gMQTTServerAddress, gDeviceID, gDevicePW);
	if (g_MQTT_interface_Handler != nullptr)
	{
		g_MQTT_interface_Handler->RegisterMQTTManagerInterface(this);
		g_MQTT_interface_Handler->MQTT_Init(path); // 2018.10.29 hwanjang - add version
	}
}

void MQTTManager::start()
{
	// MQTT Start
	if (g_MQTT_interface_Handler != nullptr)
	{
		g_MQTT_interface_Handler->MQTT_Start();
	}
}

time_t MQTTManager::getLastConnection_Time()
{
	if (g_MQTT_interface_Handler != nullptr)
	{
		return g_MQTT_interface_Handler->MQTT_lastConnection_Time();
	}

	return 0;
}

void MQTTManager::SendMessageToClient(std::string topic, std::string message, int type)
{
	if (g_MQTT_interface_Handler != nullptr)
	{
		g_MQTT_interface_Handler->SendToMQTT(topic, message, type);
	}
}

void MQTTManager::OnResponseCommandMessage(std::string topic, std::string message)
{
#if 1
	time_t now = time(NULL);
	printf("[hwanjang] MQTTManager::OnResponseCommandMessage() -> time : %lld\n", now);
	printf("MQTTManager::OnResponseCommandMessage() : topic : %s\n", topic.c_str());
	printf("MQTTManager::OnResponseCommandMessage() : message : \n%s\n", message.c_str());
#endif

	if (g_MQTT_interface_Handler != nullptr)
	{
		g_MQTT_interface_Handler->OnResponseCommandMessage(topic, message);
	}
}

void MQTTManager::OnResponseCommandMessage(std::string topic, const void* payload, int size)
{
#if 1
	time_t now = time(NULL);
	printf("[hwanjang] MQTTManager::OnResponseCommandMessage() -> time : %lld\n", now);
	printf("MQTTManager::OnResponseCommandMessage() : topic : %s\n", topic.c_str());
	printf("MQTTManager::OnResponseCommandMessage() : payload : \n%s\n", payload);
#endif

	if (g_MQTT_interface_Handler != nullptr)
	{
		g_MQTT_interface_Handler->OnResponseCommandMessage(topic, payload, size);
	}
}

// callback to PeerManager

void MQTTManager::ReceiveMessageFromPeer(mqtt::const_message_ptr mqttMsg)
{
	std::string topic = mqttMsg->get_topic();
	std::string message = mqttMsg->to_string();

#if 1 // for debug
	time_t now = time(NULL);
	printf("[hwanjang] MQTTManager::ReceiveMessageFromPeer() -> time : %lld\n", now);
	printf("[hwanjang] topic : %s\n", topic.c_str());
	printf("[hwanjang] message : \n%s\n", message.c_str());
#endif

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

void MQTTManager::OnMQTTServerConnectionSuccess(void)
{
	if (observerForHbirdManager != nullptr)
	{
		observerForHbirdManager->OnMQTTServerConnectionSuccess();
	}
}
