@ECHO OFF

cmake.exe -Hproject_file -B build/build_main_app -G "Visual Studio 16 2019" -A "Win32" -DCMAKE_CONFIGURATION_TYPES="Debug;Release"

cd build/build_main_app

msbuild iothub_tunneling_agent_Project.sln /p:Configuration=Debug /p:Platform="Win32" -target:iothub_tunnelingagent:Rebuild

msbuild iothub_tunneling_agent_Project.sln /p:Configuration=Release /p:Platform="Win32" -target:iothub_tunnelingagent:Rebuild