#pragma once

#include <string>
#include <mutex>
#include <time.h>
#include <vector>

#include "curl/curl.h"

#include "jansson.h"
#include "setting_ini.h"

#include "define_for_debug.h"

typedef struct MemoryStruct {
	char* memory;
	size_t size;
} ChunkStruct;



/////////////////////////////////////////////////////////////////////////////
//
typedef struct discovered_camera_Info {
	std::string Model;
	std::string IPAddress;
	std::string MACAddress;
} DiscoveredCameras_Infos;

//////////////////////////////////////////////////////////////////////////////
// for firmware info
//
// https://update.websamsung.net/FW/Update_FW_Info3.txt
//
// QND-6010R,NW_Camera,img,QND6010R_Series,qnd6010r_Series_4.01_210706.img,4.01_210706,NO,NO,NO,,9187880702fc44f5341978a0f0580aa7
// ① 모델명
// ② 제품군
// ③ Firmware 이미지 확장자
// ④ 대표 모델
// ⑤ Firmware 를 포함하는 ZIP (또는 img) 파일명
// ⑥ Full 버전 정보
typedef struct firmware_update_Info {
	bool update_check;
	int curl_responseCode;
	std::string model;
	std::string product;
	std::string extension;
	std::string representative_model;
	std::string fileName;
	std::string version_info;
} Firmware_Infos;


//////////////////////////////////////////////////////////////////////////////
// for 1. dashboard view  - need to bypass
//
// http://<ip>/stw-cgi/eventstatus.cgi?msubmenu=eventstatus&action=check&SystemEvent=SDInsert
//  - das_presence
// http://<ip>/stw-cgi/eventstatus.cgi?msubmenu=eventstatus&action=check&SystemEvent=NASConnect
//  - nas_presence
//
// http://<ip>/stw-cgi/system.cgi?msubmenu=storageinfo&action=view
//  - das_enable , das_recording_status , nas_enable , nas_recording_status
//
// http://<ip>/stw-cgi/recording.cgi?msubmenu=storage&action=view (not use)
//  - das_enable , das_overwrite

typedef struct storage_Info {
	time_t storage_update_time;
	bool update_check;
	bool das_presence;
	bool sdfail;
	bool nas_presence;
	bool nasfail;
	bool das_enable;			// on or off of DAS recording
	bool nas_enable;			// on or off of NAS recording
	int CheckConnectionStatus;		// 0 : Disconnected , 1 : Success , 2 : ConnectFail , 3 : timeout , 4 : authError , 5 :unknown
	int curl_responseCode;
	std::string das_status;  // normal, Active, SDfailure, ... , timeout, authError
	std::string nas_status;  // normal, Active, failure, timeout, authError
	std::string ConnectionStatus;  // Disconnected , Success , ConnectFail
} Storage_Infos;


//////////////////////////////////////////////////////////////////////////////
// for 2. deviceinfo view  - need to bypass
//
// http://<IP>/stw-cgi/media.cgi?msubmenu=cameraregister&action=view
//  - Model, UserID , HTTPPort
// http://<IP>/stw-cgi/network.cgi?msubmenu=interface&action=view  : need to bypass
//  - LinkStatus , IPv4Address , MACAddess

typedef struct gateway_Info {
	int CheckConnectionStatus;		// 0 : Disconnected , 1 : Success , 2 : ConnectFail , 3 : timeout , 4 : authError , 5 : unknown
	int curl_responseCode;
	int HttpsPort;
	int maxChannel;
	std::string DeviceModel;
	std::string IPv4Address;
	std::string MACAddress;
	std::string FirmwareVersion;
	std::string ConnectionStatus;
} GatewayInfo;

