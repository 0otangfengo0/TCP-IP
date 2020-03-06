#include <thread>
#include "EasyTcpClient.hpp"
#include "MessageHead.hpp"

//5.接收服务器的请求数据

bool g_bRun = true;
//bool g_bExit = 
void cmdThread(EasyTcpClient* client)	//通过指针传递参数
{
	while (true) {
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			client->Close();
			printf("退出cmdThread线程\n");
			break;
		}
		else if (0 == strcmp(cmdBuf, "login")) {
			Login login;
			strcpy(login.userName, "LF");
			strcpy(login.passWord, "LF123");
			client->SendData(&login);
		}
		else if (0 == strcmp(cmdBuf, "logout")) {
			Logout logout;
			strcpy(logout.userName, "LF");
			client->SendData(&logout);
		}
		else {
			printf("不支持的命令！\n");
		}
	}
}

int main()
{
	EasyTcpClient client;	//创建一个对象
	client.InitSocket();	//初始化
	client.Connect("127.0.0.1", 4567);	//连接服务器  

	EasyTcpClient client2;	//创建一个对象
	client2.InitSocket();	//初始化
	client2.Connect("192.168.61.128", 4568);	//连接服务器  

	//启动UI线程
	std::thread t1(cmdThread, &client);

	std::thread t2(cmdThread, &client2);
	 
	t1.detach();	//与主线程分离
	 
	while ( client.isRun( ) || client2.isRun())
	{
		client.OnRun( );

		client2.OnRun();
		 
		//printf("空闲时间，处理其他业务...\n");
		//Sleep(1000);
		//send();
	}
	//7.关闭套接字
	client.Close();
	client2.Close();
	 
	//WSACleanup();
	printf("任务结束，退出!\n");
	getchar();
	return 0;
}