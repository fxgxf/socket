#include"pch.h"
#include"IOmode.h"

CRITICAL_SECTION IOmode::cs;
static HANDLE hMutex = INVALID_HANDLE_VALUE;

IOmode::IOmode(int port, string ipaddr)
{
	//WSAStartup
	WSADATA wsaData;
	int nResult=WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nResult != 0)
	{
		cout << "WSAStartup failed..." << endl;
		exit(0);
	}
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	serveraddr.sin_addr.s_addr = inet_addr(ipaddr.c_str());

}

IOmode::~IOmode()
{
	WSACleanup();

	DeleteCriticalSection(&cs);
}

void IOmode::blockmode()
{
	SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in clientaddr;
	unsigned long ul = 1;//0:阻塞，否则:非阻塞
	//控制socket I/O阻塞模式
	int nret = ioctlsocket(server, FIONBIO, (unsigned long*)&ul);
	//linux中对应设置为非阻塞
	//int flags = fcntl(server, F_GETFL, 0);        //获取文件的flags值。
	//fcntl(server, F_SETFL, flags | O_NONBLOCK);   //设置成非阻塞模式；

	bind(server, (sockaddr*)&serveraddr, sizeof(serveraddr));

	char buffer[1024];

	recvfrom(server, buffer, 1023, 0, (sockaddr*)&clientaddr, 0);

	listen(server, 5);
	int len = sizeof(sockaddr);
	SOCKET client = accept(server, (sockaddr*)&clientaddr, &len);

}

void IOmode::selectmode()
{
	vector<SOCKET> clientset;//客户机套接字集合
	map<SOCKET, sockaddr_in> s2addr;//套接字，地址
	fd_set readfd;//用于检查可读取数据的套接字集合
	int ret = 0;

	SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	unsigned long ul = 1;
	ioctlsocket(server, FIONBIO, (unsigned long*)&ul);//设置server套接字为非阻塞，用于accept

	bind(server, (sockaddr*)&serveraddr, sizeof(serveraddr));
	listen(server, 5);
	//不断接收连接与使用select检查各个客户机套接字是否有数据到来
	while (1)
	{
		sockaddr_in addr;
		int len = sizeof(addr);
		int client = accept(server, (sockaddr*)&addr, &len);//接收连接
		if (client != INVALID_SOCKET)
		{
			clientset.push_back(client);
			s2addr[client] = addr;
			std::cout << inet_ntoa(addr.sin_addr) << " 已加入连接.." << "当前连接数: " << clientset.size() << endl;
		}
		FD_ZERO(&readfd);
		for (int i = 0; i < clientset.size(); i++)
			FD_SET((int)(clientset[i]), &readfd);

		//查看各个客户机是否发送数据过来。
		ret = 0;
		if (!clientset.empty())
		{
			timeval tv = { 0,0 };
			ret = select(clientset[clientset.size() - 1] + 1, &readfd, NULL, NULL, &tv);
		}
		//处理接收数据
		if (ret > 0)
		{
			vector<SOCKET> deleteclient;//要退出的套接字，待删除的套接字vector
			for (int i = 0; i < clientset.size(); i++)
				if (FD_ISSET((int)(clientset[i]), &readfd))
				{
					char data[1024] = { 0 };
					recv(clientset[i], data, 1023, 0);
					string recvdata = data;
					if (recvdata == "quit")	//这里规定一下，如果对方发送quit，我们就认为他要断开连接，我们就把相应的套接字关闭掉
						deleteclient.push_back(clientset[i]);
					else
						std::cout << "来自" << inet_ntoa(s2addr[clientset[i]].sin_addr) << " : " << data << endl;
				}

			//关闭要退出的套接字，在套接字集合中删除它
			if (!deleteclient.empty())
			{
				for (int i = 0; i < deleteclient.size(); i++)
				{
					std::cout << "客户机" << inet_ntoa(s2addr[deleteclient[i]].sin_addr) << "已退出连接，剩余连接数: " << clientset.size() - 1 << endl;
					closesocket(deleteclient[i]);
					vector<SOCKET>::iterator it = find(clientset.begin(), clientset.end(), deleteclient[i]);
					clientset.erase(it);

				}
			}
		}
	}


}

