

  void test_getaddrinfo()
  {
	  WSADATA wsaData;
	  WSAStartup(MAKEWORD(2, 2), &wsaData);

	  addrinfo hints, *res;

	  memset(&hints, 0, sizeof(hints));

	  hints.ai_family = AF_INET;
	  hints.ai_socktype = SOCK_STREAM;
	  hints.ai_flags = AI_NUMERICHOST | AI_CANONNAME;

	  getaddrinfo("bilibili.com", "80", &hints, &res);

	  map<string, int> ip2port;
	  for (addrinfo *p = res; p != NULL; p = p->ai_next)
	  {
		  sockaddr_in *addr = (sockaddr_in*)p->ai_addr;
		  int port = ntohs(addr->sin_port);
		  string ipaddr = inet_ntoa(addr->sin_addr);
		  ip2port[ipaddr] = port;
	  }

	  char hostname[1024] = { 0 }, servername[1024] = { 0 };

	  sockaddr_in baddr;
	  baddr.sin_addr.s_addr = inet_addr("110.43.34.66");
	  baddr.sin_port = htons(80);
	  baddr.sin_family = AF_INET;

	//卡死了，不用这个函数
	  int ret = getnameinfo((sockaddr*)&baddr, sizeof(baddr), hostname, NI_MAXHOST, servername, NI_MAXSERV, NI_NAMEREQD);


	  WSACleanup();

  }
