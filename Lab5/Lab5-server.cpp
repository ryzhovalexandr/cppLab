/*
Вариант 18
Задача о супермаркете ПяПерМаг. В супермаркете работают X
кассиров, покупатели заходят в супермаркет, делают покупки и
становятся в очередь к случайному кассиру. Изначально он один.

Кассиру требуется время, чтобы обслужить покупателя. Если в
очереди накапливается больше 3 человек, кассир зовет своего коллегу.

Если во всех очередях больше 3 человек – количество кассиров
увеличивается, пока не достигнет X. Если покупателей меньше не
становится (распродажа мороженого), то требуется звать
управляющего. После того, как появляется управляющий, кассиры
начинают обслуживать клиентов в два раза быстрее. Смоделировать
рабочий день супермаркета. В качестве сервера – стойка касс с
потоками-кассирами. Процессы-клиенты – отдельные покупатели.
Подвести итог в конце дня, сколько было клиентов всего и у каждого
кассира и сколько денег они оставили в магазине (аналогично, всего и
у каждого из кассиров).
*/

#include <iostream>
#include <windows.h>
#include <vector>
#include <queue>

#define BUFSIZE 1024

using namespace std;

// Данные кассы
struct Cashier
{
	int id;
	queue<Customer> queue;
	HANDLE mutex;

	Cashier(int id) {
		this->id = id;

		this->mutex = CreateMutex(NULL, 0, NULL);
		if (this->mutex) {
			printf("Ошибка при создании мьютекса: %d\n", GetLastError());
			exit(1);
		}
	};
};

struct Customer
{
	int id;
	HANDLE pipe;

	Customer(int id, HANDLE pipe) {
		this->id = id;
		this->pipe = pipe;
	};
};

vector<Cashier> cashiers;
int cashiersMaxCount;
int customerCount = 0;

char* buf = new char[BUFSIZE];
DWORD dwBytes;

int main()
{
	setlocale(LC_ALL, "Russian");
	srand(time(NULL));

	printf("Введите количество кассиров X: ");
	cin >> cashiersMaxCount;

	printf("Магазин начал свою работу.");

	while (true)
	{
		HANDLE pipe = createPipe();
		waitingForCustomer(pipe);

		// Читаем информацию по сумме
		if (!ReadFile(pipe, buf, BUFSIZE, &dwBytes, NULL)) {
			printf("Ошибка получения данных от клиента.\n");
			exit(1);
		}

		string msg(buf, 0, dwBytes);

		int cashierIndex = rand() % cashiers.size();
		Cashier* cashier = new Cashier(pipe, 0, ++customerCount);

		if (msg == "Стоматолог")
			d->doctor_id = 0;
		else if (msg == "Хирург")
			d->doctor_id = 1;
		else if (msg == "Терапевт")
			d->doctor_id = 2;
		else if (msg == "Stop") {
			CloseHandle(pipe);
			printf("Программа завершает работу...\n");
			break;
		}
		else {
			printf("Ошибка при вводе данных на стороне клиента.\n");
			exit(1);
		}

		cashiers.push_back(*cashier);
		HANDLE thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)worker, (LPVOID)&patients.at(patients.size() - 1), 0, NULL);
		if (thread == NULL) {
			printf("Ошибка при создании потока: %d\n", GetLastError());
			exit(1);
		}


	}

}

HANDLE createPipe()
{
	// Создаём именованный канал для общения клиентов
	HANDLE pipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\shop"), PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES, BUFSIZE, BUFSIZE, 0, NULL);

	if (pipe == INVALID_HANDLE_VALUE) {
		printf("Ошибка при создании именованного канала: %d\n", GetLastError());
		exit(1);
	}

	return pipe;
}

void waitingForCustomer(HANDLE& pipe)
{
	while (true) {
		if (ConnectNamedPipe(pipe, NULL))
		{
			printf("Новый покупатель\n");
			break;
		}
	}
}

void addCashier()
{
	Cashier* cashier = new Cashier(pipe, 0, ++customerCount);
}

