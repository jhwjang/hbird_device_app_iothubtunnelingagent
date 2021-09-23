/*
***********************************************************************
**  Copyright (c) 2017 Hanwha Techwin Co. Ltd. all right reserved.  
**                                                                      
** This software is the property of Hanwha Techwin and is furnished     
** under license by Hanwha Techwin. This software may be used only      
** in accordance with the terms of said license. This copyright notice  
** may not be removed, modified or obliterated without the prior        
** written permission of Hanwha Techwin.                                
**                                                                      
** This software may not be copied, transmitted, provided to or         
** otherwise made available to any other person, company, corporation   
** or other entity except as specified in the terms of said  license.   
**                                                                      
** No right, title, ownership or other interest in the software is      
** hereby granted or transferred.                                     
**                                                                    
** The information contained herein is subject to change without      
** notice and should not be construed as a commitment by
** Hanwha Techwin.                                              
***********************************************************************
*/

/*
**********************************************************************
*@      File name       : mqtt_interface.cc      
*@      Description     :                       
*@      Author Name     : hwanjang              
*@      Version         : 0.1 
*@      Date        : 28.Mar.2017                      
*@                        Initial draft        
**********************************************************************
*/

#ifdef GCC_VERSION_UNDER_4_9 // 2017.11.14 hwanjang - for hisiv300 , hisiv500 - std::to_string error
#include <sstream>
#endif
#include <time.h>

#include "mqtt_interface_for_broker.h"

#ifdef MQTT_DEBUG  // for debug
#include "../IoTHub_agent/win_time.h"
#endif

#if 0
#define MQTT_DEBUG // for debug
#endif

#define CA_FILE_INFO "config/ca-file_info.cfg"

const int       WILLING_MSG_QOS = 1;
//const int       N_RETRY_ATTEMPTS = 5;

IMQTTManagerSink_for_broker* HummingbirdMqttInterface_for_broker::g_MQTTManagerinstance = nullptr;

HummingbirdMqttInterface_for_broker::HummingbirdMqttInterface_for_broker(
	const std::string& server_address, const std::string& id, const std::string& pw)
  : //client(server_address, id),
    client(server_address, "main_agent_broker"),
   connopts(id, pw)
{
    app_id_ = id;
    hub_id_ = id;  
    hub_pw_ = pw;

    printf("[hwanjang] HummingbirdMqttInterface_for_broker_for_broker Create -> hub id : %s, pw : %s\n", hub_id_.c_str(), hub_pw_.c_str());
 
    connection_status = false;
}

HummingbirdMqttInterface_for_broker::~HummingbirdMqttInterface_for_broker()
{
printf("[hwanjang] HummingbirdMqttInterface_for_broker::~HummingbirdMqttInterface_for_broker() -> Destructor !!!\n");

  if(_bird != NULL)
	delete _bird;
}


bool HummingbirdMqttInterface_for_broker::find_appId(std::string topic, std::string* app_id)
{
    int index = topic.find("apps/");
    std::string str = topic.substr(index + 5);
    index = str.find("/");
    std::string id = str.substr(0, index);

    if (!(id.empty()))
    {
        //printf("str : %s\nappId : %s\n", str.c_str(), id.c_str());
        *app_id = id;
        return true;
    }
    else
        return false;
}

bool HummingbirdMqttInterface_for_broker::find_deviceId(std::string topic, std::string* device_id)
{
    int index = topic.find("devices/");
    std::string str = topic.substr(index + 8);
    index = str.find("/");
    std::string id = str.substr(0, index);

    if (!(id.empty()))
    {
        //printf("str : %s\ndeviceId : %s\n", str.c_str(), id.c_str());
        *device_id = id;
        return true;
    }
    else
        return false;
}

bool HummingbirdMqttInterface_for_broker::find_subdeviceId(std::string topic, std::string* subDevice_id)
{
    int index = topic.find("subdevices/");
    std::string str = topic.substr(index + 11);
    index = str.find("/");
    std::string id = str.substr(0, index);

    if (!(id.empty()))
    {
        //printf("str : %s\nsubdeviceId : %s\n", str.c_str(), id.c_str());
        *subDevice_id = id;
        return true;
    }
    else
        return false;
}

bool HummingbirdMqttInterface_for_broker::find_userId(std::string topic, std::string* user_id)
{
    int index = topic.find("users/");
    std::string str = topic.substr(index + 6);
    index = str.find("/");
    std::string id = str.substr(0, index);

    if (!(id.empty()))
    {
        //printf("str : %s\nuserId : %s\n", str.c_str(), id.c_str());
        *user_id = id;
        return true;
    }
    else
        return false;
}

