#include <stdlib.h>
//#include <unistd.h>    // 리눅스에서 usleep 함수가 선언된 헤더 파일

#include "hummingbird_for_broker.h"

#ifdef _MSC_VER 
#include <windows.h>

#define localtime_r(_time, _result) _localtime64_s(_result, _time)
#endif

using namespace std;	

using std::this_thread::sleep_for;

#if 1
hummingbird_for_broker::hummingbird_for_broker(mqtt::async_client* client, mqtt::connect_options& connOpts, const std::string app_id)
	: nretry_(0), g_connOpts_(connOpts), subListener_("Subscription") {
	g_async_client = client;
	g_app_id_ = app_id;

	lastConnectionTime = 0;
	gHummingbirdStart = false;
}
#else
hummingbird_for_broker::hummingbird_for_broker(mqtt::async_client* cli,hummingbird_curl_for_command* curl, mqtt::connect_options& connOpts, hummingbird_topic_for_broker **pub_topic_list, hummingbird_topic_for_broker **sub_topic_list)
        : nretry_(0),  connOpts_(connOpts), subListener_("Subscription") {
        cli_ = cli;
        curl_= curl;
	pub_topic_list_ = pub_topic_list;
	sub_topic_list_ = sub_topic_list; 
}
#endif


void hummingbird_for_broker::connect(){
	printf("[hwanjang] hummingbird_for_broker::connect() -> Start ..\n");

#if 0
	cli_->set_callback(*this);
	cli_->connect(connOpts_, nullptr, *this);
#else	
	g_async_client->set_callback(*this);
	g_async_client->connect(g_connOpts_, nullptr, *this);
	cout << "Waiting for the connection..." << endl;

	//cout << "[hwanjang] hummingbird_for_broker::connect() ->  ...OK" << endl;
#endif
}    

void hummingbird_for_broker::disconnect(){
	g_async_client->disconnect()->wait();
}

int hummingbird_for_broker::get_pub_topic_instance(string topic)
{
	int count = pub_topic_list.size();
	for (int i=0; i<count; i++){
		if(pub_topic_list.at(i)->get_topic() == topic){
			//cout << "get_pub_topic_instance : "<<topic << endl;
			return i;
		}
	}
	return -1 ;
}

int hummingbird_for_broker::get_sub_topic_instance(string topic)
{
	int count = sub_topic_list.size();
	for (int i=0; i<count; i++){
		if(sub_topic_list.at(i)->get_topic() == topic){
			//cout << "get_sub_topic_instance : "<<topic << endl;
			return i;
		}
	}
	return -1 ;
}

std::string hummingbird_for_broker::create_topic(const std::string app_id, const std::string topic)
{
	std::string str = "ipc/req/subdevices/";

	str.append(app_id);
	str.append("/");
	str.append(topic);

	return str;
}

void hummingbird_for_broker::add_topic(int type, hummingbird_topic_for_broker* topic)
{
	if(type == 0)
		pub_topic_list.push_back(topic);
	else if(type == 1)
		sub_topic_list.push_back(topic);
}

hummingbird_topic_for_broker* hummingbird_for_broker::get_pub_topic(const std::string topic)
{
	int count = pub_topic_list.size();

	//printf("[hwanjang] hummingbird_for_broker::get_pub_topic() -> count : %d\n",count);

    for (int i=0; i<count; i++){
#if 0 // for debug
printf("[hwanjang] get_pub_topic : %s\n", (pub_topic_list.at(i)->get_topic()).c_str());
printf("[hwanjang] topic : %s\n", topic.c_str());
#endif
		if(pub_topic_list.at(i)->get_topic() == topic){
			//cout << "get_pub_topc : "<<topic << endl;

            return pub_topic_list.at(i);
        }
    }
	return NULL;
}

