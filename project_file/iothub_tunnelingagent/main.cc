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
bool bind_check()
{
#if 1
	WSADATA wsaData;
	int iniResult = WSAStartup(MAKEWORD(2, 2), &wsaData);  // init lib
	if (iniResult != 0)
	{
		cerr << "Can't Initialize winsock! Quitiing" << endl;
		return -1;
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
	bind_check();
#endif

	int agent_mode = 0;
	std::string strMode;
	if (argc > 1)
	{
		strMode = argv[1];
		agent_mode = std::stoi(strMode);
	}
	else
		agent_mode = 2;  // broker mode

	if ((agent_mode < 0) && (agent_mode > 2))
		agent_mode = 2;  // broker mode

	std::cout << "Cloud Agent Start ... mode : " << agent_mode << std::endl;

	MainManager* mainManager = new MainManager();


	// for test --> agent_mode : bridge & broker
	agent_mode = 1;
	mainManager->StartMainManager(agent_mode);

	thread_start();

	delete mainManager;

	return 0;
}
