// GetConfigFile_Project.cpp : 애플리케이션의 진입점을 정의합니다.
//
#pragma once

#include <iostream>

#include "main_manager.h"

#include <thread>

#ifdef _MSC_VER 
#include <windows.h>

#define localtime_r(_time, _result) _localtime64_s(_result, _time)
#define sleep(x) Sleep((x*1000))
#endif

using std::this_thread::sleep_for;

void mainLoop()
{
	std::cout << "main thread start ..." << std::endl;
	int loop_count = 0;

	while (true)
	{
		loop_count++;

		// send message
		if((loop_count % 10) == 0)
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

	mainManager->StartMainManager(agent_mode);

	thread_start();

	delete mainManager;

	return 0;
}
