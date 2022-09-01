#include<iostream>
#include<stdlib.h>
#include"tcpsever.h"
#include <signal.h>
#include <string.h>
#include "mysql.h"
using namespace std;
extern Mysql Mysql_sever;

map<int,struct event*> aaa;
void sighandle(int sig)
{
	char cmd[100] = "DELETE FROM online;";
cout<<"\n exit cmd: "<<cmd<<endl<<endl;
	mysql_real_query(Mysql_sever._mpcon,cmd,strlen(cmd));
	if(mysql_real_query(Mysql_sever._mpcon,cmd,strlen(cmd)))
	{
		cout<<"delete fail"<<endl;
	}
	map<int ,struct event*>::iterator it = aaa.begin();
	for( ; it!= aaa.end() ; ++it)
	{
		event_free(it->second);
	}	
	raise(SIGKILL);
}

int main(int argc,char *argv[])
{
	if(argc < 4)
	{
		cout<<"error"<<endl;
		return 0;
	}
	signal(SIGINT,sighandle);
	//分离参数
	char *p1 = argv[1];
	char *p2 = argv[2];
	char *p3 = argv[3];
	
	char *ip = p1;
	short port = atoi(p2);
	int pth_num = atoi(p3);
	
	int _pth_num = pth_num ;    //设置子线程最多的个数
	
	Tcpsever sever(ip,port,_pth_num);
	
	cout<<" Tcpsever::Tcpsever()" <<endl;	

	sever.run();
}


