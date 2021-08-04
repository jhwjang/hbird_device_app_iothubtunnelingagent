//#include <sys/time.h>

#include <chrono>
#include "mqtt/async_client.h"
#include "hummingbird/hummingbird_topic.h"

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

#if 0 
	hummingbird_topic **pub_topic_list_ ;
	hummingbird_topic **sub_topic_list_ ;
 
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
	hummingbird(mqtt::async_client* cli,hummingbird_curl_for_command* curl, mqtt::connect_options& connOpts, hummingbird_topic **pub_topic_list, hummingbird_topic **sub_topic_list);
	void connect();
    void disconnect();
	int get_pub_topic_instance(string topic);
	int get_sub_topic_instance(string topic);

#else // hwanjang - for std::vector
	std::vector<hummingbird_topic*> pub_topic_list;
	std::vector<hummingbird_topic*> sub_topic_list;

	// Counter for the number of connection retries
	int nretry_;
	mqtt::token_ptr conntok_;
	mqtt::async_client* cli_;
	hummingbird_curl_for_command* curl_;
	// Options to use if we need to reconnect
	mqtt::connect_options& connOpts_;
	// An action listener to display the result of actions.
	action_listener subListener_;

	std::string create_topic(int mode, const std::string topic)
	{
		std::string str;

		if(mode == 0)
		{
			str = "devices/";
			str.append(hub_id_);
			str.append("/");
			str.append(topic);
		}
		else
		{
			str = "hummingbird/devices/";
			str.append(hub_id_);
			str.append("/");
			str.append(topic);
		}

		return str;
	};


	int lastConnectionTime;

    // This deomonstrates manually reconnecting to the broker by calling
    // connect() again. This is a possibility for an application that keeps
    // a copy of it's original connect_options, or if the app wants to
    // reconnect with different options.
    // Another way this can be done manually, if using the same options, is
    // to just call the async_client::reconnect() method.
    void reconnect();

	// Re-connection failure
	void on_failure(const mqtt::token& tok) override 
	{
		std::cout << "Connection failed : " << std::endl;

#if 0 // for debug
		printf("[hwanjang] MQTT Connection failed ################\n");
		struct timespec tspec;
		if(clock_gettime(CLOCK_REALTIME, &tspec) != -1)
		{
			tm *tm;
			tm = localtime(&tspec.tv_sec);
			printf("[hwanjang] %d-%d-%d , %d : %d : %d -> Connection failed !!\n", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		}
#else
		printf("[hwanjang] MQTT Connection failed ################\n");

		struct tm curr_tm;
		time_t curr_time = time(nullptr);

		localtime_r(&curr_time, &curr_tm);

		int curr_year = curr_tm.tm_year + 1900;
		int curr_month = curr_tm.tm_mon + 1;
		int curr_day = curr_tm.tm_mday;
		int curr_hour = curr_tm.tm_hour;
		int curr_minute = curr_tm.tm_min;
		int curr_second = curr_tm.tm_sec;
		printf("[hwanjang] %d-%d-%d , %d : %d : %d -> Connection failed !!\n",
			curr_year, curr_month, curr_day,
			curr_hour, curr_minute, curr_second);
#endif

		if (tok.get_message_id() != 0)
			std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;

		conn_status = false;

		printf("[hwanjang] connection_failed() -> lastConnectionTime : %d\n", lastConnectionTime);

		if(lastConnectionTime == 0)
		{
			//struct timeval last;
			//gettimeofday(&last, NULL);
			//lastConnectionTime = last.tv_sec;
			lastConnectionTime = curr_time;
			printf("[hwanjang] on_failure() -> lastConnectionTime : %d\n", lastConnectionTime);
		}

		reconnect(); 
	};

    // Re-connection success
    void on_success(const mqtt::token& tok) override 
	{
		std::cout << "\nMQTT Connection success -> on_success()" << std::endl;

    if (tok.get_message_id() != 0)
			std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
		
		pub_topic_list.at(0)->init();
		// presence msg should be generated -> hwanjang

		std::string connection_topic;
		std::string topic_name = pub_topic_list.at(0)->get_topic();
		connection_topic = create_topic(0, "connection"); // 0: pub
	
		std::string presence_str = get_camera_presence();

		#if 1 // 2019.12.11 - add -> null check
		if(presence_str.empty())
		{
			printf("[hwanjang] connection message is empty !!\n");
			return;
		}
		#endif

#if 1 // for debug
printf("[hwanjang] connection topic : %s\n", connection_topic.c_str());
printf("[hwanjang] connection message : \n%s\n",presence_str.c_str());
#endif

		pub_topic_list.at(0)->send_message(connection_topic.c_str(), presence_str.c_str(), 1, true); // retain : true


		for(size_t i=0;i<sub_topic_list.size(); i++){
				sub_topic_list.at(i)->init();             
		}

		lastConnectionTime = 0;
		nretry_ = 0;

		conn_status = true;
		std::cout << "\nMQTT Connection success -> on_success() -> End" << std::endl;
	};

		// Callback for when the connection is lost.
		// This will initiate the attempt to manually reconnect.
		void connection_lost(const std::string& cause) override
		{
			std::cout << "\nConnection lost" << std::endl;
#if 0 // for debug
			printf("[hwanjang] MQTT Connection lost ################\n");
			struct timespec tspec;
			if(clock_gettime(CLOCK_REALTIME, &tspec) != -1)
			{
				tm *tm;
				tm = localtime(&tspec.tv_sec);
				printf("[hwanjang] %d-%d-%d , %d : %d : %d -> Connection lost ...\n", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
			}
#else
			printf("[hwanjang] MQTT Connection lost ################\n");

			struct tm curr_tm;
			time_t curr_time = time(nullptr);

			localtime_r(&curr_time, &curr_tm);

			int curr_year = curr_tm.tm_year + 1900;
			int curr_month = curr_tm.tm_mon + 1;
			int curr_day = curr_tm.tm_mday;
			int curr_hour = curr_tm.tm_hour;
			int curr_minute = curr_tm.tm_min;
			int curr_second = curr_tm.tm_sec;
			printf("[hwanjang] %d-%d-%d , %d : %d : %d -> Connection lost ...\n",
				curr_year, curr_month, curr_day,
				curr_hour, curr_minute, curr_second);
#endif

			conn_status = false;

			if (!cause.empty())
				std::cout << "\tcause: " << cause << std::endl;

			std::cout << "Reconnecting..." << std::endl;

			printf("[hwanjang] connection_lost() -> lastConnectionTime : %d\n", lastConnectionTime);

			if(lastConnectionTime == 0)
			{
				//struct timeval last;
				//gettimeofday(&last, NULL);
				//lastConnectionTime = last.tv_sec;
				lastConnectionTime = curr_time;

				printf("[hwanjang] connection_lost() -> lastConnectionTime : %d\n", lastConnectionTime);
			}

			reconnect();
		};

		// Callback for when a message arrives.
		void message_arrived(mqtt::const_message_ptr msg) override{
#if 0
			struct timespec recv_tspec;
			clock_gettime(CLOCK_REALTIME, &recv_tspec);
			printf("Message arrived , topic : %s\ntime -> tv_sec : %ld, tv_nsec : %ld\n", msg->get_topic().c_str(), recv_tspec.tv_sec, recv_tspec.tv_nsec);
			//#else
			std::cout << "MQTT Message arrived" << std::endl;
			std::cout << "\ttopic: " << msg->get_topic() << std::endl;
			std::cout << "\tpayload: " << msg->to_string() << "\n" << std::endl;
#endif
			conn_status = true;

			std::string topic;
			int index;

			if(msg->get_topic().find("live") != std::string::npos)
			{
				topic = create_topic(1, "subdevices/+/users/+/live"); // 1: sub
				index = get_sub_topic_instance(topic);
				//std::cout << "topic index : " << index << std::endl;
				sub_topic_list.at(index)->mqtt_response(msg);
			}
			else if(msg->get_topic().find("playback") != std::string::npos)
			{
				topic = create_topic(1, "subdevices/+/users/+/playback"); // 1: sub
				index = get_sub_topic_instance(topic);
				//std::cout << "topic index : " << index << std::endl;
				sub_topic_list.at(index)->mqtt_response(msg);
			}
			else if(msg->get_topic().find("command") != std::string::npos)
			{
				topic = create_topic(1, "subdevices/+/users/+/command/#"); // 1: sub
				index = get_sub_topic_instance(topic);
				//std::cout << "topic index : " << index << std::endl;
				sub_topic_list.at(index)->mqtt_response(msg);
			}
			else if(msg->get_topic().find("tunneling") != std::string::npos)
			{
				topic = create_topic(1, "subdevices/+/users/+/tunneling/#"); // 1: sub
				index = get_sub_topic_instance(topic);
				std::cout << "topic index : " << index << std::endl;
				sub_topic_list.at(index)->mqtt_response(msg);
			}   

		};

        void delivery_complete(mqtt::delivery_token_ptr token) override {};

public:
				hummingbird(mqtt::async_client* cli,hummingbird_curl_for_command* curl, mqtt::connect_options& connOpts, const std::string hub_id);

				void connect();
				void disconnect();
				int get_pub_topic_instance(string topic);
				int get_sub_topic_instance(string topic);

				void add_topic(int type, hummingbird_topic* topic);
				hummingbird_topic* get_pub_topic(const std::string topic);

				std::string get_camera_presence(void) { return presence_str; };
				void set_camera_presence(const std::string presence) { presence_str = presence; };
				bool get_connection_status(void) { return conn_status; };
				long get_lastConnectionTime(void) { return lastConnectionTime; };

private:
				bool gHummingbirdStart;
				bool conn_status;
				std::string hub_id_;
				std::string presence_str;

#endif
};
