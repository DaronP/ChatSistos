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
#include "Server.h"
using namespace std;
using namespace chat;
//Server side


int usrsNum;


map<string, clientInfo> userMap;
queue <clientInfo> requestQ;





int listeningConnections(int serverSd)
{
	struct sockaddr_in clientAddr;
	int sockSize = sizeof(clientAddr);
	int requestFD = accept(serverSd, (struct sockaddr *)&clientAddr, (socklen_t*)&sockSize);
	
	if(requestFD < 0)
	{
		cout << "Error connecting to client" << endl;
		return -1;
	}
	
	cout << "Connection accepted" << endl;
	
	char ipstr[INET6_ADDRSTRLEN];
	struct sockaddr_in *temp = (struct sockaddr_in *)&clientAddr;
	inet_ntop(AF_INET, &temp->sin_addr, ipstr, sizeof(ipstr));
	
	cout << "Connecting to IP address: " << ipstr << endl;
	
	struct clientInfo newClient;
	newClient.id = usrsNum;
	usrsNum ++;
	newClient.socketCli = clientAddr;
	newClient.requestFD = requestFD;
	newClient.ip = ipstr;
	
	//Pusheando cliente nuevo
	pthread_mutex_t clireqMutex;
	pthread_mutex_lock(&clireqMutex);
	requestQ.push(newClient);
	pthread_mutex_unlock(&clireqMutex);
	
	return 0;
	
}

clientInfo requestDel()
{
	clientInfo request;
	
	pthread_mutex_t clireqMutex;
	pthread_mutex_lock(&clireqMutex);
	
	request = requestQ.front();
	requestQ.pop();
	
	pthread_mutex_unlock(&clireqMutex);
	
	return request;
}

int readReq(int requestFD, void *buffer)
{
	int size = recvfrom(requestFD, buffer, 1500, 0, NULL, NULL);
	if(size < 0)
	{
		cout << "Error reading request" << endl;
		return -1;
	}
	
	return size;
	
}

clientInfo getUser(string usr)
{
	clientInfo clien;
	pthread_mutex_t clireqMutex;
	pthread_mutex_lock(&clireqMutex);
	clien = userMap[usr];
	pthread_mutex_unlock(&clireqMutex);
	
	return clien;
}

int sendRes(int serverSd, struct sockaddr_in *dest, ServerMessage srvr)
{
	string res;
	srvr.SerializeToString(&res);
	
	char cStr[res.size() + 1];
	strcpy(cStr, res.c_str());
	
	int sending = sendto(serverSd, cStr, sizeof(cStr), 0, (struct sockaddr *) &dest, sizeof(&dest));
	
	if(sending < 0)
	{
		cout << "Error sending response" << endl;
		return -1;
	}
	
	return 0;
}

string registerUsr(MyInfoSynchronize req, clientInfo cli)
{
	cli.userName = req.username();
	cli.status = "activo";
	
	pthread_mutex_t clireqMutex;
	pthread_mutex_lock(&clireqMutex);
	userMap[cli.userName] = cli;
	pthread_mutex_unlock(&clireqMutex);
	
	clientInfo connectedUser = getUser(req.username());
	
	MyInfoResponse * my_info(new MyInfoResponse);
	
	my_info->set_userid(connectedUser.id);
	
	ServerMessage srvrmsg;
	
	srvrmsg.set_option(4);
	srvrmsg.set_allocated_myinforesponse(my_info);
	
	sendRes(connectedUser.requestFD, &connectedUser.socketCli, srvrmsg);
	
	char acknw[1500];
	readReq(connectedUser.requestFD, acknw);
	
	return(connectedUser.userName);
}

ServerMessage changeStatus(ChangeStatusRequest req, string usr)
{
	string status = req.status();
	int iD;
	
	pthread_mutex_t clireqMutex;
	pthread_mutex_lock(&clireqMutex);
	userMap[usr].status = status;
	iD = userMap[usr].id;
	pthread_mutex_unlock(&clireqMutex);
	
	ChangeStatusResponse * cs(new ChangeStatusResponse);
	cs->set_userid(iD);
	cs->set_status(status);
	
	ServerMessage res;
	res.set_option(6);
	res.set_allocated_changestatusresponse(cs);
	
	return res;
	
}

ServerMessage bcastMessage(BroadcastRequest req, clientInfo ci)
{
	string message = req.message();
	ServerMessage res;
	BroadcastResponse * bres(new BroadcastResponse);
	ServerMessage resR;
	BroadcastMessage * bcast(new BroadcastMessage);
	
	
	bcast->set_message(message);
	bcast->set_userid(ci.id);
	

	res.set_option(1);
	res.set_allocated_broadcast(bcast);
	
	bres->set_messagestatus("sent");
	
	resR.set_option(7);
	resR.set_allocated_broadcastresponse(bres);
	
	return resR;	
	
}