void hummingbird_for_broker::reconnect() {

#if 0 // hwanjang - for debug
struct timespec tspec;
if(clock_gettime(CLOCK_REALTIME, &tspec) != -1) 
{
   tm *tm;
   tm = localtime(&tspec.tv_sec);
   printf("[hwanjang] %d-%d-%d , %d : %d : %d -> Reconnect ---->\n", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
}
#else
	struct tm curr_tm;
	time_t curr_time = time(NULL);

	localtime_r(&curr_time, &curr_tm);

	int curr_year = curr_tm.tm_year + 1900;
	int curr_month = curr_tm.tm_mon + 1;
	int curr_day = curr_tm.tm_mday;
	int curr_hour = curr_tm.tm_hour;
	int curr_minute = curr_tm.tm_min;
	int curr_second = curr_tm.tm_sec;

	printf("[hwanjang] %d-%d-%d , %d : %d : %d -> broker Reconnect ---->\n",
		curr_year, curr_month, curr_day,
		curr_hour, curr_minute, curr_second);

#endif
	int sec;
    time_t now_sec, diff_sec;
    
    //now_sec = tspec.tv_sec;
	now_sec = curr_time;
    
	if (lastConnectionTime == 0)
		lastConnectionTime = now_sec;

    diff_sec = now_sec - lastConnectionTime;
    printf("[hwanjang] diff_sec : %lld\n", diff_sec);
		
	srand((unsigned)time(NULL));
    if(diff_sec < 1200)  // 1200 -> 20 min
    { 
		#if 0
    	if(nretry_ < 4)
			sec = (rand() % 10) + 1;
		else
			sec = (rand() % 60) + 30;
		#else
        sec = (rand() % 10) + 1;  // 1 ~ 10 sec
		#endif

        printf("[hwanjang] broker --> %d. reconnect after %d sec ...\n", (nretry_+1), sec);

		++nretry_;
    }   
    else
    {   
		printf("[hwanjang] broker --> exit @@@@@@@@@@@@ \n");
        exit(0); // program exit 
        // for test
        //sec = (rand() % 10) + 1;
        //printf("[hwanjang] --> do not exit .... reconnect after %d sec ...\n", sec);
    }   
		
    //sleep(sec);
	sleep_for(std::chrono::milliseconds(sec * 1000));
	try {
		//cli_->connect(connOpts_, nullptr, *this);
#if 1
		g_async_client->connect(g_connOpts_, nullptr, *this);
		cout << "broker ... Waiting for the reconnection..." << endl;       
#else	
		mqtt::token_ptr conntok = cli_->reconnect();
		cout << "Waiting for the reconnection..." << endl;

		conntok->wait();

		cout << "[hwanjang] hummingbird_for_broker::reconnect() ->  ...OK" << endl;
#endif
    }   
    catch (const mqtt::exception& exc) {
        std::cerr << "broker ... Error: " << exc.what() << std::endl;
        exit(1);
    }
    printf("\n[hwanjang] broker reconnect() --> end !!\n\n");
}

// Re-connection failure
void hummingbird_for_broker::on_failure(const mqtt::token& tok)
{
	std::cout << "hummingbird_for_broker ... Connection failed : " << std::endl;

#if 0 // for debug
	printf("[hwanjang] MQTT Connection failed ################\n");
	struct timespec tspec;
	if (clock_gettime(CLOCK_REALTIME, &tspec) != -1)
	{
		tm* tm;
		tm = localtime(&tspec.tv_sec);
		printf("[hwanjang] %d-%d-%d , %d : %d : %d -> Connection failed !!\n", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	}
#else
	printf("[hwanjang] hummingbird_for_broker ... MQTT Connection failed ################\n");

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

	g_conn_status = false;

	printf("[hwanjang] hummingbird_for_broker::on_failure() -> lastConnectionTime : %lld\n", (long long int)lastConnectionTime);

	if (lastConnectionTime == 0)
	{
		//struct timeval last;
		//gettimeofday(&last, NULL);
		//lastConnectionTime = last.tv_sec;
		lastConnectionTime = curr_time;
		printf("[hwanjang] hummingbird_for_broker::on_failure() -> lastConnectionTime : %lld\n", (long long int)lastConnectionTime);
	}

	reconnect();
}

// Re-connection success
void hummingbird_for_broker::on_success(const mqtt::token& tok)
{
	std::cout << "\nbroker ... MQTT Connection success -> hummingbird_for_broker::on_success()" << std::endl;

	if (tok.get_message_id() != 0)
		std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;

	pub_topic_list.at(0)->init();
	// presence msg should be generated -> hwanjang

	std::string topic_name = pub_topic_list.at(0)->get_topic();

	std::string subTopic = "connection";
	std::string connection_topic = create_topic(g_app_id_, subTopic); // 0: pub

	std::string presence_str = get_camera_presence();

#if 1 // 2019.12.11 - add -> null check
	if (presence_str.empty())
	{
		printf("[hwanjang] broker ... connection message is empty !!\n");
		return;
	}
#endif

#if 1 // for debug
	printf("[hwanjang] broker ... connection topic : %s\n", connection_topic.c_str());
	printf("[hwanjang] broker ... connection message : \n%s\n", presence_str.c_str());
#endif

	pub_topic_list.at(0)->send_message(connection_topic.c_str(), presence_str.c_str(), 1, true); // retain : true


	for (size_t i = 0; i < sub_topic_list.size(); i++) {
		sub_topic_list.at(i)->init();
	}

	lastConnectionTime = 0;
	nretry_ = 0;

	g_conn_status = true;
	std::cout << "\nbroker ... MQTT Connection success -> hummingbird_for_broker::on_success() -> End" << std::endl;
}

// Callback for when the connection is lost.
// This will initiate the attempt to manually reconnect.
void hummingbird_for_broker::connection_lost(const std::string& cause)
{
	std::cout << "hummingbird_for_broker::connection_lost" << std::endl;

#if 0 // for debug
	printf("[hwanjang] MQTT Connection lost ################\n");
	struct timespec tspec;
	if (clock_gettime(CLOCK_REALTIME, &tspec) != -1)
	{
		tm* tm;
		tm = localtime(&tspec.tv_sec);
		printf("[hwanjang] %d-%d-%d , %d : %d : %d -> Connection lost ...\n", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	}
#else
	printf("[hwanjang] broker ... MQTT Connection lost ################\n");

	struct tm curr_tm;
	time_t curr_time = time(nullptr);

	localtime_r(&curr_time, &curr_tm);

	int curr_year = curr_tm.tm_year + 1900;
	int curr_month = curr_tm.tm_mon + 1;
	int curr_day = curr_tm.tm_mday;
	int curr_hour = curr_tm.tm_hour;
	int curr_minute = curr_tm.tm_min;
	int curr_second = curr_tm.tm_sec;
	printf("[hwanjang] %d-%d-%d , %d : %d : %d -> broker ... Connection lost ...\n",
		curr_year, curr_month, curr_day,
		curr_hour, curr_minute, curr_second);
#endif

	g_conn_status = false;

	if (!cause.empty())
		std::cout << "\tcause: " << cause << std::endl;

	std::cout << "broker ... Reconnecting..." << std::endl;

	printf("[hwanjang] hummingbird_for_broker::connection_lost() -> lastConnectionTime : %lld\n", (long long int)lastConnectionTime);

	if (lastConnectionTime == 0)
	{
		//struct timeval last;
		//gettimeofday(&last, NULL);
		//lastConnectionTime = last.tv_sec;
		lastConnectionTime = curr_time;

		printf("[hwanjang] hummingbird_for_broker::connection_lost() -> lastConnectionTime : %lld\n", (long long int)lastConnectionTime);
	}

	reconnect();
}

// Callback for when a message arrives.
void hummingbird_for_broker::message_arrived(mqtt::const_message_ptr msg)
{
#if 0
	struct timespec recv_tspec;
	clock_gettime(CLOCK_REALTIME, &recv_tspec);
	printf("Message arrived , topic : %s\ntime -> tv_sec : %lld, tv_nsec : %lld\n", 
				msg->get_topic().c_str(), (long long int)recv_tspec.tv_sec, (long long int)recv_tspec.tv_nsec);
	//#else
	std::cout << "MQTT Message arrived" << std::endl;
	std::cout << "\ttopic: " << msg->get_topic() << std::endl;
	std::cout << "\tpayload: " << msg->to_string() << "\n" << std::endl;
#else

	struct tm curr_tm;
	time_t curr_time = time(NULL);

	localtime_r(&curr_time, &curr_tm);

	int curr_year = curr_tm.tm_year + 1900;
	int curr_month = curr_tm.tm_mon + 1;
	int curr_day = curr_tm.tm_mday;
	int curr_hour = curr_tm.tm_hour;
	int curr_minute = curr_tm.tm_min;
	int curr_second = curr_tm.tm_sec;

	printf("[hwanjang] %d-%d-%d , %d : %d : %d -> broker ... Message arrived ---->\n",
		curr_year, curr_month, curr_day,
		curr_hour, curr_minute, curr_second);

	printf("broker ... Topic : %s\n", msg->get_topic().c_str());

	std::cout << "broker ... MQTT Message arrived" << std::endl;
	std::cout << "\ttopic: " << msg->get_topic() << std::endl;
	std::cout << "\tpayload: " << msg->to_string() << "\n" << std::endl;
#endif
	g_conn_status = true;

	std::string subTopic;

	if (msg->get_topic().find("conection") != std::string::npos)
	{
		subTopic = "conection";
	}
	else if (msg->get_topic().find("command") != std::string::npos)
	{
		subTopic = "command";
	}
	else if (msg->get_topic().find("message") != std::string::npos)
	{
		subTopic = "message";
	}
	else
	{
		printf("[hwanjang] hummingbird_main::message_arrived() -> unknown message ... %s\n", msg->get_topic().c_str());

		return;
	}

	std::string topic = create_topic(g_app_id_, subTopic); // 1: sub
	int index = get_sub_topic_instance(topic);
	//std::cout << "topic index : " << index << std::endl;
	sub_topic_list.at(index)->mqtt_response(msg);

}
