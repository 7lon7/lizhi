基于udp的广播电台

src
├── client
│   ├── Makefile
│   ├── client.c
│   └── client.h
├── include
│   ├── proto.h
│   └── site_type.h
└── server
    ├── Makefile
    ├── medialib.c
    ├── medialib.h
    ├── server.c
    ├── server.h
    ├── server_conf.c
    ├── server_conf.h
    ├── tbf.c
    ├── tbf.h
    ├── thr_channel.c
    ├── thr_channel.h
    ├── thr_list.c
    └── thr_list.h


1.medialib 解析本地文件夹下的流媒体文件
 媒体库文件夹
 media
 ├── channel_1
 │   ├── black_sheep.mp3
 │   ├── desc.txt
 │   ├── sleepyhead.mp3
 │   └── whoa.mp3
 ├── channel_2
 │   ├── aaa.mp3
 │   ├── bbb.mp3
 │   └── desc.txt
 └── channel_3
     ├── ccc.mp3
     ├── ddd.mp3
     └── desc.txt


2.thr_list 起一个线程，发送频道信息

3.thr_channel 每个频道对应一个线程，负责发送本频道的流媒体数据





server 工作流程：
1.处理参数，设置
    	.运行模式
	.端口
	.广播地址
	.媒体库路径
	.网卡
	...
2.处理信号，将终止信号捕捉并设置回调，回调来处理资源，结束进程
3.创建套接字，设置套接字广播，设置接受端信息
4.thr_list
5.thr_channel
6.主线程pause



client 工作流程：
1.处理参数，选择
    	.播放器 //TODO
	.端口
	.广播地址
	...
2.创建套接字，加入多播组，绑定信息
3.创建管道，分出子进程
4.父进程接收‘频道信息’和‘流媒体数据’，将‘频道信息’打印到屏幕，将‘流媒体数据’通过管道发送给子进程
5.子进程接收父进程的数据，并调用播放器解析并播放


目前依赖于mpg123 : http://www.mpg123.org

（学习使用）
