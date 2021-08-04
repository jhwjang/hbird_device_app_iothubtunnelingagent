#ifndef API_MANAGER_H_
#define API_MANAGER_H_
#pragma once // 2017.11.22 hwanjang

#include "jansson.h"

struct IAPIManagerObserver {
  virtual ~IAPIManagerObserver() {};

  virtual void SendResponseToPeer(const std::string& topic, const std::string& message) = 0;
  virtual void SendResponseToPeerForTunneling(const std::string& topic, const void* payload, int size) = 0;
};

class APIManager {
	public:
		APIManager();	
		~APIManager();

		void RegisterObserverForHbirdManager(IAPIManagerObserver* callback);

		bool tunneling_command(const std::string& strTopic, json_t* root);

		int ThreadStartForUpdatingConfigData(int interval);

	private:

		// SUNAPI
		int file_exit(std::string& filename);
		int CalculateRetryTime(int count);

		static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
		static size_t WriteMemoryCallbackForHeader(void* contents, size_t size, size_t nmemb, void* userp);

		void thread_function_for_updating_config(int interval);
		//int ThreadStartForUpdatingConfigData(int interval);

		IAPIManagerObserver* observerForHbirdManager;

		std::string gPrivate_id;
		std::string gPrivate_pw;
		std::string gPrivate_port;

		std::string gStunURI;

};

#endif  // API_MANAGER_H_
