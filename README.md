# Linux-Instant-Messager
项目简介:该项目基于 Linux 环境下开发完成，采用 c/s 客户端服务器架构实现。实现了多用户多客户端接入服务器；服务器负责转发信息和登录注册等操作。实现了对在线和离线用户进行私聊或群发消息的功能。


使用技术:使用 socket 网络库实现客户端和服务端 TCP 连接，并使用 Json 格式进行数据交互；使用socketpair 实现主线程和子线程的通信；服务端使用 libevent 实现 IO 多路复用和 pthread 库构建线程池来提高并发量使用 Mysql 数据库进行用户相关数据存储
