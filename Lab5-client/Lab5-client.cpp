
#include <iostream>
#include <windows.h>
#include <string>

#define BUFSIZE 1024

using namespace std;

char* buf = new char[BUFSIZE];
DWORD dwBytes;

HANDLE createPipe();
string readData(HANDLE hPipe);

int main()
{
	setlocale(LC_ALL, "Russian");

	printf("Покупатель начал свои покупки. process id=%d\n", GetCurrentProcessId());

	int duration = rand() % 9000 + 1000;
	Sleep(duration);

	int sum = rand() % 14000 + 1000;
	printf("Покупатель набрал товар на сумму %d руб и пошел к кассам\n", sum);

	HANDLE hPipe = createPipe();

	string s_sum = to_string(sum);
	DWORD dwBytes;
	if (!WriteFile(hPipe, s_sum.c_str(), size(s_sum), &dwBytes, NULL)) {
		printf("Ошибка при передаче данных на сервер.\n");
		system("pause");
		exit(1);
	}

	string msg = readData(hPipe);
	printf("Порядковый номер покупателя: %s\n", msg.c_str());

	msg = readData(hPipe);
	printf("Номер кассира: %s\n", msg.c_str());

	msg = readData(hPipe);
	if (!strcmp(msg.c_str(), "Done"))
	{
		printf("Покупка успешно завершена.\n");
	}
	else
	{
		printf("Получена неизвестная команда - %s.\n", msg.c_str());
	}

	delete buf;
	CloseHandle(hPipe);


	system("pause");
	return 0;
}

HANDLE createPipe()
{
	while (true)
	{
		HANDLE hPipe = CreateFile(TEXT("\\\\.\\pipe\\shop"), GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, NULL, NULL);

		if (hPipe != INVALID_HANDLE_VALUE)
		{
			return hPipe;
		}

		if (GetLastError() != ERROR_PIPE_BUSY) {
			printf("Не могу открыть именованный канал.\n");
			system("pause");
			exit(1);
		}

		if (!WaitNamedPipe(TEXT("\\\\.\\pipe\\shop"), 10000)) {
			printf("Не дождались открытия магазина в течение 10 секунд\n");
			system("pause");
			exit(1);
		}
	}
}

string readData(HANDLE hPipe)
{
	if (!ReadFile(hPipe, buf, BUFSIZE, &dwBytes, NULL)) {
		printf("Ошибка получения данных из канала.\n");
		system("pause");
		exit(1);
	}

	string msg(buf, 0, dwBytes);
	return msg;
}