typedef struct networkinterface_Info {
	bool update_check_networkinterface;
	int CheckConnectionStatus;		// 0 : Disconnected , 1 : Success , 2 : ConnectFail , 3 : timeout , 4 : authError , 5 : unknown
	std::string ConnectionStatus; // Disconnected , Success , ConnectFail
	std::string LinkStatus;		// Connected , Disconnected
	std::string IPv4Address;
	std::string MACAddress;
} SubDevice_NetworkInterface_Info;

typedef struct channel_Info {
	time_t channel_update_time;
	bool update_check_deviceinfo;
	bool IsBypassSupported;
	int CheckConnectionStatus;		// 0 : Disconnected , 1 : Success , 2 : ConnectFail , 3 : timeout , 4 : authError , 5 : unknown
	int curl_responseCode;
	int HTTPPort;
	std::string ConnectionStatus; // Disconnected , Success , ConnectFail , authError, timeout , unknown
	std::string DeviceModel;	// camera model
	std::string DeviceName;		// 21.09.06 add - sub device name
	std::string ChannelTitle;	// 21.10.27 add - sub Channel Title
	std::string Channel_FirmwareVersion;
	SubDevice_NetworkInterface_Info NetworkInterface;
	//std::string LinkStatus;		// Connected , Disconnected
	//std::string IPv4Address;
	//std::string MACAddress;
} Channel_Infos;


typedef struct firmware_version_Info {
	time_t fw_update_time;
	time_t last_fw_update_time;
	bool fw_update_check;
	bool last_fw_update_check;
	int curl_responseCode;
	std::string FirmwareVersion;
	std::string LatestFirmwareVersion;
	std::string UpgradeStatus;
} Firmware_Version_Infos;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ISUNAPIManagerObserver {
	virtual ~ISUNAPIManagerObserver() {};

	virtual void SendResponseToPeer(const std::string& topic, const std::string& message) = 0;
	virtual void SendResponseForDashboard(const std::string& topic, const std::string& message) = 0;
};

class sunapi_manager
{
public:
	sunapi_manager();
	~sunapi_manager();

	void RegisterObserverForHbirdManager(ISUNAPIManagerObserver* callback);
#ifndef USE_ARGV_JSON
	bool SunapiManagerInit(const std::string strIP, const std::string strPW, const std::string& strDeviceID, int nWebPort);
#else
	void SunapiManagerInit(Setting_Infos* infos);
#endif

	int GetMaxChannel();
	int GetConnectionCount();

	void GetDataForDashboardAPI(const std::string& strTopic, const std::string& strPayload);

	// interface
	// command - checkPassword
	void CommandCheckPassword(const std::string& strTopic, const std::string& strPayload);

	// command - dashboard
	void GetDashboardView(const std::string& strTopic, json_t* json_strRoot);
	void GetDeviceInfoView(const std::string& strTopic, json_t* json_strRoot);
	void GetFirmwareVersionInfoView(const std::string& strTopic, json_t* json_strRoot);

	void UpdateFirmwareOfSubdevice(const std::string& strTopic, json_t* json_strRoot);

	bool SUNAPITunnelingCommand(const std::string& strTopic, json_t* json_root);

protected:

	void debug_check(std::string file_name);

	// reset
	void ResetGatewayInfo();
	void ResetNetworkInterfaceOfGateway();
	void ResetDeviceInfoOfGateway();

	void ResetStorageInfos();
	void ResetStorageInfoForChannel(int channel);

	void ResetSubdeviceInfos();
	void ResetSubdeviceInfoForChannel(int channel);

	void ResetFirmwareVersionInfos();
	void ResetFirmwareVersionInfoForChannel(int channel);

	//update
	void UpdateStorageInfos();
	void UpdateStorageInfosForChannel(int channel);
	void UpdateSubdeviceInfos();
	void UpdateSubdeviceInfosForChannel(int channel);
	void UpdateSubdeviceNetworkInterfaceInfos();
	void UpdateSubdeviceNetworkInterfaceInfosForChannel(int channel);
	void UpdateFirmwareVersionInfos();
	void UpdateFirmwareVersionInfosForChannel(int channel);

