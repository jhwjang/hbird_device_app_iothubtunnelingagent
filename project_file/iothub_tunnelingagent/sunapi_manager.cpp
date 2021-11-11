#include "sunapi_manager.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <sstream>  // for istringstream , getline
#include <fstream>
#include <chrono>
#include <vector>

#define HWANJANG_DEBUG

#define HBIRD_DEVICE_TYPE "gateway"

using std::this_thread::sleep_for;
#define SLEEP_TIME 2

// Default timeout is 0 (zero) which means it never times out during transfer.
#define GET_WAIT_TIMEOUT 5 
#define CURL_TIMEOUT 5 
#define CURL_CONNECTION_TIMEOUT 3  

#define DEFAULT_MAX_CHANNEL 128  // support max 128 channels

#define FIRMWARE_INFO_FILE "fw_info.txt"

std::mutex g_Mtx_lock_storage;
std::mutex g_Mtx_lock_device;

static int g_RegisteredCameraCnt;
static int g_ConnectionCnt;

// gateway
GatewayInfo* g_Gateway_info_;

// 1. dashboard
Storage_Infos* g_Storage_info_;
Storage_Infos* g_Worker_Storage_info_;

// 2. deviceinfo
Channel_Infos* g_SubDevice_info_;
Channel_Infos* g_Worker_SubDevice_info_;

// 3. firmware version
Firmware_Version_Infos* g_Firmware_Ver_info_;
Firmware_Version_Infos* g_Worker_Firmware_Ver_info_;

sunapi_manager::sunapi_manager(const std::string& strDeviceID, int nWebPort)
{
	g_Device_id = strDeviceID;
	g_WebPort = nWebPort;

	g_Max_Channel = 0;

	g_StrDeviceIP.clear();
	g_StrDevicePW.clear();

	g_RegisteredCameraCnt = 0;
	g_ConnectionCnt = 0;

	g_Sub_camera_reg_Cnt = 0;
	g_Sub_network_info_Cnt = 0;
	g_Sub_device_info_Cnt = 0;

	g_UpdateTimeOfRegistered = 0;
	g_UpdateTimeOfStoragePresence = 0;
	g_UpdateTimeOfStorageStatus = 0;
	g_UpdateTimeOfDeviceInfo = 0;
	g_UpdateTimeOfNetworkInterface = 0;
	g_UpdateTimeForFirmwareVersionOfSubdevices = 0;

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

bool sunapi_manager::SunapiManagerInit()
{
	/// <summary>
	///  Get Gateway IP , ID & PW
	/// </summary>

	std::string strIP, strPW;

	bool result = GetDeviceIP_PW(&strIP, &strPW);

	if ((result != true) || strIP.empty() || strPW.empty())
	{
		g_StrDeviceIP = "127.0.0.1";
		g_StrDevicePW = "admin:5tkatjd!";
	}
	else
	{
		g_StrDeviceIP = strIP;
		g_StrDevicePW = strPW;
	}

	printf("sunapi_manager_init() -> g_StrDeviceIP : %s , g_StrDevicePW : %s\n", g_StrDeviceIP.c_str(), g_StrDevicePW.c_str());

#if 0
	/// <summary>
	//  Get Max Channel
	/// </summary>

	int maxCh = 0;
	maxCh = GetMaxChannelByAttribute(g_StrDevicePW);

	if (maxCh > 0)
	{
		g_Max_Channel = maxCh;  // index = 0 ~ maxCh
	}
	else
	{
		// ???? 
		g_Max_Channel = DEFAULT_MAX_CHANNEL;
	}

	printf("sunapi_manager_init() -> Max Channel : %d\n", g_Max_Channel);

	// gateway init
	g_Gateway_info_ = new GatewayInfo;

	ResetGatewayInfo();

	result = GetNetworkInterfaceOfGateway();

	// error ??
	if (!result)
	{
		g_Gateway_info_->IPv4Address = g_StrDeviceIP;
		g_Gateway_info_->MACAddress = "unknown";
	}

	result = GetDeviceInfoOfGateway();
	
	if (!result)
	{
		g_Gateway_info_->DeviceModel = "CloudGateway";
		g_Gateway_info_->FirmwareVersion = "unknown";
	}
#else

	// gateway init
	g_Gateway_info_ = new GatewayInfo;

	ResetGatewayInfo();

	result = GetGatewayInfo(g_StrDeviceIP, g_StrDevicePW);

#endif

	int maxChannel = g_Gateway_info_->maxChannel;

	// 1. dashboard init
	g_Storage_info_ = new Storage_Infos[maxChannel];
	g_Worker_Storage_info_ = new Storage_Infos[maxChannel];

	ResetStorageInfos();

	// 2. deviceinfo init
	g_SubDevice_info_ = new Channel_Infos[maxChannel];
	g_Worker_SubDevice_info_ = new Channel_Infos[maxChannel];

	ResetSubdeviceInfos();
	
	// 3. firmware version
	g_Firmware_Ver_info_ = new Firmware_Version_Infos[maxChannel];
	g_Worker_Firmware_Ver_info_ = new Firmware_Version_Infos[maxChannel];

	ResetFirmwareVersionInfos();

	// Get ConnectionStatus of subdevice
	CURLcode resCode;
	result = GetRegiesteredCameraStatus(g_StrDeviceIP, g_StrDevicePW, &resCode);

	if (!result)
	{
		printf("failed to get GetRegiesteredCameraStatus ... 1");

		result = GetRegiesteredCameraStatus(g_StrDeviceIP, g_StrDevicePW, &resCode);
		if (!result)
		{
			printf("failed to get GetRegiesteredCameraStatus ... 2 ... return !!");

			if ((resCode == CURLE_LOGIN_DENIED) || (resCode == CURLE_AUTH_ERROR))
			{
				// authfail
			}
			else if (resCode == CURLE_OPERATION_TIMEDOUT)
			{
				// timeout
			}
			else {
				// another error

			}
		}
	}

#if 1
	//GetMacAddressOfSubdevices();
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

	printf("[hwanjang] sunapi_manager::SunapiManagerInit() ... End ... return %d\n", result);

	return result;
}

bool sunapi_manager::GetGatewayInfo(const std::string& strGatewayIP, const std::string& strID_PW)
{
	/// <summary>
	//  Get Max Channel
	/// </summary>
	std::string _gatewayIP = strGatewayIP;
	std::string _idPassword = strID_PW;

	int maxCh = 0;
	maxCh = GetMaxChannelByAttribute(_gatewayIP, _idPassword);

	if (maxCh < 1)
	{
		// ???? 
#ifdef HWANJANG_DEBUG
		printf("sunapi_manager::GetGatewayInfo() -> failed to GetMaxChannelByAttribute ...\n");
#endif
		//g_Max_Channel = DEFAULT_MAX_CHANNEL;
		g_Gateway_info_->maxChannel = DEFAULT_MAX_CHANNEL;
	}

	printf("sunapi_manager::GetGatewayInfo() -> Max Channel : %d\n", g_Gateway_info_->maxChannel);

	bool result_interface = GetNetworkInterfaceOfGateway(_gatewayIP, _idPassword);

	// error ??
	if (!result_interface)
	{
		g_Gateway_info_->IPv4Address = g_StrDeviceIP;
		g_Gateway_info_->MACAddress = "unknown";

#ifdef HWANJANG_DEBUG
		printf("sunapi_manager::GetGatewayInfo() -> failed to GetNetworkInterfaceOfGateway ...\n");
#endif
	}

	bool result_deviceInfo = GetDeviceInfoOfGateway(_gatewayIP, _idPassword);

	if (!result_deviceInfo)
	{
		g_Gateway_info_->DeviceModel = "CloudGateway";
		g_Gateway_info_->FirmwareVersion = "unknown";

#ifdef HWANJANG_DEBUG
		printf("sunapi_manager::GetGatewayInfo() -> failed to GetDeviceInfoOfGateway ...\n");
#endif
	}

	// issue ... failure due to timeout  ????
	if ((maxCh > 0) && result_interface && result_deviceInfo)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool sunapi_manager::GetNetworkInterfaceOfGateway(const std::string& strGatewayIP, const std::string& strID_PW)
{
	std::string _gatewayIP = strGatewayIP;
	std::string _idPassword = strID_PW;

	std::string request;

	CURLcode resCode;
	std::string strSUNAPIResult;

	bool json_mode = true;
	bool ssl_opt = false;

#if 0  // modified to read config file
	/// /////////////////////////////////////////////////////////////////////////////////////
	// Get HTTP Port

	request = "http://";
	request.append(g_StrDeviceIP);
	request.append("/stw-cgi/network.cgi?msubmenu=http&action=view");

#if 1  // 2019.09.05 hwanjang - sunapi debugging
	printf("sunapi_manager::GetNetworkInterfaceOfGateway() -> 1. SUNAPI Request : %s\n", request.c_str());
#endif

	resCode = CURL_Process(json_mode, ssl_opt, request, g_StrDevicePW, &strSUNAPIResult);

	if (resCode == CURLE_OK)
	{
		if (strSUNAPIResult.empty())
		{
			printf("[hwanjang] GetNetworkInterfaceOfGateway() -> strSUNAPIResult is empty !!!\n");
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
				printf("[hwanjang] Error !! GetNetworkInterfaceOfGateway() -> 1. json_unpack fail .. !!!\n");
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
	g_Gateway_info_->WebPort = g_WebPort;
#endif

	/// /////////////////////////////////////////////////////////////////////////////////////
	// Get IPv4Address , MACAddress
	//  - /stw-cgi/network.cgi?msubmenu=interface&action=view  -> IPv4Address , MACAddress
	//  - /stw-cgi/system.cgi?msubmenu=deviceinfo&action=view  -> ConnectedMACAddress , FirmwareVersion
	// 
	// reset
	g_Gateway_info_->curl_responseCode = CURLE_OPERATION_TIMEDOUT;
	g_Gateway_info_->ConnectionStatus = "timeout";

	request.clear();

	request = "http://";
	request.append(_gatewayIP);
	request.append("/stw-cgi/network.cgi?msubmenu=interface&action=view");

#if 1  // 2019.09.05 hwanjang - sunapi debugging
	printf("sunapi_manager::GetNetworkInterfaceOfGateway() -> 2. SUNAPI Request : %s\n", request.c_str());
#endif

	strSUNAPIResult.clear();

	resCode = CURL_Process(json_mode, ssl_opt, CURL_TIMEOUT, request, _idPassword, &strSUNAPIResult);

	g_Gateway_info_->curl_responseCode = resCode;

	if (resCode == CURLE_OK)
	{
		g_Gateway_info_->ConnectionStatus = "on";

		if (strSUNAPIResult.empty())
		{
			// strSUNAPIResult is empty ...
			printf("[hwanjang] sunapi_manager::GetNetworkInterfaceOfGateway() -> strSUNAPIResult is empty !!!\n");
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

				printf("[hwanjang] sunapi_manager::GetNetworkInterfaceOfGateway() -> strSUNAPIResult : \n%s\n", strSUNAPIResult.c_str());

				return false;
			}
			else
			{
#if 0 // original NVR
				json_t* json_interface = json_object_get(json_strRoot, "NetworkInterfaces");

				if (!json_interface)
				{
					printf("NG1 ... GetNetworkInterfaceOfGateway() -> json_is wrong ... NetworkInterfaces !!!\n");

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
						printf("[hwanjang] Error !! GetNetworkInterfaceOfGateway() -> 1. json_unpack fail .. Status ..index : %d !!!\n", index);
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
								printf("[hwanjang] Error !! GetNetworkInterfaceOfGateway() -> json_unpack fail .. !!!\n");

								printf("strSUNAPIResult : %s\n", strSUNAPIResult.c_str());
							}
							else
							{
								g_Gateway_info_->IPv4Address = charIPv4Address;
								g_Gateway_info_->MACAddress = charMac;

								printf("GetNetworkInterfaceOfGateway() -> strIPv4Address : %s , MACAddress : %s \n", charIPv4Address, charMac);

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
					printf("NG1 ... GetNetworkInterfaceOfGateway() -> json_is wrong ... NetworkInterfaces !!!\n");

					printf("strSUNAPIResult : %s\n", strSUNAPIResult.c_str());

					return false;
				}

				int result;
				size_t index;
				char* charInterface;
				json_t* json_sub;

				json_array_foreach(json_interface, index, json_sub)
				{
					result = json_unpack(json_sub, "{s:s}", "InterfaceName", &charInterface);

					if (result)
					{
						printf("[hwanjang] Error !! GetNetworkInterfaceOfGateway() -> 1. json_unpack fail .. Status ..index : %d !!!\n", index);
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
								printf("[hwanjang] Error !! GetNetworkInterfaceOfGateway() -> json_unpack fail ..  IPv4Address !!!\n");

								printf("strSUNAPIResult : %s\n", strSUNAPIResult.c_str());								
							}
							else
							{
								g_Gateway_info_->IPv4Address = charIPv4Address;								

								printf("GetNetworkInterfaceOfGateway() -> strIPv4Address : %s \n", charIPv4Address);
							}

							ret = json_unpack(json_sub, "{s:s}", "MACAddress", &charMac);

							if (ret)
							{
								printf("[hwanjang] Error !! GetNetworkInterfaceOfGateway() -> json_unpack fail ..  MACAddress !!!\n");

								printf("strSUNAPIResult : %s\n", strSUNAPIResult.c_str());
							}
							else
							{								
								g_Gateway_info_->MACAddress = charMac;

								printf("GetNetworkInterfaceOfGateway() -> MACAddress : %s \n", charMac);
							}

							return true;
						}
						else
						{
							// other network ...
							printf("[hwanjang] Error !! GetNetworkInterfaceOfGateway() -> charInterface is not Network1 !!!\n");

							printf("strSUNAPIResult : %s\n", strSUNAPIResult.c_str());
						}
					}
				}
#endif
			}
		}
	}
	else
	{
		std::string strError;
		// CURL_Process fail
		if ((resCode == CURLE_LOGIN_DENIED) || (resCode == CURLE_AUTH_ERROR))
		{
			// authfail
			strError = "authError";
			g_Gateway_info_->ConnectionStatus = "authError";

			printf("[hwanjang] sunapi_manager::GetNetworkInterfaceOfGateway() -> authError !!!\n");
		}
		else if (resCode == CURLE_OPERATION_TIMEDOUT)
		{
			// timeout
			printf("[hwanjang] sunapi_manager::GetNetworkInterfaceOfGateway() -> timeout !!!\n");
		}
		else {
			// another error		
			//strError = "unknown";
			strError = curl_easy_strerror(resCode);

			printf("[hwanjang] sunapi_manager::GetNetworkInterfaceOfGateway() -> CURL_Process fail .. strError : %s\n", strError.c_str());
			g_Gateway_info_->ConnectionStatus = "unknown";
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Reset
void sunapi_manager::ResetGatewayInfo()
{
	g_Gateway_info_->maxChannel = 0;
	g_Gateway_info_->WebPort = 0;
	ResetNetworkInterfaceOfGateway();
	ResetDeviceInfoOfGateway();
}

void sunapi_manager::ResetNetworkInterfaceOfGateway()
{
	g_Gateway_info_->curl_responseCode = 0;
	g_Gateway_info_->IPv4Address.clear();
	g_Gateway_info_->MACAddress.clear();
	g_Gateway_info_->ConnectionStatus.clear();

}
void sunapi_manager::ResetDeviceInfoOfGateway()
{
	g_Gateway_info_->curl_responseCode = 0;
	g_Gateway_info_->DeviceModel.clear();
	g_Gateway_info_->FirmwareVersion.clear();
	g_Gateway_info_->ConnectionStatus.clear();
}

void sunapi_manager::ResetStorageInfos()
{
	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		// storage info
		g_Storage_info_[i].update_check = true;  // In case of check, set to true. 
		g_Storage_info_[i].das_presence = false;
		g_Storage_info_[i].nas_presence = false;
		g_Storage_info_[i].sdfail = false;
		g_Storage_info_[i].nasfail = false;
		g_Storage_info_[i].das_enable = false;
		g_Storage_info_[i].nas_enable = false;
		g_Storage_info_[i].curl_responseCode = 0;
		g_Storage_info_[i].das_status.clear();
		g_Storage_info_[i].nas_status.clear();

		// worker
		g_Worker_Storage_info_[i].update_check = true;  // In case of check, set to true.
		g_Worker_Storage_info_[i].das_presence = false;
		g_Worker_Storage_info_[i].nas_presence = false;
		g_Worker_Storage_info_[i].sdfail = false;
		g_Worker_Storage_info_[i].nasfail = false;
		g_Worker_Storage_info_[i].das_enable = false;
		g_Worker_Storage_info_[i].nas_enable = false;
		g_Worker_Storage_info_[i].curl_responseCode = 0;
		g_Worker_Storage_info_[i].das_status.clear();
		g_Worker_Storage_info_[i].nas_status.clear();
	}
}

void sunapi_manager::ResetStorageInfoForChannel(int channel)
{
	// storage info
	g_Storage_info_[channel].update_check = true;  // In case of check, set to true.
	g_Storage_info_[channel].das_presence = false;
	g_Storage_info_[channel].nas_presence = false;
	g_Storage_info_[channel].sdfail = false;
	g_Storage_info_[channel].nasfail = false;
	g_Storage_info_[channel].das_enable = false;
	g_Storage_info_[channel].nas_enable = false;
	g_Storage_info_[channel].curl_responseCode = 0;
	g_Storage_info_[channel].das_status.clear();
	g_Storage_info_[channel].nas_status.clear();

	// worker
	g_Worker_Storage_info_[channel].update_check = true;  // In case of check, set to true.
	g_Worker_Storage_info_[channel].das_presence = false;
	g_Worker_Storage_info_[channel].nas_presence = false;
	g_Worker_Storage_info_[channel].sdfail = false;
	g_Worker_Storage_info_[channel].nasfail = false;
	g_Worker_Storage_info_[channel].das_enable = false;
	g_Worker_Storage_info_[channel].nas_enable = false;
	g_Worker_Storage_info_[channel].curl_responseCode = 0;
	g_Worker_Storage_info_[channel].das_status.clear();
	g_Worker_Storage_info_[channel].nas_status.clear();
}

void sunapi_manager::ResetSubdeviceInfos()
{
	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		// subdevice info
		g_SubDevice_info_[i].update_check_deviceinfo = true;  // In case of check, set to true.
		
		g_SubDevice_info_[i].IsBypassSupported = false;
		g_SubDevice_info_[i].curl_responseCode = 0;
		g_SubDevice_info_[i].CheckConnectionStatus = 0;
		g_SubDevice_info_[i].HTTPPort = 0;
		g_SubDevice_info_[i].ConnectionStatus.clear();
		g_SubDevice_info_[i].DeviceModel.clear();
		g_SubDevice_info_[i].DeviceName.clear();
		g_SubDevice_info_[i].ChannelTitle.clear();

		g_SubDevice_info_[i].NetworkInterface.update_check_networkinterface = true;  // In case of check, set to true.
		g_SubDevice_info_[i].NetworkInterface.IPv4Address.clear();
		g_SubDevice_info_[i].NetworkInterface.LinkStatus.clear();
		g_SubDevice_info_[i].NetworkInterface.MACAddress.clear();
		g_SubDevice_info_[i].NetworkInterface.ConnectionStatus.clear();

		// worker
		g_Worker_SubDevice_info_[i].update_check_deviceinfo = true;  // In case of check, set to true.
		g_Worker_SubDevice_info_[i].IsBypassSupported = false;
		g_Worker_SubDevice_info_[i].curl_responseCode = 0;
		g_Worker_SubDevice_info_[i].CheckConnectionStatus = 0;
		g_Worker_SubDevice_info_[i].HTTPPort = 0;
		g_Worker_SubDevice_info_[i].ConnectionStatus.clear();
		g_Worker_SubDevice_info_[i].DeviceModel.clear();
		g_Worker_SubDevice_info_[i].DeviceName.clear();
		g_Worker_SubDevice_info_[i].ChannelTitle.clear();

		g_Worker_SubDevice_info_[i].NetworkInterface.update_check_networkinterface = true;  // In case of check, set to true.
		g_Worker_SubDevice_info_[i].NetworkInterface.IPv4Address.clear();
		g_Worker_SubDevice_info_[i].NetworkInterface.LinkStatus.clear();
		g_Worker_SubDevice_info_[i].NetworkInterface.MACAddress.clear();
		g_Worker_SubDevice_info_[i].NetworkInterface.ConnectionStatus.clear();
	}
}


void sunapi_manager::ResetSubdeviceInfoForChannel(int channel)
{
	// subdevice info
	g_SubDevice_info_[channel].update_check_deviceinfo = true;  // In case of check, set to true.

	g_SubDevice_info_[channel].IsBypassSupported = false;
	g_SubDevice_info_[channel].curl_responseCode = 0;
	g_SubDevice_info_[channel].CheckConnectionStatus = 0;
	g_SubDevice_info_[channel].HTTPPort = 0;
	g_SubDevice_info_[channel].ConnectionStatus.clear();
	g_SubDevice_info_[channel].DeviceModel.clear();
	g_SubDevice_info_[channel].DeviceName.clear();
	g_SubDevice_info_[channel].ChannelTitle.clear();

	g_SubDevice_info_[channel].NetworkInterface.update_check_networkinterface = true;  // In case of check, set to true.
	g_SubDevice_info_[channel].NetworkInterface.IPv4Address.clear();
	g_SubDevice_info_[channel].NetworkInterface.LinkStatus.clear();
	g_SubDevice_info_[channel].NetworkInterface.MACAddress.clear();
	g_SubDevice_info_[channel].NetworkInterface.ConnectionStatus.clear();

	// worker
	g_Worker_SubDevice_info_[channel].update_check_deviceinfo = true;  // In case of check, set to true.
	g_Worker_SubDevice_info_[channel].IsBypassSupported = false;
	g_Worker_SubDevice_info_[channel].curl_responseCode = 0;
	g_Worker_SubDevice_info_[channel].CheckConnectionStatus = 0;
	g_Worker_SubDevice_info_[channel].HTTPPort = 0;
	g_Worker_SubDevice_info_[channel].ConnectionStatus.clear();
	g_Worker_SubDevice_info_[channel].DeviceModel.clear();
	g_Worker_SubDevice_info_[channel].DeviceName.clear();
	g_Worker_SubDevice_info_[channel].ChannelTitle.clear();

	g_Worker_SubDevice_info_[channel].NetworkInterface.update_check_networkinterface = true;  // In case of check, set to true.
	g_Worker_SubDevice_info_[channel].NetworkInterface.IPv4Address.clear();
	g_Worker_SubDevice_info_[channel].NetworkInterface.LinkStatus.clear();
	g_Worker_SubDevice_info_[channel].NetworkInterface.MACAddress.clear();
	g_Worker_SubDevice_info_[channel].NetworkInterface.ConnectionStatus.clear();
}

void sunapi_manager::ResetFirmwareVersionInfos()
{
	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		// subdevice info
		g_Firmware_Ver_info_[i].fw_update_check = true;  // In case of check, set to true.
		g_Firmware_Ver_info_[i].last_fw_update_check = true;  // In case of check, set to true.
		g_Firmware_Ver_info_[i].FirmwareVersion.clear();
		g_Firmware_Ver_info_[i].LatestFirmwareVersion.clear();
		g_Firmware_Ver_info_[i].UpgradeStatus.clear();  // add UpgradeStatus

		// worker
		g_Worker_Firmware_Ver_info_[i].fw_update_check = true;  // In case of check, set to true.
		g_Worker_Firmware_Ver_info_[i].last_fw_update_check = true;  // In case of check, set to true.
		g_Worker_Firmware_Ver_info_[i].FirmwareVersion.clear();
		g_Worker_Firmware_Ver_info_[i].LatestFirmwareVersion.clear();
		g_Worker_Firmware_Ver_info_[i].UpgradeStatus.clear();  // add UpgradeStatus
	}
}

void sunapi_manager::ResetFirmwareVersionInfoForChannel(int channel)
{
	// subdevice info
	g_Firmware_Ver_info_[channel].fw_update_check = true;  // In case of check, set to true.
	g_Firmware_Ver_info_[channel].last_fw_update_check = true;  // In case of check, set to true.
	g_Firmware_Ver_info_[channel].FirmwareVersion.clear();
	g_Firmware_Ver_info_[channel].LatestFirmwareVersion.clear();
	g_Firmware_Ver_info_[channel].UpgradeStatus.clear();  // add UpgradeStatus

	// worker
	g_Worker_Firmware_Ver_info_[channel].fw_update_check = true;  // In case of check, set to true.
	g_Worker_Firmware_Ver_info_[channel].last_fw_update_check = true;  // In case of check, set to true.
	g_Worker_Firmware_Ver_info_[channel].FirmwareVersion.clear();
	g_Worker_Firmware_Ver_info_[channel].LatestFirmwareVersion.clear();
	g_Worker_Firmware_Ver_info_[channel].UpgradeStatus.clear();  // add UpgradeStatus
}
// end of reset
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
// Update
void sunapi_manager::UpdateStorageInfos()
{
	//memcpy(g_Storage_info_, g_Worker_Storage_info_, sizeof(Storage_Infos) * (g_Gateway_info_->maxChannel));

	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		g_Storage_info_[i].CheckConnectionStatus = g_Worker_Storage_info_[i].CheckConnectionStatus;
		g_Storage_info_[i].ConnectionStatus = g_Worker_Storage_info_[i].ConnectionStatus;

		if (g_Worker_Storage_info_[i].CheckConnectionStatus != 0)
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
}

void sunapi_manager::UpdateSubdeviceInfos()
{
	//memcpy(g_SubDevice_info_, g_Worker_SubDevice_info_, sizeof(Channel_Infos) * (g_Gateway_info_->maxChannel));

	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		g_SubDevice_info_[i].CheckConnectionStatus = g_Worker_SubDevice_info_[i].CheckConnectionStatus;
		g_SubDevice_info_[i].ConnectionStatus = g_Worker_SubDevice_info_[i].ConnectionStatus;
		if (g_Worker_SubDevice_info_[i].CheckConnectionStatus != 0)
		{
			g_SubDevice_info_[i].IsBypassSupported = g_Worker_SubDevice_info_[i].IsBypassSupported;
			g_SubDevice_info_[i].HTTPPort = g_Worker_SubDevice_info_[i].HTTPPort;

			if(g_Worker_SubDevice_info_[i].DeviceModel.empty() != true)
				g_SubDevice_info_[i].DeviceModel = g_Worker_SubDevice_info_[i].DeviceModel;

			if(g_Worker_SubDevice_info_[i].DeviceName.empty() != true)
				g_SubDevice_info_[i].DeviceName = g_Worker_SubDevice_info_[i].DeviceName;

			if (g_Worker_SubDevice_info_[i].ChannelTitle.empty() != true)
				g_SubDevice_info_[i].ChannelTitle = g_Worker_SubDevice_info_[i].ChannelTitle;
			printf("UpdateSubdeviceInfos() -> index : %d , DeviceModel : %s, DeviceName : %s\n", i, g_SubDevice_info_[i].DeviceModel.c_str(), g_SubDevice_info_[i].DeviceName.c_str());
		}
	}
}

