#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#include<WinSock2.h>
#pragma comment(lib, "WS2_32")
#include <iostream>
#include<vector>
using namespace std;

void SinglClient()
{
	WSADATA wsadata;
	SOCKET s;
	sockaddr_in serveraddr;

	WSAStartup(MAKEWORD(2, 2), &wsadata);
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(12345);
	serveraddr.sin_addr.S_un.S_addr = inet_addr("192.168.1.102");

	int ret = connect(s, (sockaddr*)&serveraddr, sizeof(serveraddr));
	connect(s, (sockaddr*)&serveraddr, sizeof(serveraddr));
	//发送数据
	while (1)
	{
		char data[1024] = { 0 };
		cout << "请输入要发送的数据(quit退出): ";
		cin >> data;
		string senddata = data;

		if (senddata.size() > 0)
			send(s, senddata.c_str(), sizeof(senddata), 0);
		if (senddata == "quit")
			break;

	}

	closesocket(s);

	WSACleanup();

}

int main()
{
	SinglClient();
	return 0;
}





