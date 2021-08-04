#include <string>
#include <cstring>
#include <chrono>
#include "mqtt/async_client.h"
#include "mqtt/message.h"
#include "hummingbird/hummingbird_curl_for_command.h"

#if 1 // 2017.09.11 hwanjang - CFG -> Json file
#include "rtc_base_strings/json.h"
#endif

#if 0
#define HUB_PUB_TOPIC "hubs/hbtwh-hub01/devices/"
#define HUB_SUB_TOPIC "hummingbird/hubs/hbtwh-hub01/devices/"
#else
//#define HUB_PUB_TOPIC "hubs/hbtwh-a3ee0760-00d1-11e7-9983-adfa7ff293a5/devices/"
//#define HUB_SUB_TOPIC "hummingbird/hubs/hbtwh-a3ee0760-00d1-11e7-9983-adfa7ff293a5/devices/"
#endif
struct hummingbird_topic_Observer {
   virtual void OnReceiveTopicMessage(mqtt::const_message_ptr mqttMsg) = 0;
   virtual void OnReceiveTopicMessage(const std::string& topic, const std::string& message) = 0;
   virtual void OnSendTopicMessage(const std::string& message) = 0;
   virtual void OnResponseCommandMessage(const std::string& topic, const std::string& message) = 0;
   virtual void OnResponseCommandMessage(const std::string& topic, const void* payload, int size) = 0;
   virtual void OnConnectSuccess() = 0; // 2018.01.22 hwanjang - add
   virtual bool getConnectionStatus() = 0; // 2018.01.22 hwanjang - add

  protected:
   virtual ~hummingbird_topic_Observer() {};

};

class hummingbird_topic 
{
    private:
        std::string topic_;
	bool connect_status;

    protected:
        mqtt::async_client* cli_;
        hummingbird_curl_for_command* curl_for_command; 

	hummingbird_topic_Observer* callback_;

	std::string create_hubid_topic(int mode, std::string hub_id)
	{
        	std::string str;

        	if(mode == 0)
        	{
        		str = "devices/";
                	str.append(hub_id);
        		str.append("/subdevices/");
        	}
        	else
        	{
        		str = "hummingbird/devices/";
                	str.append(hub_id);
                	str.append("/subdevices/");
        	}

        	return str;
	};
	
    public:
    
        hummingbird_topic(mqtt::async_client* cli, std::string hub_id, std::string device_id, std::string user_id, hummingbird_curl_for_command* curl);
        virtual std::string get_topic()=0;
        virtual void set_topic(std::string str) = 0;
        virtual int mqtt_response(mqtt::const_message_ptr msg) =0;
        virtual int init() =0;
        void send_message(const char* topic, const char* payload, const int qos, const bool retain );
        void send_message(const char* topic, const void* payload, const int size, const int qos, const bool retain);

        void RegisterObserver(hummingbird_topic_Observer* callback);
        hummingbird_topic_Observer* getObserver() { return callback_; }

};

///////////////////////////////////////////////////////////////////////
// pub message for event monitor
class hummingbird_topic_pub_Event: public hummingbird_topic
{
    private:
        int QOS;
        std::string topic_;
    public:
        hummingbird_topic_pub_Event(mqtt::async_client* cli, std::string hub_id, std::string topic, std::string user_id, hummingbird_curl_for_command* curl);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();
};

///////////////////////////////////////////////////////////////////////
// pub message for message
class hummingbird_topic_pub_Message: public hummingbird_topic
{
    private:
        int QOS;
        std::string topic_;
    public:
        hummingbird_topic_pub_Message(mqtt::async_client* cli, std::string hub_id, std::string topic, std::string user_id, hummingbird_curl_for_command* curl);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();
};

///////////////////////////////////////////////////////////////////////
// pub message for P2P Stat Report
class hummingbird_topic_pub_Report: public hummingbird_topic
{
    private:
        int QOS;
        std::string topic_;
    public:
        hummingbird_topic_pub_Report(mqtt::async_client* cli, std::string hub_id, std::string topic, std::string user_id, hummingbird_curl_for_command* curl);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();
};

///////////////////////////////////////////////////////////////////////
// pub connection for device
class hummingbird_topic_pub_Connect: public hummingbird_topic
{
    private:
        int QOS;
        std::string topic_;
    public:
        hummingbird_topic_pub_Connect(mqtt::async_client* cli, std::string hub_id, std::string topic, std::string user_id, hummingbird_curl_for_command* curl);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();
};
//////////////////////////////////////////////////////////////////////
// sub connection for device
class hummingbird_topic_sub_Connect: public hummingbird_topic
{
    private:
        int QOS;
        std::string topic_;
        char curl_ret_msg[1024];

