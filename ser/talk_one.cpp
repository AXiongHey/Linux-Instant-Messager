#include <jsoncpp/json/json.h> 
#include <mysql/mysql.h>
#include <sys/socket.h>
#include <string>
#include <string.h>
#include<unistd.h>
#include <stdio.h>
#include "talk_one.h"
#include "mysql.h"
#include<iostream>
extern Mysql Mysql_sever;

void Talk_one::process(int fd,string json)
{
	_fd = fd;
	Json::Value root;
	Json::Reader read;

	int success = 0;

	if(-1 == read.parse(json,root))
	{
		cerr<<"json parse fail;"<<endl;
		return;
	}

	char myname[20] = {0};
	strcpy(myname,root["name"].asString().c_str());
	
	char hername[20] = {0};
	strcpy(hername,root["hername"].asString().c_str());

	char sendbuf[1024] = {0};
	strcpy(sendbuf,root["message"].asString().c_str());
		
	char cmd[100]="SELECT * FROM online WHERE name='";
	strcat(cmd,hername);
	strcat(cmd,"';");

	cout<<"talk_one cmd: "<<cmd<<endl;
	cout<<"talk_one json: "<<json<<endl;
	int ffd = -1;
	mysql_real_query(Mysql_sever._mpcon,cmd,strlen(cmd));
	Mysql_sever._mp_res = mysql_store_result(Mysql_sever._mpcon);

	//查询接收方用户是否在线
	if(Mysql_sever._mp_row=mysql_fetch_row(Mysql_sever._mp_res))
	{
		cout<<"row[0]"<<Mysql_sever._mp_row[0]<<endl;
		sscanf(Mysql_sever._mp_row[0],"%d",&ffd);
		success = 1;
	}
	
	//接收方在线直接发送消息
	if(success)
	{
		char sendbuf2[1024] = "your friend ";
		strcat(sendbuf2,myname);
		strcat(sendbuf2,":");
		strcat(sendbuf2,sendbuf);
		send(ffd,sendbuf2,strlen(sendbuf2),0);
		cout<<"ffd: "<<ffd<<endl;
		_str = "message send!";
		return;
	}
	// 用户不在线
	else
	{
		char cmd2[100]="SELECT * FROM user WHERE name='";
		strcat(cmd2,hername);
		strcat(cmd2,"';");
	cout<<"talk_one cmd: "<<cmd<<endl;
		mysql_real_query(Mysql_sever._mpcon,cmd2,strlen(cmd2));
		Mysql_sever._mp_res = mysql_store_result(Mysql_sever._mpcon);

		//查询是否有接收方用户，有则插入该用户的离线信息到offline表中
		if(Mysql_sever._mp_row=mysql_fetch_row(Mysql_sever._mp_res))//OFFLINE MESSAGE
		{
			char sendbuf2[1024] = "offline message your friend ";
			strcat(sendbuf2,myname);
			strcat(sendbuf2,":");
			strcat(sendbuf2,sendbuf);

			char cmd3[100] = "INSERT INTO offline VALUE('";
			strcat(cmd3,hername);
			strcat(cmd3,"','");
			strcat(cmd3,sendbuf2);
			strcat(cmd3,"');");
cout<<"offline message:"<<cmd3<<endl;
			mysql_real_query(Mysql_sever._mpcon,cmd3,strlen(cmd3));
			_str = "offline message send!";
			return;
		}
		
	}
	_str = "message send fail!";
}
void Talk_one::response()
{		
	//发送状态
	send(_fd,_str.c_str(),strlen(_str.c_str()),0);
}