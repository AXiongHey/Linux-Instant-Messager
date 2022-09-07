#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<memory.h>
#include<string>
#include<exception>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include <unistd.h>
#include <jsoncpp/json/json.h> 
#include<sys/types.h>
#include<event.h>
#include "tcpsever.h"
#include "mypthread.h"
#include "mysql.h"
#include <signal.h>
using namespace std;
extern map<int,struct event*> aaa;
extern Mysql Mysql_sever;

void signalCallback(int sig){
	char cmd[100] = "DELETE FROM online;";
cout<<"\n exit cmd: "<<cmd<<endl<<endl;
	mysql_real_query(Mysql_sever._mpcon,cmd,strlen(cmd));
	if(mysql_real_query(Mysql_sever._mpcon,cmd,strlen(cmd)))
	{
		cout<<"delete fail"<<endl;
	}
	cout<<"signalCallback"<<endl;
	map<int ,struct event*>::iterator it = aaa.begin();
	for( ; it!= aaa.end() ; ++it)
	{
		close(it->first);
		event_free(it->second);
		cout<<it->first<<" "<<it->second<<endl;
	}
	exit(0);
}
	//接受用户连接的回调函数
void listen_cb(int fd,short event,void *arg)
{
	std::cout<<"\n+++++in listen_cb()---->accept clietnfd,write to sun thread+++++"<<endl;
	Tcpsever *mythis = (Tcpsever*)arg;
	struct sockaddr_in caddr;
	socklen_t len = sizeof(caddr);
	int cli_fd = accept(fd,(struct sockaddr*)&caddr,&len);
	std::cout<<"cli_fd: "<<cli_fd<<endl;
	if(-1 == cli_fd)
	{
		cerr<<"accept fail"<<endl;
		return;
	}
	//查找当前监听数量最少的子线程

	int tmpfd = 0;//存储最小监听数对应的fd

	int minListen = 65536;//  存储最小监听数
	auto it = mythis->_pth_work_num.begin();

	//查找当前监听数量最少的子线程
	for( ; it != mythis->_pth_work_num.end() ; ++it )
	{
		if( it->second < minListen )
		{
			minListen = it->second;
			tmpfd = it->first;
			//cout<<"minListen: "<<minListen<<"  tmpfd: "<<tmpfd<<endl;
		}
	}

	std::cout<<"minlisten: "<<minListen<<"  minfd: "<<tmpfd<<endl;
	
	//将主线程里的 客户端套接字 通过 socktpair 发给子线程
	char abuff[16] = {0};
	sprintf( abuff,"%d",cli_fd);

	if( 0 > write(tmpfd, abuff, sizeof(abuff)) )
	{
		std::cout<<"write error"<<endl;
	}
	else
	{
		std::cout<<"write success"<<endl;
	}
	std::cout<<"+++++++++++++out listen_cb()++++++++++++++++++++++++++++++"<<endl<<endl;
}

//读子线程发来的该线程监听的客户端数量，加入map表
void sock_pair_cb(int fd,short event,void *arg)
{
	std::cout<<"+++++in sock_pair_cb()---->read clietnfd,insert _pth_work_num+++++"<<endl;
	
	Tcpsever *mythis = (Tcpsever *)arg;
	int num = 0;
	char buff[16];
	int len = read(fd,buff,sizeof(buff));
	if( 0 > len )
	{
		std::cout<<"read error"<<endl;
	}
	else if(len == 0){
		return;
	}else
	{
	num = atoi(buff);

	//更新到map表_pth_work_num  ----->fd

	map<int,int>::iterator it2 = mythis->_pth_work_num.begin();

	for( ; it2!= mythis-> _pth_work_num.end() ; ++it2)
	{
		if( it2->first == fd)
		{
			break;
		}
	}
	it2->second = num;

	auto it = mythis->_pth_work_num.begin();
	while (it != mythis->_pth_work_num.end())
	{
		std::cout<<it->first<<" "<<it->second<<endl;
		it++;
	}
	
	std::cout<<"+++++++++++++out sock_pair_cb()+++++++++++++++++++++++++"<<endl<<endl;
	//it2->second= num;
	}

}

Tcpsever::Tcpsever(char *ip,short port,int pth_num)
{
	///创建服务器
	_pth_num = pth_num;

		_listen_fd = socket(AF_INET,SOCK_STREAM,0);
		if(-1 == _listen_fd){
			perror("listen_fd:");
			return ;
		}


	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = inet_addr(ip);

		//设置端口复用
		int optval = 1;
		if(-1 ==setsockopt(_listen_fd,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof(optval))){
			perror("reuse: ");
			return;
		}
		if(-1 == bind(_listen_fd,(struct sockaddr*)&saddr,sizeof(saddr)))
		{
			perror("bind: ");
			return;
		}
		if(-1 == listen(_listen_fd,20))
		{
			perror("listen: ");
			return;
		}

	//给libevent申请空间
	_base = event_base_new();

	//创建事件，绑定监听套接子的回调函数(listen_cb)

		struct event* ev =  event_new(_base,_listen_fd,EV_READ|EV_PERSIST,listen_cb,this);
		if(ev != NULL)
		{
			std::cout<<"event create success!\n"<<endl;
		}
		else
		{
			std::cout<<"event new fail\n"<<endl;
		}

		//将事件添加到事件列表
		event_add(ev,NULL);

        std::cout<<"tcpsever listen cb already carry out\n"<<endl;

}

void Tcpsever::run()
{
	// cout<<"+++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;
	// cout<<"in run()"<<endl;
	//申请socketpair（函数自查）
	get_sock_pair();

	//创建线程   //规定  int arr[2]  arr[0]<=>主线程占用   arr[1]<=>子线程占用
	get_pthread();

	//为主线程的socktpair创建事件，绑定回调函数（sock_pair_cb）	
	for(int i = 0 ; i< _pth_num ; ++i)//由 子线程的读事件  触发
	{
		struct event* ev =  event_new(_base,_socket_pair[i].arr[0],EV_READ|EV_PERSIST,sock_pair_cb,this);
		if(NULL == ev)
		{
			perror("event create:");
			return ;
		}	
		else{
			event_add(ev,NULL);
			std::cout<<"Main thread socketpair[0] = "<<_socket_pair[i].arr[0]<<endl;
		}

	}
	signal(SIGINT,signalCallback);
	event_base_dispatch(_base);
}

Tcpsever::~Tcpsever(){
	close(_listen_fd);
	event_base_free(_base);
}
void Tcpsever::get_sock_pair()
{
	for(int i = 0;i < _pth_num;i++ )
	{
		//申请双向管道
		int pair[2];
		if( socketpair(AF_UNIX,SOCK_STREAM,0,pair) == -1  )
		{
			std::cout<<"socketpair error"<<endl;
			return;
		}
		pipe pi(pair);
		_socket_pair.push_back(pi); //将双向管道加入到_sock_pair.push_back();

		_pth_work_num.insert(make_pair(pi.arr[0],0));
	}

}

void Tcpsever::get_pthread()
{
	//开辟线程
	for(int i = 0; i< _pth_num; i++)
	{
		Pthread *th = new Pthread(_socket_pair[i].arr[1]);
		_pthread.push_back(th);
	}
	//cout<<"get_pthread() success"<<endl;
}
