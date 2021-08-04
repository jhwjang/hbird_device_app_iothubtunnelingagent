#include<iostream>
#include <cstring>
#include "mqtt/async_client.h"
#include "json/json.h"
#include <string>


#include <iomanip>
#include <sstream>


using namespace std;

extern "C" {
	#include<curl/curl.h>
}

struct MemStruct {
  char *memory;
  size_t size;
};


class hummingbird_job_tunnel {
  private:  
    // mqtt::async_client* cli_;
  public:   
    hummingbird_job_tunnel(){};
    
    
    static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
    static size_t WriteHeaderMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
    int job(const char *localhost, const char *localport, mqtt::const_message_ptr msg, const char* pubTopic, mqtt::async_client* cli );
};