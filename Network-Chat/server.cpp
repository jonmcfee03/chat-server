#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tchar.h>
#define HAVE_STRUCT_TIMESPEC
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

fd_set master;
HANDLE mutex;

//bool shutdown = false;

DWORD WINAPI handle_connection(LPVOID p_socket);
DWORD WINAPI handle_accept(LPVOID p_socket);
DWORD WINAPI handle_send(LPVOID empty_param);
void print_server() {
	cout << "\rServer: ";
}

int main() {
	cout << "========= SERVER ========== \n\n";

	SOCKET serverSocket = INVALID_SOCKET, acceptSocket = INVALID_SOCKET;
	int port = 55555;
	WSADATA wsaData;
	int wsaerr;
	WORD wVersionRequestioned = MAKEWORD(2, 2);
	wsaerr = WSAStartup(wVersionRequestioned, &wsaData);
	if (wsaerr != 0) {
		cout << "The Winsock dll not found!" << endl;
		return 0;
	}
	else {
		cout << "The Winsock dll found!" << endl;
		cout << "The status: " << wsaData.szSystemStatus << endl;
	}

	//create client socket
	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET) {
		cout << "Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return 0;
	}
	else {
		cout << "socket() is ok!" << endl;
	}

	//bind to socket
	sockaddr_in service;
	service.sin_family = AF_INET;
	InetPton(AF_INET, _T("10.0.0.193"), &service.sin_addr.s_addr);
	service.sin_port = htons(port);

	if (bind(serverSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
		cout << "Error at bind(): " << WSAGetLastError() << endl;
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	else {
		cout << "bind() is ok!" << endl;
	}

	//listen
	if (listen(serverSocket, 1) == SOCKET_ERROR) {
		cout << "listen(): Error listening on socket " << WSAGetLastError() << endl;
	}
	else {
		cout << "listen() is OK, I'm waiting for connections..." << endl;
		print_server();
	}


	mutex = CreateMutex(NULL, false, NULL);

	if (mutex == NULL) {
		cout << "\rMutex create error: " << GetLastError() << endl;
		return 1;
	}

	SOCKET* p_socket = new SOCKET;
	*p_socket = serverSocket;
	DWORD sendThreadId, acceptThreadId;
	HANDLE sendHdl, acceptHdl;
	sendHdl = CreateThread(NULL, 0, handle_send, NULL, 0, &sendThreadId);
	acceptHdl = CreateThread(NULL, 0, handle_accept, p_socket, 0, &acceptThreadId);

	HANDLE hdls[2] = { sendHdl, acceptHdl };

	WaitForMultipleObjects(2, hdls, 0, INFINITE);

	delete p_socket;

	for (int i = 0; i < master.fd_count; ++i) {
		closesocket(master.fd_array[i]);
		FD_CLR(master.fd_array[i], &master);
	}
	closesocket(serverSocket);
	WSACleanup();

	cout << "Server shutting down" << endl;
	return 0;
}

DWORD WINAPI handle_accept(LPVOID p_server_socket) {
	SOCKET acceptSocket = INVALID_SOCKET, serverSocket = *((SOCKET*)p_server_socket);
	while (true) {
		acceptSocket = accept(serverSocket, NULL, NULL);
		if (acceptSocket == INVALID_SOCKET) {
			cout << "accept failed: " << WSAGetLastError() << endl;
			WSACleanup();
			return -1;
		}
		else {
			cout << "\rAccepted connection" << endl;
			SOCKET* p_socket = new SOCKET;
			*p_socket = acceptSocket;
			DWORD threadId;
			HANDLE hdl;
			
			WaitForSingleObject(mutex, INFINITE);
			FD_SET(acceptSocket, &master);
			ReleaseMutex(mutex);

			hdl = CreateThread(NULL, 0, handle_connection, p_socket, 0, &threadId);
			p_socket = NULL;
		}
	}
}

DWORD WINAPI handle_connection(LPVOID p_socket) {
	char buffer[200];
	char clientUsername[50];

	SOCKET acceptSocket = *((SOCKET*)p_socket);
	delete p_socket;

	int usernameLen = recv(acceptSocket, clientUsername, 50, 0);
	clientUsername[usernameLen] = '\0';

	string welcomeMessage = "\rServer: Welcome to the server ";
	welcomeMessage = welcomeMessage + clientUsername;

	WaitForSingleObject(mutex, INFINITE);
	for (int i = 0; i < master.fd_count; ++i) {
		send(master.fd_array[i], welcomeMessage.c_str(), strlen(welcomeMessage.c_str()), 0);
	}
	ReleaseMutex(mutex);

	std::cout << '\r' << clientUsername << " connected" << endl;
	print_server();

	while (true) {
		int byteCount = recv(acceptSocket, buffer, 200, 0);
		if (byteCount <= 0) {
			WaitForSingleObject(mutex, INFINITE);
			closesocket(acceptSocket);
			FD_CLR(acceptSocket, &master);
			ReleaseMutex(mutex);
			
			//if the client disconnects
			if (byteCount == 0) {
				cout << "\rA client disconnected" << endl;
			}
			//if there is an error with the client
			else {
				cout << "\rServer receive error: " << WSAGetLastError() << endl;
			}
			print_server();
			return 0;
		}
		
		WaitForSingleObject(mutex, INFINITE);
		buffer[byteCount] = '\0';
		cout << '\r' << buffer << endl;
		print_server();
		ReleaseMutex(mutex);

		if (!strcmp(buffer, "/quit")) {
			WaitForSingleObject(mutex, INFINITE);
			closesocket(acceptSocket);
			FD_CLR(acceptSocket, &master);
			ReleaseMutex(mutex);
			return 0;
		}

		//print out message for all others
		WaitForSingleObject(mutex, INFINITE);
		for (int i = 0; i < master.fd_count; ++i) {
			if (master.fd_array[i] != acceptSocket)
				send(master.fd_array[i], buffer, byteCount, 0);
		}
		ReleaseMutex(mutex);
	}
	return 0;
}

DWORD WINAPI handle_send(LPVOID empty_param) {
	string data = "";
	int byteCount;
	while (true) {
		std::getline(std::cin, data);
		if (data.size() > 0) {
			//if server is to be shutdown
			WaitForSingleObject(mutex, INFINITE);
			if (data == "/shutdown") {
				ReleaseMutex(mutex);
				return 0;
			}

			data = "Server: " + data;
			WaitForSingleObject(mutex, INFINITE);
			for (int i = 0; i < master.fd_count; ++i) {
				byteCount = send(master.fd_array[i], data.c_str(), strlen(data.c_str()), 0);
			}
			print_server();
			ReleaseMutex(mutex);
		}
	}
	return 0;
}