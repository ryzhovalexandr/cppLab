#include <WinSock2.h>
#include <iostream>
#include <string>

using namespace std;

#define MY_PORT 2222
#define BUFSIZE 30

class TcpServer
{
private:
	SOCKET WSAAPI connection;

public:
	TcpServer()
	{
		// загружаем WSAStartup
		WSAData wsaData; //создаем структуру дл€ загрузки
		WORD DLLVersion = MAKEWORD(2, 1);
		if (WSAStartup(DLLVersion, &wsaData) != 0)
		{
			printf("ќшибка инициализации TCP %d\n", WSAGetLastError());
			exit(1);
		}

		SOCKADDR_IN addr;
		int sizeofaddr = sizeof(addr); //размер
		addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //адрес
		addr.sin_port = htons(MY_PORT); //порт
		addr.sin_family = AF_INET; //семейство протоколов

		//сокет дл€ прослушивани€ порта
		SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);

		//прив€зка адреса сокету
		bind(sListen, (SOCKADDR*)&addr, sizeof(addr));

		// прослушивание, сколько запросов ожидаетс€
		listen(sListen, 1);

		this->connection = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr);
		if (connection == 0)
		{
			printf("Ќе удалось создать соединение");
			exit(1);
		}
	}

	~TcpServer()
	{
		if (closesocket(this->connection))
		{
			printf("ќшибка закрыти€ сокета %d\n", WSAGetLastError());
			exit(1);
		};
		WSACleanup(); // ƒеиницилизаци€ библиотеки Winsock
	}


	void sendToClient(string msg)
	{
		send(this->connection, msg.c_str(), sizeof(msg.c_str()), NULL);
	}

	string receiveFromClient()
	{
		char* buf = new char[BUFSIZE];
		int len = recv(this->connection, buf, sizeof(buf), NULL);
		return string(buf, 0, len);
	}
};