#include "sunapi_manager.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <sstream>  // for istringstream , getline
#include <fstream>
#include <chrono>
#include <vector>

using std::this_thread::sleep_for;

// Default timeout is 0 (zero) which means it never times out during transfer.
#define CURL_TIMEOUT 5 
#define CURL_CONNECTION_TIMEOUT 3  

#define DEFAULT_MAX_CHANNEL 128  // support max 128 channels

std::mutex g_Mtx_lock_storage;
std::mutex g_Mtx_lock_device;

static int g_RegisteredCameraCnt;
static int g_ConnectionCnt;

// gateway
GatewayInfo* g_Gateway_info_;

// 1. dashboard
Storage_Infos* g_Storage_info_;
Storage_Infos* g_Worker_Storage_info_;
static int g_StorageCheckCnt;

// 2. deviceinfo
Channel_Infos* g_SubDevice_info_;
Channel_Infos* g_Worker_SubDevice_info_;

// 3. firmware version
Firmware_Version_Infos* g_Firmware_Ver_info_;
Firmware_Version_Infos* g_Worker_Firmware_Ver_info_;

sunapi_manager::sunapi_manager(const std::string& strDeviceID)
{
	g_Device_id = strDeviceID;

	gMax_Channel = 0;

	gStrDeviceIP.clear();
	gStrDevicePW.clear();

	g_RegisteredCameraCnt = 0;
	g_ConnectionCnt = 0;
	g_StorageCheckCnt = 0;

	g_Sub_camera_reg_Cnt = 0;
	g_Sub_network_info_Cnt = 0;
	g_Sub_device_info_Cnt = 0;

	g_UpdateTimeOut = 5; // init
	g_UpdateTimeOfRegistered = time(NULL);
	g_UpdateTimeOfStoragePresence = time(NULL);
	g_UpdateTimeOfStorageStatus = time(NULL);
	g_UpdateTimeOfNetworkInterface = time(NULL);
	g_UpdateTimeForFirmwareVersionOfSubdevices = time(NULL);
	g_UpdateTimeForDashboardAPI = time(NULL);

	observerForHbirdManager = nullptr;

}

sunapi_manager::~sunapi_manager()
{
	// 1. dashboard
	if(g_Storage_info_)
		delete[] g_Storage_info_;

	if(g_Worker_Storage_info_)
		delete[] g_Worker_Storage_info_;

	// 2. deviceinfo
	if (g_SubDevice_info_)
		delete[] g_SubDevice_info_;

	if (g_Worker_SubDevice_info_)
		delete[] g_Worker_SubDevice_info_;

	// 3. firmware version
	if (g_Firmware_Ver_info_)
		delete[] g_Firmware_Ver_info_;

	if (g_Worker_Firmware_Ver_info_)
		delete[] g_Worker_Firmware_Ver_info_;

}

void sunapi_manager::RegisterObserverForHbirdManager(ISUNAPIManagerObserver* callback)
{
	observerForHbirdManager = callback;
}

void sunapi_manager::SunapiManagerInit()
{
	// TODO: 여기에 구현 코드 추가.

	/// <summary>
	///  Get Gateway IP , ID & PW
	/// </summary>

	std::string strIP, strPW;

	bool result = GetDeviceIP_PW(&strIP, &strPW);

	if ((result != true) || strIP.empty() || strPW.empty())
	{
		gStrDeviceIP = "127.0.0.1";
		gStrDevicePW = "admin:5tkatjd!";
	}
	else
	{
		gStrDeviceIP = strIP;
		gStrDevicePW = strPW;
	}

	printf("sunapi_manager_init() -> gStrDeviceIP : %s , gStrDevicePW : %s\n", gStrDeviceIP.c_str(), gStrDevicePW.c_str());

	/// <summary>
	//  Get Max Channel
	/// </summary>

	int maxCh = 0;
	maxCh = GetMaxChannelByAttribute(gStrDevicePW);

	if (maxCh > 0)
	{
		gMax_Channel = maxCh;
	}
	else
	{
		// ???? 
		gMax_Channel = DEFAULT_MAX_CHANNEL;
	}

	printf("sunapi_manager_init() -> Max Channel : %d\n", gMax_Channel);

	// gateway init
	g_Gateway_info_ = new GatewayInfo;

	ResetGatewayInfo();

	result = GetGatewayInfo();

	// error ??
	if (!result)
	{
		g_Gateway_info_->IPv4Address = gStrDeviceIP;
		g_Gateway_info_->WebPort = 0;
		g_Gateway_info_->MACAddress = "unknown";
	}

	result = GetDeviceInfoOfGateway();
	
	if (!result)
	{
		g_Gateway_info_->FirmwareVersion = "unknown";
	}

	// 1. dashboard init
	g_Storage_info_ = new Storage_Infos[gMax_Channel + 1];
	g_Worker_Storage_info_ = new Storage_Infos[gMax_Channel + 1];


	for (int i = 0; i < gMax_Channel + 1; i++)
	{
		ResetStorageInfos(i);
	}

	// 2. deviceinfo init
	g_SubDevice_info_ = new Channel_Infos[gMax_Channel + 1];
	g_Worker_SubDevice_info_ = new Channel_Infos[gMax_Channel + 1];

	for (int i = 0; i < gMax_Channel + 1; i++)
	{
		ResetSubdeviceInfos(i);
	}
	
	// 3. firmware version
	g_Firmware_Ver_info_ = new Firmware_Version_Infos[gMax_Channel + 1];;
	g_Worker_Firmware_Ver_info_ = new Firmware_Version_Infos[gMax_Channel + 1];

	// Get ConnectionStatus of subdevice
	result = GetRegiesteredCameraStatus(gStrDeviceIP, gStrDevicePW);

	if (!result)
	{
		printf("failed to get 2. connectionStatus of subdevice ... ");
	}

#if 1
	GetMacAddressOfSubdevices();
#else
	GetMacAddressOfSubdevices();

	// Get Storage info
	GetStoragePresenceOfSubdevices();

	sleep_for(std::chrono::milliseconds(1 * 1000));

	GetStorageStatusOfSubdevices();

	sleep_for(std::chrono::milliseconds(2 * 1000));

	UpdateStorageInfos();

	ThreadStartGetLatestFirmwareVersionFromURL();

	GetFirmwareVersionOfSubdevices();

	sleep_for(std::chrono::milliseconds(2 * 1000));

	UpdateSubdeviceInfos();
#endif
	//ThreadStartForUpdateInfos(60*60);  // after 1 hour
}

