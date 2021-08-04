cmake CMakeLists.txt -G "Visual Studio 16 2019" -B build_vs2019 -A "Win32"

cmake CMakeLists.txt -G "Visual Studio 16 2019" -B ../build/build_vs2019 -A "Win32" -DCMAKE_CONFIGURATION_TYPES="Debug;Release"


 # 2021.07.02
1. git clone 후 hbird_device_app_iothubtunnelingagent 폴더 내에 build 폴더를 생성.
2. cmd 창에서 
cmake.exe project_file/CMakeLists.txt -G "Visual Studio 16 2019" -B build/build_vs2019 -A "Win32" -DCMAKE_CONFIGURATION_TYPES="Debug;Release"
or
cmake.exe -Hproject_file -G "Visual Studio 16 2019" -B build/build_vs2019 ...

3. iothub_tunneling_agent_Project.sln 생성 & 빌드

 # 21.07.20 - linux
cmake -Bbuild_linux -Hproject_file
or
cmake -Bbuild_linux -Hproject_file -DBUILD_LINUX=TRUE

 # 21.08.02 - arm
cmake -Bbuild_arm -Hproject_file -DBUILD_ARM=TRUE -DCMAKE_C_COMPILER=/home/hwanjang/work/camera_p_cv2_toolchain/toolchain/linaro-2018.08/armv8/aarch64/linaro-aarch64-2018.08-gcc8.2/bin/aarch64-linux-gnu-gcc -DCMAKE_CXX_COMPILER=/home/hwanjang/work/camera_p_cv2_toolchain/toolchain/linaro-2018.08/armv8/aarch64/linaro-aarch64-2018.08-gcc8.2/bin/aarch64-linux-gnu-g++

or

cmake -Bbuild_arm -Hproject_file -DBUILD_ARM=TRUE -DCMAKE_C_COMPILER=${HOME}/work/camera_p_cv2_toolchain/toolchain/linaro-2018.08/armv8/aarch64/linaro-aarch64-2018.08-gcc8.2/bin/aarch64-linux-gnu-gcc -DCMAKE_CXX_COMPILER=${HOME}/work/camera_p_cv2_toolchain/toolchain/linaro-2018.08/armv8/aarch64/linaro-aarch64-2018.08-gcc8.2/bin/aarch64-linux-gnu-g++

