#ifndef _EasyTcpClient_hpp
#define _EasyTcpClient_hpp

	#ifdef _WIN32
		#define WIN32_LEAN_AND_MEAN
		#define _WINSOCK_DEPRECATED_NO_WARNINGS
		#include <windows.h>
		#include <WinSock2.h>
	#else
		#include <unistd.h>		//uni std
		#include <arpa/inet.h>
		#include <string.h> 
		#define SOCKET int 
		#define INVALID_SOCKET (SOCKET) (~0)
		#define SOCKET_ERROR (-1)
	#endif
#include <stdio.h>
#pragma comment(lib,"ws2_32.lib")	//连接库
#include "MessageHead.hpp"
 
class EasyTcpClient
{
	SOCKET	_sock;
public:
	
	EasyTcpClient()
	{
		_sock = INVALID_SOCKET;
	
	}
	//虚析构函数
	virtual ~EasyTcpClient()
	{

		Close();
	}
	//初始化socket
	void InitSocket()
	{
		//启动win sock 2.x环境
	#ifdef _WIN32
			WORD vef = MAKEWORD(2, 2);
			WSADATA dat;
			WSAStartup(vef, &dat);	//启动Windows网络环境
	#endif
			if (INVALID_SOCKET != _sock)
			{
				printf("<_sock=%d>关闭旧连接...\n", _sock);
				Close();
			}
			_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (INVALID_SOCKET == _sock) {
				printf("ERROR,建立socket失败!\n");
				Close();
			}
			else {
				printf("建立socket成功!\n");
			}
	}
	//连接服务器
	int  Connect(const char* ip, u_short prot)
	{
		if (INVALID_SOCKET == _sock)
		{
			InitSocket();
		}
		//2.connect 连接服务器
		struct sockaddr_in _csin;
		_csin.sin_family = AF_INET;
		_csin.sin_port = htons(prot);	//host to net unsigned short
	#ifdef _WIN32
		_csin.sin_addr.S_un.S_addr = inet_addr(ip);	 //inet_addr("127.0.0.1")
	#else
		_csin.sin_addr.s_addr = inet_addr(ip);  //虚拟机IP(192.168.17.1)
	#endif
		int ret = connect(_sock, (sockaddr*)&_csin, sizeof(_csin));
		if (INVALID_SOCKET == ret) {
			printf("<_sock = % d> ERROR,连接服务器<%s : %d> 失败!\n", _sock, ip, prot);
			Close();
			 
		}
		else {
			printf("<_sock = % d>连接到服务器<%s : %d>!\n", _sock, ip, prot);
		}
		return ret;
	}
	//关闭socket
	void Close()
	{		
		if (_sock != INVALID_SOCKET) 
		{
			_sock = INVALID_SOCKET;
	#ifdef _WIN32
			closesocket(_sock);
			WSACleanup(); //关闭win sock 2.x环境
	#else
			close(_sock);
	#endif		
			
		}
	}

	//查询是否有未处理数据
	bool OnRun()
	{
		if ( isRun( ) )
		{
			//描述符集合
			fd_set fdReads;
			//清零集合
			FD_ZERO(&fdReads);
			//加入集合
			FD_SET(_sock, &fdReads);
			timeval t = { 1,0 };
			int ret = select(_sock + 1, &fdReads, NULL, NULL, &t);
			if (ret < 0)
			{
				printf("<_sock = % d> select任务1结束！\n", _sock);
				Close();
				return false;
			}
			//判断描述符是否在集合中
			if (FD_ISSET(_sock, &fdReads))
			{
				FD_CLR(_sock, &fdReads);

				if (-1 == RecvData(_sock))
				{
					printf("<_sock = % d> select任务2结束！\n", _sock);
					return false;
				}
			}
			return true;
		}
		return false;
	}
	//是否工作中
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	//接收数据 处理粘包 拆包
	int RecvData(SOCKET _cSock)
	{
		//缓冲区		
		char szRecv[1024] = {};
		//将收到的数据放到缓冲区 
		int nlen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
		DataHeader* header = (DataHeader*)szRecv;
		if (nlen <= 0) {
			printf("<_sock = % d>与服务器断开连接，任务结束!\n", _sock );
			return -1;
		}
		//判断消息头中的命令
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		
		OnNetMsg(header);
		return 0;
	}

	//响应网络消息
	void OnNetMsg( DataHeader* _header )
	{
		switch (_header->cmd) {
		case CMD_LOGIN_RESULT: {
			//recv(_cSock, szRecv + sizeof(DataHeader), _header->dataLength - sizeof(DataHeader), 0);
			LoginResult* loginResult = (LoginResult*)_header;
			printf("<_sock = % d>收到服务器返回数据：命令：%d CMD_LOGIN_RESULT, 数据长度：%d, 返回结果：%d.\n",
				_sock,(char)loginResult->cmd, loginResult->dataLength, loginResult->result);
		}break;
		case CMD_LOGOUT_RESULT: {
			//recv(_cSock, szRecv + sizeof(DataHeader), _header->dataLength - sizeof(DataHeader), 0);
			//忽略判断用户密码是否正确的过程
			LogoutResult* logoutResult = (LogoutResult*)_header;
			printf("<_sock = % d>收到服务器返回数据：命令：%d CMD_LOGOUT_RESULT, 数据长度：%d, 返回结果：%d.\n",
				_sock, logoutResult->cmd, logoutResult->dataLength, logoutResult->result);

		}break;
		case CMD_NEW_USER_JOIN: {
			//recv(_cSock, szRecv + sizeof(DataHeader), _header->dataLength - sizeof(DataHeader), 0);
			//忽略判断用户密码是否正确的过程
			NewUser* newUser = (NewUser*)_header;
			printf("<_sock = % d>收到服务器返回数据：命令：%d CMD_NEW_USER_JOIN, 数据长度：%d, 返回结果：%d.\n",
				_sock, newUser->cmd, newUser->dataLength, newUser->sock);

		}break;
		}
	}

	//发送数据
	int SendData(DataHeader* _header)
	{
		if (isRun() && _header)
		{
			send(_sock, (const char*)_header, _header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
 

private:

};
  
#endif // !EasyTcpClient_hpp