bool sunapi_manager::GetGatewayInfo()
{
	std::string request;

	/// /////////////////////////////////////////////////////////////////////////////////////
	// Get HTTP Port

	request = "http://";
	request.append(gStrDeviceIP);
	request.append("/stw-cgi/network.cgi?msubmenu=http&action=view");

#if 1  // 2019.09.05 hwanjang - sunapi debugging
	printf("sunapi_manager::GetGatewayInfo() -> 1. SUNAPI Request : %s\n", request.c_str());
#endif

	CURLcode res;
	std::string strSUNAPIResult;
	bool json_mode = true;
	bool ssl_opt = false;

#if 0

	res = CURL_Process(json_mode, ssl_opt, request, gStrDevicePW, &strSUNAPIResult);

	if (res == CURLE_OK)
	{
		if (strSUNAPIResult.empty())
		{
			printf("[hwanjang] GetGatewayInfo() -> strSUNAPIResult is empty !!!\n");
			return false;
		}
		else
		{
			json_error_t error_check;
			json_t* json_strRoot = json_loads(strSUNAPIResult.c_str(), 0, &error_check);

			if (!json_strRoot)
			{
				fprintf(stderr, "error : root\n");
				fprintf(stderr, "error : on line %d: %s\n", error_check.line, error_check.text);

				printf("strSUNAPIResult : \n%s\n", strSUNAPIResult.c_str());

				return false;
			}

			int web_port;
			int result = json_unpack(json_strRoot, "{s:i}", "Port", &web_port);

			if (result)
			{
				printf("[hwanjang] Error !! GetGatewayInfo() -> 1. json_unpack fail .. !!!\n");
				return false;
			}
			else
			{
				g_Gateway_info_->WebPort = web_port;
			}
		}
	}
	else
	{
		printf("error ... initialize cURL !!!\n");
		return false;
	}
#else
	g_Gateway_info_->WebPort = 80;
#endif

	/// /////////////////////////////////////////////////////////////////////////////////////
	// Get IPv4Address , MACAddress
	//  - /stw-cgi/network.cgi?msubmenu=interface&action=view  -> IPv4Address , MACAddress
	//  - /stw-cgi/system.cgi?msubmenu=deviceinfo&action=view  -> ConnectedMACAddress , FirmwareVersion
	// 
	request.clear();

	request = "http://";
	request.append(gStrDeviceIP);
	request.append("/stw-cgi/network.cgi?msubmenu=interface&action=view");

#if 1  // 2019.09.05 hwanjang - sunapi debugging
	printf("sunapi_manager::GetGatewayInfo() -> 2. SUNAPI Request : %s\n", request.c_str());
#endif

	strSUNAPIResult.clear();

	res = CURL_Process(json_mode, ssl_opt, CURL_TIMEOUT, request, gStrDevicePW, &strSUNAPIResult);

	if (res == CURLE_OK)
	{
		if (strSUNAPIResult.empty())
		{
			// strSUNAPIResult is empty ...
			return false;
		}
		else
		{
			json_error_t error_check;
			json_t* json_strRoot = json_loads(strSUNAPIResult.c_str(), 0, &error_check);

			if (!json_strRoot)
			{
				fprintf(stderr, "error : root\n");
				fprintf(stderr, "error : on line %d: %s\n", error_check.line, error_check.text);

				printf("strSUNAPIResult : %s\n", strSUNAPIResult.c_str());

				return false;
			}
			else
			{
#if 0 // original NVR
				json_t* json_interface = json_object_get(json_strRoot, "NetworkInterfaces");

				if (!json_interface)
				{
					printf("NG1 ... GetGatewayInfo() -> json_is wrong ... NetworkInterfaces !!!\n");

					printf("strSUNAPIResult : %s\n", strSUNAPIResult.c_str());

					return false;
				}

				int result, index;	
				char* charInterface;
				json_t* json_sub;

				json_array_foreach(json_interface, index, json_sub)
				{
					result = json_unpack(json_sub, "{s:s}", "InterfaceName", &charInterface);

					if (result)
					{
						printf("[hwanjang] Error !! GetGatewayInfo() -> 1. json_unpack fail .. Status ..index : %d !!!\n", index);
					}
					else
					{
						if (strncmp("Network1", charInterface, 7) == 0)
						{
							char* charMac, * charIPv4Address;

							//  - LinkStatus(<enum> Connected, Disconnected) , InterfaceName , MACAddress
							int ret = json_unpack(json_sub, "{s:s,s:s}", "IPv4Address", &charIPv4Address, "MACAddress", &charMac);

							if (ret)
							{
								printf("[hwanjang] Error !! GetGatewayInfo() -> json_unpack fail .. !!!\n");

								printf("strSUNAPIResult : %s\n", strSUNAPIResult.c_str());
							}
							else
							{
								g_Gateway_info_->IPv4Address = charIPv4Address;
								g_Gateway_info_->MACAddress = charMac;

								printf("GetGatewayInfo() -> strIPv4Address : %s , MACAddress : %s \n", charIPv4Address, charMac);

								return true;
							}
						}
						else
						{
							// other network ...
						}
					}
				}
#else  // Cloud Gateway
				json_t* json_interface = json_object_get(json_strRoot, "NetworkInterfaces");

				if (!json_interface)
				{
					printf("NG1 ... GetGatewayInfo() -> json_is wrong ... NetworkInterfaces !!!\n");

					printf("strSUNAPIResult : %s\n", strSUNAPIResult.c_str());

					return false;
				}

				int result, index;
				char* charInterface;
				json_t* json_sub;

				json_array_foreach(json_interface, index, json_sub)
				{
					result = json_unpack(json_sub, "{s:s}", "InterfaceName", &charInterface);

					if (result)
					{
						printf("[hwanjang] Error !! GetGatewayInfo() -> 1. json_unpack fail .. Status ..index : %d !!!\n", index);
					}
					else
					{
						if (strncmp("Network1", charInterface, 7) == 0)
						{
							char* charMac, * charIPv4Address;

							//  - LinkStatus(<enum> Connected, Disconnected) , InterfaceName , MACAddress
							int ret = json_unpack(json_sub, "{s:s}", "IPv4Address", &charIPv4Address);

							if (ret)
							{
								printf("[hwanjang] Error !! GetGatewayInfo() -> json_unpack fail ..  IPv4Address !!!\n");

								printf("strSUNAPIResult : %s\n", strSUNAPIResult.c_str());								
							}
							else
							{
								g_Gateway_info_->IPv4Address = charIPv4Address;								

								printf("GetGatewayInfo() -> strIPv4Address : %s \n", charIPv4Address);
							}

							ret = json_unpack(json_sub, "{s:s}", "MACAddress", &charMac);

							if (ret)
							{
								printf("[hwanjang] Error !! GetGatewayInfo() -> json_unpack fail ..  MACAddress !!!\n");

								printf("strSUNAPIResult : %s\n", strSUNAPIResult.c_str());
							}
							else
							{								
								g_Gateway_info_->MACAddress = charMac;

								printf("GetGatewayInfo() -> MACAddress : %s \n", charMac);
							}

							return true;
						}
						else
						{
							// other network ...
						}
					}
				}
#endif
			}
		}
	}
	else
	{
		// CURL_Process fail
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Reset
void sunapi_manager::ResetGatewayInfo()
{
	g_Gateway_info_->WebPort = 0;
	g_Gateway_info_->Model.clear();
	g_Gateway_info_->IPv4Address.clear();
	g_Gateway_info_->MACAddress.clear();
	g_Gateway_info_->FirmwareVersion.clear();
}

void sunapi_manager::ResetStorageInfos(int channel)
{
	// storage info
	g_Storage_info_[channel].update_check = false;
	g_Storage_info_[channel].das_presence = false;
	g_Storage_info_[channel].nas_presence = false;
	g_Storage_info_[channel].sdfail = false;
	g_Storage_info_[channel].nasfail = false;
	g_Storage_info_[channel].das_enable = false;
	g_Storage_info_[channel].nas_enable = false;
	g_Storage_info_[channel].das_status.clear();
	g_Storage_info_[channel].nas_status.clear();

	// worker
	g_Worker_Storage_info_[channel].update_check = false;
	g_Worker_Storage_info_[channel].das_presence = false;
	g_Worker_Storage_info_[channel].nas_presence = false;
	g_Worker_Storage_info_[channel].sdfail = false;
	g_Worker_Storage_info_[channel].nasfail = false;
	g_Worker_Storage_info_[channel].das_enable = false;
	g_Worker_Storage_info_[channel].nas_enable = false;
	g_Worker_Storage_info_[channel].das_status.clear();
	g_Worker_Storage_info_[channel].nas_status.clear();
}

void sunapi_manager::ResetSubdeviceInfos(int channel)
{
	// subdevice info
	g_SubDevice_info_[channel].update_check = false;
	g_SubDevice_info_[channel].IsBypassSupported = false;	
	g_SubDevice_info_[channel].ConnectionStatus = 0;
	g_SubDevice_info_[channel].HTTPPort = 0;
	g_SubDevice_info_[channel].ChannelStatus.clear();
	g_SubDevice_info_[channel].Model.clear();
	g_SubDevice_info_[channel].DeviceName.clear();
	g_SubDevice_info_[channel].IPv4Address.clear();
	g_SubDevice_info_[channel].LinkStatus.clear();
	g_SubDevice_info_[channel].MACAddress.clear();
	g_SubDevice_info_[channel].UserID.clear();

	// worker
	g_Worker_SubDevice_info_[channel].update_check = false;
	g_Worker_SubDevice_info_[channel].IsBypassSupported = false;
	g_Worker_SubDevice_info_[channel].ConnectionStatus = 0;
	g_Worker_SubDevice_info_[channel].HTTPPort = 0;
	g_Worker_SubDevice_info_[channel].ChannelStatus.clear();
	g_Worker_SubDevice_info_[channel].Model.clear();
	g_Worker_SubDevice_info_[channel].DeviceName.clear();
	g_Worker_SubDevice_info_[channel].IPv4Address.clear();
	g_Worker_SubDevice_info_[channel].LinkStatus.clear();
	g_Worker_SubDevice_info_[channel].MACAddress.clear();
	g_Worker_SubDevice_info_[channel].UserID.clear();
}


void sunapi_manager::ResetFirmwareVersionInfos(int channel)
{
	// subdevice info
	g_Firmware_Ver_info_[channel].update_check = false;
	g_Firmware_Ver_info_[channel].FirmwareVersion.clear();
	g_Firmware_Ver_info_[channel].LatestFirmwareVersion.clear();

	// worker
	g_Worker_Firmware_Ver_info_[channel].update_check = false;
	g_Worker_Firmware_Ver_info_[channel].FirmwareVersion.clear();
	g_Worker_Firmware_Ver_info_[channel].LatestFirmwareVersion.clear();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Update
void sunapi_manager::UpdateStorageInfos()
{
	//memcpy(g_Storage_info_, g_Worker_Storage_info_, sizeof(Storage_Infos) * (gMax_Channel + 1));

	for (int i = 0; i < gMax_Channel + 1; i++)
	{
		g_Storage_info_[i].das_presence = g_Worker_Storage_info_[i].das_presence;
		g_Storage_info_[i].nas_presence = g_Worker_Storage_info_[i].nas_presence;
		g_Storage_info_[i].sdfail = g_Worker_Storage_info_[i].sdfail;
		g_Storage_info_[i].nasfail = g_Worker_Storage_info_[i].nasfail;
		g_Storage_info_[i].das_enable = g_Worker_Storage_info_[i].das_enable;
		g_Storage_info_[i].nas_enable = g_Worker_Storage_info_[i].nas_enable;
		g_Storage_info_[i].das_status = g_Worker_Storage_info_[i].das_status;
		g_Storage_info_[i].nas_status = g_Worker_Storage_info_[i].nas_status;
	}
}

void sunapi_manager::UpdateSubdeviceInfos()
{
	//memcpy(g_SubDevice_info_, g_Worker_SubDevice_info_, sizeof(Channel_Infos) * (gMax_Channel + 1));

	for (int i = 0; i < gMax_Channel + 1; i++)
	{
		g_SubDevice_info_[i].ConnectionStatus = g_Worker_SubDevice_info_[i].ConnectionStatus;
		g_SubDevice_info_[i].IsBypassSupported = g_Worker_SubDevice_info_[i].IsBypassSupported;
		g_SubDevice_info_[i].HTTPPort = g_Worker_SubDevice_info_[i].HTTPPort;
		g_SubDevice_info_[i].ChannelStatus = g_Worker_SubDevice_info_[i].ChannelStatus;		
		g_SubDevice_info_[i].Model = g_Worker_SubDevice_info_[i].Model;
		g_SubDevice_info_[i].DeviceName = g_Worker_SubDevice_info_[i].DeviceName;
		g_SubDevice_info_[i].IPv4Address = g_Worker_SubDevice_info_[i].IPv4Address;
		g_SubDevice_info_[i].LinkStatus = g_Worker_SubDevice_info_[i].LinkStatus;
		g_SubDevice_info_[i].MACAddress = g_Worker_SubDevice_info_[i].MACAddress;
		g_SubDevice_info_[i].UserID = g_Worker_SubDevice_info_[i].UserID;
	}

	GetConnectionCount();
}

void sunapi_manager::UpdateFirmwareVersionInfos()
{
	// firmware version info
	for (int i = 0; i < gMax_Channel + 1; i++)
	{
		g_Firmware_Ver_info_[i].FirmwareVersion = g_Worker_Firmware_Ver_info_[i].FirmwareVersion;
		g_Firmware_Ver_info_[i].LatestFirmwareVersion = g_Worker_Firmware_Ver_info_[i].LatestFirmwareVersion;
	}
}

int sunapi_manager::GetDeviceIP_PW(std::string* strIP , std::string* strPW)
{
	// TODO: 여기에 구현 코드 추가.
	bool result = false;

	std::string sunapi_info_file = "config/camera.cfg";

	json_error_t j_error;
	json_t* json_out = json_load_file(sunapi_info_file.c_str(), 0, &j_error);
	if (j_error.line != -1)
	{
		printf("[hwanjang] Error !! get camera info !!!\n");
		printf("json_load_file returned an invalid line number -- > CFG file check !!\n");
	}
	else
	{
		char* SUNAPIServerIP, * devicePW;
		result = json_unpack(json_out, "{s:s, s:s}", "ip", &SUNAPIServerIP, "pw", &devicePW);

		if (result || !SUNAPIServerIP || !devicePW)
		{
			printf("[hwanjang] Error !! camera cfg is wrong -> retur false !!!\n");
		}
		else
		{
			*strIP = SUNAPIServerIP;
			*strPW = devicePW;

			result = true;
		}
	}
	json_decref(json_out);

	return result;
}


size_t sunapi_manager::WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	// TODO: 여기에 구현 코드 추가.
	size_t realsize = size * nmemb;
	ChunkStruct* mem = (ChunkStruct*)userp;

	mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);

	if (mem->memory != NULL)
	{
		memcpy(&(mem->memory[mem->size]), contents, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}
	else
	{
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	//printf("[hwanjang] WriteMemoryCallback() -> realsize : %d\n-> %s\n", realsize, mem->memory);

	return realsize;
}


#if 0  // old
CURLcode sunapi_manager::CURL_Process(bool json_mode, bool ssl_opt, std::string strRequset, std::string strPW, std::string* strResult)
#else  // add timeout option
CURLcode sunapi_manager::CURL_Process(bool json_mode, bool ssl_opt, int timeout, std::string strRequset, std::string strPW, std::string* strResult)
#endif
{
	// TODO: 여기에 구현 코드 추가.
	ChunkStruct chunk;

	chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */

	CURL* curl_handle;
	CURLcode res;

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();

#if 0  // 2019.09.05 hwanjang - sunapi debugging
	printf("curl_process() -> request : %s , pw : %s\n", strRequset.c_str(), strPW.c_str());
#endif

	if (curl_handle)
	{
#if 1
		struct curl_slist* headers = NULL;
		//headers = curl_slist_append(headers, "cache-control:no-cache");
		//headers = curl_slist_append(headers, "Content-Type: application/json");
		if(json_mode)
			headers = curl_slist_append(headers, "Accept: application/json");
		else
			headers = curl_slist_append(headers, "Accept: application/text");

		curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
#endif

		curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "GET");
		curl_easy_setopt(curl_handle, CURLOPT_URL, strRequset.c_str());
		//curl_easy_setopt(curl_handle, CURLOPT_PORT, 80L);
		curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST); // digest authentication
		curl_easy_setopt(curl_handle, CURLOPT_USERPWD, strPW.c_str());

		if (ssl_opt)
		{
			std::string ca_path = "config/ca-certificates.crt";
			curl_easy_setopt(curl_handle, CURLOPT_CAINFO, ca_path.c_str());
			curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1L);
		}
		else
		{
			curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
			curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
		}

		/* send all data to this function  */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

		/* we pass our 'chunk' struct to the callback function */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&chunk);
		//curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, CURL_TIMEOUT);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, CURL_CONNECTION_TIMEOUT);
		//curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

		res = curl_easy_perform(curl_handle);
		/* check for errors */
		if (res != CURLE_OK)
		{
			printf("curl_process() -> curl_easy_perform() failed .. request : %s, code : %d,  %s\n", strRequset.c_str(), res, curl_easy_strerror(res));
		}
		else 
		{
			/*
			* Now, our chunk.memory points to a memory block that is chunk.size
			* bytes big and contains the remote file.
			*/
			long response_code;
			curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
			//printf("response code : %d\n", res);
			if ((response_code != 200) || ((long)chunk.size == 0)) // 200 ok , 301 moved , 404 error
			{
				// default channel
				res = CURLE_CHUNK_FAILED;
			}
			else
			{
				// OK
				*strResult = chunk.memory;
			}
		}
		curl_easy_cleanup(curl_handle);
	}
	else
	{
		printf("error ... initialize cURL !!!\n");
		res = CURLE_FAILED_INIT;
	}
	free(chunk.memory);

	/* we're done with libcurl, so clean it up */
	curl_global_cleanup();	

	return res;
}

bool sunapi_manager::ByPassSUNAPI(int channel, bool json_mode, const std::string IPAddress, const std::string devicePW, const std::string bypassURI, std::string* strResult, CURLcode* resCode)
{
	if (g_SubDevice_info_[channel].IsBypassSupported != true)
	{
		printf("sunapi_manager::ByPassSUNAPI() -> channel : %d, IsBypassSupported ... false --> return !!!\n");
		return false;
	}

	// TODO: 여기에 구현 코드 추가.

	std::string request;

	request = "http://";
	request.append(IPAddress);
	request.append("/stw-cgi/bypass.cgi?msubmenu=bypass&action=control&Channel=");
	request.append(std::to_string(channel));
	request.append("&BypassURI=");
	request.append(bypassURI);

	//printf("private ID:PW = %s\n",userpw.c_str());

#if 0  // 2019.09.05 hwanjang - sunapi debugging
	printf("sunapi_manager::ByPassSUNAPI() -> SUNAPI Request : %s\n", request.c_str());
#endif

	CURLcode res;
	std::string strSUNAPIResult;
	res = CURL_Process(json_mode, false, CURL_TIMEOUT, request, devicePW, &strSUNAPIResult);

	if (res == CURLE_OK)
	{
		if (strSUNAPIResult.empty())
		{
			printf("[hwanjang] ByPassSUNAPI() -> result string is empty !!!\n");
			return false;
		}

		*strResult = strSUNAPIResult;
	}
	else
	{
		printf("failed  ... ByPassSUNAPI() -> curl failed  .. request URL : %s\n", request.c_str());
		return false;
	}

	return true;
}


