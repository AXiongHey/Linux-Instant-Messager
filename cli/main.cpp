#include<iostream>
#include<pthread.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <memory.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "fun.h"

using namespace std;

int fd = -1;
void fun(int sig)
{
	do_exit(fd);
}
//线程函数，在当前状态下监测服务端是否断开
void *main_run(void *arg)
{
	long c = (long) arg;
	char buff[1024] = {0};
	int n = recv(c,buff,1023,0);
	if(n==0){
		cout<<"Main:Server end , exit"<<endl;
		exit(0);
	}
}
int main(int argc,char *argv[])
{
	if(argc < 3)
	{
		perror("argc");
		return 0;
	}
	fd = socket(AF_INET,SOCK_STREAM,0);
	if( -1 == fd)
	{
		perror("socket");
		return 0;
	}

	//绑定信号处理函数（do_exit）;
	signal(SIGINT,fun);
	//分离ip地址和端口号
	char *p1 = argv[1];
	char *p2 = argv[2];

	struct sockaddr_in saddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(atoi(p2));
	saddr.sin_addr.s_addr = inet_addr(p1);
	//链接服务器

	int res = connect(fd,(struct sockaddr*)&saddr,sizeof(saddr));
	if( -1 == res)
	{
		perror("connet");
		return 0;
	}
	// fd_set rdset,tmp;
	// FD_ZERO(&rdset);
	// FD_SET(fd, &rdset);
	// struct timeval timeout;
	// timeout.tv_sec = 1;
	// timeout.tv_usec = 0;

	int m = 0;
	//char buf[256];

	while(1)
	{
		pthread_t id;
		pthread_create(&id,NULL,main_run,(void *)fd);
		// tmp = rdset;
		// int  ret = select(fd+1, &tmp, NULL, NULL, &timeout);
		// if(ret == -1) {
		// 	perror("select");
		// 	exit(-1);
		// } else if(ret == 0) {
		// 	;
		// } else if(ret > 0) {
		// 	if(FD_ISSET(fd, &tmp)){
		// 		int len = recv(fd,buf,sizeof(buf),0);
		// 		if(len == 0){
		// 			cout<<"while chose: Server end , exit"<<endl;
		// 			exit(0);
		// 		}
		// 	}
		// }	

		//让用户选择服务
		cout<<"Please input num to select: "<<endl;
		cout<<"\033[31m1.register  2.login   3.exit\n\033[0m"<<endl;
		cin>>m;
		cout<<endl;
		switch(m)
		{
		case 1:
			{do_register(fd);}break;
		case 2:
			{do_login(fd);}break;
		case 3:
			{do_exit(fd);}break;
		default:
			{
				cout<<"error"<<endl;
			}break;
		}
	}

	return 0;
}
