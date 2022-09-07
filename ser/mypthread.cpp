#include <iostream>
#include <event.h>
#include <pthread.h>
#include <cstdlib>
#include <jsoncpp/json/json.h> 
#include <string.h>
#include "mypthread.h"
#include "control.h"
#include "public.h"
#include <unistd.h>
#include <signal.h>
#include "View.h"
#include "register.h"
#include "login.h"
#include "get_list.h"
#include "talk_one.h"
#include "talk_group.h"
#include "exit.h" 
using namespace std;
// Control control_sever;
extern map<int,struct event*> aaa;


//转发客户端的消息
void client_cb(int fd,short event,void *arg)
{
	cout<<"\n+++++++++++++++++in  client_cd() +++++++++++++++++++++"<<endl;
	//recv  ->buff
	char buff[128] = {0};
	Pthread *mythis = (Pthread*)arg;
	//struct event_base* _base = (struct event_base*)arg;
	if(-1==recv(fd,buff,127,0))
	{
		cout<<"recv error"<<endl;
		return;
	}
	else
	{
		cout<<"last recv success"<<endl;
	}

	//将buff发给control
	cout<<"control_sever.process(fd,buff)"<<endl;

	//解析json，获取消息类型
	Json::Value root;
	Json::Reader read;

	if(-1 == read.parse(buff,root))
	{
		cout<<"json parse fail;"<<endl;
		return;
	}
	else
		cout<<" scan success "<<endl;
	
	int type = root["reason_type"].asInt();
	cout<<"type: "<<type<<endl;
	map<int ,View*>::iterator it =mythis->_type_map.find(type);
	//cout<<it->first<<" "<<it->second<<endl;
	if( it ==  mythis->_type_map.end())
	{
		cout<<"not find"<<endl;
	}
	//如果是退出消息类型，将该子线程对应的记录删掉
	if(it->first == MSG_TYPE_EXIT){
		mythis->_event_map.erase(fd);
		//给主线程回复当前监听的客户端数量
		int num = mythis->_event_map.size();
		cout<<"子线程 sockepair[1] = "<<mythis->_sock_fd<<" have "<<num<<" client fd"<<endl;
		char abuff[16];
		sprintf(abuff,"%d",num);
		if( 0> write(mythis->_sock_fd , abuff ,sizeof(abuff)))
		{
			cout<<"write error"<<endl;
		}
	}
	it->second->process(fd,buff);
	it->second->response();
	cout<<"\n+++++++++++++++++out client_cd() +++++++++++++++++++++"<<endl;	

}

//读主线程发来的消息
void sock_pair_1_cb(int fd,short event,void *arg)
{
	cout<<"\n+++++++++++++++++in 子线程 sock_pair_1_cb +++++++++++++++++++++"<<endl;
	//recv -> clien_fd
	Pthread *mythis = (Pthread *)arg;
	//struct event_base* _base = (struct event_base*)arg;
	char buff[20] = {0};

	if( 0 > read(mythis->_sock_fd , buff ,sizeof(buff)))
	{
		cout<<"recv error"<<endl;
	}

	int client_fd = atoi(buff);
	//将client_fd加入到libevent     client_cb()
	struct event* listen_event = event_new(mythis->_base,client_fd,EV_READ|EV_PERSIST,client_cb,arg);
	event_add(listen_event,NULL);

	mythis->_event_map.insert(make_pair(client_fd,listen_event));
	aaa.insert(make_pair(client_fd,listen_event));
	
	//给主线程回复当前监听的客户端数量
	int num = mythis->_event_map.size();
	cout<<"sockepair[1] = "<<mythis->_sock_fd<<" have "<<num<<" client fd"<<endl;
	char abuff[16];
	sprintf(abuff,"%d",num);
	if( 0> write(mythis->_sock_fd , abuff ,sizeof(abuff)))
	{
		cout<<"write error"<<endl;
	}

	cout<<"+++++++++++++++++out sock_pair_1_cb()+++++++++++++++++++++++"<<endl;

}

void *pthread_run(void *arg)
{
	Pthread *mythis = (Pthread *)arg;
	cout<<"pthread recv socketpair[1] = "<<mythis->_sock_fd<<endl;

	//将sock_pair_1加入到libevent  sock_pair_1_cb()
	struct event* sock_event = event_new(mythis->_base,mythis->_sock_fd,EV_READ|EV_PERSIST,sock_pair_1_cb,arg);
	//struct event* sig_event = evsignal_new(mythis->_base2, SIGINT,signal_cb,arg);

	event_add(sock_event,NULL);
	event_base_dispatch( mythis->_base);

}


Pthread::Pthread(int sock_fd)
{
	_type_map.insert(make_pair(MSG_TYPE_REGISTER,new Register));
	_type_map.insert(make_pair(MSG_TYPE_LOGIN,new Login));
	_type_map.insert(make_pair(MSG_TYPE_GET_LIST,new Get_list));
	_type_map.insert(make_pair(MSG_TYPE_TALK_TO_ONE,new Talk_one));
	_type_map.insert(make_pair(MSG_TYPE_TALK_TO_GROUP,new Talk_group));
	_type_map.insert(make_pair(MSG_TYPE_EXIT,new Exit));
	_sock_fd = sock_fd;
	_base = event_base_new();
	//启动线程
	if(0 != pthread_create(&_pthread,NULL,pthread_run,this))
	{
		cerr<<"pthread create fail"<<endl;
	}
	pthread_detach(_pthread);
}

Pthread::~Pthread(){
	cout<<"Pthread ~Pthread"<<endl;
	event_base_free(this->_base);
	close(this->_sock_fd);
}  