int sunapi_manager::GetMaxChannelByAttribute(std::string strID_PW)
{
	// TODO: 여기에 구현 코드 추가.
	int max_channel = 0;

	std::string request;

	request = "http://";
	request.append(gStrDeviceIP);
	//request.append("/stw-cgi/attributes.cgi/attributes/system/limit/MaxChannel");
	request.append("/stw-cgi/attributes.cgi");

	//printf("private ID:PW = %s\n",userpw.c_str());

#if 1  // 2019.09.05 hwanjang - sunapi debugging
	printf("sunapi_manager::GetMaxChannelByAttribute() -> SUNAPI Request : %s\n", request.c_str());
#endif

	CURLcode res;
	std::string strSUNAPIResult;
	bool json_mode = true;
	bool ssl_opt = false;

	res = CURL_Process(json_mode, ssl_opt, CURL_TIMEOUT, request, strID_PW, &strSUNAPIResult);

	if (res == CURLE_OK)
	{
		if (strSUNAPIResult.empty())
		{
			printf("[hwanjang] Attributes string is empty !!!\n");
			return 0;
		}
		else
		{
			// attributes.cgi -> The following CGI command returns device attributes and CGI information in XML format.
			//printf("GetMaxChannelByAttribute() -> strSUNAPIResult : %s\n", strSUNAPIResult.c_str());

			size_t max_index;
			// 2018.02.05 hwanjang - string check
			if ((max_index = strSUNAPIResult.find("MaxChannel")) == std::string::npos)
			{
				printf("[hwanjang] The string does not include MaxChannel !!!\n");
				return 0;
			}

			size_t value_index = strSUNAPIResult.find("value=", max_index);
			size_t ch_index = strSUNAPIResult.find("\"", value_index + 8);
			std::string ch_str = strSUNAPIResult.substr(value_index + 7, (ch_index - (value_index + 7)));

			max_channel = atoi(ch_str.c_str());

			if (max_channel == 0)
			{
				printf("GetMaxChannelByAttribute() -> max channel : %d\n", max_channel);
				printf("data : %s\n", strSUNAPIResult.c_str());
			}
		}
	}
	else
	{
		printf("error ... initialize cURL !!!\n");
		return 0;
	}

	gMax_Channel = max_channel;

	return max_channel;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// using SUNAPI Command Function
int sunapi_manager::GetMaxChannel()
{
	// TODO: 여기에 구현 코드 추가.
	return gMax_Channel;
}

int sunapi_manager::GetConnectionCount()
{
	// TODO: 여기에 구현 코드 추가.
	return g_ConnectionCnt;
}



/////////////////////////////////////////////////////////////////////////////////////////////
// 1. dashboard view

void sunapi_manager::GetDASPresenceOfSubdevice(int channel, const std::string deviceIP, const std::string devicePW)
{
	std::string strByPassURI = "/stw-cgi/eventstatus.cgi?msubmenu=eventstatus&action=check&SystemEvent=SDInsert";
	bool result = false;

	std::string strByPassResult;

	bool json_mode = false;
	CURLcode resCode;

	result = ByPassSUNAPI(channel, json_mode, deviceIP, devicePW, strByPassURI, &strByPassResult, &resCode);

	if (result)
	{		
		if (strByPassResult.empty())
		{
			// SD card none
		}
		else
		{
			printf("strByPassResult : %s\n", strByPassResult.c_str());
			if ((strByPassResult.find("True")) || (strByPassResult.find("true")))
			{
				// SD card (DAS insert = True)
				g_Worker_Storage_info_[channel].das_presence = true;
			}
			else
			{
				// SD card none
				g_Worker_Storage_info_[channel].das_presence = false;
			}
		}
	}
	else
	{
		//
		if (resCode == CURLE_OPERATION_TIMEDOUT)
		{
			// timeout
		}
	}
}

void sunapi_manager::GetNASPresenceOfSubdevice(int channel, const std::string deviceIP, const std::string devicePW)
{
	std::string strByPassURI = "/stw-cgi/eventstatus.cgi?msubmenu=eventstatus&action=check&SystemEvent=NASConnect";
	bool result = false;

	std::string strByPassResult;

	bool json_mode = false;
	CURLcode resCode;
	result = ByPassSUNAPI(channel, json_mode, deviceIP, devicePW, strByPassURI, &strByPassResult, &resCode);
		
	if (result)
	{		
		if (strByPassResult.empty())
		{
			// fail
		}
		else
		{
			printf("strByPassResult : %s\n", strByPassResult.c_str());
			if ((strByPassResult.find("True")) || (strByPassResult.find("true")))
			{
				// SystemEvent.NASConnect=True
				g_Worker_Storage_info_[channel].nas_presence = true;
			}
			else
			{
				// SystemEvent.NASConnect=False
				g_Worker_Storage_info_[channel].nas_presence = false;
			}
		}
	}
	else
	{
		//
		if (resCode == CURLE_OPERATION_TIMEDOUT)
		{
			// timeout
		}
	}
}

void sunapi_manager::GetStoragePresenceOfSubdevices()
{
	// TODO: 여기에 구현 코드 추가.

	g_UpdateTimeOfStorageStatus = time(NULL);

	int max_ch = gMax_Channel;

	std::string deviceIP = gStrDeviceIP;
	std::string devicePW = gStrDevicePW;

	// for test
	//max_ch = 10; 

	for (int i = 0; i < max_ch; i++)
	{
		//sleep_for(std::chrono::milliseconds(10));

		printf("GetStoragePresenceOfSubdevices() -> index : %d ...\n", i);
		
		ThreadStartForStoragePresence(i, deviceIP, devicePW);

		sleep_for(std::chrono::milliseconds(10));
	}
}

int sunapi_manager::ThreadStartForStoragePresence(int channel, const std::string deviceIP, const std::string devicePW)
{
	std::thread thread_function([=] { thread_function_for_storage_presence(channel, deviceIP, devicePW); });
	thread_function.detach();

	return 0;
}

void sunapi_manager::thread_function_for_storage_presence(int channel, const std::string deviceIP, const std::string devicePW)
{
	std::string strByPassURI = "/stw-cgi/eventstatus.cgi?msubmenu=eventstatus&action=check";
	bool result = false;

	std::string strByPassResult;

	bool json_mode = true;
	CURLcode resCode;
	result = ByPassSUNAPI(channel, json_mode, deviceIP, devicePW, strByPassURI, &strByPassResult, &resCode);

	if (result)
	{
		if (strByPassResult.empty())
		{
			// fail
			printf("channel : %d, strByPassResult is empty !!! \n", channel);
		}
		else
		{
			if (strByPassResult.find("System"))
			{
				json_error_t error_check;
				json_t* json_strRoot = json_loads(strByPassResult.c_str(), 0, &error_check);

				if (!json_strRoot)
				{
					fprintf(stderr, "error : root\n");
					fprintf(stderr, "error : on line %d: %s\n", error_check.line, error_check.text);

					printf("channel : %d, strByPassResult : %s\n", channel, strByPassResult.c_str());
				}
				else
				{
					int ret = 0;
					json_t* json_system = json_object_get(json_strRoot, "SystemEvent");

					if (!json_system)
					{
						printf("channel : %d, cannot find SystemEvent ... strByPassResult : %s\n", channel, strByPassResult.c_str());
					}
					else
					{
						int sdcard, sdfail, nas, nasfail;

						// Check DAS					
						ret = json_unpack(json_system, "{s:b, s:b}", "SDInsert", &sdcard, "SDFail", &sdfail);

						if (ret)
						{
							printf("[hwanjang] Error !! GetStorageCheckOfSubdeivce() -> channel : %d , json_unpack fail .. DAS !!!\n", channel);

							printf("strByPassResult : %s\n", strByPassResult.c_str());
						}
						else
						{
							//printf("GetStorageCheckOfSubdeivce() -> channel : %d, sdcard : %d , sdfail : %d\n",	channel, sdcard, sdfail);

							g_Worker_Storage_info_[channel].das_presence = sdcard;
							g_Worker_Storage_info_[channel].sdfail = sdfail;
						}

						// Check NAS
						ret = json_unpack(json_system, "{s:b, s:b}", "NASConnect", &nas, "NASFail", &nasfail);

						if (ret)
						{
							printf("[hwanjang] Error !! GetStorageCheckOfSubdeivce() -> channel : %d , json_unpack fail .. NAS !!!\n", channel);

							printf("strByPassResult : %s\n", strByPassResult.c_str());
						}
						else
						{
							//printf("GetStorageCheckOfSubdeivce() -> channel : %d, nas : %d , nasfail : %d\n", channel, nas, nasfail);

							g_Worker_Storage_info_[channel].nas_presence = nas;
							g_Worker_Storage_info_[channel].nasfail = nasfail;
						}
					}
				}
			}
			else
			{
				printf("Error ... channel : %d, not found System ...\n%s\n", channel, strByPassResult.c_str());
			}
		}
	}
	else
	{
		//
		if (resCode == CURLE_OPERATION_TIMEDOUT)
		{
			// timeout
		}
	}
}

void sunapi_manager::GetStorageStatusOfSubdevices()
{
	// TODO: 여기에 구현 코드 추가.

	g_UpdateTimeOfStorageStatus = time(NULL);

	std::cout << "GetStorageStatusOfSubdevices() -> Start ... g_UpdateTimeOfStorageStatus : " << g_UpdateTimeOfStorageStatus << std::endl;

	int max_ch = gMax_Channel;

	std::string deviceIP = gStrDeviceIP;
	std::string devicePW = gStrDevicePW;

	// for test
	//max_ch = 10; 

	for (int i = 0; i < max_ch; i++)
	{
		//sleep_for(std::chrono::milliseconds(10));
#if 1
		if (g_SubDevice_info_[i].ConnectionStatus == 1)  // only connection status
		{
			ThreadStartForStorageStatus(i, deviceIP, devicePW);
			
			sleep_for(std::chrono::milliseconds(1));
		}
#else
		//printf("GetStorageStatusOfSubdevices() -> index : %d ...\n", i);
		if( (g_Worker_Storage_info_[i].das_presence) || (g_Worker_Storage_info_[i].nas_presence) )
		{
			ThreadStartForStorageStatus(i, deviceIP, devicePW);
		}
		else
		{
			//printf("channel : %d , DAS Recording : %d , NAS Recording : %d ... skip !!\n", i, g_Worker_Storage_info_[i].das_enable , g_Worker_Storage_info_[i].nas_enable);
		}
#endif
	}
}

int sunapi_manager::ThreadStartForStorageStatus(int channel, const std::string deviceIP, const std::string devicePW)
{
	std::thread thread_function([=] { thread_function_for_storage_status(channel, deviceIP, devicePW); });
	thread_function.detach();

	return 0;
}

void sunapi_manager::thread_function_for_storage_status(int channel, const std::string deviceIP, const std::string devicePW)
{
	printf("thread_function_for_storage_status() -> channel : %d , Start to get storageinfo ...\n", channel);

	bool result = false;

	std::string strByPassResult;

#if 1  // ByPass - Cloud Gateway
	std::string strByPassURI = "/stw-cgi/system.cgi?msubmenu=storageinfo&action=view";

	bool json_mode = true;
	CURLcode resCode;
	result = ByPassSUNAPI(channel, json_mode, deviceIP, devicePW, strByPassURI, &strByPassResult, &resCode);
#else  // to each camera
	std::string request;
	std::string ipAddress = g_SubDevice_info_[channel].IPv4Address;
	int port = g_SubDevice_info_[channel].HTTPPort;

	request = "http://";
	request.append(ipAddress);
	request.append(":");
	request.append(std::to_string(port));
	request.append("/stw-cgi/system.cgi?msubmenu=storageinfo&action=view");

	bool json_mode = true;
	CURLcode res;
	std::string strSUNAPIResult;
	res = CURL_Process(json_mode, false, CURL_TIMEOUT, request, devicePW, &strSUNAPIResult);
#endif

	if (result)
	{
		// 
		if (strByPassResult.empty())
		{
			// fail
			printf("channel : %d, strByPassResult is empty !!! \n", channel);
		}
		else
		{
			json_error_t error_check;
			json_t* json_strRoot = json_loads(strByPassResult.c_str(), 0, &error_check);

			if (!json_strRoot)
			{
				fprintf(stderr, "error : root\n");
				fprintf(stderr, "error : on line %d: %s\n", error_check.line, error_check.text);

				printf("strByPassResult : %s\n", strByPassResult.c_str());
			}
			else
			{
				int ret;

				json_t* json_array = json_object_get(json_strRoot, "Storages");

				if (!json_array)
				{
					printf("NG1 ... thread_function_for_storage_status() -> channel : %d, json_is wrong .. strByPassResult : \n%s\n",
						channel, strByPassResult.c_str());
				}
				else
				{
					int ret, index;
					int bEnable;
					char* charType, * charStatus;
					json_t* obj;

					json_array_foreach(json_array, index, obj)
					{
						//  - Type(<enum> DAS, NAS) , Enable , Satus
						ret = json_unpack(obj, "{s:s, s:b, s:s}",
							"Type", &charType, "Enable", &bEnable, "Status", &charStatus);

						if (ret)
						{
							printf("[hwanjang] Error !! thread_function_for_storage_status() -> channel : %d, json_unpack fail .. !!!\n", channel);
							printf("strByPassResult : %s\n", strByPassResult.c_str());
						}
						else
						{
							//printf("thread_function_for_storage_status() -> channel : %d, type : %s , bEnable : %d\n",	channel, charType, bEnable);

							if (strncmp("DAS", charType, 3) == 0)
							{
								g_Worker_Storage_info_[channel].das_enable = bEnable;

								if (bEnable)
								{
									g_Worker_Storage_info_[channel].das_status = charStatus;
								}
								else
								{
									g_Worker_Storage_info_[channel].das_status.empty();
								}
							}
							else if (strncmp("NAS", charType, 3) == 0)
							{
								g_Worker_Storage_info_[channel].nas_enable = bEnable;

								if (bEnable)
								{
									g_Worker_Storage_info_[channel].nas_status = charStatus;
								}
								else
								{
									g_Worker_Storage_info_[channel].nas_status.empty();
								}
							}
							else
							{
								// ??? unknown

							}
						}
					}
				}
			}
		}
	}
	else
	{

	}

	g_Worker_Storage_info_[channel].update_check = true;

#if 0
	g_Mtx_lock_storage.lock();

	g_StorageCheckCnt++;
	printf("thread_function_for_storage_status() -> channel : %d , gStorageCheckCnt : %d ... UpdateStorageInfos() ...\n", channel, g_StorageCheckCnt);
	if (g_StorageCheckCnt >= g_RegisteredCameraCnt)
	//if (g_StorageCheckCnt >= gMax_Channel)
	{
		printf("\n##############################################################################################\n");
		printf("thread_function_for_storage_status() -> gStorageCheckCnt : %d ... UpdateStorageInfos() ...\n", g_StorageCheckCnt);
		UpdateStorageInfos();		

		SendResponseForDashboardView(strTopic, strTid);

		g_StorageCheckCnt = 0;
	}
	g_Mtx_lock_storage.unlock();
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Send storage info for dashboard 

void sunapi_manager::SendResponseForDashboardView(const std::string& strTopic, std::string strTid)
{
	std::string strCommand = "dashboard";
	std::string strType = "view";
	std::string strView = "detail";
	std::string strVersion = "1.5";
	// sub of message
	std::string strDeviceType = "gateway";

	json_t* main_ResponseMsg, *sub_Msg, *sub_SubdeviceMsg, *sub_ConnectionMsg, *sub_StorageMsg;

	main_ResponseMsg = json_object();
	sub_Msg = json_object();
	sub_SubdeviceMsg = json_object();
	sub_ConnectionMsg = json_object();
	sub_StorageMsg = json_object();

	//main_Msg["command"] = "dashboard";
	json_object_set(main_ResponseMsg, "command", json_string(strCommand.c_str()));
	//main_Msg["type"] = "view";
	json_object_set(main_ResponseMsg, "type", json_string(strType.c_str()));
	//main_Msg["view"] = "detail";
	json_object_set(main_ResponseMsg, "view", json_string(strView.c_str()));

	// connection of subdevice
	int i = 0, connectionMsgCnt=0, storageMsgCnt = 0;
	char charCh[8];

	for (i = 0; i < gMax_Channel; i++)
	{
		if (g_SubDevice_info_[i].ConnectionStatus != 0)
		{
			// connection of subdevice
			if (g_SubDevice_info_[i].ChannelStatus.empty() != true)
			{
				memset(charCh, 0, sizeof(charCh));
				sprintf(charCh, "%d", i);  // int -> char*
				json_object_set_new(sub_ConnectionMsg, charCh, json_string(g_SubDevice_info_[i].ChannelStatus.c_str()));
				connectionMsgCnt++;
			}

			// storage of subdevice
			// DAS
			if (g_Storage_info_[i].das_enable)
			{				
				if (g_Storage_info_[i].das_status.empty() != true)
				{
					memset(charCh, 0, sizeof(charCh));
					sprintf(charCh, "%d.DAS", i);  // int -> char*
					json_object_set_new(sub_StorageMsg, charCh, json_string(g_Storage_info_[i].das_status.c_str()));
					storageMsgCnt++;
				}
			}

			// NAS
			if (g_Storage_info_[i].nas_enable)
			{
				if (g_Storage_info_[i].nas_status.empty() != true)
				{
					memset(charCh, 0, sizeof(charCh));
					sprintf(charCh, "%d.NAS", i);  // int -> char*
					json_object_set_new(sub_StorageMsg, charCh, json_string(g_Storage_info_[i].nas_status.c_str()));
					storageMsgCnt++;
				}
			}
		}
	}

	//sub_SubdeviceMsg["connection"] = sub_ConnectionMsg;
	//sub_SubdeviceMsg["storage"] = sub_StorageMsg;
	if (connectionMsgCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "connection", sub_ConnectionMsg);
	}

	if (storageMsgCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "storage", sub_StorageMsg);
	}

	// sub of message
	json_object_set(sub_Msg, "deviceId", json_string(g_Device_id.c_str()));
	json_object_set(sub_Msg, "deviceType", json_string(strDeviceType.c_str()));
	json_object_set(sub_Msg, "ipaddress", json_string(g_Gateway_info_->IPv4Address.c_str()));
	json_object_set(sub_Msg, "webport", json_integer(g_Gateway_info_->WebPort));
	json_object_set(sub_Msg, "macaddress", json_string(g_Gateway_info_->MACAddress.c_str()));

	json_object_set(sub_Msg, "subdevice", sub_SubdeviceMsg);

	//main_Msg["message"] = sub_Msg;
	json_object_set(main_ResponseMsg, "message", sub_Msg);

	//main_Msg["tid"] = strTid;
	json_object_set(main_ResponseMsg, "tid", json_string(strTid.c_str()));
	//main_Msg["version"] = "1.5";
	json_object_set(main_ResponseMsg, "version", json_string(strVersion.c_str()));

	//std::string strMQTTMsg = writer.write(mqtt_Msg);		
	std::string strMQTTMsg = json_dumps(main_ResponseMsg, 0);

	printf("SendStorageInfoForDashboardView() -> strMQTTMsg size : %lu\n%s\n", strMQTTMsg.size(), strMQTTMsg.c_str());

	observerForHbirdManager->SendResponseForDashboard(strTopic, strMQTTMsg);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// 2. deviceinfo view

bool sunapi_manager::GetRegiesteredCameraStatus(const std::string deviceIP, const std::string devicePW)
{
	// TODO: 여기에 구현 코드 추가.

	g_UpdateTimeOfRegistered = time(NULL);

	std::cout << "GetRegiesteredCameraStatus() -> Start ... time : " << (long int)g_UpdateTimeOfRegistered << std::endl;

	g_ConnectionCnt = 0;
	g_RegisteredCameraCnt = 0;

	std::string request;

	request = "http://";
	request.append(deviceIP);
	request.append("/stw-cgi/media.cgi?msubmenu=cameraregister&action=view");

	//printf("private ID:PW = %s\n",userpw.c_str());

#if 1  // 2019.09.05 hwanjang - sunapi debugging
	printf("sunapi_manager::GetRegiesteredCameraStatus() -> SUNAPI Request : %s\n", request.c_str());
#endif

	CURLcode res;
	std::string strSUNAPIResult;
	bool json_mode = true;
	bool ssl_opt = false;

	res = CURL_Process(json_mode, ssl_opt, CURL_TIMEOUT, request, devicePW, &strSUNAPIResult);

	if (res == CURLE_OK)
	{
		time_t end_time = time(NULL);
		std::cout << "GetRegiesteredCameraStatus() -> CURLE_OK !! Received data , time : " << (long int)end_time << " , diff : "
			<< (long int)(end_time - g_UpdateTimeOfRegistered) << std::endl;

		if (strSUNAPIResult.empty())
		{
			printf("[hwanjang] GetRegiesteredCameraStatus() result string is empty !!!\n");
			return false;
		}

		json_error_t error_check;
		json_t* json_strRoot = json_loads(strSUNAPIResult.c_str(), 0, &error_check);

		if (!json_strRoot)
		{
			fprintf(stderr, "error : root\n");
			fprintf(stderr, "error : on line %d: %s\n", error_check.line, error_check.text);

			printf("strSUNAPIResult : \n%s\n", strSUNAPIResult.c_str());

			return false;
		}

		json_t* json_array = json_object_get(json_strRoot, "RegisteredCameras");

		if (!json_is_array(json_array))
		{
			printf("NG1 ... GetRegiesteredCameraStatus() -> json_is_array fail !!!\n");
			return false;
		}

		int result;
		int i = 0, j = 0, index;
		const char* charStatus, * charModel, * charUserID;
		bool* IsBypassSupported;
		json_t* obj, * array, * element;

		int webPort;

		json_array_foreach(json_array, index, obj) {
			result = json_unpack(obj, "{s:s}", "Status", &charStatus);

			if (result)
			{
				printf("[hwanjang] Error !! GetRegiesteredCameraStatus -> 1. json_unpack fail .. Status ..index : %d !!!\n", index);
				std::cout << "obj string : " << std::endl << json_dumps(obj, 0) << std::endl;
			}
			else
			{
				if (strncmp("Success", charStatus, 7) == 0)
				{
					g_ConnectionCnt++;
					g_RegisteredCameraCnt++;

					g_Worker_SubDevice_info_[index].ConnectionStatus = 1;  // Success -> Connect

					g_Worker_SubDevice_info_[index].ChannelStatus = "on";

					printf("GetRegiesteredCameraStatus() -> channel : %d , connect ... g_RegisteredCameraCnt : %d\n", index, g_RegisteredCameraCnt);
				}
				else if (strncmp("ConnectFail", charStatus, 11) == 0)
				{
					g_RegisteredCameraCnt++;

					g_Worker_SubDevice_info_[index].ConnectionStatus = 2;  // ConnectFail

					g_Worker_SubDevice_info_[index].ChannelStatus = "fail";

					printf("GetRegiesteredCameraStatus() -> channel : %d , ConnectFail ... g_RegisteredCameraCnt : %d\n", index, g_RegisteredCameraCnt);
				}
				else
				{
					//printf("[hwanjang] GetRegiesteredCameraStatus -> index : %d , disconnected ... \n", index);

					ResetSubdeviceInfos(index);
				}

				if (g_Worker_SubDevice_info_[index].ConnectionStatus != 0)  // Success || ConnectFail
				{
					//g_Worker_SubDevice_info_[index].ChannelStatus = charStatus;

					webPort = -1;
					//result = json_unpack(obj, "{s:i, s:s, s:s}", "HTTPPort", &webPort, "Model", &charModel, "UserID", &charUserID);
					result = json_unpack(obj, "{s:i, s:s, s:b}", "HTTPPort", &webPort, "Model", &charModel, "IsBypassSupported", &IsBypassSupported);
					if (result)
					{
						printf("[hwanjang] Error !! GetRegiesteredCameraStatus -> 2. json_unpack fail .. index : %d !!!\n", index);
						std::cout << "obj string : " << std::endl << json_dumps(obj, 0) << std::endl;

						result = json_unpack(obj, "{s:i}", "HTTPPort", &webPort);
						if (result)
						{
							printf("[hwanjang] Error !! GetRegiesteredCameraStatus -> 2. json_unpack fail .. index : %d , HTTPPort\n", index);
							g_Worker_SubDevice_info_[index].HTTPPort = 0;
						}
						else
						{
							g_Worker_SubDevice_info_[index].HTTPPort = webPort;
						}

						result = json_unpack(obj, "{s:s}", "Model", &charModel);
						if (result)
						{
							printf("[hwanjang] Error !! GetRegiesteredCameraStatus -> 2. json_unpack fail .. index : %d , Model\n", index);
							g_Worker_SubDevice_info_[index].Model.clear();
						}
						else
						{
							g_Worker_SubDevice_info_[index].Model = charModel;
						}

						result = json_unpack(obj, "{s:b}", "IsBypassSupported", &IsBypassSupported);
						if (result)
						{
							printf("[hwanjang] Error !! GetRegiesteredCameraStatus -> 2. json_unpack fail .. index : %d , IsBypassSupported\n", index);
						}
						else
						{
							g_Worker_SubDevice_info_[index].IsBypassSupported = IsBypassSupported;
						}
					}
					else
					{
						g_Worker_SubDevice_info_[index].IsBypassSupported = IsBypassSupported;
						g_Worker_SubDevice_info_[index].HTTPPort = webPort;
						g_Worker_SubDevice_info_[index].Model = charModel;
					}
				}			
			}
		}
	}
	else
	{
		printf("GetRegiesteredCameraStatus() .... failed !!!\n");

		time_t end_time = time(NULL);
		std::cout << "GetRegiesteredCameraStatus() -> End ... time : " << (long int)end_time << " , diff : "
			<< (long int)(end_time - g_UpdateTimeOfRegistered) << std::endl;


		return false;
	}

	UpdateSubdeviceInfos();

	time_t end_time = time(NULL);
	std::cout << "GetRegiesteredCameraStatus() -> End ... time : " << (long int)end_time << " , diff : " 
					<< (long int)(end_time - g_UpdateTimeOfRegistered) << std::endl;

	return true;
}

#if 1
bool sunapi_manager::GetDeviceNameOfSubdevices()
{
	g_Sub_device_info_Cnt = 0;

	//  - LinkStatus , IPv4Address , MACAddress

	g_UpdateTimeOfDeviceInfo = time(NULL);

	std::cout << "GetDeviceNameOfSubdevices() -> Start ... time : " << g_UpdateTimeOfDeviceInfo << std::endl;

	int max_ch = gMax_Channel;

	std::string deviceIP = gStrDeviceIP;
	std::string devicePW = gStrDevicePW;

	// for test
	//max_ch = 10; 

	for (int i = 0; i < max_ch; i++)
	{
		if (g_Worker_SubDevice_info_[i].ConnectionStatus != 0)
		{
			ThreadStartForSubDeviceInfo(i, deviceIP, devicePW);

			sleep_for(std::chrono::milliseconds(1));
		}
	}

	return true;
}

int sunapi_manager::ThreadStartForSubDeviceInfo(int channel, const std::string deviceIP, const std::string devicePW)
{
	std::thread thread_function([=] { thread_function_for_subdevice_info(channel, deviceIP, devicePW); });
	thread_function.detach();

	return 0;
}

void sunapi_manager::thread_function_for_subdevice_info(int channel, const std::string deviceIP, const std::string devicePW)
{
	printf("thread_function_for_subdevice_info() -> channel : %d , Start to get subdevice info ...\n", channel);

	bool result = false;

	std::string strByPassResult;

	std::string strByPassURI = "/stw-cgi/system.cgi?msubmenu=deviceinfo&action=view";

	bool json_mode = true;
	CURLcode resCode;
	result = ByPassSUNAPI(channel, json_mode, deviceIP, devicePW, strByPassURI, &strByPassResult, &resCode);

	if (result)
	{
		if (strByPassResult.empty())
		{
			printf("[hwanjang] Attributes string is empty !!!\n");
			return ;
		}
		else
		{
			json_error_t error_check;
			json_t* json_strRoot = json_loads(strByPassResult.c_str(), 0, &error_check);

			if (!json_strRoot)
			{
				fprintf(stderr, "error : root\n");
				fprintf(stderr, "error : on line %d: %s\n", error_check.line, error_check.text);

				printf("GetDeviceInfoOfGateway() -> strSUNAPIResult : \n%s\n", strByPassResult.c_str());
			}
			else
			{
				// get Model
				char* charModel;
				int result = json_unpack(json_strRoot, "{s:s}", "Model", &charModel);

				if (result)
				{
					printf("[hwanjang] Error !! GetDeviceInfoOfGateway() -> 1. json_unpack fail .. Model ...\n");
					printf("GetDeviceInfoOfGateway() -> strSUNAPIResult : \n%s\n", strByPassResult.c_str());
				}
				else
				{
					g_Worker_SubDevice_info_[channel].Model = charModel;
				}

				// get FirmwareVersion
				char* charFirmwareVersion;
				result = json_unpack(json_strRoot, "{s:s}", "FirmwareVersion", &charFirmwareVersion);

				if (result)
				{
					printf("[hwanjang] Error !! GetDeviceInfoOfGateway() -> 2. json_unpack fail .. FirmwareVersion ...\n");
					printf("GetDeviceInfoOfGateway() -> strSUNAPIResult : \n%s\n", strByPassResult.c_str());
				}
				else
				{
					g_Worker_SubDevice_info_[channel].FirmwareVersion = charFirmwareVersion;
				}

#if 0
				// get MACAddress
				char* charMACAddress;
				result = json_unpack(json_strRoot, "{s:s}", "ConnectedMACAddress", &charMACAddress);

				if (result)
				{
					printf("[hwanjang] Error !! GetDeviceInfoOfGateway() -> 3. json_unpack fail .. ConnectedMACAddress ...\n");
					printf("GetDeviceInfoOfGateway() -> strSUNAPIResult : \n%s\n", strByPassResult.c_str());
				}
				else
				{					
					g_Worker_SubDevice_info_[channel].MACAddress = charMACAddress;
				}
#endif

				// get DeviceName
				char* charDeviceName;
				result = json_unpack(json_strRoot, "{s:s}", "DeviceName", &charDeviceName);

				if (result)
				{
					printf("[hwanjang] Error !! GetDeviceInfoOfGateway() -> 4. json_unpack fail .. DeviceName ...\n");
					printf("GetDeviceInfoOfGateway() -> strSUNAPIResult : \n%s\n", strByPassResult.c_str());
				}
				else
				{
					g_Worker_SubDevice_info_[channel].DeviceName = charDeviceName;
				}
			}
		}
	}
	else
	{
		printf("error ... initialize cURL !!!\n");
	}
	g_Worker_SubDevice_info_[channel].update_check = true;

#if 0
	std::lock_guard<std::mutex> mtx_guard(g_Mtx_lock_device);

	g_Sub_network_info_Cnt++;

	if (g_Sub_network_info_Cnt >= g_RegisteredCameraCnt)
	{
		UpdateSubdeviceInfos();

		SendResponseForDeviceInfoView(strTopic, strTid);

		g_Sub_network_info_Cnt = 0;
	}
#endif
}
#endif

bool sunapi_manager::GetMacAddressOfSubdevices()
{
	g_Sub_network_info_Cnt = 0;

	//  - LinkStatus , IPv4Address , MACAddress
	
	g_UpdateTimeOfNetworkInterface = time(NULL);

	std::cout << "GetMacAddressOfSubdevices() -> Start ... time : " << g_UpdateTimeOfNetworkInterface << std::endl;

	int max_ch = gMax_Channel;

	std::string deviceIP = gStrDeviceIP;
	std::string devicePW = gStrDevicePW;

	// for test
	//max_ch = 10; 

	for (int i = 0; i < max_ch; i++)
	{				
		if(g_Worker_SubDevice_info_[i].ConnectionStatus != 0)
		{
			ThreadStartForNetworkInterface(i, deviceIP, devicePW);

			sleep_for(std::chrono::milliseconds(1));
		}
	}

	return true;
}


int sunapi_manager::ThreadStartForNetworkInterface(int channel, const std::string deviceIP, const std::string devicePW)
{
	std::thread thread_function([=] { thread_function_for_network_interface(channel, deviceIP, devicePW); });
	thread_function.detach();

	return 0;
}

void sunapi_manager::thread_function_for_network_interface(int channel, const std::string deviceIP, const std::string devicePW)
{	
	printf("thread_function_for_network_interface() -> channel : %d , Start to get NetworkInterface ...\n", channel);

	bool result = false;

	std::string strByPassResult;
	
	std::string strByPassURI = "/stw-cgi/network.cgi?msubmenu=interface&action=view";

	bool json_mode = true;
	CURLcode resCode;
	result = ByPassSUNAPI(channel, json_mode, deviceIP, devicePW, strByPassURI, &strByPassResult, &resCode);

	if (result)
	{
		if (!strByPassResult.empty())
		{
			json_error_t error_check;
			json_t* json_strRoot = json_loads(strByPassResult.c_str(), 0, &error_check);

			if (!json_strRoot)
			{
				fprintf(stderr, "error : root\n");
				fprintf(stderr, "error : on line %d: %s\n", error_check.line, error_check.text);

				printf("channel : %d , strByPassResult : %s\n", channel, strByPassResult.c_str());
			}
			else
			{
				json_t* json_array = json_object_get(json_strRoot, "NetworkInterfaces");

				if (!json_array)
				{
					printf("NG1 ... thread_function_for_network_interface() -> json_is wrong !!!\n");

					printf("channel : %d , strByPassResult : %s\n", channel, strByPassResult.c_str());
				}

				int ret, index;
				char* charLink, *charMac, *charIPv4Address;
				json_t* obj;

				json_array_foreach(json_array, index, obj) {

					//  - LinkStatus(<enum> Connected, Disconnected) , InterfaceName , MACAddress
					ret = json_unpack(obj, "{s:s, s:s, s:s}",
						"LinkStatus", &charLink, "IPv4Address", &charIPv4Address, "MACAddress", &charMac);

					if (ret)
					{
						printf("[hwanjang] Error !! thread_function_for_network_interface -> json_unpack fail .. !!!\n");

						printf("channel : %d , strByPassResult : %s\n", channel, strByPassResult.c_str());
					}
					else
					{
						
						g_Worker_SubDevice_info_[channel].LinkStatus = charLink;
						g_Worker_SubDevice_info_[channel].IPv4Address = charIPv4Address;
						g_Worker_SubDevice_info_[channel].MACAddress = charMac;

						printf("thread_function_for_network_interface() -> channel : %d, strIPv4Address : %s\n",
							channel, charIPv4Address);

						break;
					}
				}
			}
		}
	}

	g_Worker_SubDevice_info_[channel].update_check = true;

#if 0
	std::lock_guard<std::mutex> mtx_guard(g_Mtx_lock_device);

	g_Sub_network_info_Cnt++;

	if (g_Sub_network_info_Cnt >= g_RegisteredCameraCnt)
	{
		UpdateSubdeviceInfos();

		SendResponseForDeviceInfoView(strTopic, strTid);

		g_Sub_network_info_Cnt = 0;
	}
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Send subdevice info for deviceinfo 

void sunapi_manager::SendResponseForDeviceInfoView(const std::string& strTopic, std::string strTid)
{
#if 1
	std::string strCommand = "deviceinfo";
	std::string strType = "view";
	std::string strView = "detail";
	std::string strVersion = "1.5";
	// sub of message
	std::string strDeviceType = "gateway";

	json_t* main_ResponseMsg, *sub_Msg, *sub_SubdeviceMsg, *sub_IPAddressMsg, *sub_WebprotMsg, *sub_MacAddressMsg;

	main_ResponseMsg = json_object();
	sub_Msg = json_object();
	sub_SubdeviceMsg = json_object();
	sub_IPAddressMsg = json_object();
	sub_WebprotMsg = json_object();
	sub_MacAddressMsg = json_object();

	//main_Msg["command"] = "dashboard";
	json_object_set(main_ResponseMsg, "command", json_string(strCommand.c_str()));
	//main_Msg["type"] = "view";
	json_object_set(main_ResponseMsg, "type", json_string(strType.c_str()));
	//main_Msg["view"] = "detail";
	json_object_set(main_ResponseMsg, "view", json_string(strView.c_str()));

	// ipaddress of subdevice
	int i = 0, ipaddressMsgCnt=0, webportMsgCnt = 0, macaddressMsgCnt = 0;
	char charCh[8];

	for (i = 0; i < gMax_Channel; i++)
	{
		if (g_SubDevice_info_[i].ConnectionStatus != 0)
		{			
			memset(charCh, 0, sizeof(charCh));
			sprintf(charCh, "%d", i);  // int -> char*

			// ipaddress of subdevice
			if (g_SubDevice_info_[i].IPv4Address.empty() != true)
			{
				json_object_set_new(sub_IPAddressMsg, charCh, json_string(g_SubDevice_info_[i].IPv4Address.c_str()));
				ipaddressMsgCnt++;
			}

			// webport of subdevice
			if (g_SubDevice_info_[i].HTTPPort != 0)
			{
				json_object_set_new(sub_WebprotMsg, charCh, json_integer(g_SubDevice_info_[i].HTTPPort));
				webportMsgCnt++;
			}

			// macaddress of subdevice
			if (g_SubDevice_info_[i].MACAddress.empty() != true)
			{
				json_object_set_new(sub_MacAddressMsg, charCh, json_string(g_SubDevice_info_[i].MACAddress.c_str()));
				macaddressMsgCnt++;
			}
		}
	}

	//sub_SubdeviceMsg["ipaddress"] = sub_IPAddressMsg;
	//sub_SubdeviceMsg["webport"] = sub_WebprotMsg;
	//sub_SubdeviceMsg["macaddress"] = sub_MacAddressMsg;

	if (ipaddressMsgCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "ipaddress", sub_IPAddressMsg);
	}

	if (webportMsgCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "webport", sub_WebprotMsg);
	}

	if (macaddressMsgCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "macaddress", sub_MacAddressMsg);
	}	

	// sub of message
	json_object_set(sub_Msg, "deviceId", json_string(g_Device_id.c_str()));
	json_object_set(sub_Msg, "deviceType", json_string(strDeviceType.c_str()));
	json_object_set(sub_Msg, "ipaddress", json_string(g_Gateway_info_->IPv4Address.c_str()));
	json_object_set(sub_Msg, "webport", json_integer(g_Gateway_info_->WebPort));
	json_object_set(sub_Msg, "macaddress", json_string(g_Gateway_info_->MACAddress.c_str()));
	json_object_set(sub_Msg, "subdevice", sub_SubdeviceMsg);

	//main_Msg["message"] = sub_Msg;
	json_object_set(main_ResponseMsg, "message", sub_Msg);

	//main_Msg["tid"] = strTid;
	json_object_set(main_ResponseMsg, "tid", json_string(strTid.c_str()));
	//main_Msg["version"] = "1.5";
	json_object_set(main_ResponseMsg, "version", json_string(strVersion.c_str()));

	//std::string strMQTTMsg = writer.write(mqtt_Msg);		
	std::string strMQTTMsg = json_dumps(main_ResponseMsg, 0);

	printf("SendSubdeviceInfoForDeviceInfoView() -> strMQTTMsg size : %lu\n%s\n", strMQTTMsg.size(), strMQTTMsg.c_str());

	observerForHbirdManager->SendResponseForDashboard(strTopic, strMQTTMsg);