int IOmode::WSeventSelectmode()
{
	// TODO: 在此处添加实现代码.
	WSAEVENT eventarray[WSA_MAXIMUM_WAIT_EVENTS], newevent;//事件数组
	SOCKET socketarray[WSA_MAXIMUM_WAIT_EVENTS];//客户机套接字
	map<SOCKET, sockaddr_in> s2addr;//套接字，地址
	DWORD eventtotal = 0, index;//事件总数，这个事件总数也就是套接字与事件数组的长度，index返回通知事件的索引
	char buffer[1024] = { 0 };//接受数据字符数组

	SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	newevent = WSACreateEvent();//创建一个事件

	bind(server, (sockaddr*)&serveraddr, sizeof(serveraddr));

	listen(server, 5);
	//为服务器套接字添加接收连接的网络事件，将服务器套接字与参数二事件关联，
	//一旦服务器套接字上发生了网络事件，参数二的事件会通知我们.
	WSAEventSelect(server, newevent, FD_ACCEPT);

	//在相应的套接字数组与事件数组中添加服务器套接字
	socketarray[eventtotal] = server;
	eventarray[eventtotal] = newevent;
	eventtotal++;

	while (1)
	{
		//需要等待事件的通知，检查是否有网络事件发生，返回的index就是发生时间的套接字的下标
		index = WSAWaitForMultipleEvents(eventtotal, eventarray, FALSE, WSA_INFINITE, FALSE);
		index = index - WSA_WAIT_EVENT_0;//规范一下套接字下标。

		//创建网络事件的结构体，用于检查发生了哪些网络事件
		_WSANETWORKEVENTS networkevents;
		//检查参数1套接字对应参数2事件对应的网络事件，在参数三中保存
		WSAEnumNetworkEvents(socketarray[index], eventarray[index], &networkevents);
		//accept
		if (networkevents.lNetworkEvents & FD_ACCEPT)
		{
			//使用错误码判断是否有错误发生错误
			if (networkevents.iErrorCode[FD_ACCEPT_BIT] != 0)
			{
				std::cout << "FD_ACCEPT failed! code:" << networkevents.iErrorCode[FD_ACCEPT_BIT] << endl;
				break;
			}
			//接受一个来自客户机的连接，并将它添加到套接字以及事件数组中
			sockaddr_in clientaddr;
			int len = sizeof(clientaddr);
			SOCKET client = accept(socketarray[index], (sockaddr*)&clientaddr, &len);
			//判断是否超出最大事件数
			if (eventtotal >= WSA_MAXIMUM_WAIT_EVENTS)
			{
				std::cout << "连接超出最大事件数64" << endl;
				closesocket(client);
				break;
			}
			//记录套接字地址
			s2addr[client] = clientaddr;
			//在这里继续用WS事件选择将套接字与新事件关联起来，这里检查的网络事件有读数据与关闭连接
			newevent = WSACreateEvent();
			WSAEventSelect(client, newevent, FD_READ | FD_CLOSE);
			//在相应的套接字与事件数组中添加
			eventarray[eventtotal] = newevent;
			socketarray[eventtotal] = client;
			eventtotal++;
			//打印一下连接信息
			std::cout << inet_ntoa(clientaddr.sin_addr) << client << " connected..." << endl;
			WSAResetEvent(eventarray[index]);
		}
		//read
		if (networkevents.lNetworkEvents & FD_READ)
		{
			//判断错误
			if (networkevents.iErrorCode[FD_READ_BIT] != 0)
			{
				std::cout << "FD_ACCEPT failed! code:" << networkevents.iErrorCode[FD_READ_BIT] << endl;
				break;
			}
			//读数据
			recv(socketarray[index - WSA_WAIT_EVENT_0], buffer, 1023, 0);
			//判断是否客户机关闭了连接
			string recvdata = buffer;
			if (recvdata == "quit")
			{
				std::cout << inet_ntoa(s2addr[socketarray[index - WSA_WAIT_EVENT_0]].sin_addr) <<
					socketarray[index - WSA_WAIT_EVENT_0] << " disconnected.." << endl;
				//在这里关闭客户机的套接字，注意在这里关闭套接字操作将会触发FD_CLOSE网络事件
				closesocket(socketarray[index - WSA_WAIT_EVENT_0]);
			}
			else
				std::cout << inet_ntoa(s2addr[socketarray[index - WSA_WAIT_EVENT_0]].sin_addr) <<
				socketarray[index - WSA_WAIT_EVENT_0] << " :" << recvdata << endl;
			WSAResetEvent(eventarray[index]);
		}
		//close
		if (networkevents.lNetworkEvents & FD_CLOSE)
		{
			//错误
			if (networkevents.iErrorCode[FD_CLOSE_BIT] != 0)
			{
				std::cout << "FD_ACCEPT failed! code:" << networkevents.iErrorCode[FD_CLOSE_BIT] << endl;
				break;
			}
			//从socketarray中删除quit的套接字
			for (int j = index; j < eventtotal; j++)
			{
				socketarray[j] = socketarray[j + 1];
				eventarray[j] = eventarray[j + 1];
			}
			eventtotal--;
			WSAResetEvent(eventarray[index]);
		}

	}
	return 0;
}

