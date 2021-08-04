#include "thread.h"


class hummingbird_sunapi_listener : public Thread
{
  private:
    mqtt::async_client* cli_;
    // hummingbird_curl* curl_;
    hummingbird_topic * event_topic_;

  public:
    // hummingbird_sunapi_listener(mqtt::async_client* cli,hummingbird_curl* curl,hummingbird_topic * event_topic):Thread(){
    hummingbird_sunapi_listener(mqtt::async_client* cli,hummingbird_topic * event_topic):Thread(){    
        cli_ = cli;
        // curl_ = curl;
        event_topic_ = event_topic;

    }

    void *run() {
        printf("thread runrunrun\n");

        // curl_->temp();
        printf("thread done %lu\n", (long unsigned int)self());
        return NULL;
    }
};