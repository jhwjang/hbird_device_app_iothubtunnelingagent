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
*@      File name       : mqtt_interface.h             
*@      Description     :                               
*@      Author Name     : hwanjang                              
*@      Version         : 0.1                           
*@      Date            : 07.July.2017                      
*@      		Initial draft        
**********************************************************************
*/
#include <string>

#include "hummingbird_01/hummingbird.h"

//#include "jansson.h"

struct IMQTTManagerSink
{
   virtual ~IMQTTManagerSink(){};

   virtual void ReceiveMessageFromPeer(mqtt::const_message_ptr mqttMsg) = 0;
   //virtual void ReceiveMessageFromPeer(int type, const std::string& topic, const std::string& deviceid, const std::string& user, const std::string& message) = 0;
   virtual void OnMQTTServerConnectionSuccess(void) = 0;

};

class HummingbirdMqttInterface
  : public hummingbird_topic_Observer
{
 public:
	HummingbirdMqttInterface(const std::string& server_address, const std::string& id, const std::string& pw);
	~HummingbirdMqttInterface();

	void MQTT_Init(const std::string& path); // 2018.10.29 - add version
	int MQTT_Start();
	int MQTT_Stop();
	time_t MQTT_lastConnection_Time();


    bool SendToMQTT(const std::string& topic, const std::string& message, int type);

    // interface for MQTTManager
    void RegisterMQTTManagerInterface(IMQTTManagerSink* pInstance)
    {   
        g_MQTTManagerinstance = pInstance;
    }   

    IMQTTManagerSink* getMQTTManagerInterface()
    {   
        return g_MQTTManagerinstance;
    }   
	
	virtual void OnReceiveTopicMessage(mqtt::const_message_ptr mqttMsg);
	//virtual void OnReceiveTopicMessage(const std::string& topic, const std::string& message);
    virtual void OnSendTopicMessage(const std::string& message);
	virtual void OnResponseCommandMessage(const std::string& topic, const std::string& message);
	virtual void OnResponseCommandMessage(const std::string& topic, const void* payload, int size);

	virtual void OnConnectSuccess(); // 2018.01.22 hwanjang - add
	virtual bool getConnectionStatus(); // 2018.01.22 hwanjang - add

	void SetDeviceStatus(const std::string& message);	
	void SendEventMessage(const std::string& message);
	void SendConnectionMessage(const std::string& message);

 private:
	bool connection_status;
	// topic parser
	bool find_deviceId(std::string topic, std::string* device_id);
	bool find_subdeviceId(std::string topic, std::string* subDevice_id);
	bool find_userId(std::string topic, std::string* user_id);
	bool find_command(std::string topic, std::string* strCommand);

	int create_topic(const std::string& topic);

	std::string hub_id_;
	std::string hub_pw_;
	
	mqtt::async_client client;
	mqtt::connect_options connopts;	

	hummingbird *_bird;

	static IMQTTManagerSink* g_MQTTManagerinstance;
};
