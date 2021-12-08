// GetConfigFile_Project.cpp : 애플리케이션의 진입점을 정의합니다.
//
#pragma once

#include <iostream>
#include <thread>

#ifdef _MSC_VER 
#include <winsock.h>

// Winsock을 사용하는 애플리케이션은 ws2_32.lib와 연결(link)해주어야 합니다.
#pragma comment (lib, "ws2_32.lib")
#endif

#include "main_manager.h"

#include "HBirdCrypto.h"
#include "rafagafe_base64.h"

using std::this_thread::sleep_for;

#ifdef _MSC_VER 
bool Check_Bind()
{
#if 1
	WSADATA wsaData;
	int iniResult = WSAStartup(MAKEWORD(2, 2), &wsaData);  // init lib
	if (iniResult != 0)
	{
		cerr << "Can't Initialize winsock! Quitiing" << endl;
		return false;
	}
#endif

	int optVal = 1;
	char buffer[1024] = { 0 };
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		std::cout << "ERROR opening socket" << std::endl;

	std::cout << "Socket Created" << std::endl;
	//setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&optVal, sizeof(optVal));

	int portNo = 54321;

	struct sockaddr_in address {};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(portNo);

	if( ::bind(sock, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) == -1)
	{
		std::cout << "Binding Unsuccessful" << std::endl;
		closesocket(sock);
		WSACleanup();

		exit(1);
	}
	std::cout << "Binding successful" << std::endl;

	closesocket(sock);
	WSACleanup();

	return true;
}