int IOmode::overlappedmode() 
{
	WSABUF wsbuf = { 0 };
	char buffer[1024] = { 0 };
	//listen
	SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	bind(server, (sockaddr*)&serveraddr, sizeof(serveraddr));

	listen(server, 5);

	//accpet
	sockaddr_in clientaddr;
	int len = sizeof(clientaddr);
	SOCKET client = accept(server, (sockaddr*)&clientaddr, &len);

	//创建重叠结构体，WS事件数组
	WSAOVERLAPPED overlapped;
	ZeroMemory(&overlapped, sizeof(overlapped));
	WSAEVENT eventarray[WSA_MAXIMUM_WAIT_EVENTS];
	DWORD eventtotal = 0,recvbytes=0,flags=0;
	eventarray[eventtotal] = WSACreateEvent();
	overlapped.hEvent = eventarray[eventtotal];
	eventtotal++;
	while (1)
	{//使用重叠IO，WSArecv
		wsbuf.len = 1024;
		wsbuf.buf = buffer;

		WSARecv(client, &wsbuf, 1, &recvbytes, &flags, &overlapped, NULL);

		//等待重叠IO完成，WSAWaitForMultipleEvents，若使用完成例程，最后一个参数为TRUE
		int index = WSAWaitForMultipleEvents(eventtotal, eventarray, FALSE, WSA_INFINITE, FALSE);
		//重设事件对象，
		WSAResetEvent(eventarray[index - WSA_WAIT_EVENT_0]);
		//WSAgetoverlappedresult,判断是否成功完成，重复4-7
		WSAGetOverlappedResult(client, &overlapped, &recvbytes, TRUE, &flags);
		if (recvbytes == 0)
		{
			closesocket(client);
			WSACloseEvent(eventarray[index - WSA_WAIT_EVENT_0]);
			return 0;
		}
		cout << wsbuf.buf << endl;
		ZeroMemory(&overlapped, sizeof(overlapped));
		overlapped.hEvent = eventarray[index - WSA_WAIT_EVENT_0];
		flags = 0;

	}
	return 0;
}

