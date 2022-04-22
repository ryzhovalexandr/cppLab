
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#pragma warning(disable: 4996)
#include <string>

#define BUFSIZE 50
#define MY_PORT 2222

using namespace std;

char* buf = new char[BUFSIZE];
DWORD dwBytes;

string readData(SOCKET connection);
void writeData(SOCKET connection, string msg);

int main()
{
	setlocale(LC_ALL, "Russian");
	srand(time(0));

	//загружаем WSAStartup
	WSAData wsaData; //создаем структуру для загрузки
	WORD DLLVersion = MAKEWORD(2, 1);
	if (WSAStartup(DLLVersion, &wsaData) != 0)
	{
		//проверка на подключение библиотеки
		printf("Ошибка инициализации сокетов\n");
		system("pause");
		exit(1);
	}

	printf("Покупатель начал свои покупки. process id=%d\n", GetCurrentProcessId());

	int sum = rand() % 9000 + 1000;
	printf("Покупатель набрал товар на сумму %d руб и пошел к кассам\n", sum);

	SOCKADDR_IN addr;
	//параметры адреса сокета
	int sizeofaddr = sizeof(addr);//размер
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");//адрес
	addr.sin_port = htons(MY_PORT);//порт
	addr.sin_family = AF_INET;//семейство протоколов

	//сокет для соединения с сервером
	SOCKET connection = socket(AF_INET, SOCK_STREAM, NULL);

	//проверка на подключение к серверу
	if (connect(connection, (SOCKADDR*)&addr, sizeof(addr)) != 0)
	{
		printf("Ошибка подключения к серверу %d\n", WSAGetLastError());
		system("pause");
		exit(1);
	}

	writeData(connection, to_string(sum));

	string msg = readData(connection);
	printf("Порядковый номер покупателя: %s\n", msg.c_str());

	msg = readData(connection);
	printf("Номер кассира: %s\n", msg.c_str());

	msg = readData(connection);
	if (!strcmp(msg.c_str(), "Done"))
	{
		printf("Покупка успешно завершена.\n");
	}
	else
	{
		printf("Получена неизвестная команда - %s.\n", msg.c_str());
		system("pause");
		exit(1);
	}

	delete buf;

	closesocket(connection);
	WSACleanup();

	return 0;
}

string readData(SOCKET connection)
{
	char* buf = new char[BUFSIZE];
	int len = recv(connection, buf, sizeof(buf), NULL);
	return string(buf, 0, len);
}

void writeData(SOCKET connection, string msg)
{
	send(connection, msg.c_str(), sizeof(msg.c_str()), 0);
}


