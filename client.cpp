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
#include <vector>
#include <pthread.h>
#include "mensaje.pb.h"
using namespace std;
using namespace chat;


int code = 0;
string userName;
int estado = 0;
std::string mssg = " ";
int run = 1;


ServerMessage parse_response(char *res)
{
	ServerMessage response;
	response.ParseFromString(res);
	return response;
}

int send_request(ClientMessage request, int socket, sockaddr_in sendSockAddr)
{
	int res_code = request.option();
	string srl_req;
	request.SerializeToString(&srl_req);
	char c_str[ srl_req.size() + 1];
	strcpy(c_str, srl_req.c_str());
	
	if(sendto(socket, c_str, strlen(c_str), 0, (struct sockaddr*)&sendSockAddr, sizeof(&sendSockAddr)) < 0)
	{
		cout << "Error sending request" <<endl;
		return -1;
		exit(0);
	}
	
	cout << "Request sent. Successful" << endl;
	return 0;
	
}


int read_message(void *res, int socketNum)
{
	cout << "Reading new messages" << endl;
	int recSize;

	recSize = recvfrom(socketNum, res, 1500, 0, NULL, NULL);

	if(recSize < 0)
	{
		cout << "ERROR READING" << endl;
		cout << recSize << endl;
	}
	
	cout << "Success" << endl;
	
	return recSize;
}

void broadcastMessage(string message, int clientSd, sockaddr_in sendSockAddr)
{
	BroadcastRequest * bcReq(new BroadcastRequest);
	bcReq->set_message(message);
	ClientMessage clreq;
	clreq.set_option(4);
	clreq.set_allocated_broadcast(bcReq);
	
	if(send_request(clreq, clientSd, sendSockAddr) < 0)
	{
		cout << "Error sending message" << endl;
	}
}

void shut(int signal)
{
	run = 0;
}

void mensajePrivado(string msg, string dest, int clientSd, sockaddr_in sendSockAddr, pthread_t dmThread)
{
	int pthread_create(dmThread);
	DirectMessageRequest * dm(new DirectMessageRequest);
	dm->set_message(msg);
	dm->set_username(dest);
	
	ClientMessage req;
	req.set_option(5);
	req.set_allocated_directmessage(dm);
	
	if(send_request(req, clientSd, sendSockAddr) < 0)
	{
		cout << "Error sending messge to " << dest << endl;
	}
	
}

void statusC(string status, int clientSd, sockaddr_in sendSockAddr)
{
	ChangeStatusRequest * streq(new ChangeStatusRequest);
	streq->set_status(status);
	ClientMessage cli;
	cli.set_option(3);
	cli.set_allocated_changestatus(streq);
	
	if(send_request(cli, clientSd, sendSockAddr) < 0)
	{
		cout << "Error changing status" << endl;
	}
}

void getConnectedUsers(int clientSd, sockaddr_in sendSockAddr)
{
	connectedUserRequest * usrs(new connectedUserRequest);
	ClientMessage cli;
	cli.set_option(2);
	cli.set_allocated_connectedusers(usrs);
	
	if(send_request(cli, clientSd, sendSockAddr) < 0)
	{
		cout << "Error getting connected users" << endl;
	}
}

void getUserInfo()
{
	cout << "NULL" << endl;
}

int loggingIn(int clientSd, sockaddr_in sendSockAddr)
{
	MyInfoSynchronize * my_info(new MyInfoSynchronize);
	my_info->set_username(userName);
	
	ClientMessage mess;
	mess.set_option(1);
	mess.set_allocated_synchronize(my_info);
	
	send_request(mess, clientSd, sendSockAddr);
	
	cout << "Waiting for acknowledge" << endl;
	char ack_res[1500];
	read_message(ack_res, clientSd);
	
	ServerMessage res = parse_response(ack_res);
	
	if(res.option() == 3) 
	{
		cout << "Fatal error in server" << endl;
		return -1;
	}
	else if(res.option() != 4)
	{
		cout << "Unexpected response form server" << endl;
		return -1;
	}
	
	cout << "Returning user ID" << endl;
	
	code = res.myinforesponse().userid();
	
	//Acknowledge
	MyInfoAcknowledge * my_info_ack(new MyInfoAcknowledge);
	my_info_ack->set_userid(code);
	
	ClientMessage res_ack;
	res_ack.set_allocated_acknowledge(my_info_ack);
	res_ack.set_option(6);
	
	send_request(res_ack, clientSd, sendSockAddr);
	
	return 0;
}



