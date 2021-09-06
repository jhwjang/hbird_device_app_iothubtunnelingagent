// GetConfigFile_Project.cpp : 애플리케이션의 진입점을 정의합니다.
//
#pragma once

#include <iostream>

#ifdef _MSC_VER 
#include <winsock.h>

// Winsock을 사용하는 애플리케이션은 ws2_32.lib와 연결(link)해주어야 합니다.
#pragma comment (lib, "ws2_32.lib")
#endif

#include "main_manager.h"

#include <thread>


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
		exit(1);
	}
	std::cout << "Binding successful" << std::endl;

	return true;
}

void Checkfilelock()
{
	std::string file_name = "config/processChecker.txt";

	HANDLE indexHandle = CreateFile(file_name.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if (indexHandle == INVALID_HANDLE_VALUE) {
		printf("CreateFile failed (%d)\n", GetLastError());
		exit(EXIT_FAILURE);
	}
	
	bool bLock = false;
	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(overlapped));
	
	bLock = LockFileEx(indexHandle, LOCKFILE_EXCLUSIVE_LOCK, 0, 0, UINT_MAX, &overlapped);

	if (bLock != true)
	{
		std::cout << "Failed to get lock on file  -- Error code is [" << GetLastError() << "]" << std::endl;
		exit(EXIT_FAILURE);
	}
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
	std::cout << "main thread start ..." << std::endl;
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

	int agent_mode = 0;
	std::string strMode, strDeviceID, strDeviceKey;
	if (argc > 1)
	{
		strMode = argv[1];
		agent_mode = std::stoi(strMode);

		if (argc > 3)
		{
			strDeviceID = argv[2];
			strDeviceKey = argv[3];
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

	if ((agent_mode < 0) && (agent_mode > 2))
		agent_mode = 2;  // broker mode

	std::cout << "Cloud Agent Start ... mode : " << agent_mode << std::endl;

	MainManager* mainManager = new MainManager();


	// for test --> agent_mode : bridge & broker
	agent_mode = 1;
	mainManager->StartMainManager(agent_mode, strDeviceID, strDeviceKey);

	thread_start();

	delete mainManager;

	return 0;
}