    public:
        hummingbird_topic_sub_Connect(mqtt::async_client* cli, std::string hub_id, std::string device_id, std::string user_id, hummingbird_curl_for_command* curl);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();
};
///////////////////////////////////////////////////////////////////////
// Live
///////////////////////////////////////////////////////////////////////
// pub live for device
class hummingbird_topic_pub_Live: public hummingbird_topic
{
    private:
        int QOS;
        std::string topic_;
    public:
        hummingbird_topic_pub_Live(mqtt::async_client* cli, std::string hub_id, std::string topic, std::string user_id, hummingbird_curl_for_command* curl);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();
};

//////////////////////////////////////////////////////////////////////
// sub live for device
class hummingbird_topic_sub_Live: public hummingbird_topic
{
    private:
        int QOS;
        std::string topic_;
        char curl_ret_msg[1024];

    public:
        hummingbird_topic_sub_Live(mqtt::async_client* cli, std::string hub_id, std::string device_id, std::string user_id, hummingbird_curl_for_command* curl);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();	
};

///////////////////////////////////////////////////////////////////////
// Playback
///////////////////////////////////////////////////////////////////////
// pub playback for device
class hummingbird_topic_pub_Playback: public hummingbird_topic
{
    private:
        int QOS;
        std::string topic_;
    public:
        hummingbird_topic_pub_Playback(mqtt::async_client* cli, std::string hub_id, std::string topic, std::string user_id, hummingbird_curl_for_command* curl);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();
};

//////////////////////////////////////////////////////////////////////
// sub playback for device
class hummingbird_topic_sub_Playback: public hummingbird_topic
{
    private:
        int QOS;
        std::string topic_;
        char curl_ret_msg[1024];

    public:
        hummingbird_topic_sub_Playback(mqtt::async_client* cli, std::string hub_id, std::string device_id, std::string user_id, hummingbird_curl_for_command* curl);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();
};

///////////////////////////////////////////////////////////////////////
// pub command for device
class hummingbird_topic_pub_Command: public hummingbird_topic
{
    private:
        int QOS;
        std::string topic_;
    public:
        hummingbird_topic_pub_Command(mqtt::async_client* cli, std::string hub_id, std::string topic, std::string user_id, hummingbird_curl_for_command* curl);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();

};
//////////////////////////////////////////////////////////////////////
// sub command for device
class hummingbird_topic_sub_Command: public hummingbird_topic
{
    private:
        int QOS;
        std::string topic_;
        char curl_ret_msg[1024];

    public:
        hummingbird_topic_sub_Command(mqtt::async_client* cli, std::string hub_id, std::string device_id, std::string user_id, hummingbird_curl_for_command* curl);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();

};


///////////////////////////////////////////////////////////////////////
// pub for device_01 SUNAPI Command
class hummingbird_topic_pub_Sunapi_Command: public hummingbird_topic
{
    private:
        int QOS;
        std::string topic_;
    public:
        hummingbird_topic_pub_Sunapi_Command(mqtt::async_client* cli, std::string hub_id, std::string topic, std::string user_id, hummingbird_curl_for_command* curl);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();

};

//////////////////////////////////////////////////////////////////////
// sub for device_01 SUNAPI Command
class hummingbird_topic_sub_Sunapi_Command: public hummingbird_topic
{
    private:
        int QOS;
        std::string topic_;
        char curl_ret_msg[1024];

    public:
        hummingbird_topic_sub_Sunapi_Command(mqtt::async_client* cli, std::string hub_id, std::string device_id, std::string user_id, hummingbird_curl_for_command* curl);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();

};

#if 1 // 2019.03.25 - for googleSmartHome
///////////////////////////////////////////////////////////////////////
// pub googleSmartHome 
class hummingbird_topic_pub_googleSmartHome: public hummingbird_topic
{
    private:
        int QOS;
        std::string topic_;
    public:
        hummingbird_topic_pub_googleSmartHome(mqtt::async_client* cli, std::string hub_id, std::string topic, std::string user_id, hummingbird_curl_for_command* curl);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();
};
#endif


///////////////////////////////////////////////////////////////////////
// Tunneling
///////////////////////////////////////////////////////////////////////
// pub Tunneling
class hummingbird_topic_pub_Tunneling: public hummingbird_topic
{
    private:
        int QOS;
        std::string topic_;
    public:
        hummingbird_topic_pub_Tunneling(mqtt::async_client* cli, std::string hub_id, std::string topic, std::string user_id, hummingbird_curl_for_command* curl);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();
};

//////////////////////////////////////////////////////////////////////
// sub for Tunneling 
class hummingbird_topic_sub_Tunneling: public hummingbird_topic
{
    private:
        int QOS;
        std::string topic_;
        char curl_ret_msg[1024];

    public:
        hummingbird_topic_sub_Tunneling(mqtt::async_client* cli, std::string hub_id, std::string device_id, std::string user_id, hummingbird_curl_for_command* curl);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();

};

