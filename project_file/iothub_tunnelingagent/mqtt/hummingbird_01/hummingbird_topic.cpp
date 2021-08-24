#include "hummingbird_topic.h"

#include<iostream>
 
//#define HUMMINGBIRD_DEBUG

hummingbird_topic::hummingbird_topic(mqtt::async_client* cli, std::string hub_id, std::string topic, std::string user_id)
  : callback_(NULL)
{
    cli_ = cli;
};

//#define TIME_TEST // 2018.03.08 hwanjang
void hummingbird_topic::send_message(const char* topic,const char* payload,const int qos,const bool retain ){

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

void hummingbird_topic::send_message(const char* topic, const void *payload, const int size, const int qos, const bool retain) 
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

void hummingbird_topic::RegisterObserver(hummingbird_topic_Observer* callback)
{
	callback_ = callback;
}

////////////////////////////////////////////////////////////////////////////////
// hummingbird_topic_pub_Message
hummingbird_topic_pub_Message::hummingbird_topic_pub_Message(mqtt::async_client* cli, std::string hub_id, std::string topic, std::string user_id)
  : hummingbird_topic(cli, hub_id, topic, user_id){
    topic_ = "devices/";
    topic_.append(hub_id);
    topic_.append("/message");
}

std::string hummingbird_topic_pub_Message::get_topic(){
    return topic_;
}

void hummingbird_topic_pub_Message::set_topic(std::string topic){
    topic_ = topic;
}

int hummingbird_topic_pub_Message::mqtt_response(mqtt::const_message_ptr msg){
    //std::cout<<"topic pub [hub->device message] receive" <<std::endl;
    return 0;
}

int hummingbird_topic_pub_Message::init(){
#ifdef HUMMINGBIRD_DEBUG
    std::cout<<"it is  "<< topic_ <<"(Topic)'s init " <<std::endl;
#endif
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// hummingbird_topic_pub_Connection
hummingbird_topic_pub_Connect::hummingbird_topic_pub_Connect(mqtt::async_client* cli, std::string hub_id, std::string topic, std::string user_id)
  : hummingbird_topic(cli, hub_id, topic, user_id){
    topic_ = "devices/";
    topic_.append(hub_id);
    topic_.append("/connection");
}

std::string hummingbird_topic_pub_Connect::get_topic(){
    return topic_;
}

void hummingbird_topic_pub_Connect::set_topic(std::string topic){
    topic_ = topic;
}

int hummingbird_topic_pub_Connect::mqtt_response(mqtt::const_message_ptr msg){
    //std::cout<<"topic pub [hub->device connect] receive" <<std::endl;
    return 0;
}

int hummingbird_topic_pub_Connect::init(){
#ifdef HUMMINGBIRD_DEBUG
    std::cout<<"it is  "<< topic_ <<"(Topic)'s init " <<std::endl;
#endif
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// hummingbird_topic_sub_connection
hummingbird_topic_sub_Connect::hummingbird_topic_sub_Connect(mqtt::async_client* cli, std::string hub_id, std::string device_id, std::string user_id)
  : hummingbird_topic(cli, hub_id, device_id, user_id){
    //topic_ = "hummingbird/hubs/hub_00/devices/+/users/+/connection";
    topic_ = create_hubid_topic(1, hub_id); // 1 : sub;
    topic_.append(device_id);
    topic_.append("/users/");
    topic_.append(user_id);
    topic_.append("/connection");
    QOS = 1;
}

std::string hummingbird_topic_sub_Connect::get_topic(){
    return topic_;
}

void hummingbird_topic_sub_Connect::set_topic(std::string topic){
    topic_ = topic;
}

int hummingbird_topic_sub_Connect::mqtt_response(mqtt::const_message_ptr msg){
    //std::cout<<"[hwanjang] sub Hummingbird_Connect RESPONSE"<<std::endl;

#if 0
    std::string topic = msg->get_topic();
    callback_->OnReceiveTopicMessage(topic, msg->to_string());
#else
    callback_->OnReceiveTopicMessage(msg);
#endif


    return 0;
}

int hummingbird_topic_sub_Connect::init(){

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
hummingbird_topic_pub_Command::hummingbird_topic_pub_Command(mqtt::async_client* cli, std::string hub_id, std::string topic, std::string user_id)
  : hummingbird_topic(cli, hub_id, topic, user_id){
    topic_ = topic;
    QOS = 1;
//printf("** pub command topic : %s\n", topic_.c_str());
}

std::string hummingbird_topic_pub_Command::get_topic(){
    return topic_;
}

void hummingbird_topic_pub_Command::set_topic(std::string topic){
    topic_ = topic;
}

int hummingbird_topic_pub_Command::mqtt_response(mqtt::const_message_ptr msg){
    //std::cout<<"topic pub [hub->device->user->command] receive" <<std::endl;
    return 0;
}

int hummingbird_topic_pub_Command::init(){
#ifdef HUMMINGBIRD_DEBUG
    std::cout<<"it is  "<< topic_ <<"(Topic)'s init mqtt_response" <<std::endl;
#endif
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////
// hummingbird_topic_sub_command
hummingbird_topic_sub_Command::hummingbird_topic_sub_Command(mqtt::async_client* cli, std::string hub_id, std::string device_id, std::string user_id)
  : hummingbird_topic(cli, hub_id, device_id, user_id){
    topic_ = create_hubid_topic(1, hub_id); // 1 : sub
    topic_.append(device_id);
    topic_.append("/users/");
    topic_.append(user_id);
    topic_.append("/command");
    QOS = 1;

	printf("[hwanjang] hummingbird_topic_sub_Command() -> topic : %s\n", topic_.c_str());
}

std::string hummingbird_topic_sub_Command::get_topic(){
    return topic_;
}

void hummingbird_topic_sub_Command::set_topic(std::string topic){
    topic_ = topic;
}

int hummingbird_topic_sub_Command::mqtt_response(mqtt::const_message_ptr msg){

    //std::cout<<"[hwanjang] sub Hummingbird_Command RESPONSE"<<std::endl;

#if 0
    std::string topic = msg->get_topic();
    callback_->OnReceiveTopicMessage(topic, msg->to_string());
#else
    callback_->OnReceiveTopicMessage(msg);
#endif

    return 0;
}

int hummingbird_topic_sub_Command::init(){
#ifdef HUMMINGBIRD_DEBUG
    std::cout<<"it is  "<< topic_ <<"(Topic)'s init mqtt_response" <<std::endl;
#endif
    cli_->subscribe(topic_, QOS);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// pub SUNAPI Tunneling
hummingbird_topic_pub_SUNAPITunneling::hummingbird_topic_pub_SUNAPITunneling(mqtt::async_client* cli, std::string hub_id, std::string topic, std::string user_id)
  : hummingbird_topic(cli, hub_id, topic, user_id){
    topic_ = topic;
    QOS = 1;
//printf("** pub Tunneling topic : %s\n", topic_.c_str());
}

std::string hummingbird_topic_pub_SUNAPITunneling::get_topic(){
    return topic_;
}

void hummingbird_topic_pub_SUNAPITunneling::set_topic(std::string topic){
    topic_ = topic;
}

int hummingbird_topic_pub_SUNAPITunneling::mqtt_response(mqtt::const_message_ptr msg){
    //std::cout<<"topic pub [hub->device->user->Live] receive" <<std::endl;
    return 0;
}

int hummingbird_topic_pub_SUNAPITunneling::init(){
#ifdef HUMMINGBIRD_DEBUG
    std::cout<<"it is  "<< topic_ <<"(Topic)'s init mqtt_response" <<std::endl;
#endif
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////
// hummingbird_topic_sub_SUNAPITunneling
hummingbird_topic_sub_SUNAPITunneling::hummingbird_topic_sub_SUNAPITunneling(mqtt::async_client* cli, std::string hub_id, std::string device_id, std::string user_id)
  : hummingbird_topic(cli, hub_id, device_id, user_id){
    topic_ = create_hubid_topic(1, hub_id); // 1 : sub
    topic_.append(device_id);
    topic_.append("/users/");
    topic_.append(user_id);
    topic_.append("/sunapi");  
    QOS = 1;

	printf("[hwanjang] hummingbird_topic_sub_SUNAPITunneling() -> topic : %s\n", topic_.c_str());
}

std::string hummingbird_topic_sub_SUNAPITunneling::get_topic(){
    return topic_;
}

void hummingbird_topic_sub_SUNAPITunneling::set_topic(std::string topic){
    topic_ = topic;
}

int hummingbird_topic_sub_SUNAPITunneling::mqtt_response(mqtt::const_message_ptr msg){

    std::cout<<"[hwanjang] sub hummingbird_topic_sub_SUNAPITunneling RESPONSE"<<std::endl;
#if 0
    std::string topic = msg->get_topic();
    callback_->OnReceiveTopicMessage(topic, msg->to_string());
#else
    callback_->OnReceiveTopicMessage(msg);
#endif
    return 0;
}

int hummingbird_topic_sub_SUNAPITunneling::init(){
//#ifdef HUMMINGBIRD_DEBUG
#if 1
	std::cout<<"it is  "<< topic_ <<"(Topic)'s init mqtt_response" <<std::endl;
#endif
    cli_->subscribe(topic_, QOS);

    return 0;
}


////////////////////////////////////////////////////////////////////////////////
// pub HTTP Tunneling
hummingbird_topic_pub_HttpTunneling::hummingbird_topic_pub_HttpTunneling(mqtt::async_client* cli, std::string hub_id, std::string topic, std::string user_id)
    : hummingbird_topic(cli, hub_id, topic, user_id) {
    topic_ = topic;
    QOS = 1;
    //printf("** pub Tunneling topic : %s\n", topic_.c_str());
}

std::string hummingbird_topic_pub_HttpTunneling::get_topic() {
    return topic_;
}

void hummingbird_topic_pub_HttpTunneling::set_topic(std::string topic) {
    topic_ = topic;
}

int hummingbird_topic_pub_HttpTunneling::mqtt_response(mqtt::const_message_ptr msg) {
    //std::cout<<"topic pub [hub->device->user->Live] receive" <<std::endl;
    return 0;
}

int hummingbird_topic_pub_HttpTunneling::init() {
#ifdef HUMMINGBIRD_DEBUG
    std::cout << "it is  " << topic_ << "(Topic)'s init mqtt_response" << std::endl;
#endif
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////
// hummingbird_topic_sub_HttpTunneling
hummingbird_topic_sub_HttpTunneling::hummingbird_topic_sub_HttpTunneling(mqtt::async_client* cli, std::string hub_id, std::string device_id, std::string user_id)
    : hummingbird_topic(cli, hub_id, device_id, user_id) {
    topic_ = create_hubid_topic(1, hub_id); // 1 : sub
    topic_.append(device_id);
    topic_.append("/users/");
    topic_.append(user_id);
    topic_.append("/http");
    QOS = 1;

    printf("[hwanjang] hummingbird_topic_sub_HttpTunneling() -> topic : %s\n", topic_.c_str());
}

std::string hummingbird_topic_sub_HttpTunneling::get_topic() {
    return topic_;
}

void hummingbird_topic_sub_HttpTunneling::set_topic(std::string topic) {
    topic_ = topic;
}

int hummingbird_topic_sub_HttpTunneling::mqtt_response(mqtt::const_message_ptr msg) {

    std::cout << "[hwanjang] sub hummingbird_topic_sub_HttpTunneling RESPONSE" << std::endl;
#if 0
    std::string topic = msg->get_topic();
    callback_->OnReceiveTopicMessage(topic, msg->to_string());
#else
    callback_->OnReceiveTopicMessage(msg);
#endif
    return 0;
}

int hummingbird_topic_sub_HttpTunneling::init() {
    //#ifdef HUMMINGBIRD_DEBUG
#if 1
    std::cout << "it is  " << topic_ << "(Topic)'s init mqtt_response" << std::endl;
#endif
    cli_->subscribe(topic_, QOS);

    return 0;
}