int main(int argc, char *argv[])
{
	
	//we need 2 things: ip address and port number, in that order
	if(argc != 4){
        	cerr << "Usage: ip_address port" << endl; 
        	exit(0); 
   	 } //grab the IP address and port number 
	char *serverIp = argv[2]; 
	int port = atoi(argv[3]); 
	userName = argv[1];
	int clientSd;
	
	struct sockaddr_in sendSockAddr;
	cout << "User detected" << endl;
	
	//Coneccion con el server
	
	cout << "Creating sockets" << endl; 
	clientSd = socket(AF_INET, SOCK_STREAM, 0);
	if(clientSd < 0)
	{
		cout << clientSd << endl;
		cout << "Error creating socket. Exiting..." << endl;
		return -1;
	}
	
	//Obteniendo IP para conectar	
	sendSockAddr.sin_family = AF_INET;
	sendSockAddr.sin_port = htons(port);
	
	if(inet_pton(AF_INET, serverIp, &sendSockAddr.sin_addr) <= 0)
	{
		cout << "Error. Invalid IP address" << endl;
		return -1;
	}
	
	//Seteando coneccion
	cout << "Conectando..." << endl;
	if(connect(clientSd, (struct sockaddr *)&sendSockAddr, sizeof(sendSockAddr)) < 0)
	{
		cout << "Error connecting to socket" << endl;
		return -1;
	}
	
	cout << "Connection to server successful at: " << serverIp << port << endl;
	
	cout << "Logging in. Username: " << userName << endl;
	
	if(loggingIn(clientSd, sendSockAddr) < 0)
	{
		cout << "Client could not log in" << endl;
		return -1;
	}
	
	//Iniciando sesion
	//Chequeando socket y ID
	if(clientSd < 0)
	{
		
		cout << "Error. Not connected to any sockets. Terminating connection" << endl;
		exit(EXIT_FAILURE);
		
	}
	if(code < 0)
	{
		if(loggingIn(clientSd, sendSockAddr) < 0)
		{
			cout << "Client could not log in" << endl;
			exit(EXIT_FAILURE);
		}
		else
		{
			cout << "Use logged in." << endl;
		}		
	}
	
	pthread_t cliThread;
	int pthread_create(cliThread);	
		
		cout << "Logged into server" << endl;
		
		//Seteando variables
		string mensaje;
		string mp;
		string dest;
		bool st;
		string usrN;
		
		
    	while(1){
		cout << "Bienvenid@ al chat \n	1. Chatear con todos los usuarios\n	2. Enviar mensaje a un usuario\n	3. Cambiar de status\n	4. Ayuda\n	5. Salir\n";
		cout << "> ";
		string data;
        	getline(cin, data);
		if(data == "1") {
			pthread_t sendThread; 
			cout << "Que mensaje desea enviar?: ";
			cin >> mensaje;
		
			//Creando thread
			int pthread_create (sendThread);
			// Conexion con el server
			pthread_t receiveThread;
        		broadcastMessage(mensaje, clientSd, sendSockAddr);
        		
        		cout << "Server: " << mensaje << endl;
		}

		if(data == "2"){
			cout << "A quien quiere mandar el mensaje?" << endl;
			cin >> dest;
			cout << "Ingrese el mensaje" << endl;
			cin >> mp;
			pthread_t threadDM;
			mensajePrivado(mp, dest, clientSd, sendSockAddr, threadDM);
		}

		if(data == "3"){
			cout << "Seleccione un estado: (1: activo, 2: dormido, 3: inactivo): ";
			cin >> estado;
			if(estado == 1)
			{
				statusC("activo", clientSd, sendSockAddr);
			}
			if(estado == 2)
			{
				statusC("dormido", clientSd, sendSockAddr);
			}
			if(estado == 3)
			{
				statusC("inactivo", clientSd, sendSockAddr);
			}
		}

		if(data == "4"){
			cout << "Bienvenid@ al chat \n	1. Chatear con todos los usuarios\n	2. Enviar mensaje a un usuario\n	3. Cambiar de status\n	4. Ayuda\n	5. Salir\n";
			cout << "> ";
		}

		if(data == "5"){
			cout << "Gracias por usar el servicio de Chat\n";
			break;
		}
		if(data != "1" || data != "2"  || data != "3"  || data != "4" || data != "5"){
			cout << "Opcion invalida" << endl;
		}
	}
	close(clientSd);
	cout << "********Final de la sesion********" << endl;
	cout << "Conexion cerrada" << endl;
	return 0;    
}
