
// 2017.07.07 hwanjang - for lib paho (MQTT)
#include "mqtt/mqtt_interface.h"

struct IMQTTManagerObserver
{
    virtual ~IMQTTManagerObserver() {};

    virtual void ReceiveMessageFromPeer(mqtt::const_message_ptr mqttMsg) = 0;
    //virtual void ReceiveMessageFromPeer(int type, const std::string& topic, const std::string& deviceid, const std::string& user, const std::string& message) = 0;
    virtual void OnMQTTServerConnectionSuccess(void) = 0;

};

class MQTTManager : public IMQTTManagerSink {
    public:
        MQTTManager(std::string address, std::string id, std::string pw);
        ~MQTTManager();

        void init(const std::string& path);
        void start();

        void RegisterObserverForMQTT(IMQTTManagerObserver* callback) { observerForHbirdManager = callback; };

        time_t getLastConnection_Time();
        void SendMessageToClient(std::string topic, std::string message, int type);
        void OnResponseCommandMessage(std::string topic, std::string message);
        void OnResponseCommandMessage(std::string topic, const void* payload, int size);
        
        virtual void ReceiveMessageFromPeer(mqtt::const_message_ptr mqttMsg);
		//virtual void ReceiveMessageFromPeer(int type, const std::string& topic, const std::string& deviceid, const std::string& user, const std::string& message);
   		virtual void OnMQTTServerConnectionSuccess(void);

        
	private:

		HummingbirdMqttInterface* MQTTHandler_;

		IMQTTManagerObserver* observerForHbirdManager;

        std::string gMQTTServerAddress;
        std::string gDeviceID;
        std::string gDevicePW;

};


