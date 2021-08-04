#include <chrono>
#include "mqtt/async_client.h"
#include "hummingbird/hummingbird_topic.h"


// using namespace std;	
using std::thread;
const int topic_count = 1;


/////////////////////////////////////////////////////////////////////////////

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


/////////////////////////////////////////////////////////////////////////////

/**
 * Local callback & listener class for use with the client connection.
 * This is primarily intended to receive messages, but it will also monitor
 * the connection to the broker. If the connection is lost, it will attempt
 * to restore the connection and re-subscribe to the topic.
 */
class hummingbird : public virtual mqtt::callback,
					public virtual mqtt::iaction_listener
{
   	hummingbird_topic **topic_list_ ;
	string localhost_;

 
 	// Counter for the number of connection retries
	int nretry_;
	mqtt::async_client* cli_;
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
		std::cout << "nretry : " << nretry_ << std::endl;	


		for(int i=0;i<topic_count ; i++){
			topic_list_[i]->init();		
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
		// std::cout << "Message arrived" << std::endl;
		// std::cout << "\ntopic: '" << msg->get_topic() << "'" << std::endl;
		// std::cout << "\npayload: '" << msg->to_string() << "'\n" << std::endl;

		/* 
			200407 권혁진
			tunneling 의 경우 Topic은 1개 이다. 
			즉, topic_list의 크기는 '1' 이기 때문에 아래와 같이 mqtt_response() 함수를 호출한다. 		
			topic_list의 크기가 '1' 이상인 경우에는 topic을 파싱하여 적절한 hummingbird_topic 객체를 찾아서 mqtt_response() 함수를 호출해야 한다. 
		*/
		topic_list_[0]->mqtt_response(msg);
	};

	void delivery_complete(mqtt::delivery_token_ptr token) override {};

public:
	hummingbird(mqtt::async_client* cli, mqtt::connect_options& connOpts, hummingbird_topic **topic_list);
	void connect();
    void disconnect();
	// int get_pub_topic_instance(string topic);
	// int get_sub_topic_instance(string topic);

};