#endif

}


/////////////////////////////////////////////////////////////////////////////////////////////
// 3. firmware info view
// Get firmware version - /stw-cgi/system.cgi?msubmenu=deviceinfo&action=view


bool sunapi_manager::GetDeviceInfoOfGateway()
{
	int max_channel = 0;

	std::string request;

	request = "http://";
	request.append(gStrDeviceIP);
	request.append("/stw-cgi/system.cgi?msubmenu=deviceinfo&action=view");

	//printf("private ID:PW = %s\n",userpw.c_str());

#if 1  // 2019.09.05 hwanjang - sunapi debugging
	printf("sunapi_manager::GetDeviceInfoOfGateway() -> SUNAPI Request : %s\n", request.c_str());
#endif

	CURLcode res;
	std::string strSUNAPIResult;
	bool json_mode = true;
	bool ssl_opt = false;

	res = CURL_Process(json_mode, ssl_opt, CURL_TIMEOUT, request, gStrDevicePW, &strSUNAPIResult);

	if (res == CURLE_OK)
	{
		if (strSUNAPIResult.empty())
		{
			printf("[hwanjang] Attributes string is empty !!!\n");
			return false;
		}
		else
		{
			json_error_t error_check;
			json_t* json_strRoot = json_loads(strSUNAPIResult.c_str(), 0, &error_check);

			if (!json_strRoot)
			{
				fprintf(stderr, "error : root\n");
				fprintf(stderr, "error : on line %d: %s\n", error_check.line, error_check.text);

				printf("GetDeviceInfoOfGateway() -> strSUNAPIResult : \n%s\n", strSUNAPIResult.c_str());

				return false;
			}

			// get Model
			char* charModel;
			int result = json_unpack(json_strRoot, "{s:s}", "Model", &charModel);

			if (result)
			{
				printf("[hwanjang] Error !! GetDeviceInfoOfGateway() -> 1. json_unpack fail .. Model ...\n");
				printf("GetDeviceInfoOfGateway() -> strSUNAPIResult : \n%s\n", strSUNAPIResult.c_str());				
			}
			else
			{
				g_Gateway_info_->Model = charModel;
			}

			// get FirmwareVersion
			char* charFirmwareVersion;
			result = json_unpack(json_strRoot, "{s:s}", "FirmwareVersion", &charFirmwareVersion);

			if (result)
			{
				printf("[hwanjang] Error !! GetDeviceInfoOfGateway() -> 2. json_unpack fail .. FirmwareVersion ...\n");
				printf("GetDeviceInfoOfGateway() -> strSUNAPIResult : \n%s\n", strSUNAPIResult.c_str());
			}
			else
			{
				g_Gateway_info_->FirmwareVersion = charFirmwareVersion;
			}

			// get MACAddress
			char* charMACAddress;
			result = json_unpack(json_strRoot, "{s:s}", "ConnectedMACAddress", &charMACAddress);

			if (result)
			{
				printf("[hwanjang] Error !! GetDeviceInfoOfGateway() -> 3. json_unpack fail .. ConnectedMACAddress ...\n");
				printf("GetDeviceInfoOfGateway() -> strSUNAPIResult : \n%s\n", strSUNAPIResult.c_str());
			}
			else
			{
				g_Gateway_info_->MACAddress = charMACAddress;
			}
		}
	}
	else
	{
		printf("error ... initialize cURL !!!\n");
		return false;
	}

	return true;
}

