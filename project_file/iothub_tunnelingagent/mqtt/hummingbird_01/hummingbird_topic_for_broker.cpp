#include "hummingbird_topic_for_broker.h"
#include<iostream>
 
//#define HUMMINGBIRD_DEBUG

hummingbird_topic_for_broker::hummingbird_topic_for_broker(mqtt::async_client* cli)
  : callback_(NULL)
{
    g_async_client = cli;
};

//#define TIME_TEST // 2018.03.08 hwanjang
void hummingbird_topic_for_broker::send_message(const char* topic,const char* payload,const int qos,const bool retain ){

std::string temp = payload;
if((retain == true) && (temp.empty()))
{
  printf("hummingbird_topic_for_broker::send_message() -> topic : %s -> message is empty !! return !!\n", topic);
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

    g_async_client->publish(pubmsg);

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

    g_async_client->publish(pubmsg);

#ifdef HUMMINGBIRD_DEBUG
    printf("hummingbird_topic::send_message() -> topic : %s -> publish success !!!\nMessage : %s\n", topic, payload);
#endif
}

void hummingbird_topic_for_broker::RegisterObserver(hummingbird_topic_Observer_for_broker* callback)
{
	callback_ = callback;
}


std::string hummingbird_topic_for_broker::create_pub_topic(std::string group_id, std::string s_id, std::string subTopic)
{
    std::string strTopic = "ipc/groups/";

    strTopic.append(group_id);
    strTopic.append("/sid/");
    strTopic.append(s_id);
    strTopic.append("/");
    strTopic.append(subTopic);

    return strTopic;
};

std::string hummingbird_topic_for_broker::create_topic(std::string group_id, std::string d_id, std::string s_id, std::string subTopic)
{
    std::string strTopic = "ipc/groups/";

    strTopic.append(group_id);
    strTopic.append("/did/");
    strTopic.append(d_id);
    strTopic.append("/sid/");
    strTopic.append(s_id);
    strTopic.append("/");
    strTopic.append(subTopic);

    return strTopic;
};

////////////////////////////////////////////////////////////////////////////////
// hummingbird_topic_pub_Connect_for_broker
hummingbird_topic_pub_Connect_for_broker::hummingbird_topic_pub_Connect_for_broker(mqtt::async_client* cli, std::string group_id, std::string d_id, std::string s_id)
  : hummingbird_topic_for_broker(cli),
    g_QOS(1)
{
    std::string subTopic = "connection";
    g_Topic_ = create_topic(group_id, d_id, s_id, subTopic); // pub;

#ifdef HUMMINGBIRD_DEBUG
    std::cout << "broker ... Pub Topic : " << g_Topic_ << std::endl;
#endif
}

std::string hummingbird_topic_pub_Connect_for_broker::get_topic(){
    return g_Topic_;
}

void hummingbird_topic_pub_Connect_for_broker::set_topic(std::string topic){
    g_Topic_ = topic;
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
hummingbird_topic_sub_Connect_for_broker::hummingbird_topic_sub_Connect_for_broker(mqtt::async_client* cli,std::string group_id, std::string d_id, std::string s_id)
  : hummingbird_topic_for_broker(cli),
    g_QOS(1)
{
    std::string subTopic = "connection";
    g_Topic_ = create_topic(group_id, d_id, s_id, subTopic); // sub;

#ifdef HUMMINGBIRD_DEBUG
    std::cout << "broker ... Sub Topic : " << g_Topic_ << std::endl;
#endif
}

std::string hummingbird_topic_sub_Connect_for_broker::get_topic(){
    return g_Topic_;
}

void hummingbird_topic_sub_Connect_for_broker::set_topic(std::string topic){
    g_Topic_ = topic;
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

    g_async_client->subscribe(g_Topic_, g_QOS);

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
// pub for message
hummingbird_topic_pub_Message_for_broker::hummingbird_topic_pub_Message_for_broker(mqtt::async_client* cli,std::string group_id, std::string d_id, std::string s_id)
    : hummingbird_topic_for_broker(cli),
    g_QOS(1)
{
    std::string subTopic = "message";
    g_Topic_ = create_topic(group_id, d_id, s_id, subTopic); // pub;

#ifdef HUMMINGBIRD_DEBUG
    std::cout << "broker ... message Pub Topic : " << g_Topic_ << std::endl;
#endif
}

std::string hummingbird_topic_pub_Message_for_broker::get_topic() {
    return g_Topic_;
}

void hummingbird_topic_pub_Message_for_broker::set_topic(std::string topic) {
    g_Topic_ = topic;
}

int hummingbird_topic_pub_Message_for_broker::mqtt_response(mqtt::const_message_ptr msg) {
    //std::cout<<"topic pub [hub->device message] receive" <<std::endl;
    return 0;
}

int hummingbird_topic_pub_Message_for_broker::init() {
#ifdef HUMMINGBIRD_DEBUG
    std::cout << "it is  " << topic_ << "(Topic)'s init " << std::endl;
#endif
    return 0;
}

///////////////////////////////////////////////////////////////////////
// sub for message
hummingbird_topic_sub_Message_for_broker::hummingbird_topic_sub_Message_for_broker(mqtt::async_client* cli,std::string group_id, std::string d_id, std::string s_id)
    : hummingbird_topic_for_broker(cli),
    g_QOS(1)
{
    std::string subTopic = "message";
    g_Topic_ = create_topic(group_id, d_id, s_id, subTopic); // sub;

#ifdef HUMMINGBIRD_DEBUG
    std::cout << "broker ... message Sub Topic : " << g_Topic_ << std::endl;
#endif
}

std::string hummingbird_topic_sub_Message_for_broker::get_topic()
{
    return g_Topic_;
}
void hummingbird_topic_sub_Message_for_broker::set_topic(std::string topic)
{
    g_Topic_ = topic;
}
int hummingbird_topic_sub_Message_for_broker::mqtt_response(mqtt::const_message_ptr msg)
{
    callback_->OnReceiveTopicMessage(msg);

    return 0;
}
int hummingbird_topic_sub_Message_for_broker::init()
{
    g_async_client->subscribe(g_Topic_, g_QOS);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// pub for command
hummingbird_topic_pub_Command_for_broker::hummingbird_topic_pub_Command_for_broker(mqtt::async_client* cli,std::string group_id, std::string d_id, std::string s_id)
  : hummingbird_topic_for_broker(cli),
    g_QOS(1)
{

    // req / subdevices/ {:targetApp} / command
    std::string subTopic = "command";
    g_Topic_ = create_topic(group_id, d_id, s_id, subTopic); // pub;

    //std::cout << "broker ... command Pub Topic : " << g_Topic_ << std::endl;
}

std::string hummingbird_topic_pub_Command_for_broker::get_topic(){
    return g_Topic_;
}

void hummingbird_topic_pub_Command_for_broker::set_topic(std::string topic){
    g_Topic_ = topic;
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
// sub fo command 
hummingbird_topic_sub_Command_for_broker::hummingbird_topic_sub_Command_for_broker(mqtt::async_client* cli, std::string group_id, std::string d_id, std::string s_id)
    : hummingbird_topic_for_broker(cli),
    g_QOS(1)
{
    // req / subdevices/ {:targetApp} / command
    std::string subTopic = "command";
    g_Topic_ = create_topic(group_id, d_id, s_id, subTopic); // sub;

    //std::cout << "broker ... command Sub Topic : " << g_Topic_ << std::endl;
}

std::string hummingbird_topic_sub_Command_for_broker::get_topic() {
    return g_Topic_;
}

void hummingbird_topic_sub_Command_for_broker::set_topic(std::string topic) {
    g_Topic_ = topic;
}

int hummingbird_topic_sub_Command_for_broker::mqtt_response(mqtt::const_message_ptr msg) {

    //std::cout<<"[hwanjang] sub Hummingbird_Command RESPONSE"<<std::endl;

#if 0
    std::string topic = msg->get_topic();
    callback_->OnReceiveTopicMessage(topic, msg->to_string());
#else
    callback_->OnReceiveTopicMessage(msg);
#endif

    return 0;
}

int hummingbird_topic_sub_Command_for_broker::init() {
#ifdef HUMMINGBIRD_DEBUG
    std::cout << "it is  " << topic_ << "(Topic)'s init mqtt_response" << std::endl;
#endif
    g_async_client->subscribe(g_Topic_, g_QOS);
    return 0;
}
