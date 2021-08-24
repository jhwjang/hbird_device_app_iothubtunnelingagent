#include "hummingbird_topic_for_broker.h"
#include<iostream>
 
//#define HUMMINGBIRD_DEBUG

hummingbird_topic_for_broker::hummingbird_topic_for_broker(mqtt::async_client* cli)
  : callback_(NULL)
{
    cli_ = cli; 
};

//#define TIME_TEST // 2018.03.08 hwanjang
void hummingbird_topic_for_broker::send_message(const char* topic,const char* payload,const int qos,const bool retain ){

std::string temp = payload;
if((retain == true) && (temp.empty()))
{
  printf("hummingbird_topic::send_message() -> topic : %s -> message is empty !! return !!\n", topic);
  return;
} 

    mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
    pubmsg->set_qos(qos);
    pubmsg->set_retained(retain);

#ifdef TIME_TEST
struct timespec tspec;
clock_gettime(CLOCK_REALTIME, &tspec);
printf("** hummingbird_topic::send_message() -> topic : %s, QOS : %d, retain : %d\n", topic, qos, retain);
printf("[hwanjang] hummingbird_topic::send_message() time -> tv_sec : %lld, tv_nsec : %lld\n", 
            (long long int)tspec.tv_sec, (long long int)tspec.tv_nsec);
#endif

    cli_->publish(pubmsg);  

#ifdef HUMMINGBIRD_DEBUG
	printf("hummingbird_topic::send_message() -> topic : %s -> publish success !!!\nMessage : %s\n", topic,payload);
#endif
}

void hummingbird_topic_for_broker::send_message(const char* topic, const void *payload, const int size, const int qos, const bool retain)
{
    mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload, size);
    pubmsg->set_qos(qos);
    pubmsg->set_retained(retain);

#ifdef TIME_TEST
    struct timespec tspec;
    clock_gettime(CLOCK_REALTIME, &tspec);
    printf("** hummingbird_topic::send_message() -> topic : %s, QOS : %d, retain : %d\n", topic, qos, retain);
    printf("[hwanjang] hummingbird_topic::send_message() time -> tv_sec : %lld, tv_nsec : %lld\n", 
                (long long int)tspec.tv_sec, (long long int)tspec.tv_nsec);
#endif

    cli_->publish(pubmsg);

#ifdef HUMMINGBIRD_DEBUG
    printf("hummingbird_topic::send_message() -> topic : %s -> publish success !!!\nMessage : %s\n", topic, payload);
#endif
}

void hummingbird_topic_for_broker::RegisterObserver(hummingbird_topic_Observer_for_broker* callback)
{
	callback_ = callback;
}

std::string hummingbird_topic_for_broker::create_hubid_topic(int mode, std::string app_id, std::string userName)
{
    std::string str;

    if (mode == 0)  // pub
    {
        str = "apps/";
    }
    else  // sub
    {
        str = "hummingbird/apps/";
    }

    str.append(app_id);
    str.append("/users/");
    str.append(userName);

    return str;
};

////////////////////////////////////////////////////////////////////////////////
// hummingbird_topic_pub_Message
hummingbird_topic_pub_Message_for_broker::hummingbird_topic_pub_Message_for_broker(mqtt::async_client* cli, std::string app_id, std::string topic, std::string user_id)
  : hummingbird_topic_for_broker(cli){
    topic_ = "apps/";
    topic_.append(app_id);
    topic_.append("/message");

    std::cout << "Pub Topic : " << topic_ << std::endl;
}

std::string hummingbird_topic_pub_Message_for_broker::get_topic(){
    return topic_;
}

void hummingbird_topic_pub_Message_for_broker::set_topic(std::string topic){
    topic_ = topic;
}

int hummingbird_topic_pub_Message_for_broker::mqtt_response(mqtt::const_message_ptr msg){
    //std::cout<<"topic pub [hub->device message] receive" <<std::endl;
    return 0;
}

