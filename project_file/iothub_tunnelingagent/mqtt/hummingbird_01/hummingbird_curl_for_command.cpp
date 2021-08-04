#include <stdio.h>
#include <stdlib.h>

#include "hummingbird_curl_for_command.h"


   size_t hummingbird_curl_for_command::WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
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
    };

#if 0
  int hummingbird_curl_for_command::wait_on_socket(curl_socket_t sockfd, int for_recv, long timeout_ms)
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
    };
#endif

    int hummingbird_curl_for_command::command_sunapi(const char *url,char *return_msg)
    {
        int url_size = sizeof(url);

        //char url_[1024]={0,};
        char* url_;
        url_ = new char[url_size+1];
#ifdef _MSV_VER
        strncpy_s(url_, sizeof(url_), url, sizeof(url));
#else
        strncpy(url_, url, url_size);
#endif
        
        printf("** url : %s\n", url_);     
#if 1
        system("curl --digest -u admin:1 'http://127.0.0.1/stw-cgi/system.cgi?msubmenu=deviceinfo&action=view' >> /tmp/deviceinfo");
#else     
        char url_[1024]={0,};
        strcpy(url_, url);
        
        printf("** url : %s\n", url_);

        CURL *curl_handle;
        struct MemoryStruct chunk;

        chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
        chunk.size = 0;    /* no data at this point */

        curl_global_init(CURL_GLOBAL_ALL);
        curl_handle = curl_easy_init();

        if(curl_handle == NULL)
        {
                printf("error ... initialize cURL !!!\n");
        }

        curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10);
        curl_easy_setopt(curl_handle, CURLOPT_URL, url_);
        curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST); // digest authentication
        curl_easy_setopt(curl_handle, CURLOPT_USERPWD, "admin:5tkatjd!"); //tmp admin:1
        
        /* send all data to this function  */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

        /* we pass our 'chunk' struct to the callback function */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
        CURLcode res = curl_easy_perform(curl_handle);

        /* check for errors */
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
            return -1;
        }
        else {
            printf("%lu bytes retrieved\n", (long)chunk.size);
            // printf("data : %s\n",chunk.memory);
            strcpy(return_msg,chunk.memory);
        }

        curl_easy_cleanup(curl_handle);
        free(chunk.memory);

        /* we're done with libcurl, so clean it up */
        curl_global_cleanup();
#endif  

        delete url_;

        return 0;
    };


    