#if 1

bool sunapi_manager::GetFirmwareVersionFromText(std::string strText, std::string* strResult)
{
	if (strText.find("FirmwareVersion=") != std::string::npos)
	{
		size_t ver_index = strText.find("FirmwareVersion=", 0);
		size_t crlf_index = strText.find("\r\n", ver_index + 16);
		std::string strVersion = strText.substr(ver_index + 16, (crlf_index - (ver_index + 16)));

		if (!strVersion.empty())
		{
			printf("GetFirmwareVersionFromText() -> parsing text ... version : %s\n",
				 strVersion.c_str());

			*strResult = strVersion;			
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

int sunapi_manager::GetFirmwareVersionOfSubdevices()
{
	// Get firmware version - /stw-cgi/system.cgi?msubmenu=deviceinfo&action=view

	g_UpdateTimeForFirmwareVersionOfSubdevices = time(NULL);


	std::string deviceIP = gStrDeviceIP;
	std::string devicePW = gStrDevicePW;

	// for test
	//max_ch = 10; 

	for (int i = 0; i < gMax_Channel; i++)
	{
		if (g_Worker_SubDevice_info_[i].ConnectionStatus == 1)  // 0 : Disconnected , 1 : Success , 2 : ConnectFail
		{
			printf("GetFirmwareVersionOfSubdevices() -> index : %d ... ConnectionStatus : %d\n", i, g_Worker_SubDevice_info_[i].ConnectionStatus);

			ThreadStartForFirmwareVersion(i, deviceIP, devicePW);

			sleep_for(std::chrono::milliseconds(2));
		}
	}

	return true;
}


int sunapi_manager::ThreadStartForFirmwareVersion(int channel, const std::string deviceIP, const std::string devicePW)
{
	std::thread thread_function([=] { thread_function_for_firmware_version(channel, deviceIP, devicePW); });
	thread_function.detach();

	return 0;
}

void sunapi_manager::thread_function_for_firmware_version(int channel, const std::string deviceIP, const std::string devicePW)
{
	printf("thread_function_for_firmware_version() -> channel : %d , Start to get firmwareversion ...\n", channel);

	bool result = false;

	std::string strByPassResult, strFirmwareversion;

	std::string strByPassURI = "/stw-cgi/system.cgi?msubmenu=deviceinfo&action=view";

	bool json_mode = true;
	CURLcode resCode;
	result = ByPassSUNAPI(channel, json_mode, deviceIP, devicePW, strByPassURI, &strByPassResult, &resCode);

	if (result)
	{
		if ( !strByPassResult.empty() )
		{
			json_error_t error_check;
			json_t* json_strRoot = json_loads(strByPassResult.c_str(), 0, &error_check);

			if (!json_strRoot)
			{
				fprintf(stderr, "error : root\n");
				fprintf(stderr, "error : on line %d: %s\n", error_check.line, error_check.text);

				printf("channel : %d , strByPassResult : %s\n", channel, strByPassResult.c_str());

				std::string strResult;
				result = GetFirmwareVersionFromText(strByPassResult, &strResult);
						
				if (result)
				{
					g_Worker_Firmware_Ver_info_[channel].FirmwareVersion = strResult;

					printf("thread_function_for_firmware_version() -> 1 ... channel : %d, FirmwareVersion : %s\n",
						channel, strResult.c_str());
				}
				else
				{
					printf("thread_function_for_firmware_version() -> 1 ... channel : %d, not found firmware version ...\n", channel);
				}				
			}
			else
			{
				int ret;
				char* charFWVersion;
				ret = json_unpack(json_strRoot, "{s:s}", "FirmwareVersion", &charFWVersion);

				if (ret)
				{
					printf("[hwanjang] Error !! thread_function_for_firmware_version -> json_unpack fail .. !!!\n");

					printf("channel : %d , strByPassResult : %s\n", channel, strByPassResult.c_str());

					std::string strResult;
					result = GetFirmwareVersionFromText(strByPassResult, &strResult);

					if (result)
					{
						g_Worker_Firmware_Ver_info_[channel].FirmwareVersion = strResult;

						printf("thread_function_for_firmware_version() -> 2 ... channel : %d, FirmwareVersion : %s\n",
							channel, strResult.c_str());
					}
					else
					{
						printf("thread_function_for_firmware_version() -> 2 ... channel : %d, not found firmware version ...\n", channel);
					}
				}
				else
				{
					g_Worker_Firmware_Ver_info_[channel].FirmwareVersion = charFWVersion;

					printf("thread_function_for_firmware_version() -> channel : %d, FirmwareVersion : %s\n",
						channel, charFWVersion);					
				}
			}
		}
	}
	else
	{
		// fail
	}

	g_Worker_Firmware_Ver_info_[channel].update_check = true;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////
// Send firmware version of subdevices 

void sunapi_manager::SendResponseForFirmwareView(const std::string& strTopic, std::string strTid)
{
	std::string strCommand = "deviceinfo";
	std::string strType = "view";
	std::string strView = "detail";
	std::string strVersion = "1.5";
	// sub of message
	std::string strDeviceType = "gateway";

	json_t* main_ResponseMsg, * sub_Msg, * sub_SubdeviceMsg, * sub_FWVersionMsg, * sub_LatestVersionMsg;

	main_ResponseMsg = json_object();
	sub_Msg = json_object();
	sub_SubdeviceMsg = json_object();
	sub_FWVersionMsg = json_object();
	sub_LatestVersionMsg = json_object();

	//main_Msg["command"] = "dashboard";
	json_object_set(main_ResponseMsg, "command", json_string(strCommand.c_str()));
	//main_Msg["type"] = "view";
	json_object_set(main_ResponseMsg, "type", json_string(strType.c_str()));
	//main_Msg["view"] = "detail";
	json_object_set(main_ResponseMsg, "view", json_string(strView.c_str()));

	// ipaddress of subdevice
	int i = 0, fwVersionMsgCnt = 0, latestVersionMsgCnt = 0;
	char charCh[8];

	for (i = 0; i < gMax_Channel; i++)
	{
		if (g_SubDevice_info_[i].ConnectionStatus != 0)
		{
			memset(charCh, 0, sizeof(charCh));
			sprintf(charCh, "%d", i);  // int -> char*

			// firmwareVersion of subdevice
			if (g_Firmware_Ver_info_[i].FirmwareVersion.empty() != true)
			{
				json_object_set_new(sub_FWVersionMsg, charCh, json_string(g_Firmware_Ver_info_[i].FirmwareVersion.c_str()));
				fwVersionMsgCnt++;
			}

			// latestFirmwareVersion of subdevice
			if (g_Firmware_Ver_info_[i].LatestFirmwareVersion.empty() != true)
			{
				json_object_set_new(sub_LatestVersionMsg, charCh, json_string(g_Firmware_Ver_info_[i].LatestFirmwareVersion.c_str()));
				latestVersionMsgCnt++;
			}
		}
	}

	//sub_SubdeviceMsg["firmwareVersion"] = sub_IPAddressMsg;
	//sub_SubdeviceMsg["latestFirmwareVersion"] = sub_WebprotMsg;

	if (fwVersionMsgCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "firmwareVersion", sub_FWVersionMsg);
	}

	if (latestVersionMsgCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "latestFirmwareVersion", sub_LatestVersionMsg);
	}

	// sub of message
	json_object_set(sub_Msg, "deviceId", json_string(g_Device_id.c_str()));
	json_object_set(sub_Msg, "deviceType", json_string(strDeviceType.c_str()));
	if (g_Gateway_info_->FirmwareVersion.empty() != true)
		json_object_set(sub_Msg, "firmwareVersion", json_string(g_Gateway_info_->FirmwareVersion.c_str()));

	json_object_set(sub_Msg, "subdevice", sub_SubdeviceMsg);

	//main_Msg["message"] = sub_Msg;
	json_object_set(main_ResponseMsg, "message", sub_Msg);

	//main_Msg["tid"] = strTid;
	json_object_set(main_ResponseMsg, "tid", json_string(strTid.c_str()));
	//main_Msg["version"] = "1.5";
	json_object_set(main_ResponseMsg, "version", json_string(strVersion.c_str()));

	//std::string strMQTTMsg = writer.write(mqtt_Msg);		
	std::string strMQTTMsg = json_dumps(main_ResponseMsg, 0);

	printf("SendResponseForFirmwareView() -> strMQTTMsg size : %lu\n%s\n", strMQTTMsg.size(), strMQTTMsg.c_str());

	observerForHbirdManager->SendResponseForDashboard(strTopic, strMQTTMsg);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<std::string> split_string(std::string strLine, char delimiter)
{
	std::istringstream iss(strLine);             // istringstream에 str을 담는다.
	std::string buffer;                      // 구분자를 기준으로 절삭된 문자열이 담겨지는 버퍼

	std::vector<std::string> result;

	// istringstream은 istream을 상속받으므로 getline을 사용할 수 있다.
	while (getline(iss, buffer, delimiter)) {
		result.push_back(buffer);               // 절삭된 문자열을 vector에 저장
	}

	return result;
}

bool sunapi_manager::GetLatestFirmwareVersionFromFile(std::string file_name)
{
	std::string line;

	std::ifstream input_file(file_name);
	if (!input_file.is_open()) {
		std::cerr << "Could not open the file - '"
			<< file_name << "'" << std::endl;
		return false;
	}
	
	std::vector<std::string> str_result;
	Firmware_Infos firmware_info;

	int cnt = 0;
	while (std::getline(input_file, line)) {

		cnt++;

		//memset(&firmware_info, 0, sizeof(Firmware_Infos));

		//ines->push_back(line);
		str_result = split_string(line, ',');
		
		if (str_result.size() > 0)
		{
			firmware_info.model = str_result[0];
			firmware_info.product = str_result[1];
			firmware_info.extension = str_result[2];
			firmware_info.representative_model = str_result[3];
			firmware_info.fileName = str_result[4];
			firmware_info.version_info = str_result[5];

			g_vecFirmwareInfos.push_back(firmware_info);

			//printf("%d , sprit_string ...\n", cnt);
		}
		else
		{
			printf("%d , sprit_string size 0 ...\n", cnt);
		}
	}	
	
	int i = 0, j = 0;

	for (i = 0; i < gMax_Channel; i++)
	{
		if (g_Worker_SubDevice_info_[i].ConnectionStatus != 0)
		{
			for (j = 0; j < g_vecFirmwareInfos.size(); j++)
			{
				if (g_Worker_SubDevice_info_[i].Model.find(g_vecFirmwareInfos[j].model) != std::string::npos)
				{
					g_Worker_Firmware_Ver_info_[i].LatestFirmwareVersion = g_vecFirmwareInfos[j].version_info;
					break;
				}
			}
		}
	}

	return true;
}

static size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream)
{
	size_t written = fwrite(ptr, size, nmemb, (FILE*)stream);
	return written;
}

bool sunapi_manager::GetLatestFirmwareVersionFromURL(std::string update_FW_Info_url)
{
	std::string file_name = "fw_info.txt";
	std::ofstream fw_info_file(file_name);

	//printf("private ID:PW = %s\n",userpw.c_str());

#if 1  // 2019.09.05 hwanjang - sunapi debugging
	printf("sunapi_manager::GetLatestFirmwareVersionFromURL() -> Request URL : %s\n", update_FW_Info_url.c_str());
#endif

	CURL* curl_handle;
	static const char* pagefilename = "fw_info.txt";
	FILE* pagefile;

	curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */
	curl_handle = curl_easy_init();

	/* set URL to get here */
	curl_easy_setopt(curl_handle, CURLOPT_URL, update_FW_Info_url.c_str());

	std::string ca_path = "config/ca-certificates.crt";
	curl_easy_setopt(curl_handle, CURLOPT_CAINFO, ca_path.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1L);

	/* Switch on full protocol/debug output while testing */
	curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

	/* disable progress meter, set to 0L to enable it */
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);

	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);

	/* open the file */
	pagefile = fopen(pagefilename, "wb");
	if (pagefile) {

		/* write the page body to this file handle */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);

		/* get it! */
		curl_easy_perform(curl_handle);

		/* close the header file */
		fclose(pagefile);
	}

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);

	curl_global_cleanup();

	int result = GetLatestFirmwareVersionFromFile(pagefilename);

	return result;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
