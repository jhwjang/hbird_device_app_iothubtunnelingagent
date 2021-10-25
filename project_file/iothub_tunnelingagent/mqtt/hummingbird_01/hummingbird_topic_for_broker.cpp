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

std::string hummingbird_topic_for_broker::create_topic(int mode, std::string app_id, std::string subTopic)
{
    std::string strTopic = "ipc/";

    if (mode == 0) // req
    {
        strTopic.append("req/subdevices/");
    }
    else  // res
    {
        strTopic.append("res/subdevices/");
    }
    strTopic.append(app_id);
    strTopic.append("/");
    strTopic.append(subTopic);

    return strTopic;
};

////////////////////////////////////////////////////////////////////////////////
// hummingbird_topic_pub_Connect_for_broker
hummingbird_topic_pub_Connect_for_broker::hummingbird_topic_pub_Connect_for_broker(mqtt::async_client* cli, std::string app_id)
  : hummingbird_topic_for_broker(cli){
    std::string subTopic = "connection";
    g_Topic_ = create_topic(0, app_id, subTopic); // req pub;

    std::cout << "broker ... Pub Topic : " << g_Topic_ << std::endl;
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
hummingbird_topic_sub_Connect_for_broker::hummingbird_topic_sub_Connect_for_broker(mqtt::async_client* cli, std::string app_id)
  : hummingbird_topic_for_broker(cli){
    std::string subTopic = "connection";
    g_Topic_ = create_topic(0, app_id, subTopic); // req sub;

    std::cout << "broker ... Sub Topic : " << g_Topic_ << std::endl;

    g_QOS = 1;
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
// pub for req message
hummingbird_topic_pub_ReqMessage_for_broker::hummingbird_topic_pub_ReqMessage_for_broker(mqtt::async_client* cli, std::string app_id)
    : hummingbird_topic_for_broker(cli) {
    std::string subTopic = "message";
    g_Topic_ = create_topic(0, app_id, subTopic); // req pub;

    std::cout << "broker ... req Pub Topic : " << g_Topic_ << std::endl;
}

std::string hummingbird_topic_pub_ReqMessage_for_broker::get_topic() {
    return g_Topic_;
}

void hummingbird_topic_pub_ReqMessage_for_broker::set_topic(std::string topic) {
    g_Topic_ = topic;
}

int hummingbird_topic_pub_ReqMessage_for_broker::mqtt_response(mqtt::const_message_ptr msg) {
    //std::cout<<"topic pub [hub->device message] receive" <<std::endl;
    return 0;
}

int hummingbird_topic_pub_ReqMessage_for_broker::init() {
#ifdef HUMMINGBIRD_DEBUG
    std::cout << "it is  " << topic_ << "(Topic)'s init " << std::endl;
#endif
    return 0;
}

// pub for res message
hummingbird_topic_pub_ResMessage_for_broker::hummingbird_topic_pub_ResMessage_for_broker(mqtt::async_client* cli, std::string app_id)
    : hummingbird_topic_for_broker(cli) {
    std::string subTopic = "message";
    g_Topic_ = create_topic(1, app_id, subTopic); // res pub;

    std::cout << "broker ... res Pub Topic : " << g_Topic_ << std::endl;
}

std::string hummingbird_topic_pub_ResMessage_for_broker::get_topic() {
    return g_Topic_;
}

void hummingbird_topic_pub_ResMessage_for_broker::set_topic(std::string topic) {
    g_Topic_ = topic;
}

int hummingbird_topic_pub_ResMessage_for_broker::mqtt_response(mqtt::const_message_ptr msg) {
    //std::cout<<"topic pub [hub->device message] receive" <<std::endl;
    return 0;
}

int hummingbird_topic_pub_ResMessage_for_broker::init() {
#ifdef HUMMINGBIRD_DEBUG
    std::cout << "it is  " << topic_ << "(Topic)'s init " << std::endl;
#endif
    return 0;
}

///////////////////////////////////////////////////////////////////////
// sub for req message
hummingbird_topic_sub_ReqMessage_for_broker::hummingbird_topic_sub_ReqMessage_for_broker(mqtt::async_client* cli, std::string app_id)
    : hummingbird_topic_for_broker(cli),
    g_QOS(1)
{
    std::string subTopic = "message";
    g_Topic_ = create_topic(0, app_id, subTopic); // req sub;

    std::cout << "broker ... req Sub Topic : " << g_Topic_ << std::endl;
}

std::string hummingbird_topic_sub_ReqMessage_for_broker::get_topic()
{
    return g_Topic_;
}
void hummingbird_topic_sub_ReqMessage_for_broker::set_topic(std::string topic)
{
    g_Topic_ = topic;
}
int hummingbird_topic_sub_ReqMessage_for_broker::mqtt_response(mqtt::const_message_ptr msg)
{
    callback_->OnReceiveTopicMessage(msg);

    return 0;
}
int hummingbird_topic_sub_ReqMessage_for_broker::init()
{
    g_async_client->subscribe(g_Topic_, g_QOS);

    return 0;
}

// sub for req message
hummingbird_topic_sub_ResMessage_for_broker::hummingbird_topic_sub_ResMessage_for_broker(mqtt::async_client* cli, std::string app_id)
    : hummingbird_topic_for_broker(cli),
    g_QOS(1)
{
    std::string subTopic = "message";
    g_Topic_ = create_topic(1, app_id, subTopic); // res sub;

    std::cout << "broker ... res Sub Topic : " << g_Topic_ << std::endl;
}

std::string hummingbird_topic_sub_ResMessage_for_broker::get_topic()
{
    return g_Topic_;
}
void hummingbird_topic_sub_ResMessage_for_broker::set_topic(std::string topic)
{
    g_Topic_ = topic;
}
int hummingbird_topic_sub_ResMessage_for_broker::mqtt_response(mqtt::const_message_ptr msg)
{
    callback_->OnReceiveTopicMessage(msg);

    return 0;
}
int hummingbird_topic_sub_ResMessage_for_broker::init()
{
    g_async_client->subscribe(g_Topic_, g_QOS);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// pub for req command
hummingbird_topic_pub_ReqCommand_for_broker::hummingbird_topic_pub_ReqCommand_for_broker(mqtt::async_client* cli, std::string app_id)
  : hummingbird_topic_for_broker(cli){

    // req / subdevices/ {:targetApp} / command
    std::string subTopic = "command";
    g_Topic_ = create_topic(0, app_id, subTopic); // req pub;

    g_QOS = 1;
//printf("** pub command topic : %s\n", topic_.c_str());

    std::cout << "broker ... req Pub Topic : " << g_Topic_ << std::endl;
}

std::string hummingbird_topic_pub_ReqCommand_for_broker::get_topic(){
    return g_Topic_;
}

void hummingbird_topic_pub_ReqCommand_for_broker::set_topic(std::string topic){
    g_Topic_ = topic;
}

int hummingbird_topic_pub_ReqCommand_for_broker::mqtt_response(mqtt::const_message_ptr msg){
    //std::cout<<"topic pub [hub->device->user->command] receive" <<std::endl;
    return 0;
}

int hummingbird_topic_pub_ReqCommand_for_broker::init(){
#ifdef HUMMINGBIRD_DEBUG
    std::cout<<"it is  "<< topic_ <<"(Topic)'s init mqtt_response" <<std::endl;
#endif
	return 0;
}

// pub for res command
hummingbird_topic_pub_ResCommand_for_broker::hummingbird_topic_pub_ResCommand_for_broker(mqtt::async_client* cli, std::string app_id)
    : hummingbird_topic_for_broker(cli) {

    // req / subdevices/ {:targetApp} / command
    std::string subTopic = "command";
    g_Topic_ = create_topic(1, app_id, subTopic); // res pub;

    g_QOS = 1;
    //printf("** pub command topic : %s\n", topic_.c_str());

    std::cout << "broker ... res Pub Topic : " << g_Topic_ << std::endl;
}

std::string hummingbird_topic_pub_ResCommand_for_broker::get_topic() {
    return g_Topic_;
}

void hummingbird_topic_pub_ResCommand_for_broker::set_topic(std::string topic) {
    g_Topic_ = topic;
}

int hummingbird_topic_pub_ResCommand_for_broker::mqtt_response(mqtt::const_message_ptr msg) {
    //std::cout<<"topic pub [hub->device->user->command] receive" <<std::endl;
    return 0;
}

int hummingbird_topic_pub_ResCommand_for_broker::init() {
#ifdef HUMMINGBIRD_DEBUG
    std::cout << "it is  " << topic_ << "(Topic)'s init mqtt_response" << std::endl;
#endif
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////
// sub fo req command 
hummingbird_topic_sub_ReqCommand_for_broker::hummingbird_topic_sub_ReqCommand_for_broker(mqtt::async_client* cli, std::string app_id)
  : hummingbird_topic_for_broker(cli){
    // req / subdevices/ {:targetApp} / command
    std::string subTopic = "command";
    g_Topic_ = create_topic(0, app_id, subTopic); // req sub;
    g_QOS = 1;

    std::cout << "broker ... req Sub Topic : " << g_Topic_ << std::endl;
}

std::string hummingbird_topic_sub_ReqCommand_for_broker::get_topic(){
    return g_Topic_;
}

void hummingbird_topic_sub_ReqCommand_for_broker::set_topic(std::string topic){
    g_Topic_ = topic;
}

int hummingbird_topic_sub_ReqCommand_for_broker::mqtt_response(mqtt::const_message_ptr msg){

    //std::cout<<"[hwanjang] sub Hummingbird_Command RESPONSE"<<std::endl;

#if 0
    std::string topic = msg->get_topic();
    callback_->OnReceiveTopicMessage(topic, msg->to_string());
#else
    callback_->OnReceiveTopicMessage(msg);
#endif

    return 0;
}

int hummingbird_topic_sub_ReqCommand_for_broker::init(){
#ifdef HUMMINGBIRD_DEBUG
    std::cout<<"it is  "<< topic_ <<"(Topic)'s init mqtt_response" <<std::endl;
#endif
    g_async_client->subscribe(g_Topic_, g_QOS);
    return 0;
}

// sub fo res command 
hummingbird_topic_sub_ResCommand_for_broker::hummingbird_topic_sub_ResCommand_for_broker(mqtt::async_client* cli, std::string app_id)
    : hummingbird_topic_for_broker(cli) {
    // req / subdevices/ {:targetApp} / command
    std::string subTopic = "command";
    g_Topic_ = create_topic(1, app_id, subTopic); // res sub;
    g_QOS = 1;

    std::cout << "broker ... res Sub Topic : " << g_Topic_ << std::endl;
}

std::string hummingbird_topic_sub_ResCommand_for_broker::get_topic() {
    return g_Topic_;
}

void hummingbird_topic_sub_ResCommand_for_broker::set_topic(std::string topic) {
    g_Topic_ = topic;
}

int hummingbird_topic_sub_ResCommand_for_broker::mqtt_response(mqtt::const_message_ptr msg) {

    //std::cout<<"[hwanjang] sub Hummingbird_Command RESPONSE"<<std::endl;

#if 0
    std::string topic = msg->get_topic();
    callback_->OnReceiveTopicMessage(topic, msg->to_string());
#else
    callback_->OnReceiveTopicMessage(msg);
#endif

    return 0;
}

int hummingbird_topic_sub_ResCommand_for_broker::init() {
#ifdef HUMMINGBIRD_DEBUG
    std::cout << "it is  " << topic_ << "(Topic)'s init mqtt_response" << std::endl;
#endif
    g_async_client->subscribe(g_Topic_, g_QOS);
    return 0;
}
