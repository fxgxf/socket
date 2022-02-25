  void server_code()
  {
	  WSADATA wsaData;
	  WSAStartup(MAKEWORD(2, 2), &wsaData);

	  int server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	  sockaddr_in serveraddr,clientaddr;

	  serveraddr.sin_family = AF_INET;
	  serveraddr.sin_port = htons(12345);
	  serveraddr.sin_addr.s_addr = inet_addr("192.168.1.102");

	  bind(server, (sockaddr*)&serveraddr, sizeof(serveraddr));

	  char recvdata[1024] = { 0 };

	  while (1)
	  {
		  memset(recvdata, 0, 1024);
		  int len;
		  if (recvfrom(server, recvdata, 1023, 0, (sockaddr*)&clientaddr, &len))
			  cout << recvdata << endl;
	  }
	  WSACleanup();
  }

  void client_code()
  {
	  WSADATA wsaData;
	  WSAStartup(MAKEWORD(2, 2), &wsaData);

	  SOCKET client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	  sockaddr_in serveraddr;

	  serveraddr.sin_family = AF_INET;
	  serveraddr.sin_port = htons(12345);
	  serveraddr.sin_addr.s_addr = inet_addr("192.168.1.103");
	  

	  sendto(client, "shit..", sizeof("nihaoaahaha"), 0, (sockaddr*)&serveraddr, sizeof(serveraddr));


	  WSACleanup();
  }