bool HummingbirdMqttInterface_for_broker::find_command(std::string topic, std::string* strCommand)
{
    int index = topic.rfind("/");
    std::string strCmd = topic.substr(index + 1);
    if ((!strCmd.empty()))
    {
        *strCommand = strCmd;
        return true;
    }
    else
        return false;
}

int HummingbirdMqttInterface_for_broker::create_topic(const std::string& topic)
{
printf("[hwanjang] HummingbirdMqttInterface_for_broker::create_topic() -> topic : %s\n", topic.c_str());

	int index, type=0;
    std::string str;
    std::string user, app_id;
    if (!find_userId(topic, &user))
        return 0;

    if (!find_appId(topic, &app_id))
        return 0;

    if(topic.rfind("command") != std::string::npos)
    {
		index = topic.find("apps");
        str = topic.substr(index);
        if( _bird->get_pub_topic_instance(str) < 0)
        {
			hummingbird_topic_for_broker* _pub_topic = new hummingbird_topic_pub_Command_for_broker(&client, app_id, str, user);
            _bird->add_topic(0, _pub_topic);
            _pub_topic->RegisterObserver(this);
        }

	}

//printf("[hwanjang] HummingbirdMqttInterface_for_broker::create_topic() -> topic : %s, -> return type : %d\n", topic.c_str(), type);

	return 0;
}

void HummingbirdMqttInterface_for_broker::MQTT_Init(const std::string& path)
{
	printf("*** HummingbirdMqttInterface_for_broker::MQTT_Init() --->\n");

    connopts.set_connect_timeout(5);

    connopts.set_keep_alive_interval(90); // keep alive 30 -> 90
    //connopts.set_keep_alive_interval(30);

    connopts.set_clean_session(true);

    mqtt::ssl_options sslopts;

    if (!path.empty())
    {
        sslopts.set_trust_store(path);
    }
    sslopts.set_enable_server_cert_auth(false);
//printf("get_enable_server_cert_auth() -> %d\n", sslopts.get_enable_server_cert_auth());

	std::string willing_topic;
#if 0
	willing_topic = "devices/";
	willing_topic.append(hub_id_);
	willing_topic.append("/connection");

////////////////////////////////////////////////////////////////////////////////////
	std::string LWT_PAYLOAD;
	LWT_PAYLOAD =  "{\"command\" : \"deviceConnection\", \"type\" : \"device\",\"message\" : { ";
	LWT_PAYLOAD.append("\"device\":\"offLine\", \"tid\":\"willingMsg_12345\", \"version\":\"");
	LWT_PAYLOAD.append(sw_version);
	LWT_PAYLOAD.append("\"}");
#else
    willing_topic = "HC";
    std::string LWT_PAYLOAD = "hummingbird last willing";
#endif

	mqtt::message willmsg(willing_topic, LWT_PAYLOAD, WILLING_MSG_QOS, true);
///////////////////////////////////////////////////////////////////////////////////
    mqtt::will_options will(willmsg);
    connopts.set_will(will);
    connopts.set_ssl(sslopts);

#if 1 // debug
printf("hub id : %s\n", connopts.get_user_name().c_str());
printf("hub pw : %s\n", connopts.get_password().c_str());
printf("willing_topic : %s\n", willing_topic.c_str());
printf("willingMsg :\n%s\n", LWT_PAYLOAD.c_str());
#endif

	_bird = new hummingbird_for_broker(&client, connopts, app_id_);

	// pub list
    hummingbird_topic_for_broker* pub_topic_message = new hummingbird_topic_pub_Message_for_broker(&client, app_id_, "+","+");
    _bird->add_topic(0, pub_topic_message);
    pub_topic_message->RegisterObserver(this);

	hummingbird_topic_for_broker* pub_topic_connect = new hummingbird_topic_pub_Connect_for_broker(&client, app_id_, "+","+");
	_bird->add_topic(0, pub_topic_connect);
	pub_topic_connect->RegisterObserver(this);

	// sublist
	// Connection
	hummingbird_topic_for_broker* sub_connect = new hummingbird_topic_sub_Connect_for_broker(&client, app_id_,  "+");
	_bird->add_topic(1, sub_connect);
	sub_connect->RegisterObserver(this);

	// Command
    hummingbird_topic_for_broker* sub_command = new hummingbird_topic_sub_Command_for_broker(&client, app_id_, "+");
    _bird->add_topic(1, sub_command);
    sub_command->RegisterObserver(this);

    SetDeviceStatus("HummingbirdMqttInterface_for_broker::MQTT_Init() ... Agent Start ... Device ON");
}

