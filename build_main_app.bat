@ECHO OFF

cmake.exe -Hproject_file -B build/build_main_app -G "Visual Studio 16 2019" -A "Win32" -DCMAKE_CONFIGURATION_TYPES="Debug;Release"

cd build/build_build_app

msbuild iothub_cloud_agent_for_dashboard_project.sln /p:Configuration=Debug /p:Platform="Win32" -target:iothub_tunnelingagent:Rebuild

msbuild iothub_cloud_agent_for_dashboard_project.sln /p:Configuration=Release /p:Platform="Win32" -target:iothub_tunnelingagent:Rebuild