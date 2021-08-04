#include <iostream>
#include <cstring>

enum {
	ERROR_ARGS = 1 ,
	ERROR_CURL_INIT = 2
} ;

enum {
	OPTION_FALSE = 0 ,
	OPTION_TRUE = 1
} ;

enum {
	FLAG_DEFAULT = 0 
} ;

struct MemoryStruct {
  char *memory;
  size_t size;
};

class hummingbird_curl_for_command {
	
  private:
    
  public:
    hummingbird_curl_for_command(){

    }
 
    static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
//    static int wait_on_socket(curl_socket_t sockfd, int for_recv, long timeout_ms);
    int command_sunapi(const char *url,char *return_msg);

} ;
