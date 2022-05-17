# SimpleNPS

A Simple Network Protocol Stack

从数据链路层开始，从下至上实现一个基本的计算机网络协议栈。

1. 基于winpcap提供的接口，实现以太网数据帧的封装，解包。
2. 基于1的接口，实现ipv4的数据包的分片，封装，解包，组装。
3. 实现arp协议，实现ip地址向mac地址的转换；实现icmp协议。
4. 基于2的接口，实现传输层udp协议和tcp协议的基本功能。
5. 实现http协议，做一个简单的server（linux上）和browser。

其他：使用多线程编程思想，并对临界资源进行互斥与同步。

注：TCP功能不全，本地测试收发图片是正常的。
