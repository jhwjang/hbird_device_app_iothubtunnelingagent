#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <fstream>
#include <chrono>
#include <vector>

#include "api_manager.h"
#include "NetworkAdapter_manager.h"
#include "curl/curl.h"
#include "win_time.h"

using namespace std;
using std::this_thread::sleep_for;

//#define TEST_DEVICE_ADDRESS "192.168.123.201"  // for Web TEAM
#define TEST_DEVICE_ADDRESS "192.168.11.5"  // for my device

// Default timeout is 0 (zero) which means it never times out during transfer.
#define CURL_OPT_TIMEOUT 0  // 2018.11.30 : 3 -> 0

#define REMOTE_INFO_FILE "config/remote_info.txt"

#if 0
// QA
#define DEFAULT_CONFIG "{\"url\":\"https://config.qa.htw2.hbird-iot.com:443/v1.5/\",\"config_path\":\"config/model/device\",\"bearer\":\"authorization: bearer \"}"
#define DEVICE_TOKEN_API "{\"url\":\"https://auth.qa.htw2.hbird-iot.com/v1.5/\",\"key_path\":\"device/key/type/cloudGateway\",\"bearer\":\"authorization: bearer \"}"
#else
// DEV
#define DEFAULT_CONFIG "{\"url\":\"https://config.dev.htw2.hbird-iot.com:443/v1.5/\",\"config_path\":\"config/model/device\",\"bearer\":\"authorization: bearer \"}"
#define DEVICE_TOKEN_API "{\"url\":\"https://auth.dev.htw2.hbird-iot.com/v1.5/\",\"key_path\":\"device/key/type/cloudGateway\",\"bearer\":\"authorization: bearer \"}"
#endif

// for debug
#ifdef _DEBUG
#define SUNAPI_DEBUG
#endif
#define API_DEBUG

// for SUNAPI Command
struct MemoryStruct {
	char* memory;
	size_t size;
};

APIManager* APIHandler;

APIManager::APIManager(){

	printf("[hwanjang] APIManager -> constructor !!!\n");
	APIHandler = this;

	observerForHbirdManager = nullptr;


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

	printf("APIManager() -> gStrDeviceIP : %s , gStrDevicePW : %s\n", gStrDeviceIP.c_str(), gStrDevicePW.c_str());
}

APIManager::~APIManager() {
	printf("[hwanjang] APIManager::~APIManager() -> Destructor !!!\n");

}

void APIManager::RegisterObserverForHbirdManager(IAPIManagerObserver* callback)
{
	observerForHbirdManager = callback;
}

#if 1  // test
int APIManager::GetDeviceIP_PW(std::string* strIP, std::string* strPW)
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
#endif

