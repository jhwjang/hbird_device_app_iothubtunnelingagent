#include "sunapi_manager.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <sstream>  // for istringstream , getline
#include <fstream>
#include <chrono>
#include <vector>


#define DEVICE_TYPE "gateway"

using std::this_thread::sleep_for;
#define SLEEP_TIME 2

// Default timeout is 0 (zero) which means it never times out during transfer.
#define WAIT_TIMEOUT 5 
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

	result = GetNetworkInterfaceOfGateway();

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
		g_Gateway_info_->Model = "CloudGateway";
		g_Gateway_info_->FirmwareVersion = "unknown";
	}

	// 1. dashboard init
	g_Storage_info_ = new Storage_Infos[gMax_Channel + 1];
	g_Worker_Storage_info_ = new Storage_Infos[gMax_Channel + 1];

	ResetStorageInfos();

	// 2. deviceinfo init
	g_SubDevice_info_ = new Channel_Infos[gMax_Channel + 1];
	g_Worker_SubDevice_info_ = new Channel_Infos[gMax_Channel + 1];

	ResetSubdeviceInfos();
	
	// 3. firmware version
	g_Firmware_Ver_info_ = new Firmware_Version_Infos[gMax_Channel + 1];;
	g_Worker_Firmware_Ver_info_ = new Firmware_Version_Infos[gMax_Channel + 1];

	ResetFirmwareVersionInfos();

	// Get ConnectionStatus of subdevice
	CURLcode resCode;
	result = GetRegiesteredCameraStatus(gStrDeviceIP, gStrDevicePW, &resCode);

	if (!result)
	{
		printf("failed to get GetRegiesteredCameraStatus ... 1");

		result = GetRegiesteredCameraStatus(gStrDeviceIP, gStrDevicePW, &resCode);
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

bool sunapi_manager::GetNetworkInterfaceOfGateway()
{
	std::string request;

	CURLcode res;
	std::string strSUNAPIResult;

	bool json_mode = true;
	bool ssl_opt = false;

#if 0  // modified to read config file
	/// /////////////////////////////////////////////////////////////////////////////////////
	// Get HTTP Port

	request = "http://";
	request.append(gStrDeviceIP);
	request.append("/stw-cgi/network.cgi?msubmenu=http&action=view");

#if 1  // 2019.09.05 hwanjang - sunapi debugging
	printf("sunapi_manager::GetNetworkInterfaceOfGateway() -> 1. SUNAPI Request : %s\n", request.c_str());
#endif


#if 0

	res = CURL_Process(json_mode, ssl_opt, request, gStrDevicePW, &strSUNAPIResult);

	if (res == CURLE_OK)
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
	g_Gateway_info_->WebPort = 80;
#endif
#else
	g_Gateway_info_->WebPort = 443;
#endif


	/// /////////////////////////////////////////////////////////////////////////////////////
	// Get IPv4Address , MACAddress
	//  - /stw-cgi/network.cgi?msubmenu=interface&action=view  -> IPv4Address , MACAddress
	//  - /stw-cgi/system.cgi?msubmenu=deviceinfo&action=view  -> ConnectedMACAddress , FirmwareVersion
	// 
	// reset
	g_Gateway_info_->curl_responseCode = CURLE_OPERATION_TIMEDOUT;
	g_Gateway_info_->IPv4Address = "timeout";
	g_Gateway_info_->MACAddress = "timeout";
	g_Gateway_info_->connectionStatus = "timeout";

	request.clear();

	request = "http://";
	request.append(gStrDeviceIP);
	request.append("/stw-cgi/network.cgi?msubmenu=interface&action=view");

#if 1  // 2019.09.05 hwanjang - sunapi debugging
	printf("sunapi_manager::GetNetworkInterfaceOfGateway() -> 2. SUNAPI Request : %s\n", request.c_str());
#endif

	strSUNAPIResult.clear();

	res = CURL_Process(json_mode, ssl_opt, CURL_TIMEOUT, request, gStrDevicePW, &strSUNAPIResult);

	g_Gateway_info_->curl_responseCode = res;

	if (res == CURLE_OK)
	{
		g_Gateway_info_->connectionStatus = "on";

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
		if ((res == CURLE_LOGIN_DENIED) || (res == CURLE_AUTH_ERROR))
		{
			// authfail
			strError = "authError";
			g_Gateway_info_->IPv4Address = "authError";
			g_Gateway_info_->MACAddress = "authError";
			g_Gateway_info_->connectionStatus = "authError";
		}
		else if (res == CURLE_OPERATION_TIMEDOUT)
		{
			// timeout
		}
		else {
			// another error		
			//strError = "unknown";
			strError = curl_easy_strerror(res);

			printf("GetNetworkInterfaceOfGateway() -> CURL_Process fail .. strError : %s\n", strError.c_str());
			g_Gateway_info_->IPv4Address = "unknown";
			g_Gateway_info_->MACAddress = "unknown";
			g_Gateway_info_->connectionStatus = "unknown";
		}

	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Reset
void sunapi_manager::ResetGatewayInfo()
{
	g_Gateway_info_->curl_responseCode = 0;
	g_Gateway_info_->WebPort = 0;
	g_Gateway_info_->Model.clear();
	g_Gateway_info_->IPv4Address.clear();
	g_Gateway_info_->MACAddress.clear();
	g_Gateway_info_->FirmwareVersion.clear();
	g_Gateway_info_->connectionStatus.clear();	
}

void ResetNetworkInterfaceOfGateway()
{
	g_Gateway_info_->curl_responseCode = 0;
	g_Gateway_info_->IPv4Address.clear();
	g_Gateway_info_->MACAddress.clear();
	g_Gateway_info_->connectionStatus.clear();

}
void ResetDeviceInfoOfGateway()
{
	g_Gateway_info_->curl_responseCode = 0;
	g_Gateway_info_->Model.clear();
	g_Gateway_info_->FirmwareVersion.clear();
	g_Gateway_info_->connectionStatus.clear();
}

void sunapi_manager::ResetStorageInfos()
{
	for (int i = 0; i < gMax_Channel + 1; i++)
	{
		// storage info
		g_Storage_info_[i].update_check = false;
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
		g_Worker_Storage_info_[i].update_check = false;
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
	g_Storage_info_[channel].update_check = false;
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
	g_Worker_Storage_info_[channel].update_check = false;
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
	for (int i = 0; i < gMax_Channel + 1; i++)
	{
		// subdevice info
		g_SubDevice_info_[i].update_check_deviceinfo = false;
		g_SubDevice_info_[i].update_check_networkinterface = false;
		g_SubDevice_info_[i].IsBypassSupported = false;
		g_SubDevice_info_[i].curl_responseCode = 0;
		g_SubDevice_info_[i].ConnectionStatus = 0;
		g_SubDevice_info_[i].HTTPPort = 0;
		g_SubDevice_info_[i].ChannelStatus.clear();
		g_SubDevice_info_[i].Model.clear();
		g_SubDevice_info_[i].DeviceName.clear();
		g_SubDevice_info_[i].IPv4Address.clear();
		g_SubDevice_info_[i].LinkStatus.clear();
		g_SubDevice_info_[i].MACAddress.clear();
		g_SubDevice_info_[i].UserID.clear();
		g_SubDevice_info_[i].FirmwareVersion.clear();

		// worker
		g_Worker_SubDevice_info_[i].update_check_deviceinfo = false;
		g_Worker_SubDevice_info_[i].update_check_networkinterface = false;
		g_Worker_SubDevice_info_[i].IsBypassSupported = false;
		g_Worker_SubDevice_info_[i].curl_responseCode = 0;
		g_Worker_SubDevice_info_[i].ConnectionStatus = 0;
		g_Worker_SubDevice_info_[i].HTTPPort = 0;
		g_Worker_SubDevice_info_[i].ChannelStatus.clear();
		g_Worker_SubDevice_info_[i].Model.clear();
		g_Worker_SubDevice_info_[i].DeviceName.clear();
		g_Worker_SubDevice_info_[i].IPv4Address.clear();
		g_Worker_SubDevice_info_[i].LinkStatus.clear();
		g_Worker_SubDevice_info_[i].MACAddress.clear();
		g_Worker_SubDevice_info_[i].UserID.clear();
		g_Worker_SubDevice_info_[i].FirmwareVersion.clear();
	}
}


void sunapi_manager::ResetSubdeviceInfoForChannel(int channel)
{
	// subdevice info
	g_SubDevice_info_[channel].update_check_deviceinfo = false;
	g_SubDevice_info_[channel].update_check_networkinterface = false;
	g_SubDevice_info_[channel].IsBypassSupported = false;
	g_SubDevice_info_[channel].curl_responseCode = 0;
	g_SubDevice_info_[channel].ConnectionStatus = 0;
	g_SubDevice_info_[channel].HTTPPort = 0;
	g_SubDevice_info_[channel].ChannelStatus.clear();
	g_SubDevice_info_[channel].Model.clear();
	g_SubDevice_info_[channel].DeviceName.clear();
	g_SubDevice_info_[channel].IPv4Address.clear();
	g_SubDevice_info_[channel].LinkStatus.clear();
	g_SubDevice_info_[channel].MACAddress.clear();
	g_SubDevice_info_[channel].UserID.clear();
	g_SubDevice_info_[channel].FirmwareVersion.clear();

	// worker
	g_Worker_SubDevice_info_[channel].update_check_deviceinfo = false;
	g_Worker_SubDevice_info_[channel].update_check_networkinterface = false;
	g_Worker_SubDevice_info_[channel].IsBypassSupported = false;
	g_Worker_SubDevice_info_[channel].curl_responseCode = 0;
	g_Worker_SubDevice_info_[channel].ConnectionStatus = 0;
	g_Worker_SubDevice_info_[channel].HTTPPort = 0;
	g_Worker_SubDevice_info_[channel].ChannelStatus.clear();
	g_Worker_SubDevice_info_[channel].Model.clear();
	g_Worker_SubDevice_info_[channel].DeviceName.clear();
	g_Worker_SubDevice_info_[channel].IPv4Address.clear();
	g_Worker_SubDevice_info_[channel].LinkStatus.clear();
	g_Worker_SubDevice_info_[channel].MACAddress.clear();
	g_Worker_SubDevice_info_[channel].UserID.clear();
	g_Worker_SubDevice_info_[channel].FirmwareVersion.clear();
}

void sunapi_manager::ResetFirmwareVersionInfos()
{
	for (int i = 0; i < gMax_Channel + 1; i++)
	{
		// subdevice info
		g_Firmware_Ver_info_[i].update_check = false;
		g_Firmware_Ver_info_[i].FirmwareVersion.clear();
		g_Firmware_Ver_info_[i].LatestFirmwareVersion.clear();

		// worker
		g_Worker_Firmware_Ver_info_[i].update_check = false;
		g_Worker_Firmware_Ver_info_[i].FirmwareVersion.clear();
		g_Worker_Firmware_Ver_info_[i].LatestFirmwareVersion.clear();
	}
}


void sunapi_manager::ResetFirmwareVersionInfoForChannel(int channel)
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
// end of reset
////////////////////////////////////////////////////////////////////////////////////////////////////


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
		g_SubDevice_info_[i].FirmwareVersion = g_Worker_SubDevice_info_[i].FirmwareVersion;
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
// end of update
////////////////////////////////////////////////////////////////////////////////////////////////////


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

#if 1  // 2019.09.05 hwanjang - sunapi debugging
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
			if((long)chunk.size != 0) // 200 ok , 301 moved , 404 error
			{		
				// OK
				*strResult = chunk.memory;

				printf("CURL_Process() -> strResult : \n%s\n", chunk.memory);
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
			printf("[hwanjang] ByPassSUNAPI() -> result string is empty !!!\n");
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
		printf("failed  ... ByPassSUNAPI() -> CURLE_OPERATION_TIMEDOUT ... request URL : %s\n", request.c_str());
		return false;
	}
	else
	{
		printf("failed  ... ByPassSUNAPI() -> %d .. request URL : %s\n", res, request.c_str());		
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
		printf("GetMaxChannelByAttribute() -->  ... res : %d!!!\n", res);
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
		bool result = GetRegiesteredCameraStatus(gStrDeviceIP, gStrDevicePW, &resCode);

		if (!result)
		{
			printf("failed to get GetRegiesteredCameraStatus ... 1");

			result = GetRegiesteredCameraStatus(gStrDeviceIP, gStrDevicePW, &resCode);
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

// status of regiestered sub device 
bool sunapi_manager::GetRegiesteredCameraStatus(const std::string deviceIP, const std::string devicePW, CURLcode* resCode)
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

					ResetSubdeviceInfoForChannel(index);
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


// Send error response for gateway - authfail , timeout , unknown ...
void sunapi_manager::SendErrorResponseForGateway(const std::string& strTopic, json_t* json_strRoot, std::string strError)
{
	printf("SendErrorResponseForGateway() -> gateway fail ...\n");

	// sub of message
	std::string strDeviceType = DEVICE_TYPE;

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
	g_Worker_Storage_info_[channel].das_status = "timeout";
	g_Worker_Storage_info_[channel].nas_status = "timeout";

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
		// 
		if (strByPassResult.empty())
		{
			// fail
			printf("channel : %d, strByPassResult is empty !!! \n", channel);

			// DAS			
			g_Worker_Storage_info_[channel].das_status.empty();
			// NAS			
			g_Worker_Storage_info_[channel].nas_status.empty();
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
				g_Worker_Storage_info_[channel].das_status.empty();
				// NAS			
				g_Worker_Storage_info_[channel].nas_status.empty();
			}
			else
			{
				int ret;

				json_t* json_array = json_object_get(json_strRoot, "Storages");

				if (!json_array)
				{
					printf("NG1 ... thread_function_for_storage_status() -> channel : %d, json_is wrong .. strByPassResult : \n%s\n",
						channel, strByPassResult.c_str());

					// DAS			
					g_Worker_Storage_info_[channel].das_status.empty();
					// NAS			
					g_Worker_Storage_info_[channel].nas_status.empty();
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

							// DAS			
							g_Worker_Storage_info_[channel].das_status.empty();
							// NAS			
							g_Worker_Storage_info_[channel].nas_status.empty();
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
	else  // fail
	{
		if ((resCode == CURLE_LOGIN_DENIED) || (resCode == CURLE_AUTH_ERROR))
		{
			g_Worker_Storage_info_[channel].das_status = "authfail";
			g_Worker_Storage_info_[channel].nas_status = "authfail";
		}
		else if (resCode == CURLE_OPERATION_TIMEDOUT)
		{
			// timeout
			g_Worker_Storage_info_[channel].das_status = "timeout";
			g_Worker_Storage_info_[channel].nas_status = "timeout";
		}
		else{			
			std::string curl_error = std::to_string(resCode);

			// DAS			
			g_Worker_Storage_info_[channel].das_status = curl_error;
			// NAS			
			g_Worker_Storage_info_[channel].nas_status = curl_error;
		}
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

void sunapi_manager::SendResponseForDashboardView(const std::string& strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
	std::string strVersion = "1.5";
	// sub of message
	std::string strDeviceType = DEVICE_TYPE;

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

	std::string strStatus;

	for (i = 0; i < gMax_Channel; i++)
	{
		if (g_SubDevice_info_[i].ConnectionStatus != 0)
		{
			strStatus.clear();

			// storage of subdevice
			// DAS
			if (g_Storage_info_[i].das_enable)
			{				
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

			if (!strStatus.empty())
			{
				memset(charCh, 0, sizeof(charCh));
				sprintf(charCh, "%d", i);  // int -> char*
				json_object_set_new(sub_ConnectionMsg, charCh, json_string(strStatus.c_str()));
				connectionMsgCnt++;				
			}
			else
			{
				// connection of subdevice
				if (g_SubDevice_info_[i].ChannelStatus.empty() != true)
				{
					memset(charCh, 0, sizeof(charCh));
					sprintf(charCh, "%d", i);  // int -> char*
					json_object_set_new(sub_ConnectionMsg, charCh, json_string(g_SubDevice_info_[i].ChannelStatus.c_str()));
					connectionMsgCnt++;
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

	json_object_set(sub_Msg, "subdevice", sub_SubdeviceMsg);

	json_object_set(sub_Msg, "connection", json_string(g_Gateway_info_->connectionStatus.c_str()));

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

			sleep_for(std::chrono::milliseconds(SLEEP_TIME));
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

	g_Worker_SubDevice_info_[channel].update_check_deviceinfo = false;

	// reset timeout
	g_Worker_SubDevice_info_[channel].curl_responseCode = CURLE_OPERATION_TIMEDOUT;
	g_Worker_SubDevice_info_[channel].Model = "timeout";
	g_Worker_SubDevice_info_[channel].FirmwareVersion = "timeout";
	g_Worker_SubDevice_info_[channel].DeviceName = "timeout";

	bool result = false;

	std::string strByPassResult;

	std::string strByPassURI = "/stw-cgi/system.cgi?msubmenu=deviceinfo&action=view";

	bool json_mode = true;
	CURLcode resCode;
	result = ByPassSUNAPI(channel, json_mode, deviceIP, devicePW, strByPassURI, &strByPassResult, &resCode);

	g_Worker_SubDevice_info_[channel].curl_responseCode = resCode;

	if (result)
	{
		if (strByPassResult.empty())
		{
			printf("[hwanjang] Attributes string is empty !!!\n");
			g_Worker_SubDevice_info_[channel].Model.empty();
			g_Worker_SubDevice_info_[channel].FirmwareVersion.empty();
			g_Worker_SubDevice_info_[channel].DeviceName.empty();
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

				g_Worker_SubDevice_info_[channel].Model.empty();
				g_Worker_SubDevice_info_[channel].FirmwareVersion.empty();
				g_Worker_SubDevice_info_[channel].DeviceName.empty();
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

					g_Worker_SubDevice_info_[channel].Model.clear();
				}
				else
				{
					g_Worker_SubDevice_info_[channel].Model = charModel;
				}

				// get DeviceName
				char* charDeviceName;
				result = json_unpack(json_strRoot, "{s:s}", "DeviceName", &charDeviceName);

				if (result)
				{
					printf("[hwanjang] Error !! thread_function_for_subdevice_info() -> 4. json_unpack fail .. DeviceName ...\n");
					printf("thread_function_for_subdevice_info() -> channel : %d , strSUNAPIResult : \n%s\n",
						channel, strByPassResult.c_str());

					g_Worker_SubDevice_info_[channel].DeviceName.clear();
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

					g_Worker_SubDevice_info_[channel].FirmwareVersion.clear();
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
					printf("[hwanjang] Error !! thread_function_for_subdevice_info() -> 3. json_unpack fail .. ConnectedMACAddress ...\n");
					printf("thread_function_for_subdevice_info() -> channel : %d , strSUNAPIResult : \n%s\n", 
							channel, strByPassResult.c_str());

					g_Worker_SubDevice_info_[channel].MACAddress.empty();
				}
				else
				{					
					g_Worker_SubDevice_info_[channel].MACAddress = charMACAddress;
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
			g_Worker_SubDevice_info_[channel].Model = "authfail";
			g_Worker_SubDevice_info_[channel].FirmwareVersion = "authfail";
			g_Worker_SubDevice_info_[channel].DeviceName = "authfail";
		}
		else if (resCode == CURLE_OPERATION_TIMEDOUT)
		{
			// timeout
			g_Worker_SubDevice_info_[channel].Model = "timeout";
			g_Worker_SubDevice_info_[channel].FirmwareVersion = "timeout";
			g_Worker_SubDevice_info_[channel].DeviceName = "timeout";
		}
		else {
			// unknown error
			std::string curl_error = std::to_string(resCode);

			g_Worker_SubDevice_info_[channel].Model = curl_error;
			g_Worker_SubDevice_info_[channel].FirmwareVersion = curl_error;
			g_Worker_SubDevice_info_[channel].DeviceName = curl_error;

		}
	}

	g_Worker_SubDevice_info_[channel].update_check_deviceinfo = true;

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

			sleep_for(std::chrono::milliseconds(SLEEP_TIME));
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

	g_Worker_SubDevice_info_[channel].update_check_networkinterface = false;

	// reset timeout
	g_Worker_SubDevice_info_[channel].curl_responseCode = CURLE_OPERATION_TIMEDOUT;
	g_Worker_SubDevice_info_[channel].LinkStatus = "timeout";
	g_Worker_SubDevice_info_[channel].IPv4Address = "timeout";
	g_Worker_SubDevice_info_[channel].MACAddress = "timeout";

	bool result = false;

	std::string strByPassResult;
	
	std::string strByPassURI = "/stw-cgi/network.cgi?msubmenu=interface&action=view";

	bool json_mode = true;
	CURLcode resCode;
	result = ByPassSUNAPI(channel, json_mode, deviceIP, devicePW, strByPassURI, &strByPassResult, &resCode);

	g_Worker_SubDevice_info_[channel].curl_responseCode = resCode;

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

				g_Worker_SubDevice_info_[channel].LinkStatus.clear();
				g_Worker_SubDevice_info_[channel].IPv4Address.clear();
				g_Worker_SubDevice_info_[channel].MACAddress.clear();
			}
			else
			{
				json_t* json_array = json_object_get(json_strRoot, "NetworkInterfaces");

				if (!json_array)
				{
					printf("NG1 ... thread_function_for_network_interface() -> json_is wrong !!!\n");

					printf("channel : %d , strByPassResult : %s\n", channel, strByPassResult.c_str());

					g_Worker_SubDevice_info_[channel].LinkStatus.clear();
					g_Worker_SubDevice_info_[channel].IPv4Address.clear();
					g_Worker_SubDevice_info_[channel].MACAddress.clear();
				}
				else
				{

					int ret, index;
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

							g_Worker_SubDevice_info_[channel].LinkStatus.clear();
							g_Worker_SubDevice_info_[channel].IPv4Address.clear();
							g_Worker_SubDevice_info_[channel].MACAddress.clear();
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
	}
	else
	{
		printf("error ... thread_function_for_network_interface() -> channel : %d , ByPassSUNAPI() -> return error !!\n", channel);

		g_Worker_SubDevice_info_[channel].LinkStatus.empty();
		g_Worker_SubDevice_info_[channel].IPv4Address.empty();
		g_Worker_SubDevice_info_[channel].MACAddress.empty();
	}

	g_Worker_SubDevice_info_[channel].update_check_networkinterface = true;

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

void sunapi_manager::SendResponseForDeviceInfoView(const std::string& strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
#if 1
	std::string strVersion = "1.5";
	// sub of message
	std::string strDeviceType = DEVICE_TYPE;

	json_t* main_ResponseMsg, *sub_Msg, *sub_SubdeviceMsg, * sub_DeviceNameMsg, *sub_IPAddressMsg, *sub_WebprotMsg, *sub_MacAddressMsg;

	main_ResponseMsg = json_object();
	sub_Msg = json_object();
	sub_SubdeviceMsg = json_object();
	sub_DeviceNameMsg = json_object();
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
	int i = 0, devNameMsgCnt=0, ipaddressMsgCnt=0, webportMsgCnt = 0, macaddressMsgCnt = 0;
	char charCh[8];

	for (i = 0; i < gMax_Channel; i++)
	{
		if (g_SubDevice_info_[i].ConnectionStatus != 0)
		{			
			memset(charCh, 0, sizeof(charCh));
			sprintf(charCh, "%d", i);  // int -> char*

			// devicename of subdevice
			if (g_SubDevice_info_[i].DeviceName.empty() != true)
			{
				json_object_set_new(sub_DeviceNameMsg, charCh, json_string(g_SubDevice_info_[i].DeviceName.c_str()));
				devNameMsgCnt++;
			}

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

	// 21.09.06 add - deviceName 
	if (devNameMsgCnt > 0)
	{
		json_object_set(sub_SubdeviceMsg, "subdeviceName", sub_DeviceNameMsg);
	}

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
	//json_object_set(sub_Msg, "deviceId", json_string(g_Device_id.c_str()));
	//json_object_set(sub_Msg, "deviceType", json_string(strDeviceType.c_str()));
	json_object_set(sub_Msg, "ipaddress", json_string(g_Gateway_info_->IPv4Address.c_str()));
	json_object_set(sub_Msg, "webport", json_integer(g_Gateway_info_->WebPort));
	json_object_set(sub_Msg, "macaddress", json_string(g_Gateway_info_->MACAddress.c_str()));
	json_object_set(sub_Msg, "connection", json_string(g_Gateway_info_->connectionStatus.c_str()));

	json_object_set(sub_Msg, "subdevice", sub_SubdeviceMsg);

	//main_Msg["message"] = sub_Msg;
	json_object_set(main_ResponseMsg, "message", sub_Msg);

	//main_Msg["tid"] = strTid;
	json_object_set(main_ResponseMsg, "tid", json_string(strTid.c_str()));
	//main_Msg["version"] = "1.5";
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


bool sunapi_manager::GetDeviceInfoOfGateway()
{
	// reset
	g_Gateway_info_->curl_responseCode = CURLE_OPERATION_TIMEDOUT;
	g_Gateway_info_->Model = "timeout";
	g_Gateway_info_->FirmwareVersion = "timeout";
	g_Gateway_info_->connectionStatus = "timeout";

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

	g_Gateway_info_->curl_responseCode = res;

	if (res == CURLE_OK)
	{
		g_Gateway_info_->connectionStatus = "on";

		if (strSUNAPIResult.empty())
		{
			printf("[hwanjang] GetDeviceInfoOfGateway() strSUNAPIResult is empty !!!\n");

			g_Gateway_info_->Model.clear();
			g_Gateway_info_->FirmwareVersion.clear();

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

				g_Gateway_info_->Model.clear();
				g_Gateway_info_->FirmwareVersion.clear();

				return false;
			}

			// get Model
			char* charModel;
			int result = json_unpack(json_strRoot, "{s:s}", "Model", &charModel);

			if (result)
			{
				printf("[hwanjang] Error !! GetDeviceInfoOfGateway() -> 1. json_unpack fail .. Model ...\n");
				printf("GetDeviceInfoOfGateway() -> strSUNAPIResult : \n%s\n", strSUNAPIResult.c_str());		

				g_Gateway_info_->Model.clear();
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

				g_Gateway_info_->FirmwareVersion.clear();
			}
			else
			{
				g_Gateway_info_->FirmwareVersion = charFirmwareVersion;
			}

#if 0  // not implemented
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
#endif

		}
	}
	else
	{
		printf("GetDeviceInfoOfGateway() -> error cURL res : %d!!!\n", res);
		std::string strError;
		// CURL_Process fail
		if ((res == CURLE_LOGIN_DENIED) || (res == CURLE_AUTH_ERROR))
		{
			// authfail
			strError = "authError";
			g_Gateway_info_->Model = "authError";
			g_Gateway_info_->FirmwareVersion = "authError";
			g_Gateway_info_->connectionStatus = "authError";
		}
		else if (res == CURLE_OPERATION_TIMEDOUT)
		{
			// timeout
		}
		else {
			// another error		
			//strError = "unknown";
			strError = curl_easy_strerror(res);

			printf("GetDeviceInfoOfGateway() -> CURL_Process fail .. strError : %s\n", strError.c_str());
			g_Gateway_info_->Model = "unknown";
			g_Gateway_info_->FirmwareVersion = "unknown";
			g_Gateway_info_->connectionStatus = "unknown";
			
		}

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
			//printf("GetFirmwareVersionOfSubdevices() -> index : %d ... ConnectionStatus : %d\n", i, g_Worker_SubDevice_info_[i].ConnectionStatus);

			ThreadStartForFirmwareVersion(i, deviceIP, devicePW);

			sleep_for(std::chrono::milliseconds(SLEEP_TIME));
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
	//printf("thread_function_for_firmware_version() -> channel : %d , Start to get FirmwareVersion ...\n", channel);

	g_Worker_Firmware_Ver_info_[channel].update_check = false;

	// reset timeout
	g_Worker_Firmware_Ver_info_[channel].curl_responseCode = CURLE_OPERATION_TIMEDOUT;
	g_Worker_Firmware_Ver_info_[channel].FirmwareVersion = "timeout";

	g_Worker_SubDevice_info_[channel].Model = "timeout";
	g_Worker_SubDevice_info_[channel].FirmwareVersion = "timeout";
	g_Worker_SubDevice_info_[channel].DeviceName = "timeout";

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
			g_Worker_Firmware_Ver_info_[channel].FirmwareVersion.clear();

			g_Worker_SubDevice_info_[channel].Model.empty();
			g_Worker_SubDevice_info_[channel].FirmwareVersion.empty();
			g_Worker_SubDevice_info_[channel].DeviceName.empty();
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
					g_Worker_Firmware_Ver_info_[channel].FirmwareVersion.clear();

					g_Worker_SubDevice_info_[channel].FirmwareVersion.empty();
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

						g_Worker_SubDevice_info_[channel].FirmwareVersion = strResult;

						printf("thread_function_for_firmware_version() -> 2 ... channel : %d, FirmwareVersion : %s\n",
							channel, strResult.c_str());
					}
					else
					{
						printf("thread_function_for_firmware_version() -> 2 ... channel : %d, not found firmware version ...\n", channel);
						g_Worker_Firmware_Ver_info_[channel].FirmwareVersion.clear();

						g_Worker_SubDevice_info_[channel].FirmwareVersion.empty();
					}
				}
				else
				{
					g_Worker_Firmware_Ver_info_[channel].FirmwareVersion = charFWVersion;


					g_Worker_SubDevice_info_[channel].FirmwareVersion = charFWVersion;
					printf("thread_function_for_firmware_version() -> channel : %d, FirmwareVersion : %s\n",
						channel, charFWVersion);					
				}

				// get Model
				char* charModel;
				ret = json_unpack(json_strRoot, "{s:s}", "Model", &charModel);

				if (ret)
				{
					printf("[hwanjang] Error !! thread_function_for_firmware_version() -> 2. json_unpack fail .. Model ...\n");
					printf("thread_function_for_firmware_version() -> channel : %d , strSUNAPIResult : \n%s\n",
						channel, strByPassResult.c_str());

					g_Worker_SubDevice_info_[channel].Model.clear();
				}
				else
				{
					g_Worker_SubDevice_info_[channel].Model = charModel;
				}

				// get DeviceName
				char* charDeviceName;
				ret = json_unpack(json_strRoot, "{s:s}", "DeviceName", &charDeviceName);

				if (ret)
				{
					printf("[hwanjang] Error !! thread_function_for_firmware_version() -> 3. json_unpack fail .. DeviceName ...\n");
					printf("thread_function_for_firmware_version() -> channel : %d , strSUNAPIResult : \n%s\n",
						channel, strByPassResult.c_str());

					g_Worker_SubDevice_info_[channel].DeviceName.clear();
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
		// fail
		printf("thread_function_for_firmware_version() -> channel : %d , fail to get FirmwareVersion !!!\n", channel);

		if ((resCode == CURLE_LOGIN_DENIED) || (resCode == CURLE_AUTH_ERROR))
		{
			g_Worker_Firmware_Ver_info_[channel].FirmwareVersion = "authfail";

			g_Worker_SubDevice_info_[channel].Model = "authfail";
			g_Worker_SubDevice_info_[channel].FirmwareVersion = "authfail";
			g_Worker_SubDevice_info_[channel].DeviceName = "authfail";
		}
		else if (resCode == CURLE_OPERATION_TIMEDOUT)
		{
			// timeout
			g_Worker_Firmware_Ver_info_[channel].FirmwareVersion = "timeout";

			g_Worker_SubDevice_info_[channel].Model = "timeout";
			g_Worker_SubDevice_info_[channel].FirmwareVersion = "timeout";
			g_Worker_SubDevice_info_[channel].DeviceName = "timeout";
		}
		else {
			// unknown error
			std::string curl_error = std::to_string(resCode);

			g_Worker_Firmware_Ver_info_[channel].FirmwareVersion = curl_error;

			g_Worker_SubDevice_info_[channel].Model = curl_error;
			g_Worker_SubDevice_info_[channel].FirmwareVersion = curl_error;
			g_Worker_SubDevice_info_[channel].DeviceName = curl_error;
		}

	}

	g_Worker_Firmware_Ver_info_[channel].update_check = true;
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////
// 3. Send firmware version of subdevices 

void sunapi_manager::SendResponseForFirmwareView(const std::string& strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
	std::string strVersion = "1.5";
	// sub of message
	std::string strDeviceType = DEVICE_TYPE;

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
	//json_object_set(sub_Msg, "deviceId", json_string(g_Device_id.c_str()));
	//json_object_set(sub_Msg, "deviceType", json_string(strDeviceType.c_str()));
	if (g_Gateway_info_->FirmwareVersion.empty() != true)
		json_object_set(sub_Msg, "firmwareVersion", json_string(g_Gateway_info_->FirmwareVersion.c_str()));

	json_object_set(sub_Msg, "connection", json_string(g_Gateway_info_->connectionStatus.c_str()));
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
		// reset LatestFirmwareVersion and then update ...
		g_Worker_Firmware_Ver_info_[i].LatestFirmwareVersion.clear();

		if (g_Worker_SubDevice_info_[i].ConnectionStatus != 0)
		{
			for (j = 0; j < g_vecFirmwareInfos.size(); j++)
			{
				if (g_Worker_SubDevice_info_[i].Model.find(g_vecFirmwareInfos[j].model) != std::string::npos)
				{					
					g_Worker_Firmware_Ver_info_[i].LatestFirmwareVersion = g_vecFirmwareInfos[j].version_info;

					printf("GetLatestFirmwareVersionFromFile() -> channel : %d, Model : %s , LatestFirmwareVersion : %s\n",
								i, g_Worker_SubDevice_info_[i].Model.c_str(), g_Worker_Firmware_Ver_info_[i].LatestFirmwareVersion.c_str());
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
	FILE* pagefile;

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
		pagefile = fopen(pagefilename, "wb");
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

//	int result = GetLatestFirmwareVersionFromFile(pagefilename);

	return true;
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
}

// 1. interface for dashboard - storage
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

	char* strCommand, *strType, *strView, *strTid;
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

		sleep_for(std::chrono::milliseconds(10));

		GetMacAddressOfSubdevices();

		sleep_for(std::chrono::milliseconds(1 * 1000));
	}
	else
	{
		printf("check %ld ... Skip !!!!!\n", (long int)(update_time - g_UpdateTimeOfNetworkInterface));

		// skip to check
		Set_update_checkForDeviceInfo();
		Set_update_checkForNetworkInterface();
	}
#endif	

	char* strCommand, * strType, * strView, * strTid;
	result = json_unpack(json_strRoot, "{s:s, s:s, s:s, s:s}", "command", &strCommand, "type", &strType, "view", &strView, "tid", &strTid);

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
	printf("GetFirmwareVersionInfoView() -> Max Channel : %d ... Firmware Version Info ...\n", gMax_Channel);

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
		Set_update_check_Firmware_Ver_info_ForFirmwareVersion();
	}

	char* strCommand, *strType, *strView, *strTid;
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
	char* strCommand, *strType, *strView, *strTid;
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

		int index;
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
		request.append(gStrDeviceIP);

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

		res = CURL_Process(json_mode, ssl_opt, CURL_TIMEOUT, request, gStrDevicePW, &strSUNAPIResult);

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


////////////////////////////////////////////////////////////////////////////////////////////////
// 4. Send result of firmware update 

void sunapi_manager::SendResponseForUpdateFirmware(const std::string& strTopic,
			std::string strCommand, std::string strType, std::string strView, std::string strTid, int resCode, int rawCode, std::vector<int> updateChannel)
{
#if 1
	std::string strVersion = "1.5";
	// sub of message
	std::string strDeviceType = DEVICE_TYPE;

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

	// Get ConnectionStatus of subdevice
	CURLcode resCode;
	bool result = GetRegiesteredCameraStatus(gStrDeviceIP, gStrDevicePW, &resCode);

	if (!result)
	{
		printf("failed to get GetRegiesteredCameraStatus ... 1");

		result = GetRegiesteredCameraStatus(gStrDeviceIP, gStrDevicePW, &resCode);
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

int sunapi_manager::ThreadStartSendResponseForDashboardView(const std::string strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
	int maxChannel = gMax_Channel;
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

				SendResponseForDashboardView(strTopic, strCommand, strType, strView, strTid);

				Reset_update_checkForStorageInfo();

				time_t end_time = time(NULL);
				std::cout << "nthread_function_for_send_response_for_dashboard() -> End ... time : " << (long int)end_time << " , diff : "
					<< (long int)(end_time - g_UpdateTimeOfStorageStatus) << std::endl;

				return;
			}

		}

		printf("nthread_function_for_send_response_for_dashboard() -> i : %d\n", i);

		quit_t = time(NULL);

		if ((quit_t - start_t) > WAIT_TIMEOUT)
		{
			printf("\n#############################################################################\n");
			printf("\nthread_function_for_send_response_for_dashboard() -> timeover ... quit !!! \n ");

			UpdateStorageInfos();

			SendResponseForDashboardView(strTopic, strCommand, strType, strView, strTid);

			Reset_update_checkForStorageInfo();

			std::cout << "nthread_function_for_send_response_for_dashboard() -> End ... time : " << (long int)quit_t << " , diff : "
				<< (long int)(quit_t - start_t) << std::endl;

			return;
		}

		sleep_for(std::chrono::milliseconds(1));
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2. device info view
void sunapi_manager::Set_update_checkForDeviceInfo()
{
	// for skip to check
	for (int i = 0; i < gMax_Channel+1; i++)
	{
		g_Worker_SubDevice_info_[i].update_check_deviceinfo = true;
	}
}

void sunapi_manager::Reset_update_checkForDeviceInfo()
{
	for (int i = 0; i < gMax_Channel+1; i++)
	{
		g_Worker_SubDevice_info_[i].update_check_deviceinfo = false;
	}

#if 0  // for test
	// Get ConnectionStatus of subdevice
	bool result = GetRegiesteredCameraStatus(gStrDeviceIP, gStrDevicePW);

	if (!result)
	{
		printf("failed to get 2. connectionStatus of subdevice ... ");
	}
#endif
}

void sunapi_manager::Set_update_checkForNetworkInterface()
{
	// for skip to check
	for (int i = 0; i < gMax_Channel+1; i++)
	{
		g_Worker_SubDevice_info_[i].update_check_networkinterface = true;
	}
}

void sunapi_manager::Reset_update_checkForNetworkInterface()
{
	for (int i = 0; i < gMax_Channel+1; i++)
	{
		g_Worker_SubDevice_info_[i].update_check_networkinterface = false;
	}

#if 0  // for test
	// Get ConnectionStatus of subdevice
	bool result = GetRegiesteredCameraStatus(gStrDeviceIP, gStrDevicePW);

	if (!result)
	{
		printf("failed to get 2. connectionStatus of subdevice ... ");
	}
#endif
}

int sunapi_manager::ThreadStartSendResponseForDeviceInfoView(const std::string strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
	int maxChannel = gMax_Channel;
	std::thread thread_function([=] { thread_function_for_send_response_for_deviceInfo(maxChannel, strTopic, strCommand, strType, strView, strTid); });
	thread_function.detach();

	return 0;
}

void sunapi_manager::thread_function_for_send_response_for_deviceInfo(int maxChannel, const std::string strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
	printf("\thread_function_for_send_response_for_deviceInfo() -> Start .... \n ");

	time_t start_t = time(NULL);
	time_t quit_t;

	int i = 0;
	while (1)
	{
		for (i = 0; i < maxChannel; i++)
		{
			if (g_SubDevice_info_[i].ConnectionStatus != 0)
			{
				if( (g_Worker_SubDevice_info_[i].update_check_deviceinfo != true)
					|| (g_Worker_SubDevice_info_[i].update_check_networkinterface != true) )
				{
					printf("thread_function_for_send_response_for_deviceInfo() -> i : %d  --> break !!!\n", i);
					break;
				}
			}

			if ((maxChannel - 1) == i)
			{
				printf("\n#############################################################################\n");
				printf("\thread_function_for_send_response_for_deviceInfo() -> Update & Reday to Send  .... \n ");
				UpdateSubdeviceInfos();

				SendResponseForDeviceInfoView(strTopic, strCommand, strType, strView, strTid);

				Reset_update_checkForDeviceInfo();
				Reset_update_checkForNetworkInterface();


				time_t end_time = time(NULL);
				std::cout << "thread_function_for_send_response_for_deviceInfo() -> End ... time : " << (long int)end_time << " , diff : "
					<< (long int)(end_time - g_UpdateTimeOfNetworkInterface) << std::endl;

				return;
			}

		}

		printf("thread_function_for_send_response_for_deviceInfo() -> i : %d\n", i);

		quit_t = time(NULL);

		if( (quit_t - start_t) > WAIT_TIMEOUT)
		{
			printf("\n#############################################################################\n");
			printf("\thread_function_for_send_response_for_deviceInfo() -> timeover ... quit !!! \n ");

			UpdateSubdeviceInfos();

			SendResponseForDeviceInfoView(strTopic, strCommand, strType, strView, strTid);

			Reset_update_checkForDeviceInfo();
			Reset_update_checkForNetworkInterface();

			std::cout << "thread_function_for_send_response_for_deviceInfo() -> End ... time : " << (long int)quit_t << " , diff : "
				<< (long int)(quit_t - start_t) << std::endl;

			return;
		}

		sleep_for(std::chrono::milliseconds(1));
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 3. firmware version info view

void sunapi_manager::Set_update_check_Firmware_Ver_info_ForFirmwareVersion()
{
	// for skip to check
	for (int i = 0; i < gMax_Channel; i++)
	{
		g_Worker_Firmware_Ver_info_[i].update_check = true;
	}
}

void sunapi_manager::Reset_update_check_Firmware_Ver_info_ForFirmwareVersion()
{
	for (int i = 0; i < gMax_Channel; i++)
	{
		g_Worker_Firmware_Ver_info_[i].update_check = false;
	}
}

int sunapi_manager::ThreadStartSendResponseForFirmwareView(const std::string strTopic, 
			std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
	int maxChannel = gMax_Channel;
	std::thread thread_function([=] { thread_function_for_send_response_for_firmwareVersion(maxChannel, strTopic, strCommand, strType, strView, strTid); });
	thread_function.detach();

	return 0;
}

void sunapi_manager::thread_function_for_send_response_for_firmwareVersion(int maxChannel, const std::string strTopic, 
			std::string strCommand, std::string strType, std::string strView, std::string strTid)
{
	printf("\thread_function_for_send_response_for_firmwareVersion() -> Start .... \n ");

	time_t start_t = time(NULL);
	time_t quit_t;

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

				int result = GetLatestFirmwareVersionFromFile(FIRMWARE_INFO_FILE);

				UpdateFirmwareVersionInfos();

				SendResponseForFirmwareView(strTopic, strCommand, strType, strView, strTid);

				Reset_update_check_Firmware_Ver_info_ForFirmwareVersion();

				return;
			}

		}

		printf("thread_function_for_send_response_for_firmwareVersion() -> i : %d\n", i);


		quit_t = time(NULL);

		if ((quit_t - start_t) > WAIT_TIMEOUT)
		{
			printf("\n#############################################################################\n");
			printf("\thread_function_for_send_response_for_firmwareVersion() -> timeover ... quit !!! \n ");

			int result = GetLatestFirmwareVersionFromFile(FIRMWARE_INFO_FILE);

			UpdateFirmwareVersionInfos();

			SendResponseForFirmwareView(strTopic, strCommand, strType, strView, strTid);

			Reset_update_check_Firmware_Ver_info_ForFirmwareVersion();

			std::cout << "thread_function_for_send_response_for_firmwareVersion() -> End ... time : " << (long int)quit_t << " , diff : "
				<< (long int)(quit_t - start_t) << std::endl;

			return;
		}

		sleep_for(std::chrono::milliseconds(1));
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

#if 1
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

	//std::string strPassword = charID;
	std::string strPassword = "admin";
	strPassword.append(":");
	strPassword.append(charPassword);

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

	json_object_set(sub_Msg, "responseCode", json_integer(200));

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