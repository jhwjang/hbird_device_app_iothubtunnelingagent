#ifndef CONFIG_MANAGER_H_
#define CONFIG_MANAGER_H_
#pragma once // 2017.11.22 hwanjang


class ConfigManager {
	public:
		ConfigManager();
	
		~ConfigManager();

		// SUNAPI
		int file_exit(std::string& filename);
		int CalculateRetryTime(int count);
		
		bool GetDeviceTokenByUserAccessTocken(std::string& mac_address, std::string& access_token, std::string* id, std::string* pw);
		bool GetConfigModelDevice(const std::string& user_token, std::string* address, std::string* device_id, std::string* device_key);

	private:

		static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);

		std::string gPrivate_id;
		std::string gPrivate_pw;
		std::string gPrivate_port;

		std::string gStunURI;
};

#endif  // CONFIG_MANAGER_H_
