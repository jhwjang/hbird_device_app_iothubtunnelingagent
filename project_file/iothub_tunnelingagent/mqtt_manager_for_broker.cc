#include <stdio.h>

#include "mqtt_manager_for_broker.h"

MQTTManager_for_broker::MQTTManager_for_broker(std::string address,
				std::string broker_group_id, std::string id, std::string key)
	: gMQTTServerAddress(address),
	 g_goup_id(broker_group_id),
	 g_app_id_(id),
	 g_app_key_(key)	 
{
#if 0
	printf("MQTTManager_for_broker() Create MQTTManager , group_id : %s , app_id : %s , app_key : %s\n ", g_goup_id.c_str(), g_app_id_.c_str(), g_app_key_.c_str());
	printf("MQTTManager_for_broker() gMQTTServerAddress : %s\n", gMQTTServerAddress.c_str());
#endif

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
	g_MQTT_interface_Handler_for_broker = new HummingbirdMqttInterface_for_broker(gMQTTServerAddress, g_goup_id, g_app_id_, g_app_key_);
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

std::string MQTTManager_for_broker::create_topic(std::string d_id, std::string s_id, std::string topic_type)
{
	// topic 
	// ipc/groups/$gid/did/$dest_appid/sid/$src_appid/command
	std::string pub_topic = "ipc/groups/";
	pub_topic.append(g_goup_id);
	pub_topic.append("/did/");
	pub_topic.append(d_id);
	pub_topic.append("/sid/");
	pub_topic.append(s_id);
	pub_topic.append("/");
	pub_topic.append(topic_type);

	return pub_topic;
}

std::string MQTTManager_for_broker::create_response_pub_topic(std::string topic)
{
	// change topic -> $dest_appid <-> $src_appid 
	// ipc/groups/$gid/did/$dest_appid/sid/$src_appid/command

	int index = topic.find("sid/");
	std::string str = topic.substr(index + 4);
	index = str.find("/");
	std::string source_id = str.substr(0, index);

	if (!(source_id.empty()))
	{
		printf("find_sourceId() -> topic : %s , source_id : %s\n", topic.c_str(), source_id.c_str());
	}
	else
	{
		source_id = " unknown";
	}

	std::string pub_topic = "ipc/groups/";
	pub_topic.append(g_goup_id);
	pub_topic.append("/did/");
	pub_topic.append(source_id);
	pub_topic.append("/sid/");
	pub_topic.append(g_app_id_);
	pub_topic.append("/");

	std::string topic_type;
	if (topic.find("command") != std::string::npos)
	{
		topic_type = "command";
	}
	else if (topic.find("message") != std::string::npos)
	{
		topic_type = "message";
	}
	else
	{
		printf("Received unknown topic ... %s \n", topic.c_str());
		topic_type = "unknown";
	}

	pub_topic.append(topic_type);

	return pub_topic;
}

void MQTTManager_for_broker::SendMessageToApp(std::string target_id, std::string topic_type, std::string message)
{
	std::string pub_topic = create_topic(target_id, g_app_id_, topic_type);

#if 0
	printf("MQTTManager_for_broker::SendMessageToApp() pub topic : %s\n", pub_topic.c_str());
	printf("MQTTManager_for_broker::SendMessageToApp() message : %s\n", message.c_str());
#endif

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

#if 0
	printf("MQTTManager_for_broker::OnResponseCommandMessage() : res_pub_topic : %s\n", res_pub_topic.c_str());
	printf("MQTTManager_for_broker::OnResponseCommandMessage() : message : %s\n", message.c_str());
#endif

	if (g_MQTT_interface_Handler_for_broker != nullptr)
	{
		g_MQTT_interface_Handler_for_broker->OnResponseCommandMessage(res_pub_topic, message);
	}
}

void MQTTManager_for_broker::OnResponseCommandMessage(std::string topic, const void* payload, int size)
{
	std::string res_pub_topic = create_response_pub_topic(topic);

	//printf("MQTTManager_for_broker::OnResponseCommandMessage() : res_pub_topic : %s\n", res_pub_topic.c_str());

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

#if 0
	printf("MQTTManager_for_broker::ReceiveMessageFromPeer() : topic : %s\n", topic.c_str());
	printf("MQTTManager_for_broker::ReceiveMessageFromPeer() : message : %s\n", message.c_str());
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

void MQTTManager_for_broker::OnMQTTServerConnectionSuccess(void)
{
	if (observerForHbirdManager != nullptr)
	{
		observerForHbirdManager->OnMQTTServerConnectionSuccess();
	}
}
