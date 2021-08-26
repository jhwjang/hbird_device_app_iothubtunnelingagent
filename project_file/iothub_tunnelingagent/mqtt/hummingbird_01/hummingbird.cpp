#include <stdlib.h>
//#include <unistd.h>    // 리눅스에서 usleep 함수가 선언된 헤더 파일

#include "hummingbird.h"

#ifdef _MSC_VER 
#include <windows.h>

#define localtime_r(_time, _result) _localtime64_s(_result, _time)
#endif

using namespace std;	

using std::this_thread::sleep_for;

#if 1
hummingbird::hummingbird(mqtt::async_client* cli, mqtt::connect_options& connOpts, const std::string hub_id)
	: nretry_(0),  connOpts_(connOpts), subListener_("Subscription") {
	cli_ = cli;
	hub_id_ = hub_id;

	lastConnectionTime = 0;
	gHummingbirdStart = false;
}
#else
hummingbird::hummingbird(mqtt::async_client* cli,hummingbird_curl_for_command* curl, mqtt::connect_options& connOpts, hummingbird_topic **pub_topic_list, hummingbird_topic **sub_topic_list)
        : nretry_(0),  connOpts_(connOpts), subListener_("Subscription") {
        cli_ = cli;
        curl_= curl;
	pub_topic_list_ = pub_topic_list;
	sub_topic_list_ = sub_topic_list; 
}
#endif


void hummingbird::connect(){
printf("[hwanjang] hummingbird::connect() -> Start ..\n");

#if 0
	cli_->set_callback(*this);
	cli_->connect(connOpts_, nullptr, *this);
#else	
	cli_->set_callback(*this);
	cli_->connect(connOpts_, nullptr, *this);
	cout << "Waiting for the connection..." << endl;

	//cout << "[hwanjang] hummingbird::connect() ->  ...OK" << endl;
#endif
}    

void hummingbird::disconnect(){
	cli_->disconnect()->wait();
}

int hummingbird::get_pub_topic_instance(string topic)
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

int hummingbird::get_sub_topic_instance(string topic)
{
	int count = sub_topic_list.size();
	for (int i=0; i<count; i++){
		if(sub_topic_list.at(i)->get_topic() == topic){
			//cout << "get_sub_topic_instance : "<< topic << " has been found !!!" << endl;
			return i;
		}
	}
	return -1 ;
}

void hummingbird::add_topic(int type, hummingbird_topic* topic)
{
	if(type == 0)		// add pub
		pub_topic_list.push_back(topic);
	else if(type == 1)	// add sub
		sub_topic_list.push_back(topic);
}

hummingbird_topic* hummingbird::get_pub_topic(const std::string topic)
{
	int count = pub_topic_list.size();

	//printf("[hwanjang] hummingbird::get_pub_topic() -> count : %d\n",count);

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

void hummingbird::reconnect() {

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

	printf("[hwanjang] %d-%d-%d , %d : %d : %d -> Reconnect ---->\n",
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
    printf("[hwanjang] diff_sec : %lld\n", (long long int)diff_sec);
		
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

        printf("[hwanjang] --> %d. reconnect after %d sec ...\n", (nretry_+1), sec);

		++nretry_;
    }   
    else
    {   
		printf("[hwanjang] --> exit @@@@@@@@@@@@ \n");
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
		cli_->connect(connOpts_, nullptr, *this);
		cout << "Waiting for the reconnection..." << endl;       
#else	
		mqtt::token_ptr conntok = cli_->reconnect();
		cout << "Waiting for the reconnection..." << endl;

		conntok->wait();

		cout << "[hwanjang] hummingbird::reconnect() ->  ...OK" << endl;
#endif
    }   
    catch (const mqtt::exception& exc) {
        std::cerr << "Error: " << exc.what() << std::endl;
        exit(1);
    }
    printf("\n[hwanjang] reconnect() --> end !!\n\n");
}