// For Test
// test for dashboard
void sunapi_manager::TestDashboardView()
{
	//GetStoragePresenceOfSubdevices();

	//sleep_for(std::chrono::milliseconds(1 * 1000));

	//GetStorageStatusOfSubdevices();

	sleep_for(std::chrono::milliseconds(3 * 1000));

	UpdateStorageInfos();

	printf("TestDashboardView() -> Max Channel : %d ... Dashboard Info ...\n", gMax_Channel);

	int connectedCount = 0;

	for (int i = 0; i < gMax_Channel; i++)
	{
		if (g_SubDevice_info_[i].ConnectionStatus == 1)
		{
			connectedCount++;
			printf("TestDashboardView() -> Connected Count : %d\n", connectedCount);

			if ((g_Storage_info_[i].das_presence) || (g_Storage_info_[i].nas_presence))
			{
				printf("\n################################################\n");
				printf("channel : %d, DAS Recording : %d , NAS Recording : %d ...\n", i, g_Storage_info_[i].das_enable, g_Storage_info_[i].nas_enable);
				printf("DAS presence : %d\n", g_Storage_info_[i].das_presence);
				printf("DAS fail : %d\n", g_Storage_info_[i].sdfail);
				printf("DAS enable : %d\n", g_Storage_info_[i].das_enable);
				printf("DAS status : %s\n", g_Storage_info_[i].das_status.c_str());
				printf("NAS presence : %d\n", g_Storage_info_[i].nas_presence);
				printf("NAS fail : %d\n", g_Storage_info_[i].nasfail);
				printf("NAS enable : %d\n", g_Storage_info_[i].nas_enable);
				printf("NAS status : %s\n", g_Storage_info_[i].nas_status.c_str());
				printf("\n################################################\n");
			}
			else
			{
				printf("channel : %d, DAS : %d , NAS : %d ... not support ... Skip !!!\n", i, g_Storage_info_[i].das_presence, g_Storage_info_[i].nas_presence);
			}
		}
		else if (g_SubDevice_info_[i].ConnectionStatus == 2)
		{
			printf("channel : %d, ConnectionStatus : %d ... ConnectionFail ... Skip !!!\n", i, g_SubDevice_info_[i].ConnectionStatus);
		}
		else
		{
			printf("channel : %d, ConnectionStatus : %d ... Disconneted ... Skip !!!\n", i, g_SubDevice_info_[i].ConnectionStatus);
		}
	}

	SendResponseForDashboardView("topic", "12345");
}