ServerMessage dm(DirectMessageRequest req, clientInfo ci)
{
	clientInfo cinfo;
	
	if(req.has_username())
	{
		cinfo = getUser(req.username());
	}
	else
	{
		cout << "No username Entered" << endl;
		exit(0);
	}
	
	DirectMessage * direct(new DirectMessage);
	
	direct->set_userid(ci.id);
	direct->set_message(req.message());
	
	ServerMessage dmres;
	
	dmres.set_option(2);
	sendRes(cinfo.requestFD, &cinfo.socketCli, dmres);
	
	DirectMessageResponse * dmresponse(new DirectMessageResponse);
	dmresponse->set_messagestatus("sent");
	
	ServerMessage sm;
	sm.set_option(8);
	sm.set_allocated_directmessageresponse(dmresponse);
	
	return sm;
	
	
}

ServerMessage requests(ClientMessage cm, clientInfo ci)
{
	int option = cm.option();
	
	if(option == 3)
	{
		return changeStatus(cm.changestatus(), ci.userName);
	}
	if(option == 4)
	{
		return bcastMessage(cm.broadcast(), ci);
	}
	if(option == 5)
	{
		return dm(cm.directmessage(), ci);
	}
	else
	{
		
	}
}

void newConnection()
{
	
	
	struct clientInfo requestD = requestDel();
	char req[1500];
	readReq(requestD.requestFD, req);
	
	ClientMessage cliMsg;
	
	cliMsg.ParseFromString(req);
	
	int optn = cliMsg.option();
	
	string usr;
	
	if(optn == 1)
	{
		usr = registerUsr(cliMsg.synchronize(), requestD);
	}
	else
	{
		usr = "";
		
		ServerMessage clireq;
		ErrorResponse * err(new ErrorResponse);
		err->set_errormessage("User not logged in");
	
		clireq.set_option(3);
		clireq.set_allocated_error(err);		
		
		sendRes(requestD.requestFD, &requestD.socketCli, clireq);
		
		close(requestD.requestFD);
		pthread_exit(NULL);
	}
	
	if(usr != "")
	{
	
		cout << "Connection successful.\nWelcome to the chat" << endl;
		clientInfo userInfo = getUser(usr);
		userInfo.id = requestD.id;
		
		char wasd[1500];
		
		while(1)
		{
			int rdSize = readReq(requestD.requestFD, wasd);
			
			if(rdSize <= 0)
			{
				cout << "FATAL ERROR" << endl;
				close(requestD.requestFD);
				break;
			}
			
			ClientMessage rqst;
	
			rqst.ParseFromString(wasd);
			
			ServerMessage nose = requests(rqst, userInfo);
			
			if(sendRes(requestD.requestFD, &requestD.socketCli, nose) < -1)
			{
				cout << "Error sending response " << endl;
			}
			else
			{
				cout << "Response sent" << endl;
			}
		}
	
	}
	else
	{
		close(requestD.requestFD);
	}
	
	pthread_exit( NULL );
	
}



int main(int argc, char *argv[])
{
	

	GOOGLE_PROTOBUF_VERIFY_VERSION;

    //for the server, we only need to specify a port number
    if(argc != 2)
    {
        cerr << "Usage: port" << endl;
        exit(0);
    }
    //grab the port number
    int port = atoi(argv[1]);
    
    
    struct sockaddr_in sendSockAddr;
    int serverSd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(serverSd < 0)
    {
    	cout << "Error creating socket" << endl;
    	return -1;
    }
    
    sendSockAddr.sin_family = AF_INET;
    sendSockAddr.sin_addr.s_addr = INADDR_ANY;
    sendSockAddr.sin_port = htons(port);
    
    int binding = bind(serverSd, (struct sockaddr *)&sendSockAddr, sizeof(sendSockAddr));
    if(binding < 0)
    {
    	cout << "Error binding socket to IP" << endl;
    	cout << "Unable to start server" << endl;
    	return -1;
    }
    
    int listening = listen(serverSd, 50);
    if(listening < 0)
    {
    	cout << "Unable to lsiten clients" << endl;
    	cout << "Unable to start server" << endl;
    	return -1;
    }
    
    
	while(1)
		{
		
			
			listeningConnections(serverSd);
			//pthread_create(&serverThread, NULL, newConnection, NULL);
			newConnection();
		}
    
    
      
}