	int GetDeviceIP_PW(std::string* strIP, std::string* strPW);

	// Update Gateway info
	bool GetGatewayInfo(const std::string& strGatewayIP, const std::string& strID_PW);
	bool GetNetworkInterfaceOfGateway(const std::string& strGatewayIP, const std::string& strID_PW);
	bool GetDeviceInfoOfGateway(const std::string& strGatewayIP, const std::string& strID_PW);

	int GetMaxChannelByAttribute(std::string strIP, std::string strID_PW);
	bool GetRegiesteredCameraStatus(const std::string deviceIP, const std::string devicePW, CURLcode* resCode);
	void SendErrorResponseForGateway(const std::string& strTopic, json_t* json_strRoot, std::string strError);

	static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp);
#if 0
	CURLcode CURL_Process(bool json_mode, bool ssl_opt, std::string strRequest, std::string strPW, std::string* strResult);
#else  // add timeout option
	CURLcode CURL_Process(bool json_mode, bool ssl_opt, int timeout, std::string strRequest, std::string strPW, std::string* strResult);
	//CURLcode curlProcess(bool json_mode, bool ssl_opt, int timeout, std::string strRequest, std::string strPW, void *chunk_data);
#endif

	bool ByPassSUNAPI(int channel, bool json_mode, const std::string IPAddress, const std::string devicePW, const std::string bypassURI, std::string* strResult, CURLcode* resCode);

	/////////////////////////////////////////////////////////////////////////////////////////////
	// 1. dashboard view

	void GetDASPresenceOfSubdevice(int channel, const std::string deviceIP, const std::string devicePW);
	void GetNASPresenceOfSubdevice(int channel, const std::string deviceIP, const std::string devicePW);

	int ThreadStartForStoragePresence(int channel, const std::string deviceIP, const std::string devicePW);
	void thread_function_for_storage_presence(int channel, const std::string deviceIP, const std::string devicePW);

	void GetStorageStatusOfSubdevices();
	int ThreadStartForStorageStatus(int channel, const std::string deviceIP, const std::string devicePW);
	void thread_function_for_storage_status(int channel, const std::string deviceIP, const std::string devicePW);

	void Set_update_checkForStorageInfo();
	void Reset_update_checkForStorageInfo();
	int ThreadStartSendResponseForDashboardView(const std::string strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid);

	void thread_function_for_send_response_for_dashboard(int maxChannel, const std::string strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid);

	void SendResponseForDashboardView(const std::string& strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid);

	/////////////////////////////////////////////////////////////////////////////////////////////
	// 2. deviceinfo view

	// 21.09.06 add - sub device name
	bool GetDeviceInfoOfSubdevices();
	int ThreadStartForSubDeviceInfo(int index, const std::string deviceIP, const std::string devicePW);
	void thread_function_for_subdevice_info(int index, const std::string deviceIP, const std::string devicePW);

	bool GetMacAddressOfSubdevices();
	int ThreadStartForNetworkInterface(int index, const std::string deviceIP, const std::string devicePW);
	void thread_function_for_network_interface(int index, const std::string deviceIP, const std::string devicePW);

	void Set_update_checkForDeviceInfo();
	void Reset_update_checkForDeviceInfo();
	void Set_update_checkForNetworkInterface();
	void Reset_update_checkForNetworkInterface();
	int ThreadStartSendResponseForDeviceInfoView(const std::string strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid);
	void thread_function_for_send_response_for_deviceInfo(int maxChannel, const std::string strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid);

	void SendResponseForDeviceInfoView(const std::string& strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid);

	/////////////////////////////////////////////////////////////////////////////////////////////
	// 3. firmware info view
	bool GetDataForFirmwareVersionInfo(int skip_time);
	bool UpdateFirmwareVersionInfoFromFile(int skip_time);

	int ThreadStartForCurrentFirmwareVersionOfSubdevices();
	void thread_function_for_current_firmware_version();

	bool GetFirmwareVersionFromText(std::string strText, std::string* strResult);

	bool GetFirmwareVersionOfSubdevices();
	bool GetFirmwareVersionOfSubdevices(const std::string deviceIP, const std::string devicePW, CURLcode* resCode);

	int ThreadStartForFirmwareVersion(int channel, const std::string deviceIP, const std::string devicePW);
	void thread_function_for_firmware_version(int channel, const std::string deviceIP, const std::string devicePW);

	bool GetLatestFirmwareVersionFromFile(std::string file_name, int skip_time);
	bool GetLatestFirmwareVersionFromURL(std::string update_FW_Info_url, std::string fileName);

	void Set_update_check_Firmware_Ver_info_ForFirmwareVersion();
	void Reset_update_check_Firmware_Ver_info_ForFirmwareVersion();

	int ThreadStartSendResponseForFirmwareView(const std::string strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid);
	void thread_function_for_send_response_for_firmwareVersion(int maxChannel, const std::string strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid);

	//////////////////////////////////////////////////////////////////////////////////
	bool UpdateContainerForLatestFirmwareVersionInfo(std::string file_name);
	bool DownloadFileForLatestFirmwareVersionInfo(std::string file_name, int skip_time);
	void UpdateLatestFirmwareVersion();
	int ThreadStartUpdateLatestFirmwareVersion(int channel);
	void thread_function_for_update_latestFirmwareVersion(int channel);

	void SendResponseForFirmwareView(const std::string& strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid);

	/////////////////////////////////////////////////////////////////////////////////////////////
	// 4. firmware update
	void SendResponseForUpdateFirmware(const std::string& strTopic, std::string strCommand, std::string strType, std::string strView, std::string strTid,
										int resCode, int rawCode, std::vector<int> updateChannel);


	/////////////////////////////////////////////////////////////////////////////////////////////
	// for update
	int ThreadStartForPeriodicCheckDeviceInfos(int second, bool repetition);
	void thread_function_for_periodic_check(int second, bool repetition);

	bool RequestCameraDiscovery();

	// 1. periodic update for storage info
	void UpdateDataForStorageInfo();
	// 2. periodic update for device info
	void UpdateDataForDeviceInfo();
	bool PeriodicUpdateDeviceInfoOfSubdevices();

	// 3. periodic update for firmware version info
	void UpdateDataForFirmwareVersionInfo(int skip_time);
	bool update_firmware_info_file(std::string info_file, std::string temp_file);