// change byte -> unsigned char
bool APIManager::HttpTunnelingCommand(const std::string& strTopic, json_t* json_root)
{
	// root -> command, type, tid

	char* strCommand, * strType, * strTid;
	int result = json_unpack(json_root, "{s:s, s:s, s:s}", "command", &strCommand, "type", &strType, "tid", &strTid);

	if (result)
	{
		printf("[hwanjang] Error !! HttpTunnelingCommand -> json root is wrong -> return  !!!\n");
		return false;
	}

	std::cout << "HttpTunnelingCommand() -> type : " << strType << std::endl;

	//////////////////////////////////////////////////////////////////////
	// root -> json_t* message  
	json_t* objMessage = json_object_get(json_root, "message");

	if (!json_is_object(objMessage))
	{
		printf("[hwanjang] Error !! fail to get JSON value from sub message !!!!\n");
		return false;
	}

	json_t* jmsg_headers = json_object_get(objMessage, "headers");

	char* strMethod, * strUrl, * strBody;

#if 1 // option #1 - check null
	json_t* jmsg_body = json_object_get(objMessage, "body");
	if (json_is_null(jmsg_body))
	{
		result = json_unpack(json_root, "{s{s:s, s:s}}", "message", "method", &strMethod, "url", &strUrl);
	}
	else
	{
		result = json_unpack(json_root, "{s{s:s, s:s, s:s}}", "message", "method", &strMethod, "url", &strUrl, "body", &strBody);

		printf("[hwanjang] Tunneling param , body : %s \n", strBody);
	}

	if (result)
	{
		printf("[hwanjang] Error !! get message data !!!!\n");
		return false;
	}

#else  // option #2
	result = json_unpack(root, "{s{s:s, s:s}}", "message", "method", &strMethod, "url", &strUrl);

	if (result)
	{
		printf("[hwanjang] Error !! get message datas , method or url !!!!\n");
		return;
	}

	if (!strncmp("POST", strMethod, 4))
	{
		result = json_unpack(root, "{s{s:s}}", "message", "body", &strBody);

		if (result)
		{
			printf("[hwanjang] Error !! get body data !!!!\n");
			return;
		}

		printf("[hwanjang] Tunneling param , body : %s\n", strMethod, strUrl);
	}
#endif

	printf("[hwanjang] Tunneling param , method : %s , url : %s\n", strMethod, strUrl);

	// tunneling

	CURL* curl_handle;
	struct MemoryStruct bodyChunk, headerChunk;

	bodyChunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
	bodyChunk.size = 0;    /* no data at this point */

	headerChunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
	headerChunk.size = 0;    /* no data at this point */

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();

	if (curl_handle == NULL)
	{
		printf("error ... initialize cURL !!!\n");
		curl_easy_cleanup(curl_handle);
		/* we're done with libcurl, so clean it up */
		curl_global_cleanup();
		return false;
	}

	struct curl_slist* headerList = NULL;


	// --------------------- Requset PARSERED Headers Start -------------------------------
#if 0
	std::string strHost, strProtocol, strPort;
	std::string  headerItem;

	for (Json::Value::const_iterator itr = jmsg_headers.begin(); itr != jmsg_headers.end(); itr++) {
		headerItem.clear();
		// if(method.asString() == "POST")
		std::cout << "    " << itr.name().c_str() << " : " << jmsg_headers[itr.name()].asString() << std::endl;

		if (itr.name() == "host") {
			strHost = jmsg_headers[itr.name()].asString();
		}
		else {
			if (itr.name() == "x-forwarded-proto")
				strProtocol = jmsg_headers[itr.name()].asString();
			if (itr.name() == "x-forwarded-port")
				strPort = jmsg_headers[itr.name()].asString();

			if (itr.name() != "accept-encoding") {
				headerItem = itr.name();
				headerItem.append(" : ");
				headerItem.append(jmsg_headers[itr.name()].asString());
				headerList = curl_slist_append(headerList, headerItem.c_str());

				printf("header item : %s\n", headerItem.c_str());
			}
		}
	}
#else
	///////////////////////////////////////////////////////////////////////////
	// JANSSON	
	std::string strHost, strProtocol, strPort;
	std::string  headerItem;

	json_t* value;
	const char* key;

	json_object_foreach(jmsg_headers, key, value) {
		printf("key: %s", key);
		if (!strncmp("host", key, 4))
		{
			strHost = json_string_value(value);
		}
		else if (!strncmp("x-forwarded-proto", key, 17))
		{
			strProtocol = json_string_value(value);
		}
		else if (!strncmp("x-forwarded-port", key, 16))
		{
			strPort = json_string_value(value);
		}
		else if (strncmp("accept-encoding", key, 15))
		{
			headerItem = key;
			headerItem.append(" : ");
			headerItem.append(json_string_value(value));
			headerList = curl_slist_append(headerList, headerItem.c_str());

			printf("header item : %s\n", headerItem.c_str());
		}
		else
		{
			printf("Unnecessary data ... key : %s , value : %s\n", key, json_string_value(value));
		}
	}
#endif
	// --------------------- Requset PARSERED Headers End -------------------------------
	std::string strRepuestPort = "443";

	if (strProtocol != "https")
	{
		strRepuestPort.clear();
		strRepuestPort = "80";
	}

	// tunneling
	std::string strRequestUrl, strRepuestHost, strUserPW;

#if 0  //  for web team test

	std::string strIP, strPW;
	std::string sunapi_info_file = "config/camera.cfg";

	json_error_t j_error;
	json_t* json_out = json_load_file(sunapi_info_file.c_str(), 0, &j_error);
	if (j_error.line != -1)
	{
		printf("[hwanjang] Error !! get camera info !!!\n");
		printf("json_load_file returned an invalid line number -- > CFG file check !!\n");

		strIP = "127.0.0.1";
		strPW = "admin:5tkatjd!";
	}
	else
	{
		char* SUNAPIServerIP, * devicePW;
		result = json_unpack(json_out, "{s:s, s:s}", "ip", &SUNAPIServerIP, "pw", &devicePW);

		if (result || !SUNAPIServerIP || !devicePW)
		{
			printf("[hwanjang] Error !! camera cfg is wrong -> retur false !!!\n");

			strIP = "127.0.0.1";
			strPW = "admin:5tkatjd!";
		}
		else
		{
			strIP = SUNAPIServerIP;
			strPW = devicePW;
		}
	}
	json_decref(json_out);
#endif

	strRepuestHost = gStrDeviceIP;

	strRequestUrl = strProtocol;
	strRequestUrl.append("://");
	strRequestUrl.append(strRepuestHost);
	strRequestUrl.append(":");
	strRequestUrl.append(strRepuestPort);
	strRequestUrl.append(strUrl);  // strUrl is equal to jmsg_path.asString()

	strUserPW = gStrDevicePW;

	printf("[hwanjang] Request URL : %s\n", strRequestUrl.c_str());

	/*
	  200508 권혁진
	  resolve_str
	  ex) server0.htwda03u1803001526.dev.htw.hbird-iot.com:52061:211.221.81.152
	*/
	std::string strResolve = "server0.";
	strResolve.append(strHost);
	strResolve.append(":");
	strResolve.append(strRepuestPort);
	strResolve.append(":");
	strResolve.append(strRepuestHost);

	std::cout << "resolve_str : " << strResolve << std::endl;

	/*
	  200508 권혁진
	  connect_to_str
	  ex) htwda03u1803001526.dev.htw.hbird-iot.com:80:server0.htwda03u1803001526.dev.htw.hbird-iot.com:52061
	*/
	char connect_to_str[1024] = { 0, };
	std::string strConnection = strHost;
	strConnection.append(":");
	strConnection.append(strPort);
	strConnection.append(":");
	strConnection.append("server0.");
	strConnection.append(strHost);
	strConnection.append(":");
	strConnection.append(strRepuestPort);

	std::cout << "connect_to_str : " << strConnection << std::endl;

	struct curl_slist* resolve_slist = NULL;
	resolve_slist = curl_slist_append(NULL, strResolve.c_str());
	struct curl_slist* connect_to_slist = NULL;
	connect_to_slist = curl_slist_append(NULL, strConnection.c_str());

	curl_easy_setopt(curl_handle, CURLOPT_RESOLVE, resolve_slist);
	curl_easy_setopt(curl_handle, CURLOPT_CONNECT_TO, connect_to_slist);
	curl_easy_setopt(curl_handle, CURLOPT_ACCEPT_ENCODING, NULL);

	//headerList = curl_slist_append(headerList, "Accept: application/json");
	//curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headerList);
	//curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "GET");
	curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, strMethod);
	if (!strncmp("POST", strMethod, 4))
	{
		std::string bodyMsg = strBody;
		headerList = curl_slist_append(headerList, "Expect:");
		headerList = curl_slist_append(headerList, "content-length:");
		headerList = curl_slist_append(headerList, "content-type:");

		if (bodyMsg.length() > 0)
		{
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, bodyMsg.c_str());
			curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, bodyMsg.size());
		}
	}

	//headerList = curl_slist_append(headerList, "Accept: application/json");

	// header List
	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headerList);

	curl_easy_setopt(curl_handle, CURLOPT_URL, strRequestUrl.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST); // digest authentication
	curl_easy_setopt(curl_handle, CURLOPT_USERPWD, strUserPW.c_str());

	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, true);

	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);

	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&bodyChunk);

	curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteMemoryCallbackForHeader);
	curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void*)&headerChunk);

	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, CURL_OPT_TIMEOUT);

	CURLcode res = curl_easy_perform(curl_handle);

	/* check for errors */
	if (res != CURLE_OK) {
		printf("APIManager::HttpTunnelingCommand() -> curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	}
	else
	{
		std::string res_str;

		json_t* mqtt_MainMsg, * objSub;

		mqtt_MainMsg = json_object();
		objSub = json_object();
		/*
		 * Now, our chunk.memory points to a memory block that is chunk.size
		 * bytes big and contains the remote file.
		 *
		 * Do something nice with it!
		 */
		curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &res);
		printf("response code : %d\n", res);

		//sub_Msg["statusCode"] = res;
		json_t* intRes = json_integer(res);
		json_object_set(objSub, "statusCode", intRes);

#if 1	// debug
		printf("headerChunk size : %lu bytes \n", (long)headerChunk.size);
		printf("headerChunk data : %s\n", headerChunk.memory);

		printf("bodyChunk size : %lu bytes \n", (long)bodyChunk.size);
		printf("bodyChunk data : %s\n", bodyChunk.memory);
#endif

		// Header
		std::string responseHeader = headerChunk.memory;

		//mqtt_Msg["command"] = strCommand;
		json_object_set(mqtt_MainMsg, "command", json_string(strCommand));
		//mqtt_Msg["type"] = "response";
		json_object_set(mqtt_MainMsg, "type", json_string("response"));
		//mqtt_Msg["message"] = sub_Msg;
		json_object_set(mqtt_MainMsg, "message", objSub);
		//mqtt_Msg["tid"] = strTid;
		json_object_set(mqtt_MainMsg, "tid", json_string(strTid));

		int cnt = 0;
		std::string hsonProtocol = "HSON0001";
		std::vector<unsigned char> msgProtocolByte(hsonProtocol.length(), 0);

		std::copy(hsonProtocol.begin(), hsonProtocol.end(), msgProtocolByte.begin());

		// MQTT Message size
		std::vector<unsigned char> mqttMsgBorderlineByte;

		//std::string strMQTTMsg = writer.write(mqtt_Msg);		
		std::string strMQTTMsg = json_dumps(mqtt_MainMsg, 0);

		printf("strMQTTMsg size : %lu\n%s\n", strMQTTMsg.size(), strMQTTMsg.c_str());

		uint32_t mqttMsgBorderline = strMQTTMsg.size() + hsonProtocol.size() + 4; // 4 is mqttMsgBorderlineByte size

		printf("mqttMsgBorderline size : %d\n", mqttMsgBorderline);

		mqttMsgBorderlineByte.push_back(static_cast<unsigned char>((mqttMsgBorderline & 0xFF000000) >> 24));
		mqttMsgBorderlineByte.push_back(static_cast<unsigned char>((mqttMsgBorderline & 0x00FF0000) >> 16));
		mqttMsgBorderlineByte.push_back(static_cast<unsigned char>((mqttMsgBorderline & 0x0000FF00) >> 8));
		mqttMsgBorderlineByte.push_back(static_cast<unsigned char>(mqttMsgBorderline & 0x000000FF));

		msgProtocolByte.insert(msgProtocolByte.end(), mqttMsgBorderlineByte.begin(), mqttMsgBorderlineByte.end());

		// MQTT Message
		std::vector<unsigned char> mqttMsgByte(strMQTTMsg.length(), 0);
		std::copy(strMQTTMsg.begin(), strMQTTMsg.end(), mqttMsgByte.begin());

		msgProtocolByte.insert(msgProtocolByte.end(), mqttMsgByte.begin(), mqttMsgByte.end());

		// response header size
		std::vector<unsigned char> headerBorderlineByte;
		uint32_t headerBorderline = mqttMsgBorderline + responseHeader.size() + 4; // 4 is headerBorderlineByte size

		printf("headerBorderline size : %d\n", headerBorderline);

		headerBorderlineByte.push_back(static_cast<unsigned char>((headerBorderline & 0xFF000000) >> 24));
		headerBorderlineByte.push_back(static_cast<unsigned char>((headerBorderline & 0x00FF0000) >> 16));
		headerBorderlineByte.push_back(static_cast<unsigned char>((headerBorderline & 0x0000FF00) >> 8));
		headerBorderlineByte.push_back(static_cast<unsigned char>(headerBorderline & 0x000000FF));

		msgProtocolByte.insert(msgProtocolByte.end(), headerBorderlineByte.begin(), headerBorderlineByte.end());

		// response header
		std::vector<unsigned char> responseHeaderByte(responseHeader.length(), 0);
		std::copy(responseHeader.begin(), responseHeader.end(), responseHeaderByte.begin());

		printf("responseHeader.size : %lu\n%s\n", responseHeader.length(), responseHeader.c_str());

		msgProtocolByte.insert(msgProtocolByte.end(), responseHeaderByte.begin(), responseHeaderByte.end());

		// response body
		//std::vector<byte> responseBodyByte(responseBody.length(), 0);
		//std::copy(responseBody.begin(), responseBody.end(), responseBodyByte.begin());

		//printf("responseBody.size : %d\n%s\n", responseBody.length(), responseBody.c_str());
		//msgProtocolByte.insert(msgProtocolByte.end(), responseBodyByte.begin(), responseBodyByte.end());

		// body
		unsigned char* byteResponseBody = (unsigned char*)bodyChunk.memory;
		// response body
		std::vector<unsigned char> responseBodyByte(byteResponseBody, byteResponseBody + bodyChunk.size);

		msgProtocolByte.insert(msgProtocolByte.end(), responseBodyByte.begin(), responseBodyByte.end());


		std::string sendMsg(reinterpret_cast<const char*>(&msgProtocolByte[0]), msgProtocolByte.size());

		//printf("[hwanjang HttpTunnelingCommand() response ---> send message size: %d\n%s\n", sendMsg.size() , sendMsg.c_str());

		printf("[hwanjang HttpTunnelingCommand() response ---> send message size: %lu\n", msgProtocolByte.size());

		observerForHbirdManager->SendResponseToPeerForTunneling(strTopic, &msgProtocolByte[0], msgProtocolByte.size());
	}

	curl_slist_free_all(headerList);
	curl_slist_free_all(resolve_slist);
	curl_slist_free_all(connect_to_slist);

	curl_easy_cleanup(curl_handle);

	free(bodyChunk.memory);
	free(headerChunk.memory);

	/* we're done with libcurl, so clean it up */
	curl_global_cleanup();

	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// SUNAPI Tunneling

bool APIManager::SUNAPITunnelingCommand(const std::string& strTopic, json_t* json_root)
{

	// root -> command, type, tid

	char* strCommand, * strType, * strTid;
	int result = json_unpack(json_root, "{s:s, s:s, s:s}", "command", &strCommand, "type", &strType, "tid", &strTid);

	if (result)
	{
		printf("[hwanjang] Error !! HttpTunnelingCommand -> json root is wrong -> return  !!!\n");
		return false;
	}

	std::cout << "HttpTunnelingCommand() -> type : " << strType << std::endl;

	//////////////////////////////////////////////////////////////////////
	// root -> json_t* message  
	json_t* objMessage = json_object_get(json_root, "message");

	if (!json_is_object(objMessage))
	{
		printf("[hwanjang] Error !! fail to get JSON value from sub message !!!!\n");
		return false;
	}

	json_t* jmsg_headers = json_object_get(objMessage, "headers");

	char* strMethod, * strUrl, * strBody;

#if 1 // option #1 - check null
	json_t* jmsg_body = json_object_get(objMessage, "body");
	if (json_is_null(jmsg_body))
	{
		result = json_unpack(json_root, "{s{s:s, s:s}}", "message", "method", &strMethod, "url", &strUrl);
	}
	else
	{
		result = json_unpack(json_root, "{s{s:s, s:s, s:s}}", "message", "method", &strMethod, "url", &strUrl, "body", &strBody);

		printf("[hwanjang] Tunneling param , body : %s \n", strBody);
	}

	if (result)
	{
		printf("[hwanjang] Error !! get message data !!!!\n");
		return false;
	}

#else  // option #2
	result = json_unpack(root, "{s{s:s, s:s}}", "message", "method", &strMethod, "url", &strUrl);

	if (result)
	{
		printf("[hwanjang] Error !! get message datas , method or url !!!!\n");
		return;
	}

	if (!strncmp("POST", strMethod, 4))
	{
		result = json_unpack(root, "{s{s:s}}", "message", "body", &strBody);

		if (result)
		{
			printf("[hwanjang] Error !! get body data !!!!\n");
			return;
		}

		printf("[hwanjang] Tunneling param , body : %s\n", strMethod, strUrl);
	}
#endif

	printf("[hwanjang] Tunneling param , method : %s , url : %s\n", strMethod, strUrl);

	std::string strRepuest;

	strRepuest = "http://";
	strRepuest.append(gStrDeviceIP);
	strRepuest.append(strUrl);  // strUrl is equal to jmsg_path.asString()
	//strRepuest.append("/stw-cgi/system.cgi?msubmenu=deviceinfo&action=view");

	printf("[hwanjang] Request : %s\n", strRepuest.c_str());

	CURL* curl_handle;
	struct MemoryStruct chunk;

	chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();

	if (curl_handle == NULL)
	{
		printf("error ... initialize cURL !!!\n");
	}

	std::string userpw = gStrDevicePW;

	struct curl_slist* headers = NULL;
	//   headers = curl_slist_append(headers, "cache-control: no-cache");
	headers = curl_slist_append(headers, "Accept: application/json");

	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
	//curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "GET");
	curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, strMethod);
	curl_easy_setopt(curl_handle, CURLOPT_URL, strRepuest.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST); // digest authentication
	curl_easy_setopt(curl_handle, CURLOPT_USERPWD, userpw.c_str());

	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&chunk);
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, CURL_OPT_TIMEOUT);

	CURLcode res = curl_easy_perform(curl_handle);

	std::string res_str;

	json_t* main_ResponseMsg, * sub_Msg;

	main_ResponseMsg = json_object();
	sub_Msg = json_object();

	/* check for errors */
	if (res != CURLE_OK) {
		printf("APIManager::SUNAPITunnelingCommand() -> curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		//sub_Msg["code"] = res;
		//sub_Msg["body"] = curl_easy_strerror(res);

		// sub of message
		json_object_set(sub_Msg, "code", json_integer(res));
		json_object_set(sub_Msg, "body", json_string(curl_easy_strerror(res)));
	}
	else {
		/*
		 * Now, our chunk.memory points to a memory block that is chunk.size
		 * bytes big and contains the remote file.
		 *
		 * Do something nice with it!
		 */
		curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &res);
		printf("response code : %d\n", res);

		if ((long)chunk.size == 0) // error
		{
			json_object_set(sub_Msg, "code", json_integer(res));
			json_object_set(sub_Msg, "body", json_string("fail"));
		}
		else  // OK
		{
			//#ifdef SUNAPI_DEBUG // 2019.09.05 hwanjang - debugging
#if 1
			printf("%lu bytes retrieved\n", (long)chunk.size);
			printf("data : %s\n", chunk.memory);
#endif

			std::string strChunkData = chunk.memory;

			json_error_t error_check;
			json_t* json_strRoot = json_loads(chunk.memory, 0, &error_check);

			if (!json_strRoot)
			{
				fprintf(stderr, "error : root\n");
				fprintf(stderr, "error : on line %d: %s\n", error_check.line, error_check.text);

				json_object_set(sub_Msg, "code", json_integer(600));
				json_object_set(sub_Msg, "body", json_string("Something wrong !!!"));

			}
			else
			{
				if (strChunkData.find("Detatils") != std::string::npos)
				{
					char* charResponseData;
					int result = json_unpack(json_strRoot, "{s:s}", "Response", &charResponseData);

					if (result)
					{
						printf("[hwanjang] Error !! SUNAPITunnelingCommand() -> 1. json_unpack fail .. Response !!!\n");

						if (strChunkData.find("Error") != std::string::npos)
						{

							json_t* json_subError = json_object_get(objMessage, "Error");

							// get error code
							int errorCode;
							result = json_unpack(json_subError, "{s:i}", "Code", &errorCode);

							if (result)
							{
								json_object_set(sub_Msg, "code", json_integer(errorCode));
							}
							else
							{
								json_object_set(sub_Msg, "code", json_integer(600));
							}

							// get detail
							char* charErrorDetails;
							result = json_unpack(json_subError, "{s:s}", "Details", charErrorDetails);

							if (result)
							{
								json_object_set(sub_Msg, "body", json_string(charErrorDetails));
							}
							else
							{
								json_object_set(sub_Msg, "body", json_string("Something wrong !!!"));
							}
						}
						else
						{
							printf("--> Error not found !!!\n");

							json_object_set(sub_Msg, "code", json_integer(600));
							json_object_set(sub_Msg, "body", json_string("Something wrong !!!"));
						}
					}
					else
					{
						printf("--> Response not found !!!\n");

						json_object_set(sub_Msg, "code", json_integer(600));
						json_object_set(sub_Msg, "body", json_string("Error !!!"));
					}
				}
				else
				{
					json_object_set(sub_Msg, "code", json_integer(200));
					json_object_set(sub_Msg, "body", json_strRoot);
					json_object_set(sub_Msg, "header", json_string(""));
					
				}
			}
		}
	}