void IOmode::completionportmode()
{
	// TODO: 在此处添加实现代码.
	int nResult = 0;

	HANDLE CompletionPort;
	SYSTEM_INFO systeminfo;
	OVERLAPPED overlapped;

	WSABUF wsbuf;
	char buffer[1024] = { 0 };
	wsbuf.buf = buffer;
	wsbuf.len = 1024;

	DWORD BytesTransferred = 0, flags = 0;

	sockaddr_in clientaddr;
	int len = sizeof(clientaddr);

	SOCKET server, client;

	server = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	nResult = bind(server, (sockaddr*)&serveraddr, sizeof(serveraddr));
	if (nResult != 0)
	{
		cout << "bind failed..." << endl;
		exit(0);
	}

	nResult = listen(server, SOMAXCONN);
	if (nResult != 0)
	{
		cout << "listen failed..." << endl;
		exit(0);
	}
	
	GetSystemInfo(&systeminfo);
	//完成端口的创建
	CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	InitializeCriticalSection(&cs);
	//创建2*CPU核心数量个线程用于处理网络IO
	for (int i = 0; i < 2*systeminfo.dwNumberOfProcessors; i++)
	{
		HANDLE WorkerThreadHandle;
		WorkerThreadHandle = CreateThread(NULL, 0, &this->WorkerThread, CompletionPort, 0, NULL);
		CloseHandle(WorkerThreadHandle);
	}

	while (1)
	{
		//接收连接并且绑定到完成端口
		//创建PER_HANDLE_DATA用于关联
		PER_HANDLE_DATA *PerHandleData = (PER_HANDLE_DATA*)GlobalAlloc(GPTR,sizeof(PER_HANDLE_DATA));
		PER_IO_DATA *PerIoData= (PER_IO_DATA*)GlobalAlloc(GPTR,sizeof(PER_IO_DATA));
		ZeroMemory(PerHandleData, sizeof(PER_HANDLE_DATA));
		ZeroMemory(PerIoData, sizeof(PER_IO_DATA));

		client = accept(server, (sockaddr*)&clientaddr, &len);
		EnterCriticalSection(&cs);
		cout << client << " connected..." << endl;
		LeaveCriticalSection(&cs);
		PerHandleData->clientaddr = clientaddr;
		PerHandleData->socket = client;
		//关联客户机套接字和完成端口
		CreateIoCompletionPort((HANDLE)client, CompletionPort, (DWORD)PerHandleData, 0);

		PerIoData->client = client;
		PerIoData->wsbuf.len = 1024;
		PerIoData->wsbuf.buf = PerIoData->buffer;
		PerIoData->OperationType = RECV_POSTED;
		//投递一个WSARecv
		int nBytesRecv=WSARecv(PerHandleData->socket, &(PerIoData->wsbuf), 1, &BytesTransferred, &flags,
			&(PerIoData->Overlapped), NULL);

		if ((SOCKET_ERROR == nBytesRecv) &&
			(WSA_IO_PENDING != WSAGetLastError()))
		{
			EnterCriticalSection(&cs);
			cout << "投递第一个WSARecv失败！" << endl;
			exit(0);
			LeaveCriticalSection(&cs);
		}

	}

}

DWORD IOmode::WorkerThread(LPVOID CompletionPortID)
{
	// TODO: 在此处添加实现代码.
	int nResult = 0;
	DWORD BytesTransferred = 0, flags = 0;
	PER_HANDLE_DATA *PerHandleData=NULL;
	PER_IO_DATA *PerIoData=NULL;
	while (1)
	{
		//等待与完成端口关联的任意套接字上的I/O完成
		nResult=GetQueuedCompletionStatus(CompletionPortID, &BytesTransferred, (PULONG_PTR)&PerHandleData,
			(OVERLAPPED**)&PerIoData, INFINITE);
		//检查看是否有错误或者断开了连接
		if (BytesTransferred == 0 && PerIoData->OperationType == RECV_POSTED)
		{
			EnterCriticalSection(&cs);
			cout << PerIoData->client << " disconnected..." << endl;
			LeaveCriticalSection(&cs);
			closesocket(PerIoData->client);
			GlobalFree(PerIoData);
			PerIoData = NULL;
			continue;
		}
		//打印一下
		if (PerIoData->OperationType == RECV_POSTED)
		{
			EnterCriticalSection(&cs);
			cout <<PerIoData->client<<" : "<< PerIoData->wsbuf.buf << endl;
			LeaveCriticalSection(&cs);
		}

		WSARecv(PerIoData->client, &(PerIoData->wsbuf), 1, &BytesTransferred, &flags,
			&(PerIoData->Overlapped), NULL);
	}
	return 0;
}
