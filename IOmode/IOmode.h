#pragma once
#include"pch.h"

#define RECV_POSTED 10
//句柄关联信息
struct PER_HANDLE_DATA
{
	SOCKET socket;
	sockaddr_in clientaddr;
};
//重叠结构IO关联信息
struct PER_IO_DATA
{
	OVERLAPPED Overlapped;
	SOCKET client;
	WSABUF wsbuf;
	char buffer[1024];
	int OperationType;
};


class IOmode
{
private:
	sockaddr_in serveraddr;
	static CRITICAL_SECTION cs;

public:
	IOmode(int port, string ipaddr);
	~IOmode();
	void blockmode();
	void selectmode();
	int WSeventSelectmode();
	int overlappedmode();
	void completionportmode();

private:
	static DWORD WorkerThread(LPVOID);
};

