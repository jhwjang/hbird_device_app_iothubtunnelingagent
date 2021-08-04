#include<iostream>

extern "C" {
	#include<curl/curl.h>
}

// - - - - - - - - - - - - - - - - - - - -

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


   
size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}


int wait_on_socket(curl_socket_t sockfd, int for_recv, long timeout_ms)
{
  struct timeval tv;
  fd_set infd, outfd, errfd;
  int res;
 
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec= (timeout_ms % 1000) * 1000;
 
  FD_ZERO(&infd);
  FD_ZERO(&outfd);
  FD_ZERO(&errfd);
 
  FD_SET(sockfd, &errfd); /* always check for error */ 
 
  if(for_recv) {
    FD_SET(sockfd, &infd);
  }
  else {
    FD_SET(sockfd, &outfd);
  }
 
  /* select() returns the number of signalled sockets or -1 */ 
  res = select((int)sockfd + 1, &infd, &outfd, &errfd, &tv);
  return res;
}

// - - - - - - - - - - - - - - - - - - - -

class hummingbird_curl_for_command {
	// const char* url = argv[1] ;
  private:
    // char* url = "http://127.0.0.1:8080/" ;

  public:
    hummingbird_curl_for_command(){};
    
//     int temp(){
//     CURL *curl;
//     CURLcode res;

//     const char *request = "GET /stw-cgi/eventstatus.cgi?msubmenu=eventstatus&amp;action=monitordiff&amp;Channel.0.EventType=MotionDetection&amp;AlarmInput=1&amp;AlarmOutput=1 HTTP/1.1\r\nHost: 112.170.72.123:99\r\n\r\n";
//     size_t request_len = strlen(request);
//     curl_socket_t sockfd;
//     size_t nsent_total = 0;
//     char url_addr[1024]={0,};
  
//     curl = curl_easy_init();
//     if(curl) {
//     // strcpy(url_addr, "http://112.170.72.123:99");
//     strcpy(url_addr, "http://112.170.72.123:99/stw-cgi/eventstatus.cgi?msubmenu=eventstatus&action=monitordiff&Channel.0.EventType=MotionDetection&AlarmInput=1&AlarmOutput=1");


//     curl_easy_setopt(curl, CURLOPT_URL, url_addr);
//     // curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
//     curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST); // digest authentication
//     curl_easy_setopt(curl, CURLOPT_USERPWD, "admin:5tkatjd!"); //tmp admin:1

//     /* Do not do the transfer - only connect to host */ 
//     curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
//     res = curl_easy_perform(curl);
    

//     if(res != CURLE_OK) {
//       printf("Error: %s\n", curl_easy_strerror(res));
//       return 1;
//     }
    

//     /* Extract the socket from the curl handle - we'll need it for waiting. */ 
//     res = curl_easy_getinfo(curl, CURLINFO_ACTIVESOCKET, &sockfd);
 
//     if(res != CURLE_OK) {
//       printf("Error: %s\n", curl_easy_strerror(res));
//       return 1;
//     }
 
// //    printf("Sending request.\n");
 
//     // do {
//     //   /* Warning: This example program may loop indefinitely.
//     //    * A production-quality program must define a timeout and exit this loop
//     //    * as soon as the timeout has expired. */ 
//     //   size_t nsent;
//     //   do {
//     //     nsent = 0;
//     //     res = curl_easy_send(curl, request + nsent_total,
//     //         request_len - nsent_total, &nsent);
//     //     nsent_total += nsent;
 
//     //     if(res == CURLE_AGAIN && !wait_on_socket(sockfd, 0, 60000L)) {
//     //       printf("Error: timeout.\n");
//     //       return 1;
//     //     }
//     //   } while(res == CURLE_AGAIN);
 
//     //   if(res != CURLE_OK) {
//     //     printf("Error: %s\n", curl_easy_strerror(res));
//     //     return 1;
//     //   }
 
//     //   printf("Sent %" CURL_FORMAT_CURL_OFF_T " bytes.\n",
//     //     (curl_off_t)nsent);
 
//     // } while(nsent_total < request_len);



 
//     printf("Reading response.\n");
 
//     for(;;) {
//       /* Warning: This example program may loop indefinitely (see above). */ 
//       char buf[1024];
//       size_t nread;
//       do {
//         nread = 0;
//         res = curl_easy_recv(curl, buf, sizeof(buf), &nread);
//         if(res == CURLE_AGAIN && !wait_on_socket(sockfd, 1, 60000L)) {
//           printf("Error_: timeout.\n");
//           return 1;
//         }
        
        
//       } while(res == CURLE_AGAIN);
 
//       if(res != CURLE_OK) {
//         printf("Error: %s\n", curl_easy_strerror(res));
//         break;
//       }
//       printf("Reading response.\n");

//       if(nread == 0) {
//         /* end of the response */ 
//         break;
//       }
//       printf("Received %s \n",buf);
//       printf("Received %" CURL_FORMAT_CURL_OFF_T " bytes.\n",
//         (curl_off_t)nread);
    

//     }
//   }
//   };




    

} ;
