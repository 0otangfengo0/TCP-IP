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
#pragma comment(lib,"ws2_32.lib")	//���ӿ�
enum CMD	//ö������
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
	short dataLength;	//���ݳ���
	short cmd;			//��������
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

//5.���տͻ��˵���������
int processor(SOCKET csock) {
	//������		
	char szRecv[1024] = {};
	//���յ������ݷŵ������� 
	int nlen = recv(csock, szRecv, sizeof(DataHeader), 0);
	DataHeader* header = (DataHeader*)szRecv;
	if (nlen <= 0) {
		printf("�ͻ���<Socket=%d>���˳����������!\n", csock);
		return -1;
	}
	//�ж���Ϣͷ�е�����
	switch (header->cmd) {
	case CMD_LOGIN: {
		recv(csock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		//115���Ѿ��չ�һ�Σ���ʱӦ��ȥͷ�ĳ���
		//�����ж��û������Ƿ���ȷ�Ĺ���
		Login* login = (Login*)szRecv;
		printf("�յ��ͻ���<Socket = %d>�������CMD_LOGIN, �û�����%s, ���룺%s,  ���ݳ��ȣ�%d, ���%d.\n",
			csock, login->userName, login->passWord, login->dataLength, login->cmd);
		LoginResult loginResult;
		send(csock, (char*)&loginResult, sizeof(LoginResult), 0);

	}break;
	case CMD_LOGOUT: {

		recv(csock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		//�����ж��û������Ƿ���ȷ�Ĺ���
		Logout* logout = (Logout*)szRecv;
		printf("�յ��ͻ���<Socket = %d>�������CMD_LOGOUT, �û�����%s, ���ݳ��ȣ�%d,  ���%d.\n",
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
	//����windows socket 2.X�Ļ���
	WORD vef = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(vef, &dat);	//����Windows���绷��
#endif
	//Socket API�������׷�����


	//1.����SOCKET
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (SOCKET_ERROR == _sock) {
		printf("ERROR,����socketʧ��!\n");
	}
	else {
		printf("����socket�ɹ�!\n");
	}
	//2.bind��һ������˿�
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);	//host to net unsigned short
#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("127.0.0.1")
#else
	_sin.sin_addr.s_addr = INADDR_ANY;
#endif

	if (SOCKET_ERROR == bind(_sock, (sockaddr*)&_sin, sizeof(_sin))) {
		printf("ERROR,������˿�ʧ��!\n");
	}
	else {
		printf("������˿ڳɹ�!\n");
	}

	//3.listen �����˿�
	if (SOCKET_ERROR == listen(_sock, 5)) {
		printf("ERROR,��������ʧ��!\n");
	}
	else {
		printf("���ڼ����˿�!\n");
	}





	char cmdbuf[128] = {};
	while (true) {
		//������ socket,
		//fd_set ���������/����
		fd_set fdRead;		//������Щ�ļ��������Ķ��仯����������������һ���ļ��ɶ���select�ͻ᷵��һ������0��ֵ
		fd_set fdWrite;
		fd_set fdExp;
		//��ռ���
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);
		//��һ���������ļ����������뼯��֮��
		FD_SET(_sock, &fdRead);
		FD_SET(_sock, &fdWrite);
		FD_SET(_sock, &fdExp);
		SOCKET maxSock = _sock;
		for (int n = (int)g_clients.size() - 1; n >= 0; n--) {	//�µĿͻ���socketfd���뵽���ϣ��Ƿ������ݽ���
			FD_SET(g_clients[n], &fdRead);
			if (maxSock < g_clients[n]) {
				maxSock = g_clients[n];
			}
		}
		//nfds ��һ������ֵ ��ָfd_set������������������socket���ķ�Χ
		//���������ļ����������ֵ+1��windows�������������д0
		timeval t = { 1,0 };

		int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t); //  NULL
		if (ret < 0) {
			printf("select�������!\n");
			break;
		}
		if (FD_ISSET(_sock, &fdRead)) {		//���sock�Ƿ�ɶ������Ƿ��������пͻ��˽���
			FD_CLR(_sock, &fdRead);			//��һ���������ļ��������Ӽ�����ɾ��
			//4.accept �ȴ��ͻ��˵�����
			SOCKET csock = INVALID_SOCKET;
			sockaddr_in clientAdder = {};
			int AdderLen = sizeof(clientAdder);
#ifdef _WIN32
			csock = accept(_sock, (sockaddr*)&clientAdder, &AdderLen);
#else
			csock = accept(_sock, (sockaddr*)&clientAdder, (socklen_t*)&AdderLen);		//�Է����������socket����
#endif

			if (INVALID_SOCKET == csock) {
				printf("ERROR,������Ч�ͻ���SOCKET!\n");
			}
			else {
				printf(" accept�ɹ�!\n");
				for (int n = (int)g_clients.size() - 1; n >= 0; n--) {	//�µĿͻ���socketfd���뵽���ϣ��Ƿ������ݽ���
					NewUser userJoin;
					send(g_clients[n], (const char*)&userJoin, sizeof(NewUser), 0);
				}
				g_clients.push_back(csock);
				printf("�¿ͻ��˼��룺socket = %d, IP = %s \n", csock, inet_ntoa(clientAdder.sin_addr));
			}

		}
		for (int n = (int)g_clients.size() - 1; n >= 0; n--) //�µĿͻ���socketfd���뵽���ϣ��Ƿ������ݽ���
		{
			if (FD_ISSET(g_clients[n], &fdRead))
			{
				if (-1 == processor(g_clients[n]))
				{//exit�������
					//auto---->std::vector<SOCKET>::iterator
					auto iter = g_clients.begin()+n;	//��ǰҪ�Ƴ��Ŀͻ���
					if (iter != g_clients.end())
					{
						g_clients.erase(iter);
					}
				}

			}
		}
		//printf("����ʱ�䣬��������ҵ��..\n");

	}
	//���������׽���
	for (int n = (int)g_clients.size() - 1; n >= 0; n--) {
#ifdef _WIN32
		closesocket(g_clients[n]);
#else
		close(g_clients[n]);
#endif
	}

	//8.�ر��׽���
#ifdef _WIN32
	closesocket(_sock);
	WSACleanup();
#else
	close(_sock);
#endif


	printf("�˳����������!\n");
	getchar();
	return 0;
}