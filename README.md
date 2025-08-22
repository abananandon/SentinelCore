# SentinelCore
一个All in one的边缘智能网关，该网关能够实现：
1. 采集多种下挂设备（串口、IIC、TCP）数据。
2. 在本地进行数据处理与存储。
3. 通过MQTT协议将数据安全上报至云平台（阿里云服务器）。
4. 支持远程控制下挂设备以及网关自身的配置更新和OTA升级。
5. 可以通过Web服务和应用来优雅的展示数据和控制。

# 目前实现的功能
## 1. 设备信息监控模块-采集设备状态
通过读取内核的数据，来获取当前CPU的温度、负载以及系统的内存使用情况。
<img width="2539" height="1162" alt="image" src="https://github.com/user-attachments/assets/e17e8309-2f92-4be5-80ab-3a5dc0c22f2c" />


## 2. 环境光传感器模块-采集环境光信息 
环境光传感器型号为 AP3216C ，使用 I2C 接口与 IMX6ULL 进行通信，能够实现环境光强度（ALS）检测、接近距离（PS）检测和红外线强度（IR）检测。
<img width="2539" height="1162" alt="image" src="https://github.com/user-attachments/assets/918ffe2f-34d5-48f2-ad26-f2996c7041b9" />


## 3. 采集设备MQTT客户端-与Broker进行通信，发布和订阅Topic
这是本项目的核心，客户端将采集到的数据作为一个Tpoic发布到云服务器Broker，同时从云服务器Broker订阅控制命令Topic，来实现数据上传广播与远程控制功能。
每个采集设备会实例化一个MQTT客户端，为多线程所共享，客户端采用了MQTT3.1.1协议与云服务Broker进行通信。目前能够实现设备状态Tpoic的发布。
<img width="2539" height="1150" alt="image" src="https://github.com/user-attachments/assets/35eab37a-5d20-40c9-8298-6e74597158a8" />


# TODO List 
- [x] 设备信息监控模块（获取设备状态）
  - [x] 获取设备基本信息 
  - [x] CPU利用率、温度
  - [x] 内存使用率
- [ ] 温湿度传感器模块
- [x] 环境光传感器模块
- [ ] PMW LED控制模块
- [ ] json 命令格式解析
- [ ] 蓝牙连接模块
- [x] 采集设备MQTT客户端
- [x] MQTT上传数据
- [ ] Web客户端展示数据
- [ ] 添加MQTT设备链接登录功能
- [ ] 提升MQTT链接的安全性：SSL加密