int hummingbird_topic_pub_Message_for_broker::init(){
#ifdef HUMMINGBIRD_DEBUG
    std::cout<<"it is  "<< topic_ <<"(Topic)'s init " <<std::endl;
#endif
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// hummingbird_topic_pub_Connect_for_broker
hummingbird_topic_pub_Connect_for_broker::hummingbird_topic_pub_Connect_for_broker(mqtt::async_client* cli, std::string app_id, std::string topic, std::string user_id)
  : hummingbird_topic_for_broker(cli){
    topic_ = "apps/";
    topic_.append(app_id);
    topic_.append("/connection");

    std::cout << "Pub Topic : " << topic_ << std::endl;
}

std::string hummingbird_topic_pub_Connect_for_broker::get_topic(){
    return topic_;
}

void hummingbird_topic_pub_Connect_for_broker::set_topic(std::string topic){
    topic_ = topic;
}

int hummingbird_topic_pub_Connect_for_broker::mqtt_response(mqtt::const_message_ptr msg){
    //std::cout<<"topic pub [hub->device connect] receive" <<std::endl;
    return 0;
}

int hummingbird_topic_pub_Connect_for_broker::init(){
#ifdef HUMMINGBIRD_DEBUG
    std::cout<<"it is  "<< topic_ <<"(Topic)'s init " <<std::endl;
#endif
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// hummingbird_topic_sub_Connect_for_brokerion
hummingbird_topic_sub_Connect_for_broker::hummingbird_topic_sub_Connect_for_broker(mqtt::async_client* cli, std::string app_id, std::string user_id)
  : hummingbird_topic_for_broker(cli){
    //topic_ = "hummingbird/apps/+/users/+/connection";
    topic_ = create_hubid_topic(1, app_id, user_id); // 1 : sub;
    topic_.append("/connection");

    std::cout << "Sub Topic : " << topic_ << std::endl;

    QOS = 1;
}

std::string hummingbird_topic_sub_Connect_for_broker::get_topic(){
    return topic_;
}

void hummingbird_topic_sub_Connect_for_broker::set_topic(std::string topic){
    topic_ = topic;
}

int hummingbird_topic_sub_Connect_for_broker::mqtt_response(mqtt::const_message_ptr msg){
    //std::cout<<"[hwanjang] sub Hummingbird_Connect RESPONSE"<<std::endl;

#if 0
    std::string topic = msg->get_topic();
    callback_->OnReceiveTopicMessage(topic, msg->to_string());
#else
    callback_->OnReceiveTopicMessage(msg);
#endif


    return 0;
}

int hummingbird_topic_sub_Connect_for_broker::init(){

    cli_->subscribe(topic_, QOS);

#if 1
    if(callback_->getConnectionStatus() != true)
    {
		callback_->OnConnectSuccess();
    }
#else
    callback_->OnConnectSuccess();
#endif

#ifdef HUMMINGBIRD_DEBUG
	std::cout<<"it is  "<< topic_ <<"(Topic)'s init !!" <<std::endl;
#endif

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// pub command for device
hummingbird_topic_pub_Command_for_broker::hummingbird_topic_pub_Command_for_broker(mqtt::async_client* cli, std::string app_id, std::string topic, std::string user_id)
  : hummingbird_topic_for_broker(cli){
    topic_ = topic;
    QOS = 1;
//printf("** pub command topic : %s\n", topic_.c_str());

    std::cout << "Pub Topic : " << topic_ << std::endl;
}

std::string hummingbird_topic_pub_Command_for_broker::get_topic(){
    return topic_;
}

void hummingbird_topic_pub_Command_for_broker::set_topic(std::string topic){
    topic_ = topic;
}

int hummingbird_topic_pub_Command_for_broker::mqtt_response(mqtt::const_message_ptr msg){
    //std::cout<<"topic pub [hub->device->user->command] receive" <<std::endl;
    return 0;
}

int hummingbird_topic_pub_Command_for_broker::init(){
#ifdef HUMMINGBIRD_DEBUG
    std::cout<<"it is  "<< topic_ <<"(Topic)'s init mqtt_response" <<std::endl;
#endif
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////
// hummingbird_topic_sub_Command_for_broker
hummingbird_topic_sub_Command_for_broker::hummingbird_topic_sub_Command_for_broker(mqtt::async_client* cli, std::string app_id, std::string user_id)
  : hummingbird_topic_for_broker(cli){
    topic_ = create_hubid_topic(1, app_id, user_id); // 1 : sub
    topic_.append("/command");
    QOS = 1;

    std::cout << "Sub Topic : " << topic_ << std::endl;
}

std::string hummingbird_topic_sub_Command_for_broker::get_topic(){
    return topic_;
}

void hummingbird_topic_sub_Command_for_broker::set_topic(std::string topic){
    topic_ = topic;
}

int hummingbird_topic_sub_Command_for_broker::mqtt_response(mqtt::const_message_ptr msg){

    //std::cout<<"[hwanjang] sub Hummingbird_Command RESPONSE"<<std::endl;

#if 0
    std::string topic = msg->get_topic();
    callback_->OnReceiveTopicMessage(topic, msg->to_string());
#else
    callback_->OnReceiveTopicMessage(msg);
#endif

    return 0;
}

int hummingbird_topic_sub_Command_for_broker::init(){
#ifdef HUMMINGBIRD_DEBUG
    std::cout<<"it is  "<< topic_ <<"(Topic)'s init mqtt_response" <<std::endl;
#endif
    cli_->subscribe(topic_, QOS);
    return 0;
}