#if 0
	send_Msg["command"] = cmd_str;	
	send_Msg["type"] = "response";
	send_Msg["message"] = sub_Msg;
	send_Msg["tid"] = tid_str;
#else
	json_object_set(main_ResponseMsg, "command", json_string(strCommand));
	json_object_set(main_ResponseMsg, "type", json_string("response"));
	json_object_set(main_ResponseMsg, "message", sub_Msg);
	json_object_set(main_ResponseMsg, "tid", json_string(strTid));
#endif

	std::string strMQTTMsg = json_dumps(main_ResponseMsg, 0);

	//printf("APIManager::SUNAPITunnelingCommand()() -> strMQTTMsg size : %lu\n%s\n", strMQTTMsg.size(), strMQTTMsg.c_str());
	printf("[hwanjang APIManager::SUNAPITunnelingCommand()() response ---> size : %lu, send message : \n%s\n", strMQTTMsg.size(), strMQTTMsg.c_str());

	observerForHbirdManager->SendResponseToPeer(strTopic, strMQTTMsg);

	curl_easy_cleanup(curl_handle);

	free(chunk.memory);

	/* we're done with libcurl, so clean it up */
	curl_global_cleanup();

	return true;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
// for APIManagerObserverForAPIManager

size_t APIManager::WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
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

