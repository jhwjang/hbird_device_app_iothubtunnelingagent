#include <string>
#include <vector>
#include <cstring>
#include <chrono>
#include <thread>
#include "mqtt/async_client.h"
#include "mqtt/message.h"
#include "hummingbird/hummingbird_job.h"
// #include "json/json.h"


class hummingbird_topic 
{
    private:
        // std::string topic_; 
    protected:
        mqtt::async_client* cli_;
        hummingbird_job_tunnel *job_;
        // std::vector<std::string> command_protocol;
        // int count_of_command_protocol;
    public:

        hummingbird_topic(mqtt::async_client* cli,  hummingbird_job_tunnel *job);
        
        virtual void mqtt_response(mqtt::const_message_ptr msg) =0;
        virtual int init() =0;
        virtual const char* get_req_name() = 0;
        // void send_message(const char* topic,const char* payload,const int qos,const bool retain );
};

class hummingbird_topic_tunnel: public hummingbird_topic
{
    private:
        std::string hub_id_;
        std::string dev_id_;
        std::vector<std::string> sub_topic_;
        int qos_; 
        bool retain_;
        std::string req_name;    
        std::string localhost_;  
        std::string localport_;  
    public:
        hummingbird_topic_tunnel(mqtt::async_client* cli, hummingbird_job_tunnel *job, int qos, bool retain, string localhost, string localport);
        const char* get_req_name();
        void mqtt_response(mqtt::const_message_ptr msg);
        int init();
};

















