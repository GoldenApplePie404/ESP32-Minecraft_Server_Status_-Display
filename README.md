# 基于ESP32的Minecraft服务器状态查询摆件

## 项目概述

作为MC服务器的开发者和管理员，我时常需要获取服务器的某些状态信息，比如服务器的离线状况、在线玩家数量等。然而，目前查询这些信息的方式颇为繁琐。

我在B站上看到有人使用开发板和屏幕自主制作了一个显示B站用户状态（如粉丝数、播放量等）的项目，这给了我很大的启发。因此，我萌生了制作一个能置于桌面的MC服务器状态显示屏的想法。

我使用ESP32开发板和0.96英寸的OLED屏幕，经过半个星期的折腾，终于弄出来了第一个版本。在后续版本中，我添加了自动翻页的功能，这样就能在有限的空间内显示更多内容了。

## 硬件需求

- ESP32开发板
- 0.96英寸的OLED屏幕，屏幕驱动为SSD1306
- 杜邦线
- 成本总计在20~25元范围内

## 软件实现

### MicroPython版本

该项目使用MicroPython作为编译环境，实现了屏幕驱动和主程序。屏幕驱动用于控制OLED屏幕的显示，主程序用于连接WiFi网络、获取服务器状态信息并在屏幕上显示。

### Arduino版本

在后续的更新中，我还开发出了Arduino版本，与MicroPython版本不同的是，我加入了动态配置参数（wifi的ssid、密码、服务器地址等）的功能。

## 使用说明

### MicroPython版本

1. 将屏幕驱动和主程序上传至ESP32中
2. 根据代码里的指示正确连接好屏幕（SCK-4, SDA-5）
3. 在主程序中设置好WiFi网络参数（SSID和PASSWORD）
4. 设置要查询的MC服务器IP
5. 填写好MCSManager的管理员账户开放的APIkey与面板的网址
6. 确保网络连接成功和服务器地址存在的情况下，屏幕会正常显示服务器状态信息

### Arduino版本

1. 安装所需库（ArduinoJson和Adafruit_SSD1306）
2. 将源代码上传至ESP32中
3. 根据代码里的指示正确连接好屏幕（SCK-4, SDA-5）
4. 使用串口命令设置WiFi网络参数、MC服务器IP、MCSManager的APIkey等
5. 如果你想直接使用bin文件上传到设备，请下载本项目的MC_Server_Status.ino.merged.bin文件，利用相关工具（如esptool）烧录到设备上。

## 可用串口命令

| 指令 | 功能 |
|------|------|
| help | 显示所有可用命令 |
| print_config | 打印当前所有配置 |
| set_ssid &lt;ssid&gt; | 设置 WiFi 名称 |
| set_password &lt;password&gt; | 设置 WiFi 密码 |
| set_apikey &lt;apikey&gt; | 设置 MCSManager 的 API Key |
| set_serverip &lt;serverip&gt; | 设置 Minecraft 服务器地址 |
| set_remote_base_url &lt;url&gt; | 设置 MCSManager 面板 URL（格式：http://xxx.xxxx.xx） |
| reconnect_wifi | 手动断开并重连 WiFi |

## 显示内容

### MC服务器状态页面

- Server: Online|Offline|Unknown|Error —— 服务器在线情况：在线|离线|未知|错误
- Player:x/x —— 当前玩家人数/服务器玩家最大人数

### MCSManager数据页面

- CPU:xx.xx% —— 表示为MCSManager面板占用的CPU情况
- Mem:xx.xx% —— 表示为MCSManager面板占用的内存情况
- Instance:x/x —— 表示为MCSManager面板运行的实例与总实例的情况

## 原理讲解

### API的使用

API（Application Programming Interface, 应用程序编程接口）是一些预先定义的函数，目的是提供应用程序与开发人员基于某软件或硬件得以访问一组例程的能力，而又无需访问源码，或理解内部工作机制的细节。

该项目使用了两个API：

1. MC服务器状态查询API：http://mcstatus.goldenapplepie.xyz/api/?ip=&lt;ip&gt;
2. MCSManager API：http://xxx.xxxx.xx/api/service/remote_services_system?apikey=&lt;api_key&gt;

### ESP32的工作流程

1. 连接到WiFi网络
2. 向指定的API接口发送GET请求，获取MC服务器的状态信息
3. 将返回的JSON格式数据解析成Python字典，从中提取出我们关心的信息
4. 在OLED屏幕上显示服务器状态信息
5. 间隔5秒后，显示第二页有关MCSManager的信息
6. 重复步骤2-5

## 贡献指南

欢迎对本项目进行贡献！您可以通过以下方式参与：

- 报告bug或提出改进建议
- 提交代码修复或新功能
- 编写或更新文档

## 许可证

本项目采用MIT许可证。

## 更多说明

博客文章（附有详细的介绍）：https://blog.goldenapplepie.xyz/?p=1795

## TodoList

1. 全面升级屏幕，并采用能触屏交互、尺寸更大的全彩屏幕；
2. 增加更多查询信息，如服务器icon、服务器motd等；
3. 增加更多高级功能，如使用SmartConfig一键联网、通过触屏远程管理服务器等；
4. ……
