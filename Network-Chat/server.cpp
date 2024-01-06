#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tchar.h>
#define HAVE_STRUCT_TIMESPEC
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

DWORD WINAPI handle_connection(LPVOID p_socket);

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
	InetPton(AF_INET, _T("127.0.0.1"), &service.sin_addr.s_addr);
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
	}


	while (true) {
		acceptSocket = accept(serverSocket, NULL, NULL);
		if (acceptSocket == INVALID_SOCKET) {
			cout << "accept failed: " << WSAGetLastError() << endl;
			WSACleanup();
			return -1;
		}
		cout << "Accepted connection" << endl;


		//handle_connection(acceptSocket);

		SOCKET* p_socket = new SOCKET;
		*p_socket = acceptSocket;
		DWORD threadId;
		HANDLE hdl;
		hdl = CreateThread(NULL, 0, handle_connection, p_socket, 0, &threadId);

	}

	closesocket(serverSocket);
	WSACleanup();
	return 0;
}


DWORD WINAPI handle_connection(LPVOID p_socket) {
	char buffer[200];
	SOCKET acceptSocket = *((SOCKET*)p_socket);
	delete p_socket;
	while (true) {
		int byteCount = recv(acceptSocket, buffer, 200, 0);

		if (byteCount == SOCKET_ERROR) {
			cout << "Server receive error: " << WSAGetLastError() << endl;
			return 0;
		}
		else {
			cout << "Server: received " << byteCount << " bytes" << endl;
			cout << "Client: " << buffer << endl;
			cout << endl;
				
			if (!strcmp(buffer, "/quit")) break;
		}
	}
	return 0;
}