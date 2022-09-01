#include <iostream>
#include <string>
#include <string.h>
#include <stdio.h>
#include <jsoncpp/json/json.h> 
#include <mysql/mysql.h>
#include <sys/socket.h>
#include"login.h"
#include"mysql.h"

using namespace std;
extern Mysql Mysql_sever;
char name[20] = {0};
	
void Login::process(int fd,string json)
{   
	_fd = fd;

	Json::Value root;
	Json::Reader read;

	if(-1 == read.parse(json,root))
	{
		cerr<<"json parse fail;"<<endl;
		return;
	}

	char pw[20] = {0};
	strcpy(pw,root["pw"].asString().c_str());
	strcpy(name,root["name"].asString().c_str());

	// 从user表中查询是否存在该用户
	char cmd[100] = "SELECT * FROM user WHERE name='";
	strcat(cmd,name);
	strcat(cmd,"' and pw='");
	strcat(cmd,pw);
	strcat(cmd,"';");
cout<<"login json: "<<json<<endl;
cout<<"login cmd: "<<cmd<<endl;
	mysql_real_query(Mysql_sever._mpcon,cmd,strlen(cmd));
	Mysql_sever._mp_res = mysql_store_result(Mysql_sever._mpcon);

	if(Mysql_sever._mp_row =mysql_fetch_row(Mysql_sever._mp_res))//用户存在
	{
		//从online表中查询用户是否在线
		char cmd2[100] = "SELECT * FROM online WHERE name='";
		strcat(cmd2,name);
		strcat(cmd2,"';");
		mysql_real_query(Mysql_sever._mpcon,cmd2,strlen(cmd2));
		Mysql_sever._mp_res = mysql_store_result(Mysql_sever._mpcon);
cout<<"login cmd2: "<<cmd2<<endl;
		if((Mysql_sever._mp_row =mysql_fetch_row(Mysql_sever._mp_res)))
		{
			_str= "login fail,";
			_str+=name;
			_str+=" have login.";
			return;
		}
		//用户不在线则将fd,name 加入到数据库的online表
		else
		{
			
			char ffd[16] = {0};
			char cmd3[100] = "INSERT INTO online VALUE('";
			sprintf(ffd,"%d",fd);
			strcat(cmd3,ffd);
			strcat(cmd3,"','");
			strcat(cmd3,name);
			strcat(cmd3,"');");
cout<<"cmd3: "<<cmd3<<endl;

			if(mysql_real_query(Mysql_sever._mpcon,cmd3,strlen(cmd3)))
			{
				cerr<<"insert login fail;"<<endl;
				return;
			}	
			_str = "login sucess!";			
			return;
		}
	}
	_str= "login fail!";	
}

void Login::response()
{		
	//发送响应数据
	send(_fd,_str.c_str(),strlen(_str.c_str()),0);
	
	//若存在离线消息则一并发出
	char cmd11[100] = "SELECT * FROM offline WHERE name='";
	strcat(cmd11,name);
	strcat(cmd11,"';");

	mysql_real_query(Mysql_sever._mpcon,cmd11,strlen(cmd11));
	Mysql_sever._mp_res = mysql_store_result(Mysql_sever._mpcon);

	if(Mysql_sever._mp_row =mysql_fetch_row(Mysql_sever._mp_res))
	{
cout<<"offline message: "<<Mysql_sever._mp_row[1]<<endl;
		send(_fd,Mysql_sever._mp_row[1],strlen(Mysql_sever._mp_row[1]),0);
	}
	char cmd13[100] = "DELETE FROM offline WHERE name='";
	strcat(cmd13,name);
	strcat(cmd13,"';");
	mysql_real_query(Mysql_sever._mpcon,cmd13,strlen(cmd13));
	
}