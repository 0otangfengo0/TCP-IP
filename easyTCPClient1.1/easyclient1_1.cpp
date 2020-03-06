#include <thread>
#include "EasyTcpClient.hpp"
#include "MessageHead.hpp"

//5.���շ���������������

bool g_bRun = true;
//bool g_bExit = 
void cmdThread(EasyTcpClient* client)	//ͨ��ָ�봫�ݲ���
{
	while (true) {
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			client->Close();
			printf("�˳�cmdThread�߳�\n");
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
			printf("��֧�ֵ����\n");
		}
	}
}

int main()
{
	EasyTcpClient client;	//����һ������
	client.InitSocket();	//��ʼ��
	client.Connect("127.0.0.1", 4567);	//���ӷ�����  

	EasyTcpClient client2;	//����һ������
	client2.InitSocket();	//��ʼ��
	client2.Connect("192.168.61.128", 4568);	//���ӷ�����  

	//����UI�߳�
	std::thread t1(cmdThread, &client);

	std::thread t2(cmdThread, &client2);
	 
	t1.detach();	//�����̷߳���
	 
	while ( client.isRun( ) || client2.isRun())
	{
		client.OnRun( );

		client2.OnRun();
		 
		//printf("����ʱ�䣬��������ҵ��...\n");
		//Sleep(1000);
		//send();
	}
	//7.�ر��׽���
	client.Close();
	client2.Close();
	 
	//WSACleanup();
	printf("����������˳�!\n");
	getchar();
	return 0;
}