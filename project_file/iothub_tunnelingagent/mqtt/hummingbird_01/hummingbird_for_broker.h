//#include <sys/time.h>

#include <chrono>
//#include "mqtt/async_client.h"
#include "header_for_action_listener.h"

#include "hummingbird_topic_for_broker.h"

#ifdef GCC_VERSION_UNDER_4_9 // 2017.11.14 hwanjang - for hisiv300 , hisiv500 - std::to_string error
#include <sstream>
#endif

//#ifdef _WIN32
#ifdef _MSC_VER 
#include <windows.h>

#define localtime_r(_time, _result) _localtime64_s(_result, _time)
#define sleep(x) Sleep((x*1000))
#endif

//#define ACL

using namespace std;	
// not use -> change LIST
//const int pub_topic_count = 8;
//const int sub_topic_count = 4;

/////////////////////////////////////////////////////////////////////////////
#if 0
// Callbacks for the success or failures of requested actions.
// This could be used to initiate further action, but here we just log the
// results to the console.

class action_listener : public virtual mqtt::iaction_listener
{
	std::string name_;

	void on_failure(const mqtt::token& tok) override {
		std::cout << name_ << " failure";
		if (tok.get_message_id() != 0)
			std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
		std::cout << std::endl;
	};

	void on_success(const mqtt::token& tok) override {
		std::cout << name_ << " success_action_listener";
		if (tok.get_message_id() != 0)
			std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
		auto top = tok.get_topics();
		if (top && !top->empty())
			std::cout << "\ttoken topic: '" << (*top)[0] << "', ..." << std::endl;
		std::cout << std::endl;
	};

public:
	action_listener(const std::string& name) : name_(name) {};
};
#endif

/////////////////////////////////////////////////////////////////////////////

/**
 * Local callback & listener class for use with the client connection.
 * This is primarily intended to receive messages, but it will also monitor
 * the connection to the broker. If the connection is lost, it will attempt
 * to restore the connection and re-subscribe to the topic.
 */
class hummingbird_for_broker : public virtual mqtt::callback,
							public virtual mqtt::iaction_listener
{

#if 0 
	hummingbird_topic_for_broker**pub_topic_list_ ;
	hummingbird_topic_for_broker**sub_topic_list_ ;
 
 	// Counter for the number of connection retries
	int nretry_;
	mqtt::async_client* cli_;
	hummingbird_curl_for_command* curl_;
	// Options to use if we need to reconnect
	mqtt::connect_options& connOpts_;
	// An action listener to display the result of actions.
	action_listener subListener_;

	// This deomonstrates manually reconnecting to the broker by calling
	// connect() again. This is a possibility for an application that keeps
	// a copy of it's original connect_options, or if the app wants to
	// reconnect with different options.
	// Another way this can be done manually, if using the same options, is
	// to just call the async_client::reconnect() method.
	void reconnect();

	// Re-connection failure
	void on_failure(const mqtt::token& tok) override {
		std::cout << "Connection failed : " <<  std::endl;
		// if (++nretry_ > N_RETRY_ATTEMPTS)
		if (++nretry_ > 5)
			exit(1);
		reconnect();
	};

	// Re-connection success
	void on_success(const mqtt::token& tok) override {
		std::cout << "\nConnection success" << std::endl;	
		for(int i=0;i<pub_topic_count ; i++){
			pub_topic_list_[i]->init();		
		}
		for(int i=0;i<sub_topic_count ; i++){
			sub_topic_list_[i]->init();		
		}
	};

	// Callback for when the connection is lost.
	// This will initiate the attempt to manually reconnect.
	void connection_lost(const std::string& cause) override{
		std::cout << "\nConnection lost" << std::endl;
		if (!cause.empty())
			std::cout << "\tcause: " << cause << std::endl;

		std::cout << "Reconnecting..." << std::endl;
		nretry_ = 0;
		reconnect();
	}
;

	// Callback for when a message arrives.
	void message_arrived(mqtt::const_message_ptr msg) override{
		std::cout << "Message arrived" << std::endl;
		std::cout << "\ttopic: '" << msg->get_topic() << "'" << std::endl;
		std::cout << "\tpayload: '" << msg->to_string() << "'\n" << std::endl;
		sub_topic_list_[get_sub_topic_instance(msg->get_topic())]->mqtt_response(msg);

	};

	void delivery_complete(mqtt::delivery_token_ptr token) override {};

public:
	hummingbird_topic_for_broker(mqtt::async_client* cli,hummingbird_curl_for_command* curl, mqtt::connect_options& connOpts, hummingbird_topic **pub_topic_list, hummingbird_topic **sub_topic_list);
	void connect();
    void disconnect();
	int get_pub_topic_instance(string topic);
	int get_sub_topic_instance(string topic);

#else // hwanjang - for std::vector
	std::vector<hummingbird_topic_for_broker*> pub_topic_list;
	std::vector<hummingbird_topic_for_broker*> sub_topic_list;

	// Counter for the number of connection retries
	int nretry_;
	mqtt::token_ptr conntok_;
	mqtt::async_client* cli_;
	// Options to use if we need to reconnect
	mqtt::connect_options& connOpts_;
	// An action listener to display the result of actions.
	action_listener subListener_;

	std::string create_topic(int mode, const std::string topic);

	time_t lastConnectionTime;

    // This deomonstrates manually reconnecting to the broker by calling
    // connect() again. This is a possibility for an application that keeps
    // a copy of it's original connect_options, or if the app wants to
    // reconnect with different options.
    // Another way this can be done manually, if using the same options, is
    // to just call the async_client::reconnect() method.
    void reconnect();

	// Re-connection failure
	void on_failure(const mqtt::token& tok) override;

    // Re-connection success
	void on_success(const mqtt::token& tok) override;

	// Callback for when the connection is lost.
	// This will initiate the attempt to manually reconnect.
	void connection_lost(const std::string& cause) override;	

	// Callback for when a message arrives.
	void message_arrived(mqtt::const_message_ptr msg) override;

	void delivery_complete(mqtt::delivery_token_ptr token) override {};

public:
	hummingbird_for_broker(mqtt::async_client* cli,mqtt::connect_options& connOpts, const std::string hub_id);

	void connect();
	void disconnect();
	int get_pub_topic_instance(string topic);
	int get_sub_topic_instance(string topic);

	void add_topic(int type, hummingbird_topic_for_broker* topic);
	hummingbird_topic_for_broker* get_pub_topic(const std::string topic);

	std::string get_camera_presence(void) { return presence_str; };
	void set_camera_presence(const std::string presence) { presence_str = presence; };
	bool get_connection_status(void) { return conn_status; };
	time_t get_lastConnectionTime(void) { return lastConnectionTime; };

private:
	bool gHummingbirdStart;
	bool conn_status;

	std::string app_id_;
	std::string presence_str;

#endif
};
