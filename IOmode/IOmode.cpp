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
	unsigned long ul = 1;//0:����������:������
	//����socket I/O����ģʽ
	int nret = ioctlsocket(server, FIONBIO, (unsigned long*)&ul);
	//linux�ж�Ӧ����Ϊ������
	//int flags = fcntl(server, F_GETFL, 0);        //��ȡ�ļ���flagsֵ��
	//fcntl(server, F_SETFL, flags | O_NONBLOCK);   //���óɷ�����ģʽ��

	bind(server, (sockaddr*)&serveraddr, sizeof(serveraddr));

	char buffer[1024];

	recvfrom(server, buffer, 1023, 0, (sockaddr*)&clientaddr, 0);

	listen(server, 5);
	int len = sizeof(sockaddr);
	SOCKET client = accept(server, (sockaddr*)&clientaddr, &len);

}

void IOmode::selectmode()
{
	vector<SOCKET> clientset;//�ͻ����׽��ּ���
	map<SOCKET, sockaddr_in> s2addr;//�׽��֣���ַ
	fd_set readfd;//���ڼ��ɶ�ȡ���ݵ��׽��ּ���
	int ret = 0;

	SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	unsigned long ul = 1;
	ioctlsocket(server, FIONBIO, (unsigned long*)&ul);//����server�׽���Ϊ������������accept

	bind(server, (sockaddr*)&serveraddr, sizeof(serveraddr));
	listen(server, 5);
	//���Ͻ���������ʹ��select�������ͻ����׽����Ƿ������ݵ���
	while (1)
	{
		sockaddr_in addr;
		int len = sizeof(addr);
		int client = accept(server, (sockaddr*)&addr, &len);//��������
		if (client != INVALID_SOCKET)
		{
			clientset.push_back(client);
			s2addr[client] = addr;
			std::cout << inet_ntoa(addr.sin_addr) << " �Ѽ�������.." << "��ǰ������: " << clientset.size() << endl;
		}
		FD_ZERO(&readfd);
		for (int i = 0; i < clientset.size(); i++)
			FD_SET((int)(clientset[i]), &readfd);

		//�鿴�����ͻ����Ƿ������ݹ�����
		ret = 0;
		if (!clientset.empty())
		{
			timeval tv = { 0,0 };
			ret = select(clientset[clientset.size() - 1] + 1, &readfd, NULL, NULL, &tv);
		}
		//�����������
		if (ret > 0)
		{
			vector<SOCKET> deleteclient;//Ҫ�˳����׽��֣���ɾ�����׽���vector
			for (int i = 0; i < clientset.size(); i++)
				if (FD_ISSET((int)(clientset[i]), &readfd))
				{
					char data[1024] = { 0 };
					recv(clientset[i], data, 1023, 0);
					string recvdata = data;
					if (recvdata == "quit")	//����涨һ�£�����Է�����quit�����Ǿ���Ϊ��Ҫ�Ͽ����ӣ����ǾͰ���Ӧ���׽��ֹرյ�
						deleteclient.push_back(clientset[i]);
					else
						std::cout << "����" << inet_ntoa(s2addr[clientset[i]].sin_addr) << " : " << data << endl;
				}

			//�ر�Ҫ�˳����׽��֣����׽��ּ�����ɾ����
			if (!deleteclient.empty())
			{
				for (int i = 0; i < deleteclient.size(); i++)
				{
					std::cout << "�ͻ���" << inet_ntoa(s2addr[deleteclient[i]].sin_addr) << "���˳����ӣ�ʣ��������: " << clientset.size() - 1 << endl;
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
	// TODO: �ڴ˴����ʵ�ִ���.
	WSAEVENT eventarray[WSA_MAXIMUM_WAIT_EVENTS], newevent;//�¼�����
	SOCKET socketarray[WSA_MAXIMUM_WAIT_EVENTS];//�ͻ����׽���
	map<SOCKET, sockaddr_in> s2addr;//�׽��֣���ַ
	DWORD eventtotal = 0, index;//�¼�����������¼�����Ҳ�����׽������¼�����ĳ��ȣ�index����֪ͨ�¼�������
	char buffer[1024] = { 0 };//���������ַ�����

	SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	newevent = WSACreateEvent();//����һ���¼�

	bind(server, (sockaddr*)&serveraddr, sizeof(serveraddr));

	listen(server, 5);
	//Ϊ�������׽�����ӽ������ӵ������¼������������׽�����������¼�������
	//һ���������׽����Ϸ����������¼������������¼���֪ͨ����.
	WSAEventSelect(server, newevent, FD_ACCEPT);

	//����Ӧ���׽����������¼���������ӷ������׽���
	socketarray[eventtotal] = server;
	eventarray[eventtotal] = newevent;
	eventtotal++;

	while (1)
	{
		//��Ҫ�ȴ��¼���֪ͨ������Ƿ��������¼����������ص�index���Ƿ���ʱ����׽��ֵ��±�
		index = WSAWaitForMultipleEvents(eventtotal, eventarray, FALSE, WSA_INFINITE, FALSE);
		index = index - WSA_WAIT_EVENT_0;//�淶һ���׽����±ꡣ

		//���������¼��Ľṹ�壬���ڼ�鷢������Щ�����¼�
		_WSANETWORKEVENTS networkevents;
		//������1�׽��ֶ�Ӧ����2�¼���Ӧ�������¼����ڲ������б���
		WSAEnumNetworkEvents(socketarray[index], eventarray[index], &networkevents);
		//accept
		if (networkevents.lNetworkEvents & FD_ACCEPT)
		{
			//ʹ�ô������ж��Ƿ��д���������
			if (networkevents.iErrorCode[FD_ACCEPT_BIT] != 0)
			{
				std::cout << "FD_ACCEPT failed! code:" << networkevents.iErrorCode[FD_ACCEPT_BIT] << endl;
				break;
			}
			//����һ�����Կͻ��������ӣ���������ӵ��׽����Լ��¼�������
			sockaddr_in clientaddr;
			int len = sizeof(clientaddr);
			SOCKET client = accept(socketarray[index], (sockaddr*)&clientaddr, &len);
			//�ж��Ƿ񳬳�����¼���
			if (eventtotal >= WSA_MAXIMUM_WAIT_EVENTS)
			{
				std::cout << "���ӳ�������¼���64" << endl;
				closesocket(client);
				break;
			}
			//��¼�׽��ֵ�ַ
			s2addr[client] = clientaddr;
			//�����������WS�¼�ѡ���׽��������¼�����������������������¼��ж�������ر�����
			newevent = WSACreateEvent();
			WSAEventSelect(client, newevent, FD_READ | FD_CLOSE);
			//����Ӧ���׽������¼����������
			eventarray[eventtotal] = newevent;
			socketarray[eventtotal] = client;
			eventtotal++;
			//��ӡһ��������Ϣ
			std::cout << inet_ntoa(clientaddr.sin_addr) << client << " connected..." << endl;
			WSAResetEvent(eventarray[index]);
		}
		//read
		if (networkevents.lNetworkEvents & FD_READ)
		{
			//�жϴ���
			if (networkevents.iErrorCode[FD_READ_BIT] != 0)
			{
				std::cout << "FD_ACCEPT failed! code:" << networkevents.iErrorCode[FD_READ_BIT] << endl;
				break;
			}
			//������
			recv(socketarray[index - WSA_WAIT_EVENT_0], buffer, 1023, 0);
			//�ж��Ƿ�ͻ����ر�������
			string recvdata = buffer;
			if (recvdata == "quit")
			{
				std::cout << inet_ntoa(s2addr[socketarray[index - WSA_WAIT_EVENT_0]].sin_addr) <<
					socketarray[index - WSA_WAIT_EVENT_0] << " disconnected.." << endl;
				//������رտͻ������׽��֣�ע��������ر��׽��ֲ������ᴥ��FD_CLOSE�����¼�
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
			//����
			if (networkevents.iErrorCode[FD_CLOSE_BIT] != 0)
			{
				std::cout << "FD_ACCEPT failed! code:" << networkevents.iErrorCode[FD_CLOSE_BIT] << endl;
				break;
			}
			//��socketarray��ɾ��quit���׽���
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

	//�����ص��ṹ�壬WS�¼�����
	WSAOVERLAPPED overlapped;
	ZeroMemory(&overlapped, sizeof(overlapped));
	WSAEVENT eventarray[WSA_MAXIMUM_WAIT_EVENTS];
	DWORD eventtotal = 0,recvbytes=0,flags=0;
	eventarray[eventtotal] = WSACreateEvent();
	overlapped.hEvent = eventarray[eventtotal];
	eventtotal++;
	while (1)
	{//ʹ���ص�IO��WSArecv
		wsbuf.len = 1024;
		wsbuf.buf = buffer;

		WSARecv(client, &wsbuf, 1, &recvbytes, &flags, &overlapped, NULL);

		//�ȴ��ص�IO��ɣ�WSAWaitForMultipleEvents����ʹ��������̣����һ������ΪTRUE
		int index = WSAWaitForMultipleEvents(eventtotal, eventarray, FALSE, WSA_INFINITE, FALSE);
		//�����¼�����
		WSAResetEvent(eventarray[index - WSA_WAIT_EVENT_0]);
		//WSAgetoverlappedresult,�ж��Ƿ�ɹ���ɣ��ظ�4-7
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
	// TODO: �ڴ˴����ʵ�ִ���.
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
	//��ɶ˿ڵĴ���
	CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	InitializeCriticalSection(&cs);
	//����2*CPU�����������߳����ڴ�������IO
	for (int i = 0; i < 2*systeminfo.dwNumberOfProcessors; i++)
	{
		HANDLE WorkerThreadHandle;
		WorkerThreadHandle = CreateThread(NULL, 0, &this->WorkerThread, CompletionPort, 0, NULL);
		CloseHandle(WorkerThreadHandle);
	}

	while (1)
	{
		//�������Ӳ��Ұ󶨵���ɶ˿�
		//����PER_HANDLE_DATA���ڹ���
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
		//�����ͻ����׽��ֺ���ɶ˿�
		CreateIoCompletionPort((HANDLE)client, CompletionPort, (DWORD)PerHandleData, 0);

		PerIoData->client = client;
		PerIoData->wsbuf.len = 1024;
		PerIoData->wsbuf.buf = PerIoData->buffer;
		PerIoData->OperationType = RECV_POSTED;
		//Ͷ��һ��WSARecv
		int nBytesRecv=WSARecv(PerHandleData->socket, &(PerIoData->wsbuf), 1, &BytesTransferred, &flags,
			&(PerIoData->Overlapped), NULL);

		if ((SOCKET_ERROR == nBytesRecv) &&
			(WSA_IO_PENDING != WSAGetLastError()))
		{
			EnterCriticalSection(&cs);
			cout << "Ͷ�ݵ�һ��WSARecvʧ�ܣ�" << endl;
			exit(0);
			LeaveCriticalSection(&cs);
		}

	}

}

DWORD IOmode::WorkerThread(LPVOID CompletionPortID)
{
	// TODO: �ڴ˴����ʵ�ִ���.
	int nResult = 0;
	DWORD BytesTransferred = 0, flags = 0;
	PER_HANDLE_DATA *PerHandleData=NULL;
	PER_IO_DATA *PerIoData=NULL;
	while (1)
	{
		//�ȴ�����ɶ˿ڹ����������׽����ϵ�I/O���
		nResult=GetQueuedCompletionStatus(CompletionPortID, &BytesTransferred, (PULONG_PTR)&PerHandleData,
			(OVERLAPPED**)&PerIoData, INFINITE);
		//��鿴�Ƿ��д�����߶Ͽ�������
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
		//��ӡһ��
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