// test for deviceinfo
void sunapi_manager::TestDeviceInfoView()
{
	//GetMacAddressOfSubdevices();

	sleep_for(std::chrono::milliseconds(3 * 1000));

	UpdateSubdeviceInfos();

	printf("TestDeviceInfoView() -> Max Channel : %d ... Device Info ...\n", gMax_Channel);

	int connectedCount = 0;

	for (int i = 0; i < gMax_Channel; i++)
	{
		if (g_SubDevice_info_[i].ConnectionStatus == 1)
		{
			connectedCount++;
			printf("TestDashboardView() -> Connected Count : %d\n", connectedCount);

			printf("\n################################################\n");
			printf("channel : %d, ConnectionStatus : %d ...\n", i, g_SubDevice_info_[i].ConnectionStatus);
			printf("LinkStatus : %s\n", g_SubDevice_info_[i].LinkStatus.c_str());
			printf("Model : %s\n", g_SubDevice_info_[i].Model.c_str());
			printf("MACAddress : %s\n", g_SubDevice_info_[i].MACAddress.c_str());
			printf("IPv4Address : %s\n", g_SubDevice_info_[i].IPv4Address.c_str());
			printf("HTTPPort : %d\n", g_SubDevice_info_[i].HTTPPort);
			printf("\n################################################\n");
		}
		else if (g_SubDevice_info_[i].ConnectionStatus == 2)
		{
			printf("channel : %d, ConnectionStatus : %d ... ConnectionFail ... Skip !!!\n", i, g_SubDevice_info_[i].ConnectionStatus);
		}
		else
		{
			printf("channel : %d, ConnectionStatus : %d ... Disconneted ... Skip !!!\n", i, g_SubDevice_info_[i].ConnectionStatus);
		}
	}

	SendResponseForDeviceInfoView("topic", "12345");
}

