# 简介
本项目旨在开发基于轮履复合底盘以及六自由度舵机结构的小型救援机器人，上位机采用树莓派4B运行ROS2，下位机使用STM32F407控制底盘和舵机。

# 环境介绍
- **主机系统**: ubuntu 22.04
- **ROS2版本**: ros2-humble
- **stm32型号**: stm32F407ZGT6
- **cubemx版本**: >=6.15.0

# 目录结构(开发中)
- `lowwer_computer` :下位机代码，包括底盘控制和硬件驱动代码
- `upper_computer` :上位机代码，主要为ros2的各个功能包、视觉识别等
    - `ros2_ws` :ros2工作区
        - `robot_bring_up` :机器人启动功能包
        - `robot_base_controller` :底盘控制功能包
        - `robot_arm_controller` :机械臂控制功能包
        - `robot_state_manager` :系统状态管理功能包
        - `robot_description` :机器人URDF模型和描述文件
        - `robot_teleop` :遥控操作功能包
        - `robot_sensor_fusion` :传感器数据融合功能包
        - `robot_sensor_drivers` :传感器驱动功能包
        - `robot_nav` :导航功能包
	- `robot_application` :导航设置功能包
	- `robot_moveit_config` :机械臂仿真设置功能包
    - `detection` :视觉识别相关代码
# 待办
- [ ] 电控
    - [ ] 硬件调试
        - [x] 履带轮电机
        - [x] 麦轮电机
        - [ ] 机械臂
        - [x] 雷达
        - [ ] 六自由度IMU
    - [ ] 下位机控制
        - [ ] 履带轮控制
        - [x] 麦轮控制
        - [ ] 机械臂控制
    
    - [ ] 上位机功能包开发
        - [ ] 整体机器人启动开发
        - [ ] 底盘控制开发
        - [ ] 机械臂控制开发
        - [ ] 系统状态管理
        - [ ] 机器人URDF模型和描述文件
        - [ ] 遥控操作
        - [ ] 传感器数据融合
        - [ ] 传感器驱动

- [ ] 视觉与导航
    - [ ] 路径规划
    - [ ] 视觉识别

- [ ] 综合调试

# 使用教程(开发中)

## 下位机stm32的ros2环境部署
### 上位机端
**安装micro-Ros构建系统**

安装micro-Ros构建系统实际上是安装micro_ros_setup功能包，将micro_ros_stm32cubemx_utils构造静态库也是借助于这个功能包
具体安装步骤如下：
#### 1. 首先在终端source一下`setup.bash`
```bash
# Source the ROS 2 installation
source /opt/ros/$ROS_DISTRO/setup.bash
```
#### 2. 切换至ros2的工作区
```bash
#  切换至你的ros2工作区即可
cd ros2_ws
# Download the micro-ROS tools
git clone -b $ROS_DISTRO https://github.com/micro-ROS/micro_ros_setup.git src/micro_ros_setup
```
#### 3. 更新和安装依赖
```bash
# Update dependencies using rosdep
sudo apt update && rosdep update
rosdep install --from-paths src --ignore-src -y
```
如果没有pip的话需安装pip,有的话请忽略
```bash
sudo apt-get install python3-pip
```

#### 4. colcon构建和source
```bash
# Build micro-ROS tools and source them
colcon build
source install/local_setup.bash
```

#### 5. 配置micro_ros_agent
在ros2的工作区执行以下命令
```bash
ros2 run micro_ros_setup create_agent_ws.sh 
ros2 run micro_ros_setup build_agent.sh 
source install/local_setup.sh
```

### 下位机端

#### 1. 切换至stm32代码的工作目录
```bash
cd lowwer_computer/LowwerController
```

