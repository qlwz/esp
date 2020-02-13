# 欢迎使用 本固件

**本固件专为ESP8266(8285)打造，暂时支持继电器，窗帘，峥果浴霸**

# 介绍
1. 继电器  
  PCB：待开源  
  
2. 窗帘 向日葵KT82TV/W电机  
   原贴：https://bbs.iobroker.cn/t/topic/847  
   PCB: https://bbs.iobroker.cn/t/topic/852  

3. 峥果浴霸 ESP8285版本  
   主要代码抄SkyNet的：https://github.com/liuzhe1947/homeassistant_zinguo_mqtt  
>    致谢：  
> 以下排名不分先后，为随机。  
> SkyNet：提供固件代码 ：https://github.com/liuzhe1947/homeassistant_zinguo_mqtt  
> 老妖：SC09A驱动编写，SC09A 测试DEMO https://github.com/smarthomefans/zinguo_smart_switch  
> 楓づ回憶：提供硬件与后期代码测试与更改  
> 快乐的猪：修复代码bug与mqtt部分  
> NoThing ：前期画制原理图、测试引脚走向、协议分析、代码编写  
   
# 如何编译  
Visual Studio Code + PlatformIO ID 开发  [安装](https://www.jianshu.com/p/c36f8be8c87f)   
不同的固件由platformio.ini配置  


    [platformio]
    default_envs = zinguo

default_envs 取值：   
1. 继电器：relay  
2. 窗帘：cover  
3. 果峥浴霸：zinguo  
4. 小米AI音箱：xiaoai 
5. DC1插线板：dc1

# 如何接入HA  
### 方法一  
WEB页面开启**MQTT自动发现**  

------------

### 方法二  
手动添加MQTT主题  
以下**ESP_XXXX**都要换成你的（WEB页面可以查看）  
1. 继电器  

2. 窗帘  
  [HA配置文件](https://github.com/qlwz/esp/blob/master/file/yaml/cover.yaml)  

3. 峥果浴霸  
  [HA配置文件](https://github.com/qlwz/esp/blob/master/file/yaml/zinguo.yaml)   

# 图片效果 
![image](https://github.com/qlwz/esp/blob/master/file/images/tab1.png)  
![image](https://github.com/qlwz/esp/blob/master/file/images/tab2.png)  
![image](https://github.com/qlwz/esp/blob/master/file/images/tab4.png)  

![image](https://github.com/qlwz/esp/blob/master/file/images/relay.png)  
![image](https://github.com/qlwz/esp/blob/master/file/images/cover.png)  
![image](https://github.com/qlwz/esp/blob/master/file/images/zinguo.png)  