// test for firmware version info
void sunapi_manager::TestFirmwareVersionInfoView()
{
#if 0
	GetFirmwareVersionOfSubdevices();

	sleep_for(std::chrono::milliseconds(3 * 1000));

	UpdateSubdeviceInfos();
#endif

	printf("TestFirmwareVersionInfoView() -> Max Channel : %d ... Device Info ...\n", gMax_Channel);

	int connectedCount = 0;

	for (int i = 0; i < gMax_Channel; i++)
	{
		if (g_SubDevice_info_[i].ConnectionStatus == 1)
		{
			connectedCount++;
			printf("TestFirmwareVersionInfoView() -> Connected Count : %d\n", connectedCount);

			printf("\n################################################\n");
			printf("channel : %d, model : %s, firmwareVersion : %s , latestFirmwareVersion : %s\n",
				i, g_SubDevice_info_[i].Model.c_str(),
				g_Firmware_Ver_info_[i].FirmwareVersion.c_str(), g_Firmware_Ver_info_[i].LatestFirmwareVersion.c_str());
			printf("\n################################################\n");
		}
		else if (g_SubDevice_info_[i].ConnectionStatus == 2)
		{
			printf("channel : %d, ConnectionStatus : %d ... ConnectionFail ... Skip !!!\n", i, g_SubDevice_info_[i].ConnectionStatus);
		}
		else
		{
			printf("channel : %d, ConnectionStatus : %d ... Disconneted ... Skip !!!\n", i, g_SubDevice_info_[i].ConnectionStatus);
		}
	}

	SendResponseForFirmwareView("topic", "12345");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// interface for dashboard
void sunapi_manager::GetDataForDashboardAPI(const std::string& strTopic, const std::string& strPayload)
{
	json_error_t error_check;
	json_t* json_strRoot = json_loads(strPayload.c_str(), 0, &error_check);

	//cout << "process_command() -> " << json_dumps(json_strData, 0) << endl;

	int ret;
	char* charCommand, * charType;
	ret = json_unpack(json_strRoot, "{s:s, s:s}", "command", &charCommand, "type", &charType);

	if (ret)
	{
		printf("[hwanjang] Error !! GetDataForDashboardAPI() -> json_unpack fail .. !!!\n");

		printf("strPayload : %s\n", strPayload.c_str());
	}
	else
	{
		// Get ConnectionStatus of subdevice
		bool result = GetRegiesteredCameraStatus(gStrDeviceIP, gStrDevicePW);

		if (!result)
		{
			printf("failed to get 2. connectionStatus of subdevice ... ");
		}

		if (strncmp("dashboard", charCommand, 9) == 0)
		{
			GetDashboardView(strTopic, json_strRoot);
		}
		else if (strncmp("deviceinfo", charCommand, 10) == 0)
		{
			GetDeviceInfoView(strTopic, json_strRoot);
		}
		else if (strncmp("firmware", charCommand, 10) == 0)
		{
			if (strncmp("view", charType, 4) == 0)
			{
				GetFirmwareVersionInfoView(strTopic, json_strRoot);
			}
			else if (strncmp("update", charType, 4) == 0)
			{
				// not yet
				UpdateFirmwareOfSubdevice(strTopic, json_strRoot);
			}
		}
	}

	json_decref(json_strRoot);

#if 0
	time_t updateTime = time(NULL);

	if ((updateTime - g_UpdateTimeForDashboardAPI) > 30)
	{
		printf("\n##########################################################################################\n");
		printf("GetDataForDashboardAPI() -> updateTime : %ld , g_UpdateTimeForDashboardAPI : %ld , time : %ld\n",
			(long int)updateTime, (long int)g_UpdateTimeForDashboardAPI, (long int)(updateTime - g_UpdateTimeForDashboardAPI));
		UpdateDataForDashboardView();
	}
#endif
}


void sunapi_manager::GetDashboardView(const std::string& strTopic, json_t* json_strRoot)
{
	printf("GetDashboardView() -> Max Channel : %d ... Dashboard Info ...\n", gMax_Channel);	

	time_t update_time = time(NULL);

	if ((update_time - g_UpdateTimeOfStorageStatus) > g_UpdateTimeOut)
	{
		GetStorageStatusOfSubdevices();

		sleep_for(std::chrono::milliseconds(1 * 1000));
	}
	else
	{
		printf("check %ld ... Skip !!!!!\n", (long int)(update_time - g_UpdateTimeOfStorageStatus));

		// skip to check
		Set_update_checkForStorageInfo();
	}

	char* strCommand, * strType, * strTid;
	int result = json_unpack(json_strRoot, "{s:s, s:s, s:s}", "command", &strCommand, "type", &strType, "tid", &strTid);

	if (result)
	{
		printf("fail ... json_unpack() ...\n");
	}
	else
	{
		ThreadStartSendResponseForDashboardView(strTopic, strTid);
		//SendResponseForDashboardView(strTopic, strTid);
	}
}

// interface for deviceinfo
void sunapi_manager::GetDeviceInfoView(const std::string& strTopic, json_t* json_strRoot)
{
	printf("GetDeviceInfoView() -> Max Channel : %d ... Device Info ...\n", gMax_Channel);

#if 0
	GetDataForSubdeviceInfo();

	sleep_for(std::chrono::milliseconds(1 * 100));
#else
	bool result = false;

	time_t update_time = time(NULL);

	if ((update_time - g_UpdateTimeOfNetworkInterface) > g_UpdateTimeOut)
	{
		// add Device Name
		GetDeviceNameOfSubdevices();

		sleep_for(std::chrono::milliseconds(100));

		GetMacAddressOfSubdevices();

		sleep_for(std::chrono::milliseconds(1 * 1000));
	}
	else
	{
		printf("check %ld ... Skip !!!!!\n", (long int)(update_time - g_UpdateTimeOfNetworkInterface));

		// skip to check
		Set_update_checkForDeviceInfo();
	}
#endif	
	
	char* strCommand, * strType, * strTid;
	result = json_unpack(json_strRoot, "{s:s, s:s, s:s}", "command", &strCommand, "type", &strType, "tid", &strTid);

	if (result)
	{
		printf("fail ... json_unpack() ...\n");
	}
	else
	{
		ThreadStartSendResponseForDeviceInfoView(strTopic, strTid);

		//SendResponseForDeviceInfoView(strTopic, strTid);
	}
}

// interface for firmware version info
void sunapi_manager::GetFirmwareVersionInfoView(const std::string& strTopic, json_t* json_strRoot)
{
	printf("GetFirmwareVersionInfoView() -> Max Channel : %d ... Device Info ...\n", gMax_Channel);

	time_t update_time = time(NULL);

	if ((update_time - g_UpdateTimeForFirmwareVersionOfSubdevices) > g_UpdateTimeOut)
	{
		GetDataForFirmwareVersionInfo();

		sleep_for(std::chrono::milliseconds(1 * 1000));
	}
	else
	{
		printf("check %ld ... Skip !!!!!\n", (long int)(update_time - g_UpdateTimeForFirmwareVersionOfSubdevices));

		// skip to check
		Set_update_checkForFirmwareVersion();
	}

	char* strCommand, * strType, * strTid;
	int result = json_unpack(json_strRoot, "{s:s, s:s, s:s}", "command", &strCommand, "type", &strType, "tid", &strTid);

	if (result)
	{
		printf("fail ... json_unpack() ...\n");
	}
	else
	{
		ThreadStartSendResponseForFirmwareView(strTopic, strTid);
		//SendResponseForFirmwareView(strTopic, strTid);
	}
}

// interface for firmware update of subdevice
void sunapi_manager::UpdateFirmwareOfSubdevice(const std::string& strTopic, json_t* json_strRoot)
{
	char* strCommand, * strType, * strTid;
	int result = json_unpack(json_strRoot, "{s:s, s:s, s:s}", "command", &strCommand, "type", &strType, "tid", &strTid);

	if (result)
	{
		printf("[hwanjang] Error !! UpdateFirmwareOfSubdevice() -> json_unpack fail .. !!!\n");
	}
	else
	{
		json_t* json_message = json_object_get(json_strRoot, "message");

		if (!json_message)
		{
			printf("NG1 ... UpdateFirmwareOfSubdevice() -> json_is wrong ... message !!!\n");

			return;
		}

		json_t* json_subDevices = json_object_get(json_message, "subdevices");

		if (!json_subDevices)
		{
			printf("NG2 ... UpdateFirmwareOfSubdevice() -> json_is wrong ... subdevices !!!\n");

			return;
		}

		int index;
		int channel;
		json_t* json_sub;

		std::vector<int> updateChannel;

		json_array_foreach(json_subDevices, index, json_sub)
		{
			result = json_unpack(json_sub, "i", &channel);

			if (result)
			{
				printf("[hwanjang] Error !! GetGatewayInfo() -> 1. json_unpack fail .. Status ..index : %d !!!\n", index);
			}
			else
			{
				printf("Update ... channel : %d \n", channel);

				updateChannel.push_back(channel);

				// send update command to cloud gateway ...
			}
		}

		SendResponseForUpdateFirmware(strTopic, strTid, updateChannel);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////
// Send result of firmware update 

void sunapi_manager::SendResponseForUpdateFirmware(const std::string& strTopic, std::string strTid, std::vector<int> updateChannel)
{
#if 1
	std::string strCommand = "firmware";
	std::string strType = "view";
	std::string strView = "detail";
	std::string strVersion = "1.5";
	// sub of message
	std::string strDeviceType = "gateway";

	json_t* main_ResponseMsg, * sub_Msg, * sub_SubdeviceMsg, * sub_RawCodeMsg;

	main_ResponseMsg = json_object();
	sub_Msg = json_object();
	sub_SubdeviceMsg = json_object();
	sub_RawCodeMsg = json_object();

	//main_Msg["command"] = "dashboard";
	json_object_set(main_ResponseMsg, "command", json_string(strCommand.c_str()));
	//main_Msg["type"] = "view";
	json_object_set(main_ResponseMsg, "type", json_string(strType.c_str()));
	//main_Msg["view"] = "detail";
	json_object_set(main_ResponseMsg, "view", json_string(strView.c_str()));

	// ipaddress of subdevice
	int i = 0, rawCodeMsgCnt = 0;
	char charCh[8];

	for (int i = 0; i < updateChannel.size(); i++)
	{
		memset(charCh, 0, sizeof(charCh));
		sprintf(charCh, "%d", updateChannel[i]);  // int -> char*

		json_object_set_new(sub_RawCodeMsg, charCh, json_integer(200));
		rawCodeMsgCnt++;
	}

	//sub_SubdeviceMsg["firmwareVersion"] = sub_IPAddressMsg;
	//sub_SubdeviceMsg["latestFirmwareVersion"] = sub_WebprotMsg;

	json_object_set(sub_SubdeviceMsg, "resCode", json_integer(250));

	if (rawCodeMsgCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "rawCode", sub_RawCodeMsg);
	}

	// sub of message
	json_object_set(sub_Msg, "deviceId", json_string(g_Device_id.c_str()));
	json_object_set(sub_Msg, "deviceType", json_string(strDeviceType.c_str()));
	json_object_set(sub_Msg, "subdevice", sub_SubdeviceMsg);

	//main_Msg["message"] = sub_Msg;
	json_object_set(main_ResponseMsg, "message", sub_Msg);

	//main_Msg["tid"] = strTid;
	json_object_set(main_ResponseMsg, "tid", json_string(strTid.c_str()));
	//main_Msg["version"] = "1.5";
	json_object_set(main_ResponseMsg, "version", json_string(strVersion.c_str()));

	//std::string strMQTTMsg = writer.write(mqtt_Msg);		
	std::string strMQTTMsg = json_dumps(main_ResponseMsg, 0);

	printf("SendResponseForUpdateFirmware() -> strMQTTMsg size : %lu\n%s\n", strMQTTMsg.size(), strMQTTMsg.c_str());

	observerForHbirdManager->SendResponseForDashboard(strTopic, strMQTTMsg);
#endif

}

///////////////////////////////////////////////////////////////////////////////////////////////
// for update infos 
//
int sunapi_manager::ThreadStartForUpdateInfos(int second)
{
	std::thread thread_function([=] { thread_function_for_update_infos(second); });
	thread_function.detach();

	return 0;
}

void sunapi_manager::thread_function_for_update_infos(int second)
{
	std::cout << "main thread start ..." << time(NULL) << std::endl;

	// after second ... 
	sleep_for(std::chrono::milliseconds(second * 1000));

	UpdateDataForDashboardView();

	ThreadStartForUpdateInfos(60*60);  // after 1 hour

}

void sunapi_manager::UpdateDataForDashboardView()
{
	g_UpdateTimeForDashboardAPI = time(NULL);

	GetDataForFirmwareVersionInfo();

	sleep_for(std::chrono::milliseconds(1 * 1000));

	//GetDataForSubdeviceInfo();

	sleep_for(std::chrono::milliseconds(2 * 1000));

	GetDataForStorageInfo();

	sleep_for(std::chrono::milliseconds(2 * 1000));

	UpdateSubdeviceInfos();

	UpdateStorageInfos();
}

void sunapi_manager::GetDataForSubdeviceInfo()
{
	std::cout << "GetDataForSubdeviceInfo() start ..." << time(NULL) << std::endl;

	bool result = false;

	result = GetRegiesteredCameraStatus(gStrDeviceIP, gStrDevicePW);

	if (!result)
	{
		printf("failed to get registered status of subdevice ... ");
	}

	//int connectionCnt = GetConnectionCount();
	//std::cout << "GetDataForSubdeviceInfo() -> connectionCnt : " << connectionCnt << std::endl;

	time_t update_time = time(NULL);

	if ((update_time - g_UpdateTimeOfNetworkInterface) > 10)
	{
		GetMacAddressOfSubdevices();
	}

	//UpdateSubdeviceInfos();
}

void sunapi_manager::GetDataForStorageInfo()
{
	std::cout << "GetDataForStorageInfo() start ..." << time(NULL) << std::endl;

	//int connectionCnt = GetConnectionCount();
	//std::cout << "GetDataForStorageInfo() -> connectionCnt : " << connectionCnt << std::endl;

	// Get Storage info
	//GetStoragePresenceOfSubdevices();

	//sleep_for(std::chrono::milliseconds(1 * 1000));

	//GetStorageStatusOfSubdevices();

	//UpdateStorageInfos();
}

void sunapi_manager::GetDataForFirmwareVersionInfo()
{
	std::cout << "GetDataForFirmwareVersionInfo() start ..." << time(NULL) << std::endl;

	g_UpdateTimeForFirmwareVersionOfSubdevices = time(NULL);

	ThreadStartGetLatestFirmwareVersionFromURL();

	GetFirmwareVersionOfSubdevices();
}


int sunapi_manager::ThreadStartGetLatestFirmwareVersionFromURL()
{
	std::thread thread_function([=] { thread_function_for_get_latestFirmwareVersion(); });
	thread_function.detach();

	return 0;
}

void sunapi_manager::thread_function_for_get_latestFirmwareVersion()
{
	// Update Firmware version info
	std::string update_FW_Info_url = "https://update.websamsung.net/FW/Update_FW_Info3.txt";
	int result = GetLatestFirmwareVersionFromURL(update_FW_Info_url);
	if (!result)
	{
		printf("GetLatestFirmwareVersionFromURL() -> fail to get latest version ... \n");
	}
}

//ThreadStartForUpdateInfos(20);  // after 20 seconds

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 1. dashboard view
void sunapi_manager::Set_update_checkForStorageInfo()
{
	// for skip to check
	for (int i = 0; i < gMax_Channel; i++)
	{
		g_Worker_Storage_info_[i].update_check = true;
	}
}

void sunapi_manager::Reset_update_checkForStorageInfo()
{
	for (int i = 0; i < gMax_Channel; i++)
	{
		g_Worker_Storage_info_[i].update_check = false;
	}
}

int sunapi_manager::ThreadStartSendResponseForDashboardView(const std::string strTopic, std::string strTid)
{
	int maxChannel = gMax_Channel;
	std::thread thread_function([=] { thread_function_for_send_response_for_dashboard(maxChannel, strTopic, strTid); });
	thread_function.detach();

	return 0;
}

void sunapi_manager::thread_function_for_send_response_for_dashboard(int maxChannel, const std::string strTopic, std::string strTid)
{
	printf("\nthread_function_for_send_response_for_dashboard() -> Start .... \n ");
	
	int i = 0;
	while (1)
	{		
		for (i = 0; i < maxChannel; i++)
		{
			if (g_SubDevice_info_[i].ConnectionStatus == 1)
			{
				if (g_Worker_Storage_info_[i].update_check != true)
				{
					printf("nthread_function_for_send_response_for_dashboard() -> i : %d  --> break !!!\n", i);
					break;
				}
			}			

			if ((maxChannel - 1) == i)
			{
				printf("\n#############################################################################\n");
				printf("\nthread_function_for_send_response_for_dashboard() -> Start .... \n ");
				UpdateStorageInfos();

				SendResponseForDashboardView(strTopic, strTid);

				Reset_update_checkForStorageInfo();

				time_t end_time = time(NULL);
				std::cout << "nthread_function_for_send_response_for_dashboard() -> End ... time : " << (long int)end_time << " , diff : "
					<< (long int)(end_time - g_UpdateTimeOfStorageStatus) << std::endl;

				return;
			}

		}

		printf("nthread_function_for_send_response_for_dashboard() -> i : %d\n", i);

		sleep_for(std::chrono::milliseconds(1));
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2. device info view
void sunapi_manager::Set_update_checkForDeviceInfo()
{
	// for skip to check
	for (int i = 0; i < gMax_Channel; i++)
	{
		g_Worker_SubDevice_info_[i].update_check = true;
	}
}

void sunapi_manager::Reset_update_checkForDeviceInfo()
{
	for (int i = 0; i < gMax_Channel; i++)
	{
		g_Worker_SubDevice_info_[i].update_check = false;
	}
}

int sunapi_manager::ThreadStartSendResponseForDeviceInfoView(const std::string strTopic, std::string strTid)
{
	int maxChannel = gMax_Channel;
	std::thread thread_function([=] { thread_function_for_send_response_for_deviceInfo(maxChannel, strTopic, strTid); });
	thread_function.detach();

	return 0;
}

void sunapi_manager::thread_function_for_send_response_for_deviceInfo(int maxChannel, const std::string strTopic, std::string strTid)
{
	printf("\thread_function_for_send_response_for_deviceInfo() -> Start .... \n ");

	int i = 0;
	while (1)
	{
		for (i = 0; i < maxChannel; i++)
		{
			if (g_SubDevice_info_[i].ConnectionStatus != 0)
			{
				if (g_Worker_SubDevice_info_[i].update_check != true)
				{
					printf("thread_function_for_send_response_for_deviceInfo() -> i : %d  --> break !!!\n", i);
					break;
				}
			}

			if ((maxChannel - 1) == i)
			{
				printf("\n#############################################################################\n");
				printf("\thread_function_for_send_response_for_deviceInfo() -> Start .... \n ");
				UpdateSubdeviceInfos();

				SendResponseForDeviceInfoView(strTopic, strTid);

				Reset_update_checkForDeviceInfo();


				time_t end_time = time(NULL);
				std::cout << "thread_function_for_send_response_for_deviceInfo() -> End ... time : " << (long int)end_time << " , diff : "
					<< (long int)(end_time - g_UpdateTimeOfNetworkInterface) << std::endl;

				return;
			}

		}

		printf("thread_function_for_send_response_for_deviceInfo() -> i : %d\n", i);

		sleep_for(std::chrono::milliseconds(1));
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 3. firmware version info view
void sunapi_manager::Set_update_checkForFirmwareVersion()
{
	// for skip to check
	for (int i = 0; i < gMax_Channel; i++)
	{
		g_Worker_SubDevice_info_[i].update_check = true;
	}
}

void sunapi_manager::Reset_update_checkForFirmwareVersion()
{
	for (int i = 0; i < gMax_Channel; i++)
	{
		g_Worker_SubDevice_info_[i].update_check = false;
	}
}

int sunapi_manager::ThreadStartSendResponseForFirmwareView(const std::string strTopic, std::string strTid)
{
	int maxChannel = gMax_Channel;
	std::thread thread_function([=] { thread_function_for_send_response_for_firmwareVersion(maxChannel, strTopic, strTid); });
	thread_function.detach();

	return 0;
}

void sunapi_manager::thread_function_for_send_response_for_firmwareVersion(int maxChannel, const std::string strTopic, std::string strTid)
{
	printf("\thread_function_for_send_response_for_firmwareVersion() -> Start .... \n ");

	int i = 0;
	while (1)
	{
		for (i = 0; i < maxChannel; i++)
		{
			if (g_SubDevice_info_[i].ConnectionStatus == 1)
			{
				if (g_Worker_Firmware_Ver_info_[i].update_check != true)
				{
					printf("thread_function_for_send_response_for_firmwareVersion() -> i : %d  --> break !!!\n", i);
					break;
				}
			}

			if ((maxChannel - 1) == i)
			{
				printf("\n#############################################################################\n");
				printf("\thread_function_for_send_response_for_firmwareVersion() -> Start .... \n ");				

				UpdateFirmwareVersionInfos();

				SendResponseForFirmwareView(strTopic, strTid);

				Reset_update_checkForFirmwareVersion();

				return;
			}

		}

		printf("thread_function_for_send_response_for_firmwareVersion() -> i : %d\n", i);

		sleep_for(std::chrono::milliseconds(1));
	}
}



/////////////////////////////////////////////////////////////////////////////////////////////////
// process 'command' topic - checkPassword

void sunapi_manager::CommandCheckPassword(const std::string& strTopic, json_t* json_root)
{
#if 1
	std::string strMQTTMsg;
	std::string strCommand, strType, strView, strTid, strVersion;

	int ret;
	char* charCommand, * charType, *charView, *charTid, *charVersion;
	ret = json_unpack(json_root, "{s:s, s:s, s:s, s:s, s:s}", "command", &charCommand, "type", &charType, "view", &charView, "tid", &charTid, "version", &charVersion);

	if (ret)
	{
		strCommand = "checkPassword";
		strType = "view";
		strView = "detail";
		strTid = "tmpTid_12345";
		strVersion = "1.5";
	}
	else
	{
		strCommand = charCommand;
		strType = charType;
		strView = charView;
		strTid = charTid;
		strVersion = charVersion;
	}

	json_t* main_ResponseMsg, * sub_Msg;

	main_ResponseMsg = json_object();
	sub_Msg = json_object();

	json_object_set(main_ResponseMsg, "command", json_string(strCommand.c_str()));
	json_object_set(main_ResponseMsg, "type", json_string(strType.c_str()));
	json_object_set(main_ResponseMsg, "view", json_string(strView.c_str()));
	json_object_set(main_ResponseMsg, "tid", json_string(strTid.c_str()));
	json_object_set(main_ResponseMsg, "version", json_string(strVersion.c_str()));

	json_t* json_message = json_object_get(json_root, "message");

	if (!json_message)
	{
		printf("error ... CommandCheckPassword() -> json_is wrong ... message !!!\n");

		json_object_set(sub_Msg, "responseCode", json_string("600"));
		json_object_set(sub_Msg, "deviceModel", json_string(""));
		json_object_set(sub_Msg, "deviceFirmwareVersion", json_string(""));
		json_object_set(sub_Msg, "maxChannels", json_integer(0));

		json_object_set(main_ResponseMsg, "message", sub_Msg);

		strMQTTMsg = json_dumps(main_ResponseMsg, 0);

		observerForHbirdManager->SendResponseToPeer(strTopic, strMQTTMsg);
		return ;
	}

	char* charID, * charPassword;
	ret = json_unpack(json_message, "{ s:s , s:s }", "id", &charID, "password", &charPassword);

	if (ret)
	{
		printf("Error ... CommandCheckPassword() -> json_is wrong ... ID & Pass !!!\n");

		json_object_set(sub_Msg, "responseCode", json_string("600"));
		json_object_set(sub_Msg, "deviceModel", json_string(""));
		json_object_set(sub_Msg, "deviceFirmwareVersion", json_string(""));
		json_object_set(sub_Msg, "maxChannels", json_integer(0));

		json_object_set(main_ResponseMsg, "message", sub_Msg);

		strMQTTMsg = json_dumps(main_ResponseMsg, 0);

		observerForHbirdManager->SendResponseToPeer(strTopic, strMQTTMsg);

		return ;
	}

	std::string strPassword = charID;
	strPassword.append(":");
	strPassword.append(charPassword);

	int maxCh = 0;
	maxCh = GetMaxChannelByAttribute(strPassword);

	if (maxCh < 1)
	{
		printf("Error ... CommandCheckPassword() -> It means that the wrong username or password were sent in the request.\n");

		json_object_set(sub_Msg, "responseCode", json_string("403"));
		json_object_set(sub_Msg, "deviceModel", json_string(""));
		json_object_set(sub_Msg, "deviceFirmwareVersion", json_string(""));
		json_object_set(sub_Msg, "maxChannels", json_integer(0));

		json_object_set(main_ResponseMsg, "message", sub_Msg);

		strMQTTMsg = json_dumps(main_ResponseMsg, 0);

		observerForHbirdManager->SendResponseToPeer(strTopic, strMQTTMsg);

		return ;
	}

	GetDeviceInfoOfGateway();

	//json_object_set(sub_Msg, "responseCode", json_integer(200));
	json_object_set(sub_Msg, "responseCode", json_string("200"));

	std::string strModel = g_Gateway_info_->Model;

	json_object_set(sub_Msg, "deviceModel", json_string(strModel.c_str()));

	std::string strDeviceVersion = g_Gateway_info_->FirmwareVersion;

	json_object_set(sub_Msg, "deviceFirmwareVersion", json_string(strDeviceVersion.c_str()));

	json_object_set(sub_Msg, "maxChannels", json_integer(maxCh));

	json_object_set(main_ResponseMsg, "message", sub_Msg);

	strMQTTMsg = json_dumps(main_ResponseMsg, 0);

	printf("[hwanjang APIManager::CommandCheckPassword()() response ---> size : %lu, send message : \n%s\n", strMQTTMsg.size(), strMQTTMsg.c_str());

	observerForHbirdManager->SendResponseToPeer(strTopic, strMQTTMsg);
	
#endif

}