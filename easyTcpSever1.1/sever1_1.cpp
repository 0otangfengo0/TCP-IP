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
#include <thread>
#include <vector>
#pragma comment(lib,"ws2_32.lib")	//连接库
enum CMD	//枚举类型
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR
};

// DataPackage
struct DataHeader
{
	short dataLength;	//数据长度
	short cmd;			//数据命令
};

struct Login :public DataHeader
{
	Login() {
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char userName[32];
	char passWord[32];
};

struct LoginResult :public DataHeader
{
	LoginResult() {
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 0;
	}

	int result;

};
struct Logout :public DataHeader
{
	Logout() {
		dataLength = sizeof(Logout);
		cmd = CMD_LOGOUT;
	}
	char userName[32];
};
struct LogoutResult :public DataHeader
{
	LogoutResult() {
		dataLength = sizeof(LogoutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 0;
	}
	int result;

};
struct NewUser :public DataHeader
{
	NewUser() {
		dataLength = sizeof(LogoutResult);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	int sock;

};
std::vector<SOCKET> g_clients;

//5.接收客户端的请求数据
int processor(SOCKET csock) {
	//缓冲区		
	char szRecv[1024] = {};
	//将收到的数据放到缓冲区 
	int nlen = recv(csock, szRecv, sizeof(DataHeader), 0);
	DataHeader* header = (DataHeader*)szRecv;
	if (nlen <= 0) {
		printf("客户端<Socket=%d>已退出，任务结束!\n", csock);
		return -1;
	}
	//判断消息头中的命令
	switch (header->cmd) {
	case CMD_LOGIN: {
		recv(csock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		//115行已经收过一次，此时应除去头的长度
		//忽略判断用户密码是否正确的过程
		Login* login = (Login*)szRecv;
		printf("收到客户端<Socket = %d>请求命令：CMD_LOGIN, 用户名：%s, 密码：%s,  数据长度：%d, 命令：%d.\n",
			csock, login->userName, login->passWord, login->dataLength, login->cmd);
		LoginResult loginResult;
		send(csock, (char*)&loginResult, sizeof(LoginResult), 0);

	}break;
	case CMD_LOGOUT: {

		recv(csock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		//忽略判断用户密码是否正确的过程
		Logout* logout = (Logout*)szRecv;
		printf("收到客户端<Socket = %d>请求命令：CMD_LOGOUT, 用户名：%s, 数据长度：%d,  命令：%d.\n",
			csock, logout->userName, logout->dataLength, logout->cmd);
		LogoutResult logoutResult;
		send(csock, (char*)&logoutResult, sizeof(LogoutResult), 0);

	}break;
	default: {
		DataHeader header = { 0,CMD_ERROR };
		send(csock, (char*)&header, sizeof(DataHeader), 0);
	}break;
	}
}


int main()
{
#ifdef _WIN32
	//启动windows socket 2.X的环境
	WORD vef = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(vef, &dat);	//启动Windows网络环境
#endif
	//Socket API建立简易服务器


	//1.建立SOCKET
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (SOCKET_ERROR == _sock) {
		printf("ERROR,创建socket失败!\n");
	}
	else {
		printf("创建socket成功!\n");
	}
	//2.bind绑定一个网络端口
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);	//host to net unsigned short
#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("127.0.0.1")
#else
	_sin.sin_addr.s_addr = INADDR_ANY;
#endif

	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin))) {
		printf("ERROR,绑定网络端口失败!\n");
	}
	else {
		printf("绑定网络端口成功!\n");
	}

	//3.listen 监听端口
	if (SOCKET_ERROR == listen(_sock, 5)) {
		printf("ERROR,监听网络失败!\n");
	}
	else {
		printf("正在监听端口!\n");
	}





	char cmdbuf[128] = {};
	while (true) {
		//伯克利 socket,
		//fd_set 存放描述符/集合
		fd_set fdRead;		//监视这些文件描述符的读变化，如果这个集合中有一个文件可读，select就会返回一个大于0的值
		fd_set fdWrite;
		fd_set fdExp;
		//清空集合
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);
		//将一个给定的文件描述符加入集合之中
		FD_SET(_sock, &fdRead);
		FD_SET(_sock, &fdWrite);
		FD_SET(_sock, &fdExp);
		SOCKET maxSock = _sock;
		for (int n = (int)g_clients.size() - 1; n >= 0; n--) {	//新的客户端socketfd加入到集合：是否有数据接收
			FD_SET(g_clients[n], &fdRead);
			if (maxSock < g_clients[n]) {
				maxSock = g_clients[n];
			}
		}
		//nfds 是一个整数值 是指fd_set集合中所有描述符（socket）的范围
		//既是所有文件描述符最大值+1，windows中这个参数可以写0
		timeval t = { 1,0 };

		int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t); //  NULL
		if (ret < 0) {
			printf("select任务结束!\n");
			break;
		}
		if (FD_ISSET(_sock, &fdRead)) {		//检查sock是否可读，即是否网络上有客户端接入
			FD_CLR(_sock, &fdRead);			//将一个给定的文件描述符从集合中删除
			//4.accept 等待客户端的连接
			SOCKET csock = INVALID_SOCKET;
			sockaddr_in clientAdder = {};
			int AdderLen = sizeof(clientAdder);
#ifdef _WIN32
			csock = accept(_sock, (sockaddr*)&clientAdder, &AdderLen);
#else
			csock = accept(_sock, (sockaddr*)&clientAdder, (socklen_t*)&AdderLen);		//对服务器自身的socket处理
#endif

			if (INVALID_SOCKET == csock) {
				printf("ERROR,接受无效客户端SOCKET!\n");
			}
			else {
				printf(" accept成功!\n");
				for (int n = (int)g_clients.size() - 1; n >= 0; n--) {	//新的客户端socketfd加入到集合：是否有数据接收
					NewUser userJoin;
					send(g_clients[n], (const char*)&userJoin, sizeof(NewUser), 0);
				}
				g_clients.push_back(csock);
				printf("新客户端加入：socket = %d, IP = %s \n", csock, inet_ntoa(clientAdder.sin_addr));
			}

		}
		for (int n = (int)g_clients.size() - 1; n >= 0; n--) //新的客户端socketfd加入到集合：是否有数据接收
		{
			if (FD_ISSET(g_clients[n], &fdRead))
			{
				if (-1 == processor(g_clients[n]))
				{//exit命令进入
					//auto---->std::vector<SOCKET>::iterator
					auto iter = g_clients.begin()+n;	//当前要移除的客户端
					if (iter != g_clients.end())
					{
						g_clients.erase(iter);
					}
				}

			}
		}
		//printf("空闲时间，处理其他业务..\n");

	}
	//清理连接套接字
	for (int n = (int)g_clients.size() - 1; n >= 0; n--) {
#ifdef _WIN32
		closesocket(g_clients[n]);
#else
		close(g_clients[n]);
#endif
	}

	//8.关闭套接字
#ifdef _WIN32
	closesocket(_sock);
	WSACleanup();
#else
	close(_sock);
#endif


	printf("退出，任务结束!\n");
	getchar();
	return 0;
}