void sunapi_manager::UpdateSubdeviceNetworkInterfaceInfos()
{
	//memcpy(g_SubDevice_info_, g_Worker_SubDevice_info_, sizeof(Channel_Infos) * (g_Gateway_info_->maxChannel));

	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		g_SubDevice_info_[i].NetworkInterface.ConnectionStatus = g_Worker_SubDevice_info_[i].NetworkInterface.ConnectionStatus;

		if (g_Worker_SubDevice_info_[i].CheckConnectionStatus != 0)
		{
			g_SubDevice_info_[i].NetworkInterface.IPv4Address = g_Worker_SubDevice_info_[i].NetworkInterface.IPv4Address;
			g_SubDevice_info_[i].NetworkInterface.LinkStatus = g_Worker_SubDevice_info_[i].NetworkInterface.LinkStatus;

			if(g_Worker_SubDevice_info_[i].NetworkInterface.MACAddress.empty() != true)
				g_SubDevice_info_[i].NetworkInterface.MACAddress = g_Worker_SubDevice_info_[i].NetworkInterface.MACAddress;			
		}
	}
}

void sunapi_manager::UpdateFirmwareVersionInfos()
{
	// firmware version info
	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		if (g_Worker_SubDevice_info_[i].CheckConnectionStatus != 0)
		{
			if(g_Worker_Firmware_Ver_info_[i].FirmwareVersion.empty() != true)
				g_Firmware_Ver_info_[i].FirmwareVersion = g_Worker_Firmware_Ver_info_[i].FirmwareVersion;

			if(g_Worker_Firmware_Ver_info_[i].LatestFirmwareVersion.empty() != true)
				g_Firmware_Ver_info_[i].LatestFirmwareVersion = g_Worker_Firmware_Ver_info_[i].LatestFirmwareVersion;

			if(g_Worker_Firmware_Ver_info_[i].UpgradeStatus.empty() != true)
				g_Firmware_Ver_info_[i].UpgradeStatus = g_Worker_Firmware_Ver_info_[i].UpgradeStatus;
		}
	}
}
// end of update
////////////////////////////////////////////////////////////////////////////////////////////////////

// for temp
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
	time_t startTimeOfCURL_Process = time(NULL);
	std::cout << "sunapi_manager::CURL_Process() -> timeout : " << timeout << ", Start ... time : " << (long int)startTimeOfCURL_Process << std::endl;

	ChunkStruct chunk;

	chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */

	CURL* curl_handle;
	CURLcode res;

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();