//printf("[hwanjang] WriteMemoryCallback() -> realsize : %d\n-> %s\n", realsize, mem->memory);

   return realsize;
}

///////////////////////////////////////////////////////////////////////////////
//SUNAPI
///////////////////////////////////////////////////////////////////////////////

int APIManager::file_exit(std::string& filename)
{
	FILE* file;
	if ((file = fopen(filename.c_str(), "r")))
	{
		fclose(file);
		return 1;
	}
	return 0;
}

int APIManager::CalculateRetryTime(int count)
{
	int retryTime = 0;
	if(count < 4)
		retryTime = (rand() % 10) + 1;  // 1 ~ 10 sec
	else
		retryTime = (rand() % 30) + 30;  // 30 ~ 60 sec

	return retryTime;
}

size_t APIManager::WriteMemoryCallbackForHeader(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct* mem = (struct MemoryStruct*)userp;

	mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	//printf("[hwanjang] WriteMemoryCallback() -> realsize : %d\n-> %s\n", realsize, mem->memory);

	return realsize;
}

// thread
/////////////////////////////////////////////////////////////////////////////////////////////
/// Thread for Message
void APIManager::thread_function_for_updating_config(int interval)
{
	// ...
	std::cout << "Hello waiter\n" << std::flush;
	auto start = std::chrono::high_resolution_clock::now();
	//sleep(interval);
	sleep_for(std::chrono::milliseconds(interval*1000));
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed = end - start;
	std::cout << "Waited " << elapsed.count() << " ms\n";
}

int APIManager::ThreadStartForUpdatingConfigData(int interval)
{
	std::thread thread_function_th([=] { thread_function_for_updating_config(interval); });
	thread_function_th.detach();

	return 0;
}
