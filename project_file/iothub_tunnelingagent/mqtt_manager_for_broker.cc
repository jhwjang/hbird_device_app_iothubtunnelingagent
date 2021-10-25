#include <stdio.h>

#include "mqtt_manager_for_broker.h"

MQTTManager_for_broker::MQTTManager_for_broker(std::string address,
							std::string id, std::string key)
	: gMQTTServerAddress(address),
	 g_app_id_(id),
	 g_app_key_(key)	 
{
	printf("Create MQTTManager , app_id : %s , app_key : %s\n ", g_app_id_.c_str(), g_app_key_.c_str());
	printf("gMQTTServerAddress : %s\n", gMQTTServerAddress.c_str());

	g_MQTT_interface_Handler_for_broker = nullptr;
	observerForHbirdManager = nullptr;
}

MQTTManager_for_broker::~MQTTManager_for_broker()
{
	if(g_MQTT_interface_Handler_for_broker)
		delete g_MQTT_interface_Handler_for_broker;
}

void MQTTManager_for_broker::init(const std::string& path)
{
	g_MQTT_interface_Handler_for_broker = new HummingbirdMqttInterface_for_broker(gMQTTServerAddress, g_app_id_, g_app_key_);
	if (g_MQTT_interface_Handler_for_broker != nullptr)
	{
		g_MQTT_interface_Handler_for_broker->RegisterMQTTManagerInterface(this);
		g_MQTT_interface_Handler_for_broker->MQTT_Init(path); // 2018.10.29 hwanjang - add version
	}
}

void MQTTManager_for_broker::start()
{
	// MQTT Start
	if (g_MQTT_interface_Handler_for_broker != nullptr)
	{
		g_MQTT_interface_Handler_for_broker->MQTT_Start();
	}
}

time_t MQTTManager_for_broker::getLastConnection_Time()
{
	if (g_MQTT_interface_Handler_for_broker != nullptr)
	{
		return g_MQTT_interface_Handler_for_broker->MQTT_lastConnection_Time();
	}

	return 0;
}

std::string MQTTManager_for_broker::create_pub_topic(std::string app_id, std::string topic_type)
{
	// pub topic of app is res !!!
	std::string pub_topic = "ipc/res/subdevices/";
	pub_topic.append(app_id);
	pub_topic.append("/");
	pub_topic.append(topic_type);

	return pub_topic;
}

std::string MQTTManager_for_broker::create_response_pub_topic(std::string topic)
{
	int index = topic.find("ipc/req/");
	std::string subdeviceTopic = topic.substr(index + 8);

	std::string res_pub_topic = "ipc/res/";
	res_pub_topic.append(subdeviceTopic);

	return res_pub_topic;
}

void MQTTManager_for_broker::SendMessageToApp(std::string target_id, std::string topic_type, std::string message)
{
	std::string pub_topic = create_pub_topic(target_id, topic_type);

	printf("MQTTManager_for_broker::SendMessageToApp() pub topic : %s\n", pub_topic.c_str());
	printf("MQTTManager_for_broker::SendMessageToApp() message : %s\n", message.c_str());

	if (g_MQTT_interface_Handler_for_broker != nullptr)
	{
		g_MQTT_interface_Handler_for_broker->SendMQTTMessageToPeer(pub_topic, message);
	}
	else
	{
		printf("MQTTManager_for_broker::SendMessageToApp() g_MQTT_interface_Handler_for_broker is NULL !!!\n");
	}
}

void MQTTManager_for_broker::OnResponseCommandMessage(std::string topic, std::string message)
{
	std::string res_pub_topic = create_response_pub_topic(topic);

	printf("MQTTManager_for_broker::OnResponseCommandMessage() : res_pub_topic : %s\n", res_pub_topic.c_str());
	printf("MQTTManager_for_broker::OnResponseCommandMessage() : message : %s\n", message.c_str());
	if (g_MQTT_interface_Handler_for_broker != nullptr)
	{
		g_MQTT_interface_Handler_for_broker->OnResponseCommandMessage(res_pub_topic, message);
	}
}

void MQTTManager_for_broker::OnResponseCommandMessage(std::string topic, const void* payload, int size)
{
	std::string res_pub_topic = create_response_pub_topic(topic);

	printf("MQTTManager_for_broker::OnResponseCommandMessage() : res_pub_topic : %s\n", res_pub_topic.c_str());

	if (g_MQTT_interface_Handler_for_broker != nullptr)
	{
		g_MQTT_interface_Handler_for_broker->OnResponseCommandMessage(res_pub_topic, payload, size);
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