#if 1  // 2019.09.05 hwanjang - sunapi debugging
	printf("sunapi_manager::curl_process() -> request : %s , pw : %s\n", strRequset.c_str(), strPW.c_str());
#endif

	if (curl_handle)
	{
#if 1
		struct curl_slist* headers = NULL;
		//headers = curl_slist_append(headers, "cache-control:no-cache");
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
			printf("sunapi_manager::curl_process() -> curl_easy_perform() failed .. request : %s, code : %d,  %s\n", strRequset.c_str(), res, curl_easy_strerror(res));
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
			if((long)chunk.size != 0) // 200 ok , 301 moved , 404 error
			{		
				// OK
				*strResult = chunk.memory;

				printf("sunapi_manager::CURL_Process() -> strResult : \n%s\n", chunk.memory);
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

	time_t endTimeOfCURL_Process = time(NULL);
	std::cout << "sunapi_manager::CURL_Process() -> timeout : " << timeout << ", End ... time : " << (long int)endTimeOfCURL_Process << " -> " <<
		(long int)(endTimeOfCURL_Process - startTimeOfCURL_Process) << std::endl;

	return res;
}

bool sunapi_manager::ByPassSUNAPI(int channel, bool json_mode, const std::string IPAddress, const std::string devicePW, const std::string bypassURI, std::string* strResult, CURLcode* resCode)
{
	if (g_SubDevice_info_[channel].IsBypassSupported != true)
	{
		printf("sunapi_manager::ByPassSUNAPI() -> channel : %d, IsBypassSupported ... false --> return !!!\n", channel);

		*resCode = CURLE_UNSUPPORTED_PROTOCOL;

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

	*resCode = res;

	if (res == CURLE_OK)
	{
		if (strSUNAPIResult.empty())
		{
			printf("[hwanjang] sunapi_manager::ByPassSUNAPI() -> result string is empty !!!\n");
			return false;
		}
		else
		{
			*strResult = strSUNAPIResult;
		}
	}
	else if (res == CURLE_OPERATION_TIMEDOUT)
	{
		// timeout
		printf("failed  ... sunapi_manager::ByPassSUNAPI() -> CURLE_OPERATION_TIMEDOUT ... request URL : %s\n", request.c_str());
		return false;
	}
	else
	{
		printf("failed  ... sunapi_manager::ByPassSUNAPI() -> %d .. request URL : %s\n", res, request.c_str());		
		return false;
	}

	return true;
}

int sunapi_manager::GetMaxChannelByAttribute(std::string strGatewayIP, std::string strID_PW)
{
	std::string _gatewayIP = strGatewayIP;
	std::string _idPassword = strID_PW;

	int max_channel = 0;

	std::string request;

	request = "http://";
	request.append(_gatewayIP);
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

	res = CURL_Process(json_mode, ssl_opt, CURL_TIMEOUT, request, _idPassword, &strSUNAPIResult);

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

			if (max_channel > 0)
			{
				g_Gateway_info_->maxChannel = max_channel;
			}
			else
			{
				printf("GetMaxChannelByAttribute() -> max channel : %d\n", max_channel);
				printf("data : %s\n", strSUNAPIResult.c_str());
			}
		}
	}
	else
	{
		printf("GetMaxChannelByAttribute() -->  ... res : %d!!!\n", res);
		return 0;
	}

	return max_channel;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// using SUNAPI Command Function
int sunapi_manager::GetMaxChannel()
{
	if (g_Gateway_info_)
		return g_Gateway_info_->maxChannel;
	else
		return 0;
}

int sunapi_manager::GetConnectionCount()
{
	// TODO: 여기에 구현 코드 추가.
	return g_ConnectionCnt;
}

// status of regiestered sub device 
bool sunapi_manager::GetRegiesteredCameraStatus(const std::string deviceIP, const std::string devicePW, CURLcode* resCode)
{
	time_t update_time = time(NULL);

	if ((update_time - g_UpdateTimeOfRegistered) > GET_WAIT_TIMEOUT)
	{
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

		*resCode = res;

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

			//printf("sunapi_manager::GetRegiesteredCameraStatus() -> strSUNAPIResult : \n%s\n", strSUNAPIResult.c_str());

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
			int i = 0, j = 0;
			size_t index;
			char* charStatus, * charIPAddress, * charModel, *charTitle;
			bool* IsBypassSupported;
			json_t* obj;

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

						g_Worker_SubDevice_info_[index].CheckConnectionStatus = 1;  // Success -> Connect

						g_Worker_SubDevice_info_[index].ConnectionStatus = "on";

						printf("GetRegiesteredCameraStatus() -> channel : %d , connect ... g_RegisteredCameraCnt : %d\n", index, g_RegisteredCameraCnt);
					}
					else if (strncmp("ConnectFail", charStatus, 11) == 0)
					{
						g_RegisteredCameraCnt++;

						g_Worker_SubDevice_info_[index].CheckConnectionStatus = 2;  // ConnectFail

						g_Worker_SubDevice_info_[index].ConnectionStatus = "fail";

						printf("GetRegiesteredCameraStatus() -> channel : %d , ConnectFail ... g_RegisteredCameraCnt : %d\n", index, g_RegisteredCameraCnt);
					}
					else if (strncmp("AuthFail", charStatus, 8) == 0)
					{
						g_RegisteredCameraCnt++;

						g_Worker_SubDevice_info_[index].CheckConnectionStatus = 4;  // AuthFail

						g_Worker_SubDevice_info_[index].ConnectionStatus = "authError";

						printf("GetRegiesteredCameraStatus() -> channel : %d , ConnectFail ... g_RegisteredCameraCnt : %d\n", index, g_RegisteredCameraCnt);

					}
					else if (strncmp("MaxUserLimit", charStatus, 12) == 0)
					{
						g_RegisteredCameraCnt++;

						g_Worker_SubDevice_info_[index].CheckConnectionStatus = 3;  // timeout

						g_Worker_SubDevice_info_[index].ConnectionStatus = "timeout";

						printf("GetRegiesteredCameraStatus() -> channel : %d , ConnectFail ... g_RegisteredCameraCnt : %d\n", index, g_RegisteredCameraCnt);

					}
					else
					{
						//printf("[hwanjang] GetRegiesteredCameraStatus -> index : %d , disconnected ... \n", index);

						// reset channel
						ResetStorageInfoForChannel(index);
						ResetSubdeviceInfoForChannel(index);
						ResetFirmwareVersionInfoForChannel(index);

					}

					if (g_Worker_SubDevice_info_[index].CheckConnectionStatus != 0)  // Success , ConnectFail , timeout , AuthFail
					{
						webPort = -1;
						result = json_unpack(obj, "{s:s, s:s, s:s, s:i, s:b}", "Model", &charModel, "Title", &charTitle, "IPAddress", &charIPAddress, "HTTPPort", &webPort, "IsBypassSupported", &IsBypassSupported);
						if (result)
						{
							printf("[hwanjang] Error !! GetRegiesteredCameraStatus -> 2. json_unpack fail .. index : %d !!!\n", index);
							std::cout << "obj string : " << std::endl << json_dumps(obj, 0) << std::endl;

							// IPAddress
							result = json_unpack(obj, "{s:s}", "IPAddress", &charIPAddress);
							if (result)
							{
								printf("[hwanjang] Error !! GetRegiesteredCameraStatus -> 2. json_unpack fail .. index : %d , IPAddress\n", index);
							}
							else
							{
								g_Worker_SubDevice_info_[index].NetworkInterface.IPv4Address = charIPAddress;
							}

							// HTTPPort
							result = json_unpack(obj, "{s:i}", "HTTPPort", &webPort);
							if (result)
							{
								printf("[hwanjang] Error !! GetRegiesteredCameraStatus -> 2. json_unpack fail .. index : %d , HTTPPort\n", index);
							}
							else
							{
								g_Worker_SubDevice_info_[index].HTTPPort = webPort;
							}

							// Model
							result = json_unpack(obj, "{s:s}", "Model", &charModel);
							if (result)
							{
								printf("[hwanjang] Error !! GetRegiesteredCameraStatus -> 2. json_unpack fail .. index : %d , Model\n", index);
							}
							else
							{
								g_Worker_SubDevice_info_[index].DeviceModel = charModel;
							}

							// IsBypassSupported
							result = json_unpack(obj, "{s:b}", "IsBypassSupported", &IsBypassSupported);
							if (result)
							{
								printf("[hwanjang] Error !! GetRegiesteredCameraStatus -> 2. json_unpack fail .. index : %d , IsBypassSupported\n", index);
							}
							else
							{
								g_Worker_SubDevice_info_[index].IsBypassSupported = IsBypassSupported;
							}

							// Title - channel name
							result = json_unpack(obj, "{s:s}", "Title", &charTitle);
							if (result)
							{
								printf("[hwanjang] Error !! GetRegiesteredCameraStatus -> 2. json_unpack fail .. index : %d , Title\n", index);
							}
							else
							{
								g_Worker_SubDevice_info_[index].ChannelTitle = charTitle;
							}
						}
						else
						{
							g_Worker_SubDevice_info_[index].NetworkInterface.IPv4Address = charIPAddress;
							g_Worker_SubDevice_info_[index].ChannelTitle = charTitle;
							g_Worker_SubDevice_info_[index].HTTPPort = webPort;
							g_Worker_SubDevice_info_[index].DeviceModel = charModel;
							g_Worker_SubDevice_info_[index].IsBypassSupported = IsBypassSupported;
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
		UpdateSubdeviceNetworkInterfaceInfos();

		time_t end_time = time(NULL);
		printf("GetRegiesteredCameraStatus() -> End ... g_ConnectionCnt : %d , time : %ld , diff : %ld\n",
			g_ConnectionCnt, (long int)end_time, (long int)(end_time - g_UpdateTimeOfRegistered));
	}
	else
	{
		printf("GetRegiesteredCameraStatus() -> check %ld ... Skip !!!!!\n", (long int)(update_time - g_UpdateTimeOfRegistered));
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// interface for dashboard
void sunapi_manager::GetDataForDashboardAPI(const std::string& strTopic, const std::string& strPayload)
{
	json_error_t error_check;
	json_t* json_strRoot = json_loads(strPayload.c_str(), 0, &error_check);

	if (!json_strRoot)
	{
		printf("GetDataForDashboardAPI() -> json_loads fail .. strPayload : \n%s\n", strPayload.c_str());

		return;
	}
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
		CURLcode resCode;
		bool result = GetRegiesteredCameraStatus(g_StrDeviceIP, g_StrDevicePW, &resCode);

		if (!result)
		{
			printf("failed to get GetRegiesteredCameraStatus ... 1");

			result = GetRegiesteredCameraStatus(g_StrDeviceIP, g_StrDevicePW, &resCode);
			if (!result)
			{
				std::string strError;

				printf("failed to get GetRegiesteredCameraStatus ... 2 ... return !!");

				if ((resCode == CURLE_LOGIN_DENIED) || (resCode == CURLE_AUTH_ERROR))
				{
					// authfail
					strError = "authError";					
				}
				else if (resCode == CURLE_OPERATION_TIMEDOUT)
				{
					// timeout
					strError = "timeout";
				}
				else {
					// another error		
					//strError = "unknown";
					strError = curl_easy_strerror(resCode);
				}

				SendErrorResponseForGateway(strTopic, json_strRoot, strError);

				json_decref(json_strRoot);
				return;
			}
		}

		if (strncmp("dashboard", charCommand, 9) == 0)
		{
			GetDashboardView(strTopic, json_strRoot);
		}
		else if (strncmp("deviceinfo", charCommand, 10) == 0)
		{
			GetDeviceInfoView(strTopic, json_strRoot);
		}
		else if (strncmp("firmware", charCommand, 8) == 0)
		{
			if (strncmp("view", charType, 4) == 0)
			{
				GetFirmwareVersionInfoView(strTopic, json_strRoot);
			}
			else if (strncmp("update", charType, 6) == 0)
			{
				// not yet
				UpdateFirmwareOfSubdevice(strTopic, json_strRoot);
			}
		}
	}

	json_decref(json_strRoot);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// 1. interface for dashboard - storage
void sunapi_manager::GetDashboardView(const std::string& strTopic, json_t* json_strRoot)
{
	printf("GetDashboardView() -> Max Channel : %d ... Dashboard Info ...\n", g_Gateway_info_->maxChannel);

	time_t update_time = time(NULL);

	if ((update_time - g_UpdateTimeOfStorageStatus) > GET_WAIT_TIMEOUT)
	{
		GetStorageStatusOfSubdevices();

		sleep_for(std::chrono::milliseconds(1 * 1000));  // after 1 sec
	}
	else
	{
		printf("check %ld ... Skip !!!!!\n", (long int)(update_time - g_UpdateTimeOfStorageStatus));

		// skip to check
		Set_update_checkForStorageInfo();
	}

	char* strCommand, * strType, * strView, * strTid;
	int result = json_unpack(json_strRoot, "{s:s, s:s, s:s, s:s}", "command", &strCommand, "type", &strType, "view", &strView, "tid", &strTid);

	if (result)
	{
		printf("fail ... json_unpack() ...\n");
	}
	else
	{
		ThreadStartSendResponseForDashboardView(strTopic, strCommand, strType, strView, strTid);
		//SendResponseForDashboardView(strTopic, strTid);
	}
}

// 2. interface for deviceinfo
void sunapi_manager::GetDeviceInfoView(const std::string& strTopic, json_t* json_strRoot)
{
	printf("GetDeviceInfoView() -> Max Channel : %d ... Device Info ...\n", g_Gateway_info_->maxChannel);

#if 0
	GetDataForSubdeviceInfo();

	sleep_for(std::chrono::milliseconds(1 * 100));
#else

	// update gateway info
	if((g_Gateway_info_->MACAddress == "unknown") || g_Gateway_info_->MACAddress.empty())
	{
		bool ret = GetGatewayInfo(g_StrDeviceIP, g_StrDevicePW);

		// error ??
		if(!ret)
		{
			printf("[hwanjang] sunapi_manager::GetDeviceInfoView() -> Failed to update gateway info !!!\n");
		}
	}

	// add Device Name
	GetDeviceInfoOfSubdevices();

	//sleep_for(std::chrono::milliseconds(10));

	//GetMacAddressOfSubdevices();

	sleep_for(std::chrono::milliseconds(1 * 1000));  // after 1 sec

#endif	

	char* strCommand, * strType, * strView, * strTid;
	bool result = json_unpack(json_strRoot, "{s:s, s:s, s:s, s:s}", "command", &strCommand, "type", &strType, "view", &strView, "tid", &strTid);

	if (result)
	{
		printf("fail ... json_unpack() ...\n");
	}
	else
	{
		ThreadStartSendResponseForDeviceInfoView(strTopic, strCommand, strType, strView, strTid);

		//SendResponseForDeviceInfoView(strTopic, strTid);
	}
}

// 3. interface for firmware version info
void sunapi_manager::GetFirmwareVersionInfoView(const std::string& strTopic, json_t* json_strRoot)
{
	printf("GetFirmwareVersionInfoView() -> Max Channel : %d ... Firmware Version Info ...\n", g_Gateway_info_->maxChannel);

	time_t update_time = time(NULL);

	if ((update_time - g_UpdateTimeForFirmwareVersionOfSubdevices) > GET_WAIT_TIMEOUT)
	{
		GetDataForFirmwareVersionInfo();

		sleep_for(std::chrono::milliseconds(1 * 1000));  // after 1 sec
	}
	else
	{
		printf("check %ld ... Skip !!!!!\n", (long int)(update_time - g_UpdateTimeForFirmwareVersionOfSubdevices));

		// skip to check
		Set_update_check_Firmware_Ver_info_ForFirmwareVersion();
	}

	char* strCommand, * strType, * strView, * strTid;
	int result = json_unpack(json_strRoot, "{s:s, s:s, s:s, s:s}", "command", &strCommand, "type", &strType, "view", &strView, "tid", &strTid);

	if (result)
	{
		printf("GetFirmwareVersionInfoView() -> fail ... json_unpack() ...\n");
	}
	else
	{
		ThreadStartSendResponseForFirmwareView(strTopic, strCommand, strType, strView, strTid);
		//SendResponseForFirmwareView(strTopic, strTid);
	}
}

// 4. interface for firmware update of subdevice
void sunapi_manager::UpdateFirmwareOfSubdevice(const std::string& strTopic, json_t* json_strRoot)
{
	char* strCommand, * strType, * strView, * strTid;
	int result = json_unpack(json_strRoot, "{s:s, s:s, s:s, s:s}", "command", &strCommand, "type", &strType, "view", &strView, "tid", &strTid);

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
				
		size_t index;
		int channel;
		json_t* json_sub;

		std::vector<int> updateChannel;
		std::string strUpdateChannel;
		strUpdateChannel.clear();

		json_array_foreach(json_subDevices, index, json_sub)
		{
			if (!strUpdateChannel.empty())
			{
				strUpdateChannel.append(",");
			}

			result = json_unpack(json_sub, "i", &channel);

			if (result)
			{
				printf("[hwanjang] Error !! UpdateFirmwareOfSubdevice() -> 1. json_unpack fail .. Status ..index : %d !!!\n", index);
			}
			else
			{
				printf("Update ... channel : %d \n", channel);

				strUpdateChannel.append(std::to_string(channel));


				updateChannel.push_back(channel);

				// send update command to cloud gateway ...
			}
		}

		// Send to gateway
		std::string request;

		request = "http://";
		request.append(g_StrDeviceIP);

		request.append("/stw-cgi/media.cgi?msubmenu=cameraupgrade&action=set&Mode=Start&ChannelIDList=");
		request.append(strUpdateChannel);

		//printf("private ID:PW = %s\n",userpw.c_str());

#if 1  // 2019.09.05 hwanjang - sunapi debugging
		printf("UpdateFirmwareOfSubdevice() -> SUNAPI Request : %s\n", request.c_str());
#endif
		int resCode = 250;  // response code of agent
		int rawCode = 200;

		CURLcode res;
		std::string strSUNAPIResult;
		bool json_mode = true;
		bool ssl_opt = false;

		res = CURL_Process(json_mode, ssl_opt, CURL_TIMEOUT, request, g_StrDevicePW, &strSUNAPIResult);

		if (res == CURLE_OK)
		{
			if (strSUNAPIResult.empty())
			{
				printf("[hwanjang] UpdateFirmwareOfSubdevice() -> strSUNAPIResult is empty !!!\n");
			}
			else
			{
				// attributes.cgi -> The following CGI command returns device attributes and CGI information in XML format.
				printf("UpdateFirmwareOfSubdevice() -> strSUNAPIResult : %s\n", strSUNAPIResult.c_str());
			}
			rawCode = 200;
		}
		else
		{
			printf("error UpdateFirmwareOfSubdevice() ->  error CURL_Process() ... res : %d!!!\n", res);
			rawCode = res;
		}

		SendResponseForUpdateFirmware(strTopic, strCommand, strType, strView, strTid, resCode, rawCode, updateChannel);
	}
}

// Send error response for gateway - authfail , timeout , unknown ...
void sunapi_manager::SendErrorResponseForGateway(const std::string& strTopic, json_t* json_strRoot, std::string strError)
{
	printf("SendErrorResponseForGateway() -> gateway fail ...\n");

	// sub of message
	std::string strDeviceType = HBIRD_DEVICE_TYPE;

	char* strCommand, * strType, * strView, * strTid, *strVersion;
	int result = json_unpack(json_strRoot, "{s:s, s:s, s:s, s:s, s:s}", "command", &strCommand, "type", &strType, "view", &strView, "tid", &strTid, "version", &strVersion);

	if (result)
	{
		printf("fail ... json_unpack() ...\n");
	}
	else
	{
		std::string strMQTTMsg;

		json_t* main_ResponseMsg, *sub_Msg;

		main_ResponseMsg = json_object();
		sub_Msg = json_object();

		json_object_set(main_ResponseMsg, "command", json_string(strCommand));
		json_object_set(main_ResponseMsg, "type", json_string(strType));
		json_object_set(main_ResponseMsg, "view", json_string(strView));
		json_object_set(main_ResponseMsg, "tid", json_string(strTid));
		json_object_set(main_ResponseMsg, "version", json_string(strVersion));

		// sub of message
		json_object_set(sub_Msg, "deviceId", json_string(g_Device_id.c_str()));
		json_object_set(sub_Msg, "deviceType", json_string(strDeviceType.c_str()));
		json_object_set(sub_Msg, "connstcion", json_string(strError.c_str()));

		json_object_set(main_ResponseMsg, "message", sub_Msg);

		strMQTTMsg = json_dumps(main_ResponseMsg, 0);

		printf("[hwanjang APIManager::CommandCheckPassword()() response ---> size : %lu, send message : \n%s\n", strMQTTMsg.size(), strMQTTMsg.c_str());

		observerForHbirdManager->SendResponseToPeer(strTopic, strMQTTMsg);
	}
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
			g_Worker_Storage_info_[channel].das_presence = false;
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
			g_Worker_Storage_info_[channel].nas_presence = 0;
		}
		else
		{
			printf("strByPassResult : %s\n", strByPassResult.c_str());
			if ((strByPassResult.find("True")) || (strByPassResult.find("true")))
			{
				// SystemEvent.NASConnect=True
				g_Worker_Storage_info_[channel].nas_presence = 0;
			}
			else
			{
				// SystemEvent.NASConnect=False
				g_Worker_Storage_info_[channel].nas_presence = 0;
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
#if 0  // not use
	g_UpdateTimeOfStorageStatus = time(NULL);

	int maxChannel = g_Gateway_info_->maxChannel;

	std::string deviceIP = g_StrDeviceIP;
	std::string devicePW = g_StrDevicePW;

	// for test
	//max_ch = 10; 

	for (int i = 0; i < maxChannel; i++)
	{
		//sleep_for(std::chrono::milliseconds(10));

		printf("GetStoragePresenceOfSubdevices() -> index : %d ...\n", i);
		
		ThreadStartForStoragePresence(i, deviceIP, devicePW);

		sleep_for(std::chrono::milliseconds(10));
	}
#endif
}

int sunapi_manager::ThreadStartForStoragePresence(int channel, const std::string deviceIP, const std::string devicePW)
{
	std::thread thread_function([=] { thread_function_for_storage_presence(channel, deviceIP, devicePW); });
	thread_function.detach();

	return 0;
}

void sunapi_manager::thread_function_for_storage_presence(int channel, const std::string deviceIP, const std::string devicePW)
{
	// reset timeout
	g_Worker_Storage_info_[channel].curl_responseCode = CURLE_OPERATION_TIMEDOUT;

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

	int maxChannel = g_Gateway_info_->maxChannel;

	std::string deviceIP = g_StrDeviceIP;
	std::string devicePW = g_StrDevicePW;

	for (int i = 0; i < maxChannel; i++)
	{
		//sleep_for(std::chrono::milliseconds(10));
#if 1
		if ( (g_SubDevice_info_[i].CheckConnectionStatus == 1)     // connection
			|| (g_SubDevice_info_[i].CheckConnectionStatus == 3) ) // timeout
		{
			ThreadStartForStorageStatus(i, deviceIP, devicePW);
			
			sleep_for(std::chrono::milliseconds(SLEEP_TIME));
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

	g_Worker_Storage_info_[channel].update_check = false;

	// reset timeout
	g_Worker_Storage_info_[channel].curl_responseCode = CURLE_OPERATION_TIMEDOUT;
	g_Worker_Storage_info_[channel].ConnectionStatus = "timeout";

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

	g_Worker_Storage_info_[channel].curl_responseCode = resCode;

	if (result)
	{
		g_Worker_Storage_info_[channel].ConnectionStatus = "on";

		// 
		if (strByPassResult.empty())
		{
			// fail
			printf("channel : %d, strByPassResult is empty !!! \n", channel);

			// DAS			
			g_Worker_Storage_info_[channel].das_status = "unknown";
			// NAS			
			g_Worker_Storage_info_[channel].nas_status = "unknown";
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

				// DAS			
				g_Worker_Storage_info_[channel].das_status = "unknown";
				// NAS			
				g_Worker_Storage_info_[channel].nas_status = "unknown";
			}
			else
			{
				json_t* json_array = json_object_get(json_strRoot, "Storages");

				if (!json_array)
				{
					printf("NG1 ... thread_function_for_storage_status() -> channel : %d, json_is wrong .. strByPassResult : \n%s\n",
						channel, strByPassResult.c_str());

					// DAS			
					g_Worker_Storage_info_[channel].das_status = "unknown";
					// NAS			
					g_Worker_Storage_info_[channel].nas_status = "unknown";
				}
				else
				{
					int ret;
					int bEnable;
					size_t index;
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
									g_Worker_Storage_info_[channel].das_status.clear();
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
									g_Worker_Storage_info_[channel].nas_status.clear();
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
	else  // fail
	{
		if ((resCode == CURLE_LOGIN_DENIED) || (resCode == CURLE_AUTH_ERROR))
		{
			// authfail
			g_Worker_Storage_info_[channel].ConnectionStatus = "authError";
		}
		else if (resCode == CURLE_OPERATION_TIMEDOUT)
		{
			// timeout
		}
		else {
			// another error			
			printf("thread_function_for_storage_status() -> CURL_Process fail .. strError : %s\n", curl_easy_strerror(resCode));
			g_Worker_Storage_info_[channel].ConnectionStatus = "unknown";
		}
	}

	g_Worker_Storage_info_[channel].update_check = true;
}

// 1. send data for dashboard view
void sunapi_manager::Set_update_checkForStorageInfo()
{
	// for skip to check
	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		g_Worker_Storage_info_[i].update_check = true;
	}
}

void sunapi_manager::Reset_update_checkForStorageInfo()
{
	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		g_Worker_Storage_info_[i].update_check = false;
	}
}

int sunapi_manager::ThreadStartSendResponseForDashboardView(const std::string strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
	int maxChannel = g_Gateway_info_->maxChannel;
	std::thread thread_function([=] { thread_function_for_send_response_for_dashboard(maxChannel, strTopic, strCommand, strType, strView, strTid); });
	thread_function.detach();

	return 0;
}

void sunapi_manager::thread_function_for_send_response_for_dashboard(int maxChannel, const std::string strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
	printf("\nthread_function_for_send_response_for_dashboard() -> Start .... \n ");

	time_t start_t = time(NULL);
	time_t quit_t;

	int i = 0;
	while (1)
	{
		for (i = 0; i < maxChannel; i++)
		{
			if ((g_SubDevice_info_[i].CheckConnectionStatus == 1)     // connection
				|| (g_SubDevice_info_[i].CheckConnectionStatus == 3)) // timeout
			{
				if (g_Worker_Storage_info_[i].update_check != true)
				{
					printf("\nthread_function_for_send_response_for_dashboard() -> i : %d  , update_check : %d --> break !!!\n", i , g_Worker_Storage_info_[i].update_check);
					break;
				}
			}

			if ((maxChannel - 1) == i)
			{
				printf("\n#############################################################################\n");
				printf("\n thread_function_for_send_response_for_dashboard() -> Start .... \n ");
				UpdateStorageInfos();

				SendResponseForDashboardView(strTopic, strCommand, strType, strView, strTid);

				Set_update_checkForStorageInfo();

				time_t end_time = time(NULL);
				std::cout << "thread_function_for_send_response_for_dashboard() -> End ... time : " << (long int)end_time << " , diff : "
					<< (long int)(end_time - g_UpdateTimeOfStorageStatus) << std::endl;

				return;
			}

		}

		printf("\nthread_function_for_send_response_for_dashboard() -> i : %d\n", i);

		quit_t = time(NULL);

		if ((quit_t - start_t) > GET_WAIT_TIMEOUT)
		{
			printf("\n#############################################################################\n");
			printf("\n thread_function_for_send_response_for_dashboard() -> timeover ... quit !!! \n ");

			UpdateStorageInfos();

			SendResponseForDashboardView(strTopic, strCommand, strType, strView, strTid);

			Set_update_checkForStorageInfo();

			std::cout << "thread_function_for_send_response_for_dashboard() -> End ... time : " << (long int)quit_t << " , diff : "
				<< (long int)(quit_t - start_t) << std::endl;

			return;
		}

		sleep_for(std::chrono::milliseconds(1));
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Send storage info for dashboard 
void sunapi_manager::SendResponseForDashboardView(const std::string& strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
	std::string strVersion = "1.5";
	// sub of message
	std::string strDeviceType = HBIRD_DEVICE_TYPE;

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

	std::string strDasStatus, strNasStatus;

	for (i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		if (g_SubDevice_info_[i].CheckConnectionStatus != 0)
		{
			strDasStatus.clear();
			strNasStatus.clear();

			// sub_SubdeviceMsg["connection"]
			memset(charCh, 0, sizeof(charCh));
			//sprintf(charCh, "%d", i);  // int -> char*
			sprintf_s(charCh, sizeof(charCh), "%d", i);  // int -> char*

			if (!g_SubDevice_info_[i].ConnectionStatus.empty())
				json_object_set_new(sub_ConnectionMsg, charCh, json_string(g_SubDevice_info_[i].ConnectionStatus.c_str()));
			else
				json_object_set_new(sub_ConnectionMsg, charCh, json_string("unknown"));

			connectionMsgCnt++;

#if 0
			// storage of subdevice
			// DAS
			if (g_Storage_info_[i].das_enable)
			{
#if 0 // old
				if ((g_Storage_info_[i].das_status.find("timeout") != std::string::npos)
					|| (g_Storage_info_[i].das_status.find("authfail") != std::string::npos))
				{
					strStatus = g_Storage_info_[i].das_status;
				}
				else
				{
					if (g_Storage_info_[i].das_status.empty() != true)
					{
						memset(charCh, 0, sizeof(charCh));
						sprintf(charCh, "%d.DAS", i);  // int -> char*
						json_object_set_new(sub_StorageMsg, charCh, json_string(g_Storage_info_[i].das_status.c_str()));
						storageMsgCnt++;
					}
				}
#else
				if (g_Storage_info_[i].ConnectionStatus.find("on") != std::string::npos)
				{
					if (g_Storage_info_[i].das_status.empty() != true)
					{
						memset(charCh, 0, sizeof(charCh));
						sprintf(charCh, "%d.DAS", i);  // int -> char*
						json_object_set_new(sub_StorageMsg, charCh, json_string(g_Storage_info_[i].das_status.c_str()));
						storageMsgCnt++;
					}
					else
					{
						// ??
						// das_enable = true -> das_status is empty ?????
					}
				}
				else if (!g_Storage_info_[i].ConnectionStatus.empty())
				{
					strStatus = g_Storage_info_[i].ConnectionStatus;
				}
#endif
			}

			// NAS
			if (g_Storage_info_[i].nas_enable)
			{
				if ((g_Storage_info_[i].nas_status.find("timeout") != std::string::npos)
					|| (g_Storage_info_[i].nas_status.find("authfail") != std::string::npos))
				{
					strStatus = g_Storage_info_[i].nas_status;
				}
				else
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
#else
			// storage of subdevice
			// DAS
			if (g_Storage_info_[i].das_enable)
			{
				memset(charCh, 0, sizeof(charCh));
				//sprintf(charCh, "%d.DAS", i);  // int -> char*
				sprintf_s(charCh, sizeof(charCh), "%d.DAS", i);  // int -> char*

				if (g_Storage_info_[i].das_status.empty() != true)
				{
					json_object_set_new(sub_StorageMsg, charCh, json_string(g_Storage_info_[i].das_status.c_str()));					
				}
				else
				{
					// das_enable = true -> das_status is empty ?????
					json_object_set_new(sub_StorageMsg, charCh, json_string("unknown"));
				}
				storageMsgCnt++;
			}

			// NAS
			if (g_Storage_info_[i].nas_enable)
			{
				memset(charCh, 0, sizeof(charCh));
				//sprintf(charCh, "%d.NAS", i);  // int -> char*
				sprintf_s(charCh, sizeof(charCh), "%d.NAS", i);  // int -> char*

				if (g_Storage_info_[i].nas_status.empty() != true)
				{

					json_object_set_new(sub_StorageMsg, charCh, json_string(g_Storage_info_[i].nas_status.c_str()));

				}
				else
				{
					// nas_enable = true -> nas_status is empty ?????
					json_object_set_new(sub_StorageMsg, charCh, json_string("unknown"));
				}
				storageMsgCnt++;
			}
#endif
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

	json_object_set(sub_Msg, "subdevice", sub_SubdeviceMsg);

	json_object_set(sub_Msg, "connection", json_string(g_Gateway_info_->ConnectionStatus.c_str()));

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

bool sunapi_manager::GetDeviceInfoOfSubdevices()
{
	time_t update_time = time(NULL);

	if ((update_time - g_UpdateTimeOfDeviceInfo) > GET_WAIT_TIMEOUT)
	{
		g_Sub_device_info_Cnt = 0;

		//  - LinkStatus , IPv4Address , MACAddress

		g_UpdateTimeOfDeviceInfo = time(NULL);

		int maxChannel = g_Gateway_info_->maxChannel;

		//std::cout << "GetDeviceInfoOfSubdevices() -> Start ... maxChannel : " << maxChannel << ", time : " << g_UpdateTimeOfDeviceInfo << std::endl;
		printf("GetDeviceInfoOfSubdevices() -> Start ... maxChannel : %d , time : %lld\n", maxChannel, g_UpdateTimeOfDeviceInfo);

		std::string deviceIP = g_StrDeviceIP;
		std::string devicePW = g_StrDevicePW;

		for (int i = 0; i < maxChannel; i++)
		{
			if ((g_SubDevice_info_[i].CheckConnectionStatus == 1)     // connection
				|| (g_SubDevice_info_[i].CheckConnectionStatus == 3)) // timeout
			{
				ThreadStartForSubDeviceInfo(i, deviceIP, devicePW);

				sleep_for(std::chrono::milliseconds(SLEEP_TIME));
			}
		}
	}
	else
	{
		printf("GetDeviceInfoOfSubdevices()-> check %ld ... Skip !!!!!\n", (long int)(update_time - g_UpdateTimeOfNetworkInterface));

		// skip to check
		Set_update_checkForDeviceInfo();
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

	g_Worker_SubDevice_info_[channel].update_check_deviceinfo = false;
	g_Worker_Firmware_Ver_info_[channel].fw_update_check = false;

	// reset timeout
	g_Worker_SubDevice_info_[channel].curl_responseCode = CURLE_OPERATION_TIMEDOUT;
	g_Worker_SubDevice_info_[channel].ConnectionStatus = "timeout";

	bool result = false;

	std::string strByPassResult;

	std::string strByPassURI = "/stw-cgi/system.cgi?msubmenu=deviceinfo&action=view";

	bool json_mode = true;
	CURLcode resCode;
	result = ByPassSUNAPI(channel, json_mode, deviceIP, devicePW, strByPassURI, &strByPassResult, &resCode);

	g_Worker_SubDevice_info_[channel].curl_responseCode = resCode;

	if (result)
	{
		g_Worker_SubDevice_info_[channel].ConnectionStatus = "on";

		if (strByPassResult.empty())
		{
			printf("[hwanjang] strByPassResult is empty !!!\n");
		}
		else
		{
			json_error_t error_check;
			json_t* json_strRoot = json_loads(strByPassResult.c_str(), 0, &error_check);

			if (!json_strRoot)
			{
				fprintf(stderr, "error : root\n");
				fprintf(stderr, "error : on line %d: %s\n", error_check.line, error_check.text);

				printf("thread_function_for_subdevice_info() -> channel : %d , strSUNAPIResult : \n%s\n", 
							channel, strByPassResult.c_str());
			}
			else
			{
				// get Model
				char* charModel;
				int result = json_unpack(json_strRoot, "{s:s}", "Model", &charModel);

				if (result)
				{
					printf("[hwanjang] Error !! thread_function_for_subdevice_info() -> 1. json_unpack fail .. Model ...\n");
					printf("thread_function_for_subdevice_info() -> channel : %d , strSUNAPIResult : \n%s\n", 
								channel, strByPassResult.c_str());
				}
				else
				{
					g_Worker_SubDevice_info_[channel].DeviceModel = charModel;
				}

				// get DeviceName
				char* charDeviceName;
				result = json_unpack(json_strRoot, "{s:s}", "DeviceName", &charDeviceName);

				if (result)
				{
					printf("[hwanjang] Error !! thread_function_for_subdevice_info() -> 4. json_unpack fail .. DeviceName ...\n");
					printf("thread_function_for_subdevice_info() -> channel : %d , strSUNAPIResult : \n%s\n",
						channel, strByPassResult.c_str());
				}
				else
				{
					g_Worker_SubDevice_info_[channel].DeviceName = charDeviceName;
				}

				// get FirmwareVersion
				char* charFirmwareVersion;
				result = json_unpack(json_strRoot, "{s:s}", "FirmwareVersion", &charFirmwareVersion);

				if (result)
				{
					printf("[hwanjang] Error !! thread_function_for_subdevice_info() -> 2. json_unpack fail .. FirmwareVersion ...\n");
					printf("thread_function_for_subdevice_info() -> channel : %d , strSUNAPIResult : \n%s\n", 
							channel, strByPassResult.c_str()); 
				}
				else
				{
					g_Worker_Firmware_Ver_info_[channel].FirmwareVersion = charFirmwareVersion;
				}

#if 1
				// get MACAddress
				char* charMACAddress;
				result = json_unpack(json_strRoot, "{s:s}", "ConnectedMACAddress", &charMACAddress);

				if (result)
				{
					printf("[hwanjang] Error !! thread_function_for_subdevice_info() -> 3. json_unpack fail .. ConnectedMACAddress ...\n");
					printf("thread_function_for_subdevice_info() -> channel : %d , strSUNAPIResult : \n%s\n", 
							channel, strByPassResult.c_str());					
				}
				else
				{					
					g_Worker_SubDevice_info_[channel].NetworkInterface.MACAddress = charMACAddress;
				}
#endif
			}
		}
	}
	else
	{
		printf("error ... thread_function_for_subdevice_info() -> channel : %d , ByPassSUNAPI() -> return error !!\n", channel);

		if ((resCode == CURLE_LOGIN_DENIED) || (resCode == CURLE_AUTH_ERROR))
		{
			// authfail
			g_Worker_Storage_info_[channel].ConnectionStatus = "authError";
		}
		else if (resCode == CURLE_OPERATION_TIMEDOUT)
		{
			// timeout
		}
		else {
			// another error
			printf("[hwanjang] %s : %d -> CURL_Process fail .. strError : %s\n", __FUNCTION__, __LINE__, curl_easy_strerror(resCode));
			g_Worker_Storage_info_[channel].ConnectionStatus = "unknown";
		}
	}

	g_Worker_SubDevice_info_[channel].update_check_deviceinfo = true;
	g_Worker_Firmware_Ver_info_[channel].fw_update_check = true;
}

bool sunapi_manager::GetMacAddressOfSubdevices()
{
	time_t update_time = time(NULL);

	if ((update_time - g_UpdateTimeOfNetworkInterface) > GET_WAIT_TIMEOUT)
	{
		g_Sub_network_info_Cnt = 0;

		//  - LinkStatus , IPv4Address , MACAddress

		g_UpdateTimeOfNetworkInterface = time(NULL);

		std::cout << "GetMacAddressOfSubdevices() -> Start ... time : " << g_UpdateTimeOfNetworkInterface << std::endl;

		int maxChannel = g_Gateway_info_->maxChannel;

		std::string deviceIP = g_StrDeviceIP;
		std::string devicePW = g_StrDevicePW;

		// for test
		//max_ch = 10; 

		for (int i = 0; i < maxChannel; i++)
		{
			if ((g_SubDevice_info_[i].CheckConnectionStatus == 1)     // connection
				|| (g_SubDevice_info_[i].CheckConnectionStatus == 3)) // timeout
			{
				ThreadStartForNetworkInterface(i, deviceIP, devicePW);

				sleep_for(std::chrono::milliseconds(SLEEP_TIME));
			}
		}
	}
	else
	{
		printf("GetMacAddressOfSubdevices() -> check %ld ... Skip !!!!!\n", (long int)(update_time - g_UpdateTimeOfNetworkInterface));

		// skip to check
		Set_update_checkForNetworkInterface();
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

	g_Worker_SubDevice_info_[channel].NetworkInterface.update_check_networkinterface = false;

	// reset timeout
	g_Worker_SubDevice_info_[channel].curl_responseCode = CURLE_OPERATION_TIMEDOUT;
	g_Worker_SubDevice_info_[channel].NetworkInterface.ConnectionStatus = "timeout";

	bool result = false;

	std::string strByPassResult;
	
	std::string strByPassURI = "/stw-cgi/network.cgi?msubmenu=interface&action=view";

	bool json_mode = true;
	CURLcode resCode;
	result = ByPassSUNAPI(channel, json_mode, deviceIP, devicePW, strByPassURI, &strByPassResult, &resCode);

	g_Worker_SubDevice_info_[channel].curl_responseCode = resCode;

	if (result)
	{
		g_Worker_SubDevice_info_[channel].NetworkInterface.ConnectionStatus = "on";

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
				else
				{
					int ret;
					size_t index;
					char* charLink, * charMac, * charIPv4Address;
					json_t* obj;

					json_array_foreach(json_array, index, obj) {

						//  - LinkStatus(<enum> Connected, Disconnected) , InterfaceName , MACAddress
						ret = json_unpack(obj, "{s:s, s:s, s:s}",
							"LinkStatus", &charLink, "IPv4Address", &charIPv4Address, "MACAddress", &charMac);

						if (ret)
						{
							printf("[hwanjang] Error !! thread_function_for_network_interface -> json_unpack fail .. !!!\n");

							printf("channel : %d , strByPassResult : %s\n", channel, strByPassResult.c_str());

							// get LinkStatus
							int result = json_unpack(obj, "{s:s}", "LinkStatus", &charLink);

							if (result)
							{
								printf("[hwanjang] Error !! thread_function_for_network_interface() -> 1. json_unpack fail .. LinkStatus ...\n");								
							}
							else
							{
								g_Worker_SubDevice_info_[channel].NetworkInterface.LinkStatus = charLink;
							}

							// get IPv4Address							
							result = json_unpack(obj, "{s:s}", "IPv4Address", &charIPv4Address);

							if (result)
							{
								printf("[hwanjang] Error !! thread_function_for_subdevice_info() -> 2. json_unpack fail .. IPv4Address ...\n");	
							}
							else
							{
								g_Worker_SubDevice_info_[channel].NetworkInterface.IPv4Address = charIPv4Address;
							}

							// get MACAddress
							result = json_unpack(obj, "{s:s}", "MACAddress", &charMac);

							if (result)
							{
								printf("[hwanjang] Error !! thread_function_for_subdevice_info() -> 3. json_unpack fail .. MACAddress ...\n");
							}
							else
							{
								g_Worker_SubDevice_info_[channel].NetworkInterface.MACAddress = charMac;
							}
						}
						else
						{
							g_Worker_SubDevice_info_[channel].NetworkInterface.LinkStatus = charLink;
							g_Worker_SubDevice_info_[channel].NetworkInterface.IPv4Address = charIPv4Address;
							g_Worker_SubDevice_info_[channel].NetworkInterface.MACAddress = charMac;
						}
					}
				}
			}
		}
	}
	else
	{
		printf("error ... thread_function_for_network_interface() -> channel : %d , ByPassSUNAPI() -> return error !!\n", channel);

		if ((resCode == CURLE_LOGIN_DENIED) || (resCode == CURLE_AUTH_ERROR))
		{
			// authfail
			g_Worker_SubDevice_info_[channel].NetworkInterface.ConnectionStatus = "authError";
		}
		else if (resCode == CURLE_OPERATION_TIMEDOUT)
		{
			// timeout
		}
		else {
			// another error			
			printf("thread_function_for_network_interface() -> CURL_Process fail .. strError : %s\n", curl_easy_strerror(resCode));
			g_Worker_SubDevice_info_[channel].NetworkInterface.ConnectionStatus = "unknown";
		}
	}

	g_Worker_SubDevice_info_[channel].NetworkInterface.update_check_networkinterface = true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2. send data for device info view
void sunapi_manager::Set_update_checkForDeviceInfo()
{
	// for skip to check
	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		g_Worker_SubDevice_info_[i].update_check_deviceinfo = true;
	}
}

void sunapi_manager::Reset_update_checkForDeviceInfo()
{
	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		g_Worker_SubDevice_info_[i].update_check_deviceinfo = false;
	}

#if 0  // for test
	// Get ConnectionStatus of subdevice
	bool result = GetRegiesteredCameraStatus(g_StrDeviceIP, g_StrDevicePW);

	if (!result)
	{
		printf("failed to get 2. connectionStatus of subdevice ... ");
	}
#endif
}

void sunapi_manager::Set_update_checkForNetworkInterface()
{
	// for skip to check
	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		g_Worker_SubDevice_info_[i].NetworkInterface.update_check_networkinterface = true;
	}
}

void sunapi_manager::Reset_update_checkForNetworkInterface()
{
	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		g_Worker_SubDevice_info_[i].NetworkInterface.update_check_networkinterface = false;
	}

#if 0  // for test
	// Get ConnectionStatus of subdevice
	bool result = GetRegiesteredCameraStatus(g_StrDeviceIP, g_StrDevicePW);

	if (!result)
	{
		printf("failed to get 2. connectionStatus of subdevice ... ");
	}
#endif
}

int sunapi_manager::ThreadStartSendResponseForDeviceInfoView(const std::string strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
	int maxChannel = g_Gateway_info_->maxChannel;
	std::thread thread_function([=] { thread_function_for_send_response_for_deviceInfo(maxChannel, strTopic, strCommand, strType, strView, strTid); });
	thread_function.detach();

	return 0;
}

void sunapi_manager::thread_function_for_send_response_for_deviceInfo(int maxChannel, const std::string strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
	printf("\n thread_function_for_send_response_for_deviceInfo() -> Start .... \n ");

	time_t start_t = time(NULL);
	time_t quit_t;

	int i = 0;
	while (1)
	{
		for (i = 0; i < maxChannel; i++)
		{
			if ((g_SubDevice_info_[i].CheckConnectionStatus == 1)     // connection
				|| (g_SubDevice_info_[i].CheckConnectionStatus == 3)) // timeout
			{
#if 0
				if ((g_Worker_SubDevice_info_[i].update_check_deviceinfo != true)
					|| (g_Worker_SubDevice_info_[i].NetworkInterface.update_check_networkinterface != true))
#else
				if (g_Worker_SubDevice_info_[i].update_check_deviceinfo != true)
#endif
				{
					printf("thread_function_for_send_response_for_deviceInfo() -> i : %d  , update_check_deviceinfo : %d , NetworkInterface.update_check_networkinterface : %d --> break !!!\n", 
									i, g_Worker_SubDevice_info_[i].update_check_deviceinfo, g_Worker_SubDevice_info_[i].NetworkInterface.update_check_networkinterface);
					break;
				}
			}

			if ((maxChannel - 1) == i)
			{
				printf("\n#############################################################################\n");
				printf("\n thread_function_for_send_response_for_deviceInfo() -> Update & Reday to Send  .... \n ");

				UpdateSubdeviceInfos();
				UpdateSubdeviceNetworkInterfaceInfos();
				UpdateFirmwareVersionInfos();

				SendResponseForDeviceInfoView(strTopic, strCommand, strType, strView, strTid);

				Set_update_checkForDeviceInfo();
				Set_update_checkForNetworkInterface();


				time_t end_time = time(NULL);
				std::cout << "thread_function_for_send_response_for_deviceInfo() -> End ... time : " << (long int)end_time << " , diff : "
					<< (long int)(end_time - g_UpdateTimeOfNetworkInterface) << std::endl;

				return;
			}
		}

		printf("thread_function_for_send_response_for_deviceInfo() -> i : %d\n", i);

		quit_t = time(NULL);

		if ((quit_t - start_t) > GET_WAIT_TIMEOUT)
		{
			printf("\n#############################################################################\n");
			printf("\n thread_function_for_send_response_for_deviceInfo() -> timeover ... quit !!! \n ");

			UpdateSubdeviceInfos();
			UpdateSubdeviceNetworkInterfaceInfos();
			UpdateFirmwareVersionInfos();

			SendResponseForDeviceInfoView(strTopic, strCommand, strType, strView, strTid);

			Set_update_checkForDeviceInfo();
			Set_update_checkForNetworkInterface();

			std::cout << "thread_function_for_send_response_for_deviceInfo() -> End ... time : " << (long int)quit_t << " , diff : "
				<< (long int)(quit_t - start_t) << std::endl;

			return;
		}

		sleep_for(std::chrono::milliseconds(1));
	}
}

// Send subdevice info for deviceinfo 
void sunapi_manager::SendResponseForDeviceInfoView(const std::string& strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
#if 1	
	// sub of message
	//std::string strDeviceType = HBIRD_DEVICE_TYPE;
	std::string strDeviceModel = HBIRD_DEVICE_TYPE;	// temp

	json_t* main_ResponseMsg, *sub_Msg, *sub_SubdeviceMsg, *sub_ConnectionMsg, *sub_IPAddressMsg, *sub_WebprotMsg, *sub_MacAddressMsg, *sub_DeviceModelMsg, *sub_DeviceNameMsg, *sub_ChannelTitleMsg;

	main_ResponseMsg = json_object();
	sub_Msg = json_object();
	sub_SubdeviceMsg = json_object();	
	sub_ConnectionMsg = json_object();
	sub_IPAddressMsg = json_object();
	sub_WebprotMsg = json_object();
	sub_MacAddressMsg = json_object();
	sub_DeviceModelMsg = json_object();
	sub_DeviceNameMsg = json_object();
	sub_ChannelTitleMsg = json_object();

	//main_Msg["command"] = "dashboard";
	json_object_set(main_ResponseMsg, "command", json_string(strCommand.c_str()));
	//main_Msg["type"] = "view";
	json_object_set(main_ResponseMsg, "type", json_string(strType.c_str()));
	//main_Msg["view"] = "detail";
	json_object_set(main_ResponseMsg, "view", json_string(strView.c_str()));

	// ipaddress of subdevice
	int i = 0, connectionMsgCnt = 0, ipaddressMsgCnt = 0, webportMsgCnt = 0, macaddressMsgCnt = 0, devModelMsgCnt = 0, devNameMsgCnt = 0, channelTitleMsgCnt=0;
	char charCh[8];

	for (i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		if (g_SubDevice_info_[i].CheckConnectionStatus != 0)
		{			
			memset(charCh, 0, sizeof(charCh));
			//sprintf(charCh, "%d", i);  // int -> char*
			sprintf_s(charCh, sizeof(charCh), "%d", i);  // int -> char*
			 
			// connection of subdevice -  21.09.30 add
#if 0
			if ( (g_SubDevice_info_[i].ConnectionStatus.empty() != true) 
				&& (g_SubDevice_info_[i].ConnectionStatus.find("on") == std::string::npos) )
		
			{
				json_object_set_new(sub_ConnectionMsg, charCh, json_string(g_SubDevice_info_[i].ConnectionStatus.c_str()));
				connectionMsgCnt++;
			}
			else if ((g_SubDevice_info_[i].NetworkInterface.ConnectionStatus.empty() != true)
				&& (g_SubDevice_info_[i].NetworkInterface.ConnectionStatus.find("on") == std::string::npos))
			{
				json_object_set_new(sub_ConnectionMsg, charCh, json_string(g_SubDevice_info_[i].NetworkInterface.ConnectionStatus.c_str()));
				connectionMsgCnt++;
			}
#else
			if (g_SubDevice_info_[i].ConnectionStatus.empty() != true)
			{
				json_object_set_new(sub_ConnectionMsg, charCh, json_string(g_SubDevice_info_[i].ConnectionStatus.c_str()));
			}
			else if (g_SubDevice_info_[i].NetworkInterface.ConnectionStatus.empty() != true)
			{
				json_object_set_new(sub_ConnectionMsg, charCh, json_string(g_SubDevice_info_[i].NetworkInterface.ConnectionStatus.c_str()));
			}
			else
			{
				json_object_set_new(sub_ConnectionMsg, charCh, json_string("unknown"));
			}

			connectionMsgCnt++;
#endif

			// ipaddress of subdevice
			if (g_SubDevice_info_[i].NetworkInterface.IPv4Address.empty() != true)
			{
				json_object_set_new(sub_IPAddressMsg, charCh, json_string(g_SubDevice_info_[i].NetworkInterface.IPv4Address.c_str()));
				ipaddressMsgCnt++;
			}

			// webport of subdevice
			if (g_SubDevice_info_[i].HTTPPort != 0)
			{			
				json_object_set_new(sub_WebprotMsg, charCh, json_integer(g_SubDevice_info_[i].HTTPPort));
				webportMsgCnt++;
			}

			// macaddress of subdevice
			if (g_SubDevice_info_[i].NetworkInterface.MACAddress.empty() != true)
			{
				json_object_set_new(sub_MacAddressMsg, charCh, json_string(g_SubDevice_info_[i].NetworkInterface.MACAddress.c_str()));
				macaddressMsgCnt++;
			}

			// deviceModel of subdevice -  21.09.30 add
			if (g_SubDevice_info_[i].DeviceModel.empty() != true)
			{
				printf("index : %d , DeviceModel : %s\n", i, g_SubDevice_info_[i].DeviceModel.c_str());

				json_object_set_new(sub_DeviceModelMsg, charCh, json_string(g_SubDevice_info_[i].DeviceModel.c_str()));
				devModelMsgCnt++;
			}

			// deviceName of subdevice -  21.09.06 add
			if (g_SubDevice_info_[i].DeviceName.empty() != true)
			{
				json_object_set_new(sub_DeviceNameMsg, charCh, json_string(g_SubDevice_info_[i].DeviceName.c_str()));
				devNameMsgCnt++;
			}

			// ChannelTitle of subdevice -  21.10.27 add
			if (g_SubDevice_info_[i].ChannelTitle.empty() != true)
			{
				json_object_set_new(sub_ChannelTitleMsg, charCh, json_string(g_SubDevice_info_[i].ChannelTitle.c_str()));
				channelTitleMsgCnt++;
			}
		}
	}

	// connection
	if (connectionMsgCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "connection", sub_ConnectionMsg);
	}

	// ipaddress
	if (ipaddressMsgCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "ipaddress", sub_IPAddressMsg);
	}

	// webport
	if (webportMsgCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "webport", sub_WebprotMsg);
	}

	// macaddress
	if (macaddressMsgCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "macaddress", sub_MacAddressMsg);
	}

	// 21.09.30 add - subdeviceModel 
	if (devModelMsgCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "subdeviceModel", sub_DeviceModelMsg);
	}

	// 21.09.06 add - subdeviceName 
	if (devNameMsgCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "subdeviceName", sub_DeviceNameMsg);
	}

	// 21.10.27 add - subChannelTitle 
	if (channelTitleMsgCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "Title", sub_ChannelTitleMsg);
	}

	// sub of message
	//json_object_set(sub_Msg, "deviceId", json_string(g_Device_id.c_str()));
	//json_object_set(sub_Msg, "deviceType", json_string(strDeviceType.c_str()));
	json_object_set(sub_Msg, "deviceModel", json_string(strDeviceModel.c_str()));
	json_object_set(sub_Msg, "ipaddress", json_string(g_Gateway_info_->IPv4Address.c_str()));
	json_object_set(sub_Msg, "webport", json_integer(g_Gateway_info_->WebPort));
	json_object_set(sub_Msg, "macaddress", json_string(g_Gateway_info_->MACAddress.c_str()));
	json_object_set(sub_Msg, "connection", json_string(g_Gateway_info_->ConnectionStatus.c_str()));

	json_object_set(sub_Msg, "subdevice", sub_SubdeviceMsg);

	//main_Msg["message"] = sub_Msg;
	json_object_set(main_ResponseMsg, "message", sub_Msg);

	//main_Msg["tid"] = strTid;
	json_object_set(main_ResponseMsg, "tid", json_string(strTid.c_str()));
	//main_Msg["version"] = "1.5";
	std::string strVersion = "1.5";
	json_object_set(main_ResponseMsg, "version", json_string(strVersion.c_str()));

	//std::string strMQTTMsg = writer.write(mqtt_Msg);		
	std::string strMQTTMsg = json_dumps(main_ResponseMsg, 0);

	printf("SendResponseForDeviceInfoView() -> strMQTTMsg size : %lu\n%s\n", strMQTTMsg.size(), strMQTTMsg.c_str());

	observerForHbirdManager->SendResponseForDashboard(strTopic, strMQTTMsg);
#endif

}


/////////////////////////////////////////////////////////////////////////////////////////////
// 3. firmware info view
// Get firmware version - /stw-cgi/system.cgi?msubmenu=deviceinfo&action=view

bool sunapi_manager::GetDeviceInfoOfGateway(const std::string& strGatewayIP, const std::string& strID_PW)
{
	std::string _gatewayIP = strGatewayIP;
	std::string _idPassword = strID_PW;

	// reset
	g_Gateway_info_->curl_responseCode = CURLE_OPERATION_TIMEDOUT;
	g_Gateway_info_->ConnectionStatus = "timeout";

	int max_channel = 0;

	std::string request;

	request = "http://";
	request.append(_gatewayIP);
	request.append("/stw-cgi/system.cgi?msubmenu=deviceinfo&action=view");

	//printf("private ID:PW = %s\n",userpw.c_str());

#if 1  // 2019.09.05 hwanjang - sunapi debugging
	printf("sunapi_manager::GetDeviceInfoOfGateway() -> SUNAPI Request : %s\n", request.c_str());
#endif

	CURLcode resCode;
	std::string strSUNAPIResult;
	bool json_mode = true;
	bool ssl_opt = false;

	resCode = CURL_Process(json_mode, ssl_opt, CURL_TIMEOUT, request, _idPassword, &strSUNAPIResult);

	g_Gateway_info_->curl_responseCode = resCode;

	if (resCode == CURLE_OK)
	{
		g_Gateway_info_->ConnectionStatus = "on";

		if (strSUNAPIResult.empty())
		{
			printf("[hwanjang] GetDeviceInfoOfGateway() strSUNAPIResult is empty !!!\n");

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
				g_Gateway_info_->DeviceModel = charModel;
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
		}
	}
	else
	{
		printf("GetDeviceInfoOfGateway() -> error cURL res : %d!!!\n", resCode);
		std::string strError;
		// CURL_Process fail
		if ((resCode == CURLE_LOGIN_DENIED) || (resCode == CURLE_AUTH_ERROR))
		{
			// authfail
			strError = "authError";
			g_Gateway_info_->ConnectionStatus = "authError";
		}
		else if (resCode == CURLE_OPERATION_TIMEDOUT)
		{
			// timeout
		}
		else {
			// another error		
			//strError = "unknown";
			strError = curl_easy_strerror(resCode);

			printf("GetDeviceInfoOfGateway() -> CURL_Process fail .. strError : %s\n", strError.c_str());
			g_Gateway_info_->ConnectionStatus = "unknown";
			
		}
		return false;
	}

	return true;
}

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

bool sunapi_manager::GetFirmwareVersionOfSubdevices()
{
	// Get firmware version - /stw-cgi/system.cgi?msubmenu=deviceinfo&action=view
	g_UpdateTimeForFirmwareVersionOfSubdevices = time(NULL);

	std::string deviceIP = g_StrDeviceIP;
	std::string devicePW = g_StrDevicePW;

	printf("GetFirmwareVersionOfSubdevices() -> Start ... g_UpdateTimeForFirmwareVersionOfSubdevices : %lld\n", g_UpdateTimeForFirmwareVersionOfSubdevices);

	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		if ((g_SubDevice_info_[i].CheckConnectionStatus == 1)     // connection
			|| (g_SubDevice_info_[i].CheckConnectionStatus == 3)) // timeout
		{
			//printf("GetFirmwareVersionOfSubdevices() -> index : %d ... ConnectionStatus : %d\n", i, g_Worker_SubDevice_info_[i].CheckConnectionStatus);

			ThreadStartForFirmwareVersion(i, deviceIP, devicePW);

			sleep_for(std::chrono::milliseconds(SLEEP_TIME));
		}
	}
	return true;
}


bool sunapi_manager::GetFirmwareVersionOfSubdevices(const std::string deviceIP, const std::string devicePW, CURLcode* resCode)
{
	g_UpdateTimeForFirmwareVersionOfSubdevices = time(NULL);

	std::cout << "GetFirmwareVersionOfSubdevices() -> Start ... time : " << (long int)g_UpdateTimeForFirmwareVersionOfSubdevices << std::endl;

	std::string request;

	request = "http://";
	request.append(deviceIP);
	request.append("/stw-cgi/media.cgi?msubmenu=cameraupgrade&action=view");

#if 1  // 2019.09.05 hwanjang - sunapi debugging
	printf("sunapi_manager::GetFirmwareVersionOfSubdevices() -> SUNAPI Request : %s\n", request.c_str());
#endif

	CURLcode res;
	std::string strSUNAPIResult;
	bool json_mode = true;
	bool ssl_opt = false;

	res = CURL_Process(json_mode, ssl_opt, CURL_TIMEOUT, request, devicePW, &strSUNAPIResult);

	*resCode = res;

	if (res == CURLE_OK)
	{
		time_t end_time = time(NULL);
		std::cout << "GetFirmwareVersionOfSubdevices() -> CURLE_OK !! Received data , time : " << (long int)end_time << " , diff : "
			<< (long int)(end_time - g_UpdateTimeForFirmwareVersionOfSubdevices) << std::endl;

		if (strSUNAPIResult.empty())
		{
			printf("[hwanjang] GetFirmwareVersionOfSubdevices() result string is empty !!!\n");
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

		json_t* json_array = json_object_get(json_strRoot, "Data");

		if (!json_is_array(json_array))
		{
			printf("NG1 ... GetFirmwareVersionOfSubdevices() -> json_is_array fail !!!\n");
			return false;
		}

		int result;
		int i = 0, j = 0;
		size_t index;
		char* charFirmwareVersion, *charStatus;
		json_t* obj;

		// get FirmwareVersion
		json_array_foreach(json_array, index, obj) {
			result = json_unpack(obj, "{s:s}", "CurrentVersion", &charFirmwareVersion);

			if (result)
			{
				printf("[hwanjang] Error !! GetFirmwareVersionOfSubdevices -> 1. json_unpack fail .. CurrentVersion ... index : %d !!!\n", index);
				std::cout << "obj string : " << std::endl << json_dumps(obj, 0) << std::endl;
			}
			else
			{
				if (strlen(charFirmwareVersion) != 0)
				{
					//printf("index : %d , CurrentVersion = %s\n", index, charFirmwareVersion);
					g_Worker_Firmware_Ver_info_[index].FirmwareVersion = charFirmwareVersion;
				}
			}

			result = json_unpack(obj, "{s:s}", "Status", &charStatus);

			if (result)
			{
				printf("[hwanjang] Error !! GetFirmwareVersionOfSubdevices -> 2. json_unpack fail .. Status ... index : %d !!!\n", index);
				std::cout << "obj string : " << std::endl << json_dumps(obj, 0) << std::endl;
			}
			else
			{
				if (strlen(charStatus) != 0)
				{
					//printf("index : %d , charStatus = %s\n", index, charStatus);
					g_Worker_Firmware_Ver_info_[index].UpgradeStatus = charStatus;
				}
			}
		}
	}
	else
	{
		time_t end_time = time(NULL);
		std::cout << "GetFirmwareVersionOfSubdevices() -> failed ... End time : " << (long int)end_time << " , diff : "
			<< (long int)(end_time - g_UpdateTimeForFirmwareVersionOfSubdevices) << std::endl;

		return false;
	}

	time_t end_time = time(NULL);
	std::cout << "GetFirmwareVersionOfSubdevices() -> End ... time : " << (long int)end_time << " , diff : "
		<< (long int)(end_time - g_UpdateTimeForFirmwareVersionOfSubdevices) << std::endl;

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
	//printf("thread_function_for_firmware_version() -> channel : %d , Start to get FirmwareVersion ...\n", channel);

	g_Worker_Firmware_Ver_info_[channel].fw_update_check = false;

	// reset timeout
	g_Worker_Firmware_Ver_info_[channel].curl_responseCode = CURLE_OPERATION_TIMEDOUT;

	bool result = false;

	std::string strByPassResult, strFirmwareversion;

	std::string strByPassURI = "/stw-cgi/system.cgi?msubmenu=deviceinfo&action=view";

	bool json_mode = true;
	CURLcode resCode;
	result = ByPassSUNAPI(channel, json_mode, deviceIP, devicePW, strByPassURI, &strByPassResult, &resCode);

	g_Worker_Firmware_Ver_info_[channel].curl_responseCode = resCode;

	if (result)
	{
		if (strByPassResult.empty())
		{
			printf("thread_function_for_firmware_version() -> channel : %d , strByPassResult is empty !!\n", channel);
		}
		else
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
				char* charFirmwareVersion;
				ret = json_unpack(json_strRoot, "{s:s}", "FirmwareVersion", &charFirmwareVersion);

				if (ret)
				{
					printf("[hwanjang] Error !! thread_function_for_firmware_version -> json_unpack fail .. !!!\n");

					printf("channel : %d , strByPassResult : %s\n", channel, strByPassResult.c_str());
				}
				else
				{
					g_Worker_Firmware_Ver_info_[channel].FirmwareVersion = charFirmwareVersion;
					
					printf("thread_function_for_firmware_version() -> channel : %d, FirmwareVersion : %s\n",
						channel, charFirmwareVersion);
				}
#if 0	// no need
				// get Model
				char* charModel;
				ret = json_unpack(json_strRoot, "{s:s}", "Model", &charModel);

				if (ret)
				{
					printf("[hwanjang] Error !! thread_function_for_firmware_version() -> 2. json_unpack fail .. Model ...\n");
					printf("thread_function_for_firmware_version() -> channel : %d , strSUNAPIResult : \n%s\n",
						channel, strByPassResult.c_str());					
				}
				else
				{
					g_Worker_SubDevice_info_[channel].DeviceModel = charModel;
				}

				// get DeviceName
				char* charDeviceName;
				ret = json_unpack(json_strRoot, "{s:s}", "DeviceName", &charDeviceName);

				if (ret)
				{
					printf("[hwanjang] Error !! thread_function_for_firmware_version() -> 3. json_unpack fail .. DeviceName ...\n");
					printf("thread_function_for_firmware_version() -> channel : %d , strSUNAPIResult : \n%s\n",
						channel, strByPassResult.c_str());					
				}
				else
				{
					g_Worker_SubDevice_info_[channel].DeviceName = charDeviceName;
				}
#endif
			}
		}
	}
	else
	{
		// fail
		printf("thread_function_for_firmware_version() -> channel : %d , fail to get FirmwareVersion !!!\n", channel);
	}

	g_Worker_Firmware_Ver_info_[channel].fw_update_check = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// 3. Send firmware version of subdevices 

void sunapi_manager::SendResponseForFirmwareView(const std::string& strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
	// sub of message
	std::string strDeviceType = HBIRD_DEVICE_TYPE;

	json_t* main_ResponseMsg, * sub_Msg, * sub_SubdeviceMsg, * sub_FWVersionMsg, * sub_LatestVersionMsg, * sub_UpgradeStatusMsg;

	main_ResponseMsg = json_object();
	sub_Msg = json_object();
	sub_SubdeviceMsg = json_object();
	sub_FWVersionMsg = json_object();
	sub_LatestVersionMsg = json_object();
	sub_UpgradeStatusMsg = json_object();	

	//main_Msg["command"] = "dashboard";
	json_object_set(main_ResponseMsg, "command", json_string(strCommand.c_str()));
	//main_Msg["type"] = "view";
	json_object_set(main_ResponseMsg, "type", json_string(strType.c_str()));
	//main_Msg["view"] = "detail";
	json_object_set(main_ResponseMsg, "view", json_string(strView.c_str()));

	// ipaddress of subdevice
	int i = 0, fwVersionMsgCnt = 0, latestVersionMsgCnt = 0, upgradeStatusCnt = 0;
	char charCh[8];

	for (i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		if (g_SubDevice_info_[i].CheckConnectionStatus != 0)
		{
			memset(charCh, 0, sizeof(charCh));
			//sprintf(charCh, "%d", i);  // int -> char*
			sprintf_s(charCh, sizeof(charCh), "%d", i);  // int -> char*

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

			// latestFirmwareVersion of subdevice
			if (g_Firmware_Ver_info_[i].UpgradeStatus.empty() != true)
			{
				json_object_set_new(sub_UpgradeStatusMsg, charCh, json_string(g_Firmware_Ver_info_[i].UpgradeStatus.c_str()));
				upgradeStatusCnt++;
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

	if (upgradeStatusCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "firmwareStatus", sub_UpgradeStatusMsg);
	}

	// sub of message
	//json_object_set(sub_Msg, "deviceId", json_string(g_Device_id.c_str()));
	//json_object_set(sub_Msg, "deviceType", json_string(strDeviceType.c_str()));
	if (g_Gateway_info_->FirmwareVersion.empty() != true)
		json_object_set(sub_Msg, "firmwareVersion", json_string(g_Gateway_info_->FirmwareVersion.c_str()));
	else
	{
		char tempVersion[16] = { 0, };
		sprintf(tempVersion,"%s", __DATE__);

		json_object_set(sub_Msg, "firmwareVersion", json_string(tempVersion));
	}

	json_object_set(sub_Msg, "connection", json_string(g_Gateway_info_->ConnectionStatus.c_str()));
	json_object_set(sub_Msg, "subdevice", sub_SubdeviceMsg);


	//main_Msg["message"] = sub_Msg;
	json_object_set(main_ResponseMsg, "message", sub_Msg);

	//main_Msg["tid"] = strTid;
	json_object_set(main_ResponseMsg, "tid", json_string(strTid.c_str()));
	//main_Msg["version"] = "1.5";
	std::string strVersion = "1.5";
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

	for (i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		// reset LatestFirmwareVersion and then update ...
		g_Worker_Firmware_Ver_info_[i].LatestFirmwareVersion.clear();
		g_Worker_Firmware_Ver_info_[i].last_fw_update_check = false;

		if (g_Worker_SubDevice_info_[i].CheckConnectionStatus != 0)
		{
			for (j = 0; j < (int)g_vecFirmwareInfos.size(); j++)
			{
				if (g_Worker_SubDevice_info_[i].DeviceModel.find(g_vecFirmwareInfos[j].model) != std::string::npos)
				{					
					g_Worker_Firmware_Ver_info_[i].LatestFirmwareVersion = g_vecFirmwareInfos[j].version_info;					

					printf("GetLatestFirmwareVersionFromFile() -> channel : %d, Model : %s , LatestFirmwareVersion : %s\n",
								i, g_Worker_SubDevice_info_[i].DeviceModel.c_str(), g_Worker_Firmware_Ver_info_[i].LatestFirmwareVersion.c_str());

					break;
				}
			}
		}
		g_Worker_Firmware_Ver_info_[i].last_fw_update_check = true;
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
#if 0
	std::string file_name = "fw_info.txt";
	std::ofstream fw_info_file(file_name);
#endif
	//printf("private ID:PW = %s\n",userpw.c_str());

#if 1  // 2019.09.05 hwanjang - sunapi debugging
	printf("sunapi_manager::GetLatestFirmwareVersionFromURL() -> Request URL : %s\n", update_FW_Info_url.c_str());
#endif

	CURL* curl_handle;
	static const char* pagefilename = FIRMWARE_INFO_FILE;
	FILE* pagefile = NULL;

	curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */
	curl_handle = curl_easy_init();
	if (curl_handle)
	{
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
		//pagefile = fopen(pagefilename, "wb");
		fopen_s(&pagefile, pagefilename, "wb");
		if (pagefile) {

			/* write the page body to this file handle */
			curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);

			/* Perform the request, res will get the return code */
			CURLcode res = curl_easy_perform(curl_handle);
			/* Check for errors */
			if (res != CURLE_OK) {
				//fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
				printf("GetLatestFirmwareVersionFromURL() -> curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
				
				return false;
			}

			/* close the header file */
			fclose(pagefile);
		}

		/* cleanup curl stuff */
		curl_easy_cleanup(curl_handle);

	}
	else
	{
		printf("GetLatestFirmwareVersionFromURL() -> curl_easy_init fail ...\n");
		return false;
	}
	curl_global_cleanup();

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// 4. Send result of firmware update 

void sunapi_manager::SendResponseForUpdateFirmware(const std::string& strTopic,
			std::string strCommand, std::string strType, std::string strView, std::string strTid, int resCode, int rawCode, std::vector<int> updateChannel)
{
#if 1
	std::string strVersion = "1.5";
	// sub of message
	std::string strDeviceType = HBIRD_DEVICE_TYPE;

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

	for (int i = 0; i < (int)updateChannel.size(); i++)
	{
		memset(charCh, 0, sizeof(charCh));
		//sprintf(charCh, "%d", updateChannel[i]);  // int -> char*
		sprintf_s(charCh, sizeof(charCh), "%d", updateChannel[i]);  // int -> char*

		json_object_set_new(sub_RawCodeMsg, charCh, json_integer(rawCode));
		rawCodeMsgCnt++;
	}

	//sub_SubdeviceMsg["firmwareVersion"] = sub_IPAddressMsg;
	//sub_SubdeviceMsg["latestFirmwareVersion"] = sub_WebprotMsg;

	json_object_set(sub_SubdeviceMsg, "resCode", json_integer(resCode));

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
	sleep_for(std::chrono::milliseconds(second * 1000));  // after second

	UpdateDataForDashboardView();

	ThreadStartForUpdateInfos(60*60);  // after 1 hour

}

void sunapi_manager::UpdateDataForDashboardView()
{
	GetDataForFirmwareVersionInfo();

	sleep_for(std::chrono::milliseconds(1 * 1000));

	//GetDataForSubdeviceInfo();

	sleep_for(std::chrono::milliseconds(2 * 1000));

	GetDataForStorageInfo();

	sleep_for(std::chrono::milliseconds(2 * 1000));

	UpdateSubdeviceInfos();
	UpdateSubdeviceNetworkInterfaceInfos();

	UpdateStorageInfos();
}

// not used
void sunapi_manager::GetDataForSubdeviceInfo()
{
#if 0
	std::cout << "GetDataForSubdeviceInfo() start ..." << time(NULL) << std::endl;

	// Get ConnectionStatus of subdevice
	CURLcode resCode;
	bool result = GetRegiesteredCameraStatus(g_StrDeviceIP, g_StrDevicePW, &resCode);

	if (!result)
	{
		printf("failed to get GetRegiesteredCameraStatus ... 1");

		result = GetRegiesteredCameraStatus(g_StrDeviceIP, g_StrDevicePW, &resCode);
		if (!result)
		{
			printf("failed to get GetRegiesteredCameraStatus ... 2 ... return !!");

			if ((resCode == CURLE_LOGIN_DENIED) || (resCode == CURLE_AUTH_ERROR))
			{
				// authfail
			}
			else if (resCode == CURLE_OPERATION_TIMEDOUT)
			{
				// timeout
			}
			else {
				// another error

			}
			return;
		}
	}

	//int connectionCnt = GetConnectionCount();
	//std::cout << "GetDataForSubdeviceInfo() -> connectionCnt : " << connectionCnt << std::endl;

	time_t update_time = time(NULL);

	if ((update_time - g_UpdateTimeOfNetworkInterface) > 10)
	{
		GetMacAddressOfSubdevices();
	}

	//UpdateSubdeviceInfos();
#endif
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

	// get LatestFirmwareVersion from URL
	ThreadStartGetLatestFirmwareVersionFromURL();

	// bypass
	//GetFirmwareVersionOfSubdevices();
	//GetDeviceInfoOfSubdevices();

#if 0
	time_t update_time = time(NULL);

	if ((update_time - g_UpdateTimeOfDeviceInfo) > GET_WAIT_TIMEOUT)
	{
		CURLcode resCode;
		bool result = GetFirmwareVersionOfSubdevices(g_StrDeviceIP, g_StrDevicePW, &resCode);

		if (!result)
		{
			printf("failed to get GetFirmwareVersionOfSubdevices ... ");
		}
	}
#else

	CURLcode resCode;
	bool result = GetFirmwareVersionOfSubdevices(g_StrDeviceIP, g_StrDevicePW, &resCode);

	if (!result)
	{
		printf("failed to get GetFirmwareVersionOfSubdevices ... ");
	}
#endif
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

	result = GetLatestFirmwareVersionFromFile(FIRMWARE_INFO_FILE);
	
}

//ThreadStartForUpdateInfos(20);  // after 20 seconds

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 3. firmware version info view

void sunapi_manager::Set_update_check_Firmware_Ver_info_ForFirmwareVersion()
{
	// for skip to check
	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		g_Worker_Firmware_Ver_info_[i].fw_update_check = true;
		g_Worker_Firmware_Ver_info_[i].last_fw_update_check = true;
	}
}

void sunapi_manager::Reset_update_check_Firmware_Ver_info_ForFirmwareVersion()
{
	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		g_Worker_Firmware_Ver_info_[i].fw_update_check = false;
		g_Worker_Firmware_Ver_info_[i].last_fw_update_check = false;
	}
}

int sunapi_manager::ThreadStartSendResponseForFirmwareView(const std::string strTopic, 
			std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
	int maxChannel = g_Gateway_info_->maxChannel;
	std::thread thread_function([=] { thread_function_for_send_response_for_firmwareVersion(maxChannel, strTopic, strCommand, strType, strView, strTid); });
	thread_function.detach();

	return 0;
}

void sunapi_manager::thread_function_for_send_response_for_firmwareVersion(int maxChannel, const std::string strTopic, 
			std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
	printf("\n thread_function_for_send_response_for_firmwareVersion() -> Start .... \n ");

	time_t start_t = time(NULL);
	time_t quit_t;

	int i = 0;
	while (1)
	{
		for (i = 0; i < maxChannel; i++)
		{
			if ((g_SubDevice_info_[i].CheckConnectionStatus == 1)     // connection
				|| (g_SubDevice_info_[i].CheckConnectionStatus == 3)) // timeout
			{
				if ((g_Worker_Firmware_Ver_info_[i].fw_update_check != true)
					|| (g_Worker_Firmware_Ver_info_[i].last_fw_update_check != true))
				{
					printf("thread_function_for_send_response_for_firmwareVersion() -> i : %d  , fw_update_check : %d , last_fw_update_check : %d --> break !!!\n", 
								i , g_Worker_Firmware_Ver_info_[i].fw_update_check , g_Worker_Firmware_Ver_info_[i].last_fw_update_check);
					break;
				}
			}
			
			if ((maxChannel - 1) == i)
			{
				printf("\n#############################################################################\n");
				printf("\n thread_function_for_send_response_for_firmwareVersion() -> Start .... \n ");				

				//int result = GetLatestFirmwareVersionFromFile(FIRMWARE_INFO_FILE);

				UpdateFirmwareVersionInfos();

				SendResponseForFirmwareView(strTopic, strCommand, strType, strView, strTid);

				Set_update_check_Firmware_Ver_info_ForFirmwareVersion();

				return;
			}

		}

		printf("thread_function_for_send_response_for_firmwareVersion() -> i : %d\n", i);


		quit_t = time(NULL);

		if ((quit_t - start_t) > GET_WAIT_TIMEOUT)
		{
			printf("\n#############################################################################\n");
			printf("\n thread_function_for_send_response_for_firmwareVersion() -> timeover ... quit !!! \n ");

			//int result = GetLatestFirmwareVersionFromFile(FIRMWARE_INFO_FILE);

			UpdateFirmwareVersionInfos();

			SendResponseForFirmwareView(strTopic, strCommand, strType, strView, strTid);

			Set_update_check_Firmware_Ver_info_ForFirmwareVersion();

			std::cout << "thread_function_for_send_response_for_firmwareVersion() -> End ... time : " << (long int)quit_t << " , diff : "
				<< (long int)(quit_t - start_t) << std::endl;

			return;
		}

		sleep_for(std::chrono::milliseconds(1));
	}
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

	printf("TestDashboardView() -> Max Channel : %d ... Dashboard Info ...\n", g_Gateway_info_->maxChannel);

	int connectedCount = 0;

	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		if (g_SubDevice_info_[i].CheckConnectionStatus == 1)
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
		else if (g_SubDevice_info_[i].CheckConnectionStatus == 2)
		{
			printf("channel : %d, ConnectionStatus : %d ... ConnectionFail ... Skip !!!\n", i, g_SubDevice_info_[i].CheckConnectionStatus);
		}
		else
		{
			printf("channel : %d, ConnectionStatus : %d ... Disconneted ... Skip !!!\n", i, g_SubDevice_info_[i].CheckConnectionStatus);
		}
	}
}

// test for deviceinfo
void sunapi_manager::TestDeviceInfoView()
{
	//GetMacAddressOfSubdevices();

	sleep_for(std::chrono::milliseconds(3 * 1000));

	UpdateSubdeviceInfos();
	UpdateSubdeviceNetworkInterfaceInfos();

	printf("TestDeviceInfoView() -> Max Channel : %d ... Device Info ...\n", g_Gateway_info_->maxChannel);

	int connectedCount = 0;

	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		if (g_SubDevice_info_[i].CheckConnectionStatus == 1)
		{
			connectedCount++;
			printf("TestDashboardView() -> Connected Count : %d\n", connectedCount);

			printf("\n################################################\n");
			printf("channel : %d, ConnectionStatus : %d ...\n", i, g_SubDevice_info_[i].CheckConnectionStatus);
			printf("LinkStatus : %s\n", g_SubDevice_info_[i].NetworkInterface.LinkStatus.c_str());
			printf("DeviceModel : %s\n", g_SubDevice_info_[i].DeviceModel.c_str());
			printf("MACAddress : %s\n", g_SubDevice_info_[i].NetworkInterface.MACAddress.c_str());
			printf("IPv4Address : %s\n", g_SubDevice_info_[i].NetworkInterface.IPv4Address.c_str());
			printf("HTTPPort : %d\n", g_SubDevice_info_[i].HTTPPort);
			printf("\n################################################\n");
		}
		else if (g_SubDevice_info_[i].CheckConnectionStatus == 2)
		{
			printf("channel : %d, ConnectionStatus : %d ... ConnectionFail ... Skip !!!\n", i, g_SubDevice_info_[i].CheckConnectionStatus);
		}
		else
		{
			printf("channel : %d, ConnectionStatus : %d ... Disconneted ... Skip !!!\n", i, g_SubDevice_info_[i].CheckConnectionStatus);
		}
	}

}

// test for firmware version info
void sunapi_manager::TestFirmwareVersionInfoView()
{
#if 0
	GetFirmwareVersionOfSubdevices();

	sleep_for(std::chrono::milliseconds(3 * 1000));

	UpdateSubdeviceInfos();
#endif

	printf("TestFirmwareVersionInfoView() -> Max Channel : %d ... Device Info ...\n", g_Gateway_info_->maxChannel);

	int connectedCount = 0;

	for (int i = 0; i < g_Gateway_info_->maxChannel; i++)
	{
		if (g_SubDevice_info_[i].CheckConnectionStatus == 1)
		{
			connectedCount++;
			printf("TestFirmwareVersionInfoView() -> Connected Count : %d\n", connectedCount);

			printf("\n################################################\n");
			printf("channel : %d, DeviceModel : %s, firmwareVersion : %s , latestFirmwareVersion : %s\n",
				i, g_SubDevice_info_[i].DeviceModel.c_str(),
				g_Firmware_Ver_info_[i].FirmwareVersion.c_str(), g_Firmware_Ver_info_[i].LatestFirmwareVersion.c_str());
			printf("\n################################################\n");
		}
		else if (g_SubDevice_info_[i].CheckConnectionStatus == 2)
		{
			printf("channel : %d, ConnectionStatus : %d ... ConnectionFail ... Skip !!!\n", i, g_SubDevice_info_[i].CheckConnectionStatus);
		}
		else
		{
			printf("channel : %d, ConnectionStatus : %d ... Disconneted ... Skip !!!\n", i, g_SubDevice_info_[i].CheckConnectionStatus);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// process 'command' topic - checkPassword

void sunapi_manager::CommandCheckPassword(const std::string& strTopic, const std::string& strPayload)
{
	json_error_t error_check;
	json_t* json_strRoot = json_loads(strPayload.c_str(), 0, &error_check);

	if (!json_strRoot)
	{
		printf("CommandCheckPassword() -> json_loads fail .. strPayload : \n%s\n", strPayload.c_str());

		return;
	}

	std::string strMQTTMsg;
	std::string strCommand, strType, strView, strTid, strVersion;

	int ret;
	char* charCommand, * charType, *charView, *charTid, *charVersion;
	ret = json_unpack(json_strRoot, "{s:s, s:s, s:s, s:s, s:s}", "command", &charCommand, "type", &charType, "view", &charView, "tid", &charTid, "version", &charVersion);

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

	json_t* json_message = json_object_get(json_strRoot, "message");

	if (!json_message)
	{
		printf("error ... CommandCheckPassword() -> json_is wrong ... message !!!\n");

		json_object_set(sub_Msg, "responseCode", json_integer(600));
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

		json_object_set(sub_Msg, "responseCode", json_integer(600));
		json_object_set(sub_Msg, "deviceModel", json_string(""));
		json_object_set(sub_Msg, "deviceFirmwareVersion", json_string(""));
		json_object_set(sub_Msg, "maxChannels", json_integer(0));

		json_object_set(main_ResponseMsg, "message", sub_Msg);

		strMQTTMsg = json_dumps(main_ResponseMsg, 0);

		observerForHbirdManager->SendResponseToPeer(strTopic, strMQTTMsg);

		return ;
	}

	//std::string strPassword = "admin";
	std::string strPassword = charID;	
	strPassword.append(":");
	strPassword.append(charPassword);

#if 0
	int maxCh = 0;
	maxCh = GetMaxChannelByAttribute(strPassword);

	if (maxCh < 1)
	{
		printf("Error ... CommandCheckPassword() -> It means that the wrong username or password were sent in the request.\n");

		json_object_set(sub_Msg, "responseCode", json_integer(403));
		json_object_set(sub_Msg, "deviceModel", json_string(""));
		json_object_set(sub_Msg, "deviceFirmwareVersion", json_string(""));
		json_object_set(sub_Msg, "maxChannels", json_integer(0));

		json_object_set(main_ResponseMsg, "message", sub_Msg);

		strMQTTMsg = json_dumps(main_ResponseMsg, 0);

		observerForHbirdManager->SendResponseToPeer(strTopic, strMQTTMsg);

		return ;
	}

	GetDeviceInfoOfGateway();
#else
	bool result = GetDeviceInfoOfGateway(g_StrDeviceIP,strPassword);

	if (!result)
	{
		printf("Error ... CommandCheckPassword() -> It means that the wrong username or password were sent in the request.\n");

		json_object_set(sub_Msg, "responseCode", json_integer(403));
		json_object_set(sub_Msg, "deviceModel", json_string(""));
		json_object_set(sub_Msg, "deviceFirmwareVersion", json_string(""));
		json_object_set(sub_Msg, "maxChannels", json_integer(0));

		json_object_set(main_ResponseMsg, "message", sub_Msg);

		strMQTTMsg = json_dumps(main_ResponseMsg, 0);

		observerForHbirdManager->SendResponseToPeer(strTopic, strMQTTMsg);

		return;
	}
#endif

	json_object_set(sub_Msg, "responseCode", json_integer(200));

	std::string strModel = g_Gateway_info_->DeviceModel;

	json_object_set(sub_Msg, "deviceModel", json_string(strModel.c_str()));

	std::string strDeviceVersion = g_Gateway_info_->FirmwareVersion;

	if(!strDeviceVersion.empty())
		json_object_set(sub_Msg, "deviceFirmwareVersion", json_string(strDeviceVersion.c_str()));
	else
		json_object_set(sub_Msg, "deviceFirmwareVersion", json_string("unknown"));

	json_object_set(sub_Msg, "maxChannels", json_integer(g_Gateway_info_->maxChannel));

	json_object_set(main_ResponseMsg, "message", sub_Msg);

	strMQTTMsg = json_dumps(main_ResponseMsg, 0);

	printf("[hwanjang] APIManager::CommandCheckPassword() response ---> size : %lu, send message : \n%s\n", strMQTTMsg.size(), strMQTTMsg.c_str());

	observerForHbirdManager->SendResponseToPeer(strTopic, strMQTTMsg);

}
