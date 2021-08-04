#include<iostream>
#include <cstring>
#include "mqtt/async_client.h"

extern "C" {
	#include<curl/curl.h>
}

struct MemoryStruct {
  char *memory;
  size_t size;
};


class hummingbird_curl_for_command {
	
  private:
    mqtt::async_client* cli_;
  public:
    hummingbird_curl_for_command(mqtt::async_client* cli){
      cli_ = cli;
    };

 
    static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
    static int wait_on_socket(curl_socket_t sockfd, int for_recv, long timeout_ms);
    int command_sunapi(const char *url,char *return_msg, const char* pubTopic);
    

    

} ;