#### 2.配置micro_ros_stm32cubemx_utils
(此步可参考官方的配置教程[micro_ros_stm32cubemx_utils](https://github.com/micro-ROS/micro_ros_stm32cubemx_utils))

首先按照我们项目给出的`LowwerController.ioc`配置好串口DMA和FreeRTOS(或可直接使用本项目的ioc文件)
接着下载源码
```bash
# Download the utils
git clone https://github.com/micro-ROS/micro_ros_stm32cubemx_utils.git
cd micro_ros_stm32cubemx_utils/
# 切换ros2版本至humble,根据你的ros2版本进行选择
git checkout humble
```

修改项目的Makefile文件(注意cubemx生成源码时勾选makefile类型)
在`build the application`部分前添加配置代码:

```makefile
#######################################
# micro-ROS addons
#######################################
LDFLAGS += micro_ros_stm32cubemx_utils/microros_static_library/libmicroros/libmicroros.a
C_INCLUDES += -Imicro_ros_stm32cubemx_utils/microros_static_library/libmicroros/microros_include

# Add micro-ROS utils
C_SOURCES += micro_ros_stm32cubemx_utils/extra_sources/custom_memory_manager.c
C_SOURCES += micro_ros_stm32cubemx_utils/extra_sources/microros_allocators.c
C_SOURCES += micro_ros_stm32cubemx_utils/extra_sources/microros_time.c

# Set here the custom transport implementation
C_SOURCES += micro_ros_stm32cubemx_utils/extra_sources/microros_transports/dma_transport.c

print_cflags:
   @echo $(CFLAGS)
```

配置好docker环境，根据自己的ros2版本选择镜像，拉取镜像并运行（此过程可能因为网络原因报错，有时候需要多次执行，多次执行后成功）

```bash
sudo docker pull microros/micro_ros_static_library_builder:humble
sudo docker run -it --rm -v $(pwd):/project --env MICROROS_LIBRARY_FOLDER=micro_ros_stm32cubemx_utils/microros_static_library microros/micro_ros_static_library_builder:humble
```

若在该步骤因为网络问题持续报错，可修改docker代理(配置为自己VPN的代理),给出步骤如下:
```bash
sudo mkdir /etc/systemd/system/docker.service.d
sudo vim /etc/systemd/system/docker.service.d/http-proxy.conf 
```
在该文件写入:
```
 [Service]
 Environment="HTTP_PROXY=http://127.0.0.1:7890"
 Environment="HTTPS_PROXY=http://127.0.0.1:7890"
```
重启docker即可:
```bash
 sudo systemctl daemon-reload
 sudo systemctl restart docker
```


最后配置`freertos.c`文件的tasks,可直接使用本项目的`freertos.c`
需要根据自己配置的串口编号修改`void StartDefaultTask(void *argument)`的` rmw_uros_set_custom_transport`部分

#### 3.编译与烧录

首先需要安装编译器 `arm-none-eabi-gcc`
```bash
sudo apt install gcc-arm-none-eabi
```
然后在Makefile文件所在目录下执行:
```bash
make -j8
```
生成hex与bin文件至`build`文件夹下。
切换至`build`文件夹下进行烧录，这里以`st-link`为例

安装`st-link`(如已安装，可跳过):
```bash
#安装必要的依赖
sudo apt update
sudo apt install -y git make cmake gcc g++ libusb-1.0-0-dev
#从源码编译安装​
git clone https://github.com/stlink-org/stlink
cd stlink
make release
sudo make install
#更新动态库链接
sudo ldconfig
#安装后验证
st-info --version
```
将stm32板子通过烧录器连接至电脑，执行烧录指令（需在`build`目录下）：
```bash
st-flash write LowwerController.bin 0x8000000
```

### 测试

使用之前配置好的串口连接至上位机，在上位机ros2工作区目录下启动agent:
```bash
ros2 run micro_ros_agent micro_ros_agent serial -b 115200 --dev /dev/ttyUSB0
```
这里的USB端口号可根据自己的情况进行调整，使用`lsusb`指令查看自己的串口

上位机再开新的终端运行：
```bash
ros2 topic list
ros2 node list
```
分别显示
```bash
/cubemx_publisher
/parameter_events
/rosout
```
以及
```bash
/cubemx_node
```
即为成功

## 雷达驱动(有线串口)

### 说明
本项目采用的为鱼香ROS的FishBot二驱机器人的配套雷达EAI-X2，在其基础上进行二次开发

### 驱动安装步骤

#### 1.下载源码到工作区目录
```bash
git clone https://github.com/fishros/ydlidar_ros2 -b  v1.0.0/fishbot 
```

#### 2.修改配置文件
将文件`ydlidar_ros2/params/ydlidar.yaml`的串口编号修改为自己的串口编号(一般为`/dev/ttyUSB0`,可通过`ls /dev/ttyUSB*`进行查询),如下:

```yaml
ydlidar_node:
  ros__parameters:
    port: /dev/ttyUSB0
    frame_id: laser_frame
    ignore_array: ""

```

#### 3.编译并运行
```bash
colcon build
#修改串口权限
sudo chmod 666 /dev/ttyUSB0
source install/setup.bash
ros2 launch ydlidar ydlidar_launch.py
```
出现以下结果即表明成功:

```bash
---
[INFO] [launch]: All log files can be found below /home/pi/.ros/log/2023-07-21-23-13-28-893425-raspberrypi-4518
[INFO] [launch]: Default logging verbosity is set to INFO
[INFO] [ydlidar_node-1]: process started with pid [4539]
[INFO] [static_transform_publisher-2]: process started with pid [4541]
[static_transform_publisher-2] [WARN] [1689952409.891692804] []: Old-style arguments are deprecated; see --help for new-style arguments
[static_transform_publisher-2] [INFO] [1689952409.975433434] [static_tf_pub_laser]: Spinning until stopped - publishing transform
[static_transform_publisher-2] translation: ('0.020000', '0.000000', '0.000000')
[static_transform_publisher-2] rotation: ('0.000000', '0.000000', '0.000000', '1.000000')
[static_transform_publisher-2] from 'base_link' to 'laser_frame'
[ydlidar_node-1] [YDLIDAR INFO] Current ROS Driver Version: 1.4.5
[ydlidar_node-1] [YDLIDAR]:SDK Version: 1.4.5
[ydlidar_node-1] [YDLIDAR]:Lidar running correctly ! The health status: good
[ydlidar_node-1] [YDLIDAR] Connection established in [/dev/ttyUSB0][115200]:
[ydlidar_node-1] Firmware version: 1.5
[ydlidar_node-1] Hardware version: 1
[ydlidar_node-1] Model: S4
[ydlidar_node-1] Serial: 2020112400007024
[ydlidar_node-1] [YDLIDAR]:Fixed Size: 370
[ydlidar_node-1] [YDLIDAR]:Sample Rate: 3K
[ydlidar_node-1] [YDLIDAR INFO] Current Sampling Rate : 3K
[ydlidar_node-1] [YDLIDAR INFO] Now YDLIDAR is scanning ......
```

## 导航

### 配置环境
安装好`navigation2`
```bash
sudo apt install ros-$ROS_DISTRO-navigation2
sudo apt install ros-$ROS_DISTRO-nav2-bringup
```
### 导航参数配置
在`robot_nav/config/nav2_param.config`文件进行导航参数的配置。具体参数的含义见[Nav2 Configuration Guide](https://docs.nav2.org/configuration/index.html)


## 仿真

说明:仿真小车模型使用fishbot开源模型[fishbot](https://github.com/fishros/ros2bookcode/tree/master/chapt6/chapt6_ws/src/fishbot_description)

### 环境配置
若想进行仿真,请确保安装好以下依赖
```bash
sudo apt install ros-$ROS_DISTRO-slam-toolbox
sudo apt install ros-$ROS_DISTRO-ros2-control
sudo apt install ros-$ROS_DISTRO-ros2-controllers
sudo apt install ros-$ROS_DISTRO-gazebo-ros2-control
sudo apt install ros-$ROS_DISTRO-rqt-*
sudo apt install ros-$ROS_DISTRO-robot-state-publisher
sudo apt install ros-$ROS_DISTRO-joint-state-publisher
sudo apt install ros-$ROS_DISTRO-gazebo-ros-pkgs
```


# 温馨提示

若使用VsCode进行源码编辑时爆红，可在`.vscode`文件夹下新建`c_cpp_properties.json`文件，写入：
```json
{
  "configurations": [
    {
      "name": "Linux",
      "includePath": [
        "${workspaceFolder}/**"
      ],
      "defines": [
        "USE_HAL_DRIVER",
        "STM32F407xx"
      ],
      "compilerPath": "/usr/bin/clang-14",
      "cStandard": "c17",
      "cppStandard": "c++14",
      "intelliSenseMode": "linux-clang-x64"
    },
    {
      "name": "STM32",
      "includePath": [
        "Core/Inc",
        "Drivers/CMSIS/Device/ST/STM32F4xx/Include",
        "Drivers/CMSIS/Include",
        "Drivers/STM32F4xx_HAL_Driver/Inc",
        "Drivers/STM32F4xx_HAL_Driver/Inc/Legacy",
        "Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2",
        "Middlewares/Third_Party/FreeRTOS/Source/include",
        "Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F",
        "micro_ros_stm32cubemx_utils/microros_static_library"
      ],
      "defines": [
        "STM32F407xx",
        "USE_HAL_DRIVER"
      ],
      "compilerPath": "/usr/bin/arm-none-eabi-gcc"
    }
  ],
  "version": 4
}
```
将报红库的路径添加进`includePath`字段中，其他字段可根据自己的项目进行更改。（刷新重进即可生效）


