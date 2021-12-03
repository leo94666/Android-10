sudo xcode-select -switch /Library/Developer/CommandLineTools

#### envsetup:




- hmm


#### 刷写设备

- make fastboot adb
- adb reboot bootloader
- fastboot flashall -w


#### adb 命令
- adb reboot
- adb reboot recovery
- adb reboot bootloader
- fastboot flash boot boot.img
- fastboot flash system system.img
- fastboot flash userdata userdata.img
- fastboot flash aboot emmc_appsboot.mbn 
- fastboot flash recovery recovery.img 
- fastboot flash cache cache.img
- fastboot flash mdtp mdtp.img
- fastboot flash persist persist.img
- fastboot reboot





#### [tcp](https://www.cnblogs.com/RichardTAO/p/12097469.html)

[调试TCP](https://www.cnblogs.com/simoncook/p/9662060.html)
[TCP](http://c.biancheng.net/view/6376.html)


#### tcpdump

tcpdump是一个用于截取网络分组，并输出分组内容的工具。
tcpdump 支持针对网络层、协议、主机、网络或端口的过滤，并提供and、or、not等逻辑语句来帮助你去掉无用的信息

```
tcpdump [ -DenNqvX ] [ -c count ] [ -F file ] [ -i interface ] [ -r file ]
        [ -s snaplen ] [ -w file ] [ expression ]
```

|  抓包选项   | 说明  |
|  ----  | ----  |
| -c  | 指定要抓取的包数量 |
| -i interface  | 指定tcpdump需要监听的接口。默认会抓取第一个网络接口|
|-n|对地址以数字方式显式，否则显式为主机名，也就是说-n选项不做主机名解析|
|-nn|除了-n的作用外，还把端口显示为数值，否则显示端口服务名|
|-P||
|-s len||


|  输出选项   | 说明  |
|  ----  | ----  |
|-e|输出的每行中都将包括数据链路层头部信息，例如源MAC和目标MAC|
|-q|快速打印输出。即打印很少的协议相关信息，从而输出行都比较简短|
|-X|输出包的头部数据，会以16进制和ASCII两种方式同时输出|
|-XX|输出包的头部数据，会以16进制和ASCII两种方式同时输出，更详细|
|-v|当分析和打印的时候，产生详细的输出|
|-vv|产生比-v更详细的输出|
|-vvv|产生比-vv更详细的输出|


|  其他   | 说明  |
|  ----  | ----  |
|-D|列出可用于抓包的接口。将会列出接口的数值编号和接口名，它们都可以用于"-i"后|
|-F|从文件中读取抓包的表达式。若使用该选项，则命令行中给定的其他表达式都将失效|
|-r|从给定的数据包文件中读取数据。使用"-"表示从标准输入中读取|

#### nc命令

用于设置路由器
执行本指令可设置路由器的相关参数。


nc [-hlnruz][-g<网关...>][-G<指向器数目>][-i<延迟秒数>][-o<输出文件>][-p<通信端口>][-s<来源位址>][-v...][-w<超时秒数>][主机名称][通信端口...]

|  选项   | 说明  |
|  ----  | ----  |
|-g<网关> |设置路由器跃程通信网关，最多可设置8个。
|-G<指向器数目>| 设置来源路由指向器，其数值为4的倍数。
|-h |在线帮助。
|-i<延迟秒数>| 设置时间间隔，以便传送信息及扫描通信端口。
|-l |使用监听模式，管控传入的资料。
|-n |直接使用IP地址，而不通过域名服务器。
|-o<输出文件>| 指定文件名称，把往来传输的数据以16进制字码倾倒成该文件保存。
|-p<通信端口> |设置本地主机使用的通信端口。
|-r |乱数指定本地与远端主机的通信端口。
|-s<来源位址>| 设置本地主机送出数据包的IP地址。
|-u |使用UDP传输协议。
|-v |显示指令执行过程。
|-w |<超时秒数> 设置等待连线的时间。
|-z |使用0输入/输出模式，只在扫描通信端口时使用。
