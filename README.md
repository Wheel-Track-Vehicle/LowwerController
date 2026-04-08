# 简介
本项目旨在开发基于轮履复合底盘以及六自由度舵机结构的小型救援机器人，上位机采用树莓派4B运行ROS2，下位机使用STM32F407控制底盘和舵机。

# 环境介绍
- **主机系统**: ubuntu 22.04
- **ROS2版本**: ros2-humble
- **stm32型号**: stm32F407ZGT6
- **cubemx版本**: >=6.15.0

# 使用教程

## stm32的ros2环境部署
### 上位机端
**安装micro-Ros构建系统**

安装micro-Ros构建系统实际上是安装micro_ros_setup功能包，将micro_ros_stm32cubemx_utils构造静态库也是借助于这个功能包
具体安装步骤如下：
#### 1. 运行`setup.bash`脚本
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

使用docker,根据自己的ros2版本选择镜像，拉取镜像并运行（此过程可能因为网络原因报错，有时候需要多次执行，多次执行后成功）

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