void WriteLog(std::string strLog)
{
	time_t timer = 0;
	struct tm base_date_local;
	timer = time(NULL); // 1970년 1월 1일 0시 0분 0초부터 시작하여 현재까지의 초
	localtime_s(&base_date_local , &timer); // 포맷팅을 위해 구조체에 넣기

	char log_data[128] = { 0, };
	sprintf_s(log_data, sizeof(log_data), "%d-%d-%d , %d:%d:%d -> ",
		base_date_local.tm_year + 1900, base_date_local.tm_mon + 1, base_date_local.tm_mday, 
		base_date_local.tm_hour, base_date_local.tm_min, base_date_local.tm_sec);

	printf("WriteLog() -> %s\n", log_data);

	std::string strTime_log = log_data;
	strTime_log.append(strLog);

	HANDLE hFile;
	DWORD tempByte;
	char* pszDir = "mainagent_log.txt";

	hFile = CreateFile(pszDir,
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	SetFilePointer(hFile, 0, NULL, FILE_END);
	WriteFile(hFile, strTime_log.c_str(), (sizeof(char) * strTime_log.size()), &tempByte, 0);

	CloseHandle(hFile);
}

void Checkfilelock()
{
#if 0
	std::string checker_file_name = "\\MainAgentChecker.txt";
	std::string file_path;

	char cPath[256];

	if (GetCurrentDirectory(256, cPath) > 0)
	{
		file_path = cPath;

		printf("file path : %s\n", cPath);
	}

	file_path.append(checker_file_name);

	printf("Full path : %s\n", file_path);
#else

	char cPath[256];

	if (GetCurrentDirectory(256, cPath) > 0)
	{
		printf("file path : %s\n", cPath);
	}

	std::string file_path = "config/MainAgentChecker.txt";

#endif

#if 0
	HANDLE indexHandle = CreateFile(file_name.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
#else
	HANDLE indexHandle = CreateFile(file_path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0,
		OPEN_ALWAYS, FILE_FLAG_BACKUP_SEMANTICS, 0);
#endif

	if (indexHandle == INVALID_HANDLE_VALUE) {

		//printf("CreateFile failed (%d)\n", GetLastError());

		char log_data[128] = { 0, };
		sprintf_s(log_data, sizeof(log_data), "CreateFile failed (%d)", GetLastError());

		printf("Checkfilelock() -> %s\n", log_data);
		WriteLog(log_data);
		exit(EXIT_FAILURE);
	}
	
	bool bLock = false;
	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(overlapped));
	
	bLock = LockFileEx(indexHandle, LOCKFILE_EXCLUSIVE_LOCK, 0, 0, UINT_MAX, &overlapped);

	if (bLock != true)
	{		
		//printf("Failed to get lock on file  -- Error code is [ %d ]\n", GetLastError());

		char log_data[128] = { 0, };
		sprintf_s(log_data, sizeof(log_data), "Failed to get lock on file  -- Error code is [ %d ]", GetLastError());

		printf("Checkfilelock() -> %s\n", log_data);
		WriteLog(log_data);

		CloseHandle(indexHandle);

		exit(EXIT_FAILURE);
	}

	CloseHandle(indexHandle);
}
#else

#if 0  // for record locking - fcntl()
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#endif

void Checkfilelock()
{
	std::string file_name = "config/processChecker.txt";

	int fd = open(file_name.c_str(), O_CREAT | O_RDWR);

	if (fd == -1)
	{
		perror("file open error : ");
		exit(0);
	}

	struct flock lock;				
	lock.l_type = F_WRLCK;			// 잠금 종류 : F_RDLCK, F_WRLCK, F_UNLCK
	lock.l_start = 0;		// 잠금 시작 위치
	lock.l_whence = SEEK_SET;		// 기준 위치 : SEEK_SET, SEEK_CUR, SEEK_END
	lock.l_len = 0;					// 잠금 길이 : 바이트 수
	if(fcntl(fd, F_SETLKW, &lock) == -1)
	{
		perror("file is lock ");
		exit(0);
	}
}
#endif

void mainLoop()
{
	std::cout << "[hwanjang] mainAgent thread start ..." << std::endl;

	int loop_count = 0;	

	while (true)
	{
		loop_count++;

		// send message
		if((loop_count % 60) == 0)
			std::cout << loop_count << " , thread loop ..." << std::endl;

		//Sleep(1000);
		//sleep(1);

		sleep_for(std::chrono::milliseconds(1*1000));

	}
}

void thread_start()
{
	std::thread main_th([=] {mainLoop(); });
	//main_th.detach();
	if (main_th.joinable())
	{
		main_th.join();
	}	
}

int main(int argc, char* argv[])
{
#ifdef _MSC_VER 	// hide console
	HWND hWndConsole = GetConsoleWindow();
	//ShowWindow(hWndConsole, SW_HIDE);
#endif
	
#ifdef _MSC_VER 
	// 중복 실행 체크
	//Check_Bind();

	Checkfilelock();
#endif

#if 1  // ver 0.0.1
	printf("\n###############################################################\n");
	printf("\nmainagent 0.0.1 , %s , %s\npre-installed version\n", __DATE__, __TIME__);
	printf("\n###############################################################\n");
#else // ver 0.0.2
	printf("\n###############################################################\n");
	printf("\nmainagent 0.0.2 , %s , %s\ncloud update version\n", __DATE__, __TIME__);
	printf("\n###############################################################\n");
#endif  // end of version


#if 0  // old - function Parameter
	int nWebPort = 443;
	int agent_mode = 0;
	std::string strMode, strDeviceID, strDeviceKey;
	if (argc > 1)
	{
#if 0
		strMode = argv[1];
		agent_mode = std::stoi(strMode);
#else
		agent_mode = std::atoi(argv[1]);
#endif

		if (argc > 3)
		{
			strDeviceID = argv[2];
			strDeviceKey = argv[3];

			if(argv[4])
				nWebPort = std::atoi(argv[4]);
		}
		else
		{
			strDeviceID = "file";
			strDeviceKey = "file";
		}
	}
	else
	{
		agent_mode = 2;  // broker mode

		strDeviceID = "file";
		strDeviceKey = "file";
	}

	std::cout << "Receive Cloud Agent Start ... mode : " << agent_mode << std::endl;

	if ((agent_mode < 0) && (agent_mode > 2))
		agent_mode = 0;  // test -> bridge & broker mode

	std::cout << "Cloud Agent Start ... mode : " << agent_mode << std::endl;

	MainManager* mainManager = new MainManager();
	mainManager->StartMainManager(agent_mode, strDeviceID, strDeviceKey, nWebPort);


#else  // new - function Parameter JSON

#if 0
	LPWSTR* szArglist;
	int nArgs;
	int i;
	
	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if (NULL == szArglist)
	{
		wprintf(L"CommandLineToArgvW failed\n");
		return 0;
	}
	else
	{
		for (i = 0; i < nArgs; i++)
		{
			printf("%d: %ws\n", i, szArglist[i]);			
		}

		FILE* Output_fp = nullptr;
		if (0 != fopen_s(&Output_fp,"receive_data.txt", "w+b")) {
			printf("\n file open fail !!! exit !!!\n");
			return false;
		}

		size_t writeSize = 0;
		unsigned char tempBuffer[2048] = { 0, };
		memcpy(&tempBuffer, szArglist[3], 2048);

		writeSize = fwrite(&tempBuffer, sizeof(char), 2048, Output_fp);
		printf("[hwanjang] writeSize : %d\n", writeSize);

		fclose(Output_fp);
	}
#endif

	// mainagent.exe [mode] [random-string] [original json size] [enc json size] [enc string]

#ifdef HBIRDCRYPTO_DEBUG
	printf("[hwanjang] mainagent argc : %d\n", argc);

	for (int i = 0; i < argc; i++)
	{
		printf("[hwanjang] mainagent argv[%d] size : %d\n%s\n", i, strlen(argv[i]), argv[i]);		
	}
#endif

	int agent_mode = std::atoi(argv[1]);
	char szHalfPassword[32] = { 0, };
	memcpy(&szHalfPassword, argv[2], (strlen(argv[2])));
	int original_JsonSize = std::atoi(argv[3]);
	int enc_BinarySize = std::atoi(argv[4]);

	int enc_StringSize = strlen(argv[5]);
	char* szEncString;
	szEncString = (char*)malloc((sizeof(char) * enc_StringSize));

	memcpy(szEncString, argv[5], enc_StringSize);

#ifdef HBIRDCRYPTO_DEBUG
	printf("[hwanjang] original_JsonSize size : %d , enc_BinarySize : %d , szEncString.size : %d\n", 
						original_JsonSize, enc_BinarySize, strlen(argv[5]));
#endif

	char* base64_output;
	base64_output = (char*)malloc((sizeof(char) * enc_BinarySize));

	char* const enc_data = (char*)b64tobin(base64_output, szEncString);

	if (!enc_data) {
		fputs("Bad base64 format.", stderr);
		return -1;
	}

	pHBirdCrypto pCrypto = HBirdCrypto_Create(__CRYPT_SIZE_256__, eString, szHalfPassword, 32);

	char* pOutTextData = (char*)calloc(sizeof(char), original_JsonSize + 1);
	int output_JsonSize = original_JsonSize + 1;
	int ret = decrypt_Text_C(pCrypto, base64_output, enc_BinarySize, &pOutTextData, &output_JsonSize, NULL, 0);

	if (ret < 0) {
		printf("[%s %d] FAIL!!! decrypt!!! exit !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \r\n", __FUNCTION__, __LINE__);
	}
	else {
		printf("[%s %d] SUCCESS. decrypt!!!  \r\n", __FUNCTION__, __LINE__);
#ifdef HBIRDCRYPTO_DEBUG
		printf("[%s %d] output text: \t size : %d , %s \r\n", __FUNCTION__, __LINE__, output_JsonSize, pOutTextData);
#endif

	}

	//("[hwanjang] output_JsonSize : %d , output_JsonSize : %d\n", original_JsonSize, output_JsonSize);

	std::string strJSONData(pOutTextData, original_JsonSize);

#ifdef HBIRDCRYPTO_DEBUG
	printf("received JSON data : \n%s\n", pOutTextData);
#endif

	std::cout << "[hwanjang] Cloud mainAgent Start ... mode : " << agent_mode << std::endl;

	MainManager* mainManager = new MainManager();
	mainManager->StartMainManager(agent_mode, strJSONData);
#endif

	thread_start();

	delete mainManager;

	return 0;
}
