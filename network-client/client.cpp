#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tchar.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

int main() {
	cout << "======== CLIENT ======== \n\n";

	SOCKET clientSocket = INVALID_SOCKET;
	int port = 55555;
	WSADATA wsaData;
	int wsaerr;
	WORD wVersionRequestioned = MAKEWORD(2, 2);
	wsaerr = WSAStartup(wVersionRequestioned, &wsaData);

	if (wsaerr != 0) {
		cout << "The winsock dll not found!" << endl;
		return 0;
	}
	else {
		cout << "The Winsock dll found!" << endl;
		cout << "The status: " << wsaData.szSystemStatus << endl;
	}

	//create client socket
	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET) {
		cout << "Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return 0;
	}
	else {
		cout << "socket() is ok!" << endl;
	}

	//connect to socket
	sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	InetPton(AF_INET, _T("127.0.0.1"), &clientService.sin_addr.s_addr);
	clientService.sin_port = htons(port);

	if (connect(clientSocket, (SOCKADDR*)&clientService, sizeof(clientService)) == SOCKET_ERROR) {
		cout << "Error at bind(): " << WSAGetLastError() << endl;
		WSACleanup();
		return 0;
	}
	else {
		cout << "Client: connect() is OK!" << endl;
		cout << "Client: Can start sending and receiving data..." << endl;
	}
	system("pause");

	closesocket(clientSocket);
	WSACleanup();
	return 0;
}