int HummingbirdMqttInterface_for_broker::MQTT_Start()
{
printf("*** HummingbirdMqttInterface_for_broker::MQTT_Start() --->\n");
    // Start the connection.
    try {
		std::cout << "Connecting to the MQTT server...\n" << std::flush;
        _bird->connect();		
    }
    catch (const mqtt::exception&) {
		std::cerr << "\nERROR: Unable to connect to MQTT server !!!\n" << std::endl;
        return 1;
    }

    return 1;
}

int HummingbirdMqttInterface_for_broker::MQTT_Stop()
{
//printf("*** HummingbirdMqttInterface_for_broker::MQTT_Stop() --->\n");

    try {
		std::cout << "\nDisconnecting from the MQTT server...\n" << std::flush;
        _bird->disconnect();
        std::cout << "OK" << std::endl;
    }
    catch (const mqtt::exception& exc) {
		std::cerr << exc.what() << std::endl;
        return 1;
    }

    return 1;
}

time_t HummingbirdMqttInterface_for_broker::MQTT_lastConnection_Time()
{
	return _bird->get_lastConnectionTime();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// hwanjang - Send to MQTT
bool HummingbirdMqttInterface_for_broker::SendToMQTT(const std::string& topic, const std::string& message, int type)
{
#ifdef MQTT_DEBUG  // for debug
    struct tm tmStart;
    time_t curr_time_st = time(nullptr);

    localtime_r(&curr_time_st, &tmStart);

	//printf("[hwanjang] HummingbirdMqttInterface_for_broker::SendToMQTT() -> Start !!\n");
	printf("[hwanjang] HummingbirdMqttInterface_for_broker::SendToMQTT() -> %d-%d-%d , %d : %d : %d , Start !!\n",
    	tmStart.tm_year+1900, tmStart.tm_mon+1, tmStart.tm_mday, tmStart.tm_hour, tmStart.tm_min, tmStart.tm_sec);

	printf("--> topic : %s\nmessage :\n%s\n", topic.c_str(), message.c_str());
#endif

	// 2018.02.06 hwanjang - hummingbird connection status check
	if(!_bird->get_connection_status())
	{
		printf("HummingbirdMqttInterface_for_broker::SendToMQTT() -> The connection was lost ... -> return !!!\n");
		return false;
	}

    std::string sendToTopic, pubTopic;
    pubTopic = "apps/";
    pubTopic.append(app_id_);
    pubTopic.append("/message");

#if 0
    sendToTopic = "devices/";
    sendToTopic.append(hub_id_);  // Global variable
    sendToTopic.append("/subdevices/");
    sendToTopic.append(sendTo);
#else
    int index = topic.find("hummingbird/");
    sendToTopic = topic.substr(index + 12);
#endif

#if 0		
    printf("--> sendToTopic : %s\n", sendToTopic.c_str());
    printf("--> pubTopic : %s\n", pubTopic.c_str());
#endif

    hummingbird_topic_for_broker* _pub_topic = _bird->get_pub_topic(pubTopic);
    if(_pub_topic != NULL)
    {
        _pub_topic->send_message(sendToTopic.c_str(), message.c_str(), 1, false);
    }
    else
    {
        printf("[hwanjang] HummingbirdMqttInterface_for_broker::SendToMQTT() -> pub topic %s not found !!!!\n", pubTopic.c_str());
    }

#ifdef MQTT_DEBUG  // for debug
  struct tm tmEnd;
  time_t curr_time_et = time(nullptr);

  localtime_r(&curr_time_et, &tmEnd);

    printf("** HummingbirdMqttInterface_for_broker::SendToMQTT() -> %d-%d-%d , %d : %d : %d , End !!\n",
		tmEnd.tm_year+1900, tmEnd.tm_mon+1, tmEnd.tm_mday, tmEnd.tm_hour, tmEnd.tm_min, tmEnd.tm_sec);
    printf("--> sendTo : %s\n\n", topic.c_str());

#endif

	return true;
}

void HummingbirdMqttInterface_for_broker::OnReceiveTopicMessage(mqtt::const_message_ptr mqttMsg)
{
#ifdef MQTT_DEBUG  // for debug
    printf("\n\n** HummingbirdMqttInterface_for_broker::OnReceiveTopicMessage() -> Start , subDeviceId : %s ---->\n", subDeviceId.c_str());
#endif

#if 0
    struct timespec tspec;
    clock_gettime(CLOCK_REALTIME, &tspec);
    printf("[hwanjang] HummingbirdMqttInterface_for_broker::OnReceiveTopicMessage(), time -> tv_sec : %lld, tv_nsec : %lld\n",
                (long long int)tspec.tv_sec, (long long int)tspec.tv_nsec);

    //printf("--> subDeviceId : %s\nuser : %s\ntopic: %s\n", subDeviceId.c_str(), user.c_str(), topic.c_str());
    //printf("--> command: %s\n", command.c_str());
    //printf("--> message : %s\n", message.c_str());
#endif


    if (g_MQTTManagerinstance != nullptr)
    {
        //g_MQTTManagerinstance->ReceiveMessageFromPeer(type, topic, subDeviceId, connection_name, message);
        g_MQTTManagerinstance->ReceiveMessageFromPeer(mqttMsg);
    }
    else
    {
        printf("[hwanjang] HummingbirdMqttInterface_for_broker::OnReceiveTopicMessage() -> g_MQTTManagerinstance is NULL !!\n");
    }
}

#if 0
void HummingbirdMqttInterface_for_broker::OnReceiveTopicMessage(const std::string& topic, const std::string& message)
{
	std::string user = find_userId(topic);
	std::string subDeviceId = find_subdeviceId(topic);
	std::string command = find_command(topic);

#ifdef MQTT_DEBUG  // for debug
	printf("\n\n** HummingbirdMqttInterface_for_broker::OnReceiveTopicMessage() -> Start , subDeviceId : %s ---->\n", subDeviceId.c_str());
#endif

#if 0
	struct timespec tspec;
	clock_gettime(CLOCK_REALTIME, &tspec);
	printf("[hwanjang] HummingbirdMqttInterface_for_broker::OnReceiveTopicMessage(), time -> tv_sec : %lld, tv_nsec : %lld\n", 
                (long long int)tspec.tv_sec, (long long int)tspec.tv_nsec);

	//printf("--> subDeviceId : %s\nuser : %s\ntopic: %s\n", subDeviceId.c_str(), user.c_str(), topic.c_str());
    //printf("--> command: %s\n", command.c_str());
	//printf("--> message : %s\n", message.c_str());
#endif

	//int type = create_topic(topic);

	std::string connection_name = subDeviceId;
	connection_name.append("/users/");
	connection_name.append(user);
	connection_name.append("/");
	connection_name.append(command);

	if(g_MQTTManagerinstance != nullptr)
	{
		g_MQTTManagerinstance->ReceiveMessageFromPeer(type, topic, subDeviceId, connection_name, message);
	}
	else
	{
		printf("[hwanjang] HummingbirdMqttInterface_for_broker::OnReceiveTopicMessage() -> g_MQTTManagerinstance is NULL !!\n");
	}
}
#endif

void HummingbirdMqttInterface_for_broker::OnSendTopicMessage(const std::string& message)
{
//   printf("HummingbirdMqttInterface_for_broker::OnSendTopicMessage() -> message : \n%s\n", message.c_str());
}

void HummingbirdMqttInterface_for_broker::OnResponseCommandMessage(const std::string& topic, const std::string& message)
{
#ifdef MQTT_DEBUG // for debug
    printf("** HummingbirdMqttInterface_for_broker::OnResponseCommandMessage() -> Start !!\n");
	printf("--> topic : %s\nmessage :\n%s\n", topic.c_str(), message.c_str());
#endif

	// 2018.02.06 hwanjang - hummingbird connection status check
	if(!_bird->get_connection_status())
    {
    	printf("HummingbirdMqttInterface_for_broker::OnResponseCommandMessage() -> The connection was lost ... -> return !!!\n");
        return;
    }
    std::string sendToTopic, pubTopic;
    pubTopic = "apps/";
    pubTopic.append(app_id_);
    pubTopic.append("/message");

#if 0
    sendToTopic = "devices/";
    sendToTopic.append(hub_id_);  // Global variable
    sendToTopic.append("/subdevices/");
    sendToTopic.append(sendTo);
#else
    int index = topic.find("hummingbird/");
    sendToTopic = topic.substr(index + 12);
#endif

#if 1		
    printf("--> sendToTopic : %s\n", sendToTopic.c_str());
    printf("--> pubTopic : %s\n", pubTopic.c_str());
#endif

    hummingbird_topic_for_broker* _pub_topic = _bird->get_pub_topic(pubTopic);
    if(_pub_topic != NULL)
    {
        _pub_topic->send_message(sendToTopic.c_str(), message.c_str(), 1, false);
    }
    else
    {
        printf("[hwanjang] HummingbirdMqttInterface_for_broker::OnResponseCommandMessage() -> pub topic %s not found !!!!\n", pubTopic.c_str());
    }
}


void HummingbirdMqttInterface_for_broker::OnResponseCommandMessage(const std::string& topic, const void* payload, int size)
{
#ifdef MQTT_DEBUG // for debug
    printf("** HummingbirdMqttInterface_for_broker::OnResponseCommandMessage() -> Start !!\n");
    printf("--> topic : %s\n", topic.c_str());
#endif

    // 2018.02.06 hwanjang - hummingbird connection status check
    if (!_bird->get_connection_status())
    {
        printf("HummingbirdMqttInterface_for_broker::OnResponseCommandMessage() -> The connection was lost ... -> return !!!\n");
        return;
    }
    std::string sendToTopic, pubTopic;
    pubTopic = "apps/";
    pubTopic.append(app_id_);
    pubTopic.append("/message");

#if 0
    sendToTopic = "devices/";
    sendToTopic.append(hub_id_);  // Global variable
    sendToTopic.append("/subdevices/");
    sendToTopic.append(sendTo);
#else
    int index = topic.find("hummingbird/");
    sendToTopic = topic.substr(index + 12);
#endif

#if 1		
    printf("--> sendToTopic : %s\n", sendToTopic.c_str());
    printf("--> pubTopic : %s\n", pubTopic.c_str());
#endif

    hummingbird_topic_for_broker* _pub_topic = _bird->get_pub_topic(pubTopic);
    if (_pub_topic != NULL)
    {
        _pub_topic->send_message(sendToTopic.c_str(), payload, size, 1, false);
    }
    else
    {
        printf("[hwanjang] HummingbirdMqttInterface_for_broker::OnResponseCommandMessage() -> pub topic %s not found !!!!\n", pubTopic.c_str());
    }
}

// 2018.01.22 hwanjang - add
void HummingbirdMqttInterface_for_broker::OnConnectSuccess(void)
{
#if 1
printf("[hwanjang] HummingbirdMqttInterface_for_broker::OnConnectSuccess() -> Start ...\n");
#endif
	connection_status = true;

	// 2019.12.11 add
    if(g_MQTTManagerinstance != NULL)
        g_MQTTManagerinstance->OnMQTTServerConnectionSuccess();
    else
        printf("[hwanjang] HummingbirdMqttInterface_for_broker::OnConnectSuccess() -> g_MQTTManagerinstance is NULL !!!\n");
}

// 2018.01.22 hwanjang - add
bool HummingbirdMqttInterface_for_broker::getConnectionStatus(void) 
{
#if 0
printf("[hwanjang] HummingbirdMqttInterface_for_broker::getConnectionStatus() -> Start ...%d\n", connection_status);
#endif
   return connection_status;
}

// 2018.01.18 hwanjang - for Camera Status using SUNAPI
void HummingbirdMqttInterface_for_broker::SetDeviceStatus(const std::string& message)
{
   //printf("HummingbirdMqttInterface_for_broker::SetDeviceStatus() -> message : \n%s\n", message.c_str());
   _bird->set_camera_presence(message);
}

void HummingbirdMqttInterface_for_broker::SendConnectionMessage(const std::string& message)
{
        std::string topic;
        topic = "apps/";
        topic.append(app_id_);  // Global variable
        topic.append("/connection");

#if 0  // for debug
        printf("** HummingbirdMqttInterface_for_broker::SendConnectionMessage() -> Start !!\n");
        printf("--> topic : %s\n", topic.c_str());
        printf("--> message :\n%s\n", message.c_str());
#endif

        // 2018.02.06 hwanjang - hummingbird connection status check
        if(!_bird->get_connection_status())
        {
                printf("HummingbirdMqttInterface_for_broker::SendConnectionMessage() -> The connection was lost ... -> return !!!\n");
                return;
        }

        hummingbird_topic_for_broker* _pub_topic = _bird->get_pub_topic(topic);

        if(_pub_topic != NULL)
        {
			_pub_topic->send_message(topic.c_str(), message.c_str(), 1, true);  // true : retain
        }
        else
        {
			printf("pub topic %s not found !!!!\n", topic.c_str());
        }

printf("** HummingbirdMqttInterface_for_broker::SendConnectionMessage() -> End !!\n");		
}
