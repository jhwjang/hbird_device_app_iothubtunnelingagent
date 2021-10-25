#include <string>
#include <cstring>
#include <chrono>
#include "mqtt/async_client.h"
#include "mqtt/message.h"

#if 0
#define HUB_PUB_TOPIC "hubs/hbtwh-hub01/devices/"
#define HUB_SUB_TOPIC "hummingbird/hubs/hbtwh-hub01/devices/"
#else
//#define HUB_PUB_TOPIC "hubs/hbtwh-a3ee0760-00d1-11e7-9983-adfa7ff293a5/devices/"
//#define HUB_SUB_TOPIC "hummingbird/hubs/hbtwh-a3ee0760-00d1-11e7-9983-adfa7ff293a5/devices/"
#endif
struct hummingbird_topic_Observer_for_broker {
   virtual void OnReceiveTopicMessage(mqtt::const_message_ptr mqttMsg) = 0;
   //virtual void OnReceiveTopicMessage(const std::string& topic, const std::string& message) = 0;
   virtual void OnSendTopicMessage(const std::string& message) = 0;
   virtual void OnResponseCommandMessage(const std::string& topic, const std::string& message) = 0;
   virtual void OnResponseCommandMessage(const std::string& topic, const void* payload, int size) = 0;
   virtual void OnConnectSuccess() = 0; // 2018.01.22 hwanjang - add
   virtual bool getConnectionStatus() = 0; // 2018.01.22 hwanjang - add

  protected:
   virtual ~hummingbird_topic_Observer_for_broker() {};

};

class hummingbird_topic_for_broker 
{
    private:
        //std::string topic_;

    protected:
        mqtt::async_client* g_async_client;

        hummingbird_topic_Observer_for_broker* callback_;

        std::string create_topic(int mode, std::string app_id, std::string subTopic);
	
    public:
    
        hummingbird_topic_for_broker(mqtt::async_client* cli);
        virtual std::string get_topic()=0;
        virtual void set_topic(std::string str) = 0;
        virtual int mqtt_response(mqtt::const_message_ptr msg) =0;
        virtual int init() =0;
        void send_message(const char* topic, const char* payload, const int qos, const bool retain );
        void send_message(const char* topic, const void* payload, const int size, const int qos, const bool retain);

        void RegisterObserver(hummingbird_topic_Observer_for_broker* callback);
        hummingbird_topic_Observer_for_broker* getObserver() { return callback_; }

};

///////////////////////////////////////////////////////////////////////
// pub connection for device
class hummingbird_topic_pub_Connect_for_broker : public hummingbird_topic_for_broker
{
    private:
        int g_QOS;
        std::string g_Topic_;
    public:
        hummingbird_topic_pub_Connect_for_broker(mqtt::async_client* cli, std::string app_id);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();
};
//////////////////////////////////////////////////////////////////////
// sub connection for device
class hummingbird_topic_sub_Connect_for_broker : public hummingbird_topic_for_broker
{
    private:
        int g_QOS;
        std::string g_Topic_;
        char curl_ret_msg[1024];

    public:
        hummingbird_topic_sub_Connect_for_broker(mqtt::async_client* cli, std::string app_id);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();
};

///////////////////////////////////////////////////////////////////////
// pub for req message
class hummingbird_topic_pub_ReqMessage_for_broker : public hummingbird_topic_for_broker
{
private:
    int g_QOS;
    std::string g_Topic_;
public:
    hummingbird_topic_pub_ReqMessage_for_broker(mqtt::async_client* cli, std::string app_id);
    std::string get_topic();
    void set_topic(std::string topic);
    int mqtt_response(mqtt::const_message_ptr msg);
    int init();
};

// pub for res message
class hummingbird_topic_pub_ResMessage_for_broker : public hummingbird_topic_for_broker
{
private:
    int g_QOS;
    std::string g_Topic_;
public:
    hummingbird_topic_pub_ResMessage_for_broker(mqtt::async_client* cli, std::string app_id);
    std::string get_topic();
    void set_topic(std::string topic);
    int mqtt_response(mqtt::const_message_ptr msg);
    int init();
};

///////////////////////////////////////////////////////////////////////
// sub for req message
class hummingbird_topic_sub_ReqMessage_for_broker : public hummingbird_topic_for_broker
{
private:
    int g_QOS;
    std::string g_Topic_;
public:
    hummingbird_topic_sub_ReqMessage_for_broker(mqtt::async_client* cli, std::string app_id);
    std::string get_topic();
    void set_topic(std::string topic);
    int mqtt_response(mqtt::const_message_ptr msg);
    int init();
};

// sub for res message
class hummingbird_topic_sub_ResMessage_for_broker : public hummingbird_topic_for_broker
{
private:
    int g_QOS;
    std::string g_Topic_;
public:
    hummingbird_topic_sub_ResMessage_for_broker(mqtt::async_client* cli, std::string app_id);
    std::string get_topic();
    void set_topic(std::string topic);
    int mqtt_response(mqtt::const_message_ptr msg);
    int init();
};


///////////////////////////////////////////////////////////////////////
// pub for req command 
class hummingbird_topic_pub_ReqCommand_for_broker : public hummingbird_topic_for_broker
{
    private:
        int g_QOS;
        std::string g_Topic_;
    public:
        hummingbird_topic_pub_ReqCommand_for_broker(mqtt::async_client* cli, std::string app_id);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();
};

// pub for res command 
class hummingbird_topic_pub_ResCommand_for_broker : public hummingbird_topic_for_broker
{
private:
    int g_QOS;
    std::string g_Topic_;
public:
    hummingbird_topic_pub_ResCommand_for_broker(mqtt::async_client* cli, std::string app_id);
    std::string get_topic();
    void set_topic(std::string topic);
    int mqtt_response(mqtt::const_message_ptr msg);
    int init();

};

//////////////////////////////////////////////////////////////////////
// sub fo req command 
class hummingbird_topic_sub_ReqCommand_for_broker : public hummingbird_topic_for_broker
{
    private:
        int g_QOS;
        std::string g_Topic_;
        char curl_ret_msg[1024];

    public:
        hummingbird_topic_sub_ReqCommand_for_broker(mqtt::async_client* cli, std::string app_id);
        std::string get_topic();
        void set_topic(std::string topic);
        int mqtt_response(mqtt::const_message_ptr msg);
        int init();

};

// sub fo res command 
class hummingbird_topic_sub_ResCommand_for_broker : public hummingbird_topic_for_broker
{
private:
    int g_QOS;
    std::string g_Topic_;
    char curl_ret_msg[1024];

public:
    hummingbird_topic_sub_ResCommand_for_broker(mqtt::async_client* cli, std::string app_id);
    std::string get_topic();
    void set_topic(std::string topic);
    int mqtt_response(mqtt::const_message_ptr msg);
    int init();

};