private:
	int g_Max_Channel;
	int g_debug_check;
	int g_debug_storage;

	std::string g_Device_id;

	std::string g_StrDeviceIP;
	std::string g_StrDevicePW;
	int g_HttpsPort;

	bool g_CheckNeedSleepForRequest;
	bool g_CheckUpdateOfRegistered;
	int g_RetryCountCheckUpdateOfRegistered;
	time_t g_UpdateTimeOfRegistered;
	time_t g_UpdateTimeOfDiscovery;

	time_t g_UpdateTimeOfNetworkInterface;  // for 2. device info - 21.11.04 no longer used

	bool g_CheckUpdateForlatestFirmwareVersion;
	bool g_CheckUpdateForFirmwareVersionOfSubdevices;
	time_t g_UpdateTimeForFirmwareVersionOfSubdevices; // for 3. firmware version
	time_t g_UpdateTimeForlatestFirmwareVersion;

	time_t g_StartTimeOfStorageStatus;
	time_t g_StartTimeOfDeviceInfo;
	time_t g_StartTimeForFirmwareVersionView;

	time_t g_UpdateTimeOfPeriodicCheck;

	ISUNAPIManagerObserver* observerForHbirdManager;

	// firmware version info
	std::vector< firmware_update_Info> g_vecFirmwareInfos;

	// discovered camera info
	std::vector< discovered_camera_Info> g_vecDiscoveredCamerasInfos;
};

