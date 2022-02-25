#include<sys/socket.h>
#include<arpa/inet.h>
#include<iostream>
using namespace std;

int main()
{

	int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in serveraddr;

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(12345);
	serveraddr.sin_addr.s_addr = inet_addr("192.168.1.103");

	bind(server, (sockaddr*)&serveraddr, sizeof(serveraddr));

	listen(server, 5);
	cout<<"listen..."<<endl;

	sockaddr_in clientaddr;
	socklen_t len = sizeof(clientaddr);
	int client = accept(server, (sockaddr*)&clientaddr, &len);

	char recvdata[1024] = { 0 };
	recv(client, recvdata, 1023, 0);

	cout<<recvdata<<endl;

	return 0;
}