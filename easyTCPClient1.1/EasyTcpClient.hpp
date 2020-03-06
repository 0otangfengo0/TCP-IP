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
#pragma comment(lib,"ws2_32.lib")	//���ӿ�
#include "MessageHead.hpp"
 
class EasyTcpClient
{
	SOCKET	_sock;
public:
	
	EasyTcpClient()
	{
		_sock = INVALID_SOCKET;
	
	}
	//����������
	virtual ~EasyTcpClient()
	{

		Close();
	}
	//��ʼ��socket
	void InitSocket()
	{
		//����win sock 2.x����
	#ifdef _WIN32
			WORD vef = MAKEWORD(2, 2);
			WSADATA dat;
			WSAStartup(vef, &dat);	//����Windows���绷��
	#endif
			if (INVALID_SOCKET != _sock)
			{
				printf("<_sock=%d>�رվ�����...\n", _sock);
				Close();
			}
			_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (INVALID_SOCKET == _sock) {
				printf("ERROR,����socketʧ��!\n");
				Close();
			}
			else {
				printf("����socket�ɹ�!\n");
			}
	}
	//���ӷ�����
	int  Connect(const char* ip, u_short prot)
	{
		if (INVALID_SOCKET == _sock)
		{
			InitSocket();
		}
		//2.connect ���ӷ�����
		struct sockaddr_in _csin;
		_csin.sin_family = AF_INET;
		_csin.sin_port = htons(prot);	//host to net unsigned short
	#ifdef _WIN32
		_csin.sin_addr.S_un.S_addr = inet_addr(ip);	 //inet_addr("127.0.0.1")
	#else
		_csin.sin_addr.s_addr = inet_addr(ip);  //�����IP(192.168.17.1)
	#endif
		int ret = connect(_sock, (sockaddr*)&_csin, sizeof(_csin));
		if (INVALID_SOCKET == ret) {
			printf("<_sock = % d> ERROR,���ӷ�����<%s : %d> ʧ��!\n", _sock, ip, prot);
			Close();
			 
		}
		else {
			printf("<_sock = % d>���ӵ�������<%s : %d>!\n", _sock, ip, prot);
		}
		return ret;
	}
	//�ر�socket
	void Close()
	{		
		if (_sock != INVALID_SOCKET) 
		{
			_sock = INVALID_SOCKET;
	#ifdef _WIN32
			closesocket(_sock);
			WSACleanup(); //�ر�win sock 2.x����
	#else
			close(_sock);
	#endif		
			
		}
	}

	//��ѯ�Ƿ���δ��������
	bool OnRun()
	{
		if ( isRun( ) )
		{
			//����������
			fd_set fdReads;
			//���㼯��
			FD_ZERO(&fdReads);
			//���뼯��
			FD_SET(_sock, &fdReads);
			timeval t = { 1,0 };
			int ret = select(_sock + 1, &fdReads, NULL, NULL, &t);
			if (ret < 0)
			{
				printf("<_sock = % d> select����1������\n", _sock);
				Close();
				return false;
			}
			//�ж��������Ƿ��ڼ�����
			if (FD_ISSET(_sock, &fdReads))
			{
				FD_CLR(_sock, &fdReads);

				if (-1 == RecvData(_sock))
				{
					printf("<_sock = % d> select����2������\n", _sock);
					return false;
				}
			}
			return true;
		}
		return false;
	}
	//�Ƿ�����
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	//�������� ����ճ�� ���
	int RecvData(SOCKET _cSock)
	{
		//������		
		char szRecv[1024] = {};
		//���յ������ݷŵ������� 
		int nlen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
		DataHeader* header = (DataHeader*)szRecv;
		if (nlen <= 0) {
			printf("<_sock = % d>��������Ͽ����ӣ��������!\n", _sock );
			return -1;
		}
		//�ж���Ϣͷ�е�����
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		
		OnNetMsg(header);
		return 0;
	}

	//��Ӧ������Ϣ
	void OnNetMsg( DataHeader* _header )
	{
		switch (_header->cmd) {
		case CMD_LOGIN_RESULT: {
			//recv(_cSock, szRecv + sizeof(DataHeader), _header->dataLength - sizeof(DataHeader), 0);
			LoginResult* loginResult = (LoginResult*)_header;
			printf("<_sock = % d>�յ��������������ݣ����%d CMD_LOGIN_RESULT, ���ݳ��ȣ�%d, ���ؽ����%d.\n",
				_sock,(char)loginResult->cmd, loginResult->dataLength, loginResult->result);
		}break;
		case CMD_LOGOUT_RESULT: {
			//recv(_cSock, szRecv + sizeof(DataHeader), _header->dataLength - sizeof(DataHeader), 0);
			//�����ж��û������Ƿ���ȷ�Ĺ���
			LogoutResult* logoutResult = (LogoutResult*)_header;
			printf("<_sock = % d>�յ��������������ݣ����%d CMD_LOGOUT_RESULT, ���ݳ��ȣ�%d, ���ؽ����%d.\n",
				_sock, logoutResult->cmd, logoutResult->dataLength, logoutResult->result);

		}break;
		case CMD_NEW_USER_JOIN: {
			//recv(_cSock, szRecv + sizeof(DataHeader), _header->dataLength - sizeof(DataHeader), 0);
			//�����ж��û������Ƿ���ȷ�Ĺ���
			NewUser* newUser = (NewUser*)_header;
			printf("<_sock = % d>�յ��������������ݣ����%d CMD_NEW_USER_JOIN, ���ݳ��ȣ�%d, ���ؽ����%d.\n",
				_sock, newUser->cmd, newUser->dataLength, newUser->sock);

		}break;
		}
	}

	//��������
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
