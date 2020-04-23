#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <pthread.h>
#include <queue>
#include <stdarg.h>
#include <sys/syscall.h>
#include "mensaje.pb.h"
#include <map>
using namespace std;
using namespace chat;

#ifndef clientInfo
struct clientInfo
{
	int id;
	struct sockaddr_in socketCli;
	int socketFD;
	int requestFD;
	string userName;
	string ip;
	string status;
};
#endif

#ifndef Server
class Server
{
	public:
		int usrsNum;
		static void * newConnection(void * con);
		int listeningConnections(int serverSd);
		clientInfo requestDel();
		int sendRes(int serverSd, struct sockaddr_in *dest, ServerMessage srvr);
		string registerUsr(MyInfoSynchronize req, clientInfo cli);
		int readReq(int requestFD, void *buffer);
		clientInfo getUser(string usr);
		int pot;

	private:
		queue <clientInfo> requestQ;
		map<string, clientInfo> userMap;
};
#endif
