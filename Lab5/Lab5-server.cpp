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
#include <vector>
#include <queue>
#include <string>
#include <chrono>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

#pragma warning(disable: 4996)

#define BUFSIZE 30
#define MY_PORT 2222
#define SERVE_TIME_IN_SEC 5
#define WORK_TIME_IN_SEC 60

using namespace std;
using namespace std::chrono;

DWORD WINAPI worker(LPVOID lpParam);

// Покупатель
struct Customer
{
	int id;
	SOCKET connection;
	int sum;

	Customer(int id, SOCKET connection, int sum) {
		this->id = id;
		this->connection = connection;
		this->sum = sum;
	};

	~Customer()
	{
		printf("Покупатель %d ушел\n", this->id);
		shutdown(this->connection, SD_SEND);
		closesocket(this->connection);
	}

	void sendMessage(string message)
	{
		send(this->connection, message.c_str(), sizeof(message.c_str()), NULL);
	}
};

// Кассир
struct Cashier
{
	int id;
	queue<Customer*> queue;
	HANDLE mutex;
	HANDLE thread;
	int total_sum = 0;
	int total_customers = 0;
	bool waitForClosing = false;

	Cashier(int id, Customer* customer) {
		this->id = id;
		addToQueue(customer);

		this->mutex = CreateMutex(NULL, 0, NULL);
		if (!this->mutex) {
			printf("Ошибка при создании мьютекса: %d\n", GetLastError());
			system("pause");
			exit(1);
		}

		thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)worker, (LPVOID)this, 0, NULL);
		if (thread == NULL)
		{
			printf("Ошибка при создании потока: %d\n", GetLastError());
			system("pause");
			exit(1);
		}
	};

	~Cashier()
	{
		this->waitForClosing = true;
		while (this->waitForClosing)
		{
			Sleep(100);
		}

		printf("Кассир %d уходит домой. Он успел обслужить %d покупателей на сумму %d рублей\n",
			this->id, total_customers, total_sum);
		CloseHandle(this->mutex);
		CloseHandle(this->thread);

		while (!this->queue.empty())
		{
			Customer* customer = queue.front();
			queue.pop();
			printf("Покупатель %d не успел купить товары\n", customer->id);
			delete customer;
		}
	}
	void addToQueue(Customer* customer)
	{
		printf("Покупатель %d выбрал кассу %d\n", customer->id, id);
		this->queue.push(customer);
		this->total_customers++;
		this->total_sum += customer->sum;
	}

	bool lockQueue()
	{
		while (true) {
			DWORD dwWaitResult = WaitForSingleObject(this->mutex, INFINITE);
			if (dwWaitResult == WAIT_OBJECT_0)
			{
				return true;
			}

			printf("Ждем блокировки очереди на кассе %d: результат %d\n", id, dwWaitResult);
			Sleep(1000);
		}
	}

	void unlockQueue()
	{
		ReleaseMutex(this->mutex);
	}
};

void chooseCashier(Customer* customer);
bool addCashier(Customer* customer);
string readFromSocket(SOCKET connection);

vector<Cashier*> cashiers;
int cashiersMaxCount;
int customerCount = 0;
bool withManager = false;

int main()
{
	setlocale(LC_ALL, "Russian");
	srand(time(NULL));

	printf("Время обслуживания одного клиента: %d секунд\n", SERVE_TIME_IN_SEC);
	printf("Время работы магазина: %d секунд\n", WORK_TIME_IN_SEC);
	printf("Введите количество кассиров X: ");
	cin >> cashiersMaxCount;

	printf("Магазин начал свою работу. Ждем покупателей.\n");

	// Запоминаем время открытия магазина, чтобы закрыться во время
	steady_clock::time_point start = high_resolution_clock::now();

	// загружаем WSAStartup
	WSAData wsaData; //создаем структуру для загрузки
	WORD DLLVersion = MAKEWORD(2, 1);
	if (WSAStartup(DLLVersion, &wsaData) != 0)
	{
		printf("Ошибка инициализации TCP %d\n", WSAGetLastError());
		exit(1);
	}

	SOCKADDR_IN addr;
	int sizeofaddr = sizeof(addr); //размер
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(MY_PORT); //порт
	addr.sin_family = AF_INET; //семейство протоколов

	//сокет для прослушивания порта
	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);

	//привязка адреса сокету
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));

	// прослушивание, сколько запросов ожидается
	listen(sListen, 1000);

	while (true)
	{
		steady_clock::time_point stop = high_resolution_clock::now();
		auto duration = duration_cast<seconds>(stop - start);
		if (duration.count() > WORK_TIME_IN_SEC)
		{
			printf("Магазин отработал %d секунд, пора закрываться приходите завтра\n", WORK_TIME_IN_SEC);
			break;
		}

		SOCKET connection = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr);
		if (connection == 0)
		{
			printf("Не удалось создать соединение");
			exit(1);
		}

		int customerId = ++customerCount;
		printf("Новый покупатель %d подошел к кассам\n", customerId);

		// Читаем информацию по сумме
		string msg = readFromSocket(connection);
		int sum = atoi(msg.c_str());

		Customer* customer = new Customer(customerId, connection, sum);

		// Пишем информацию о номере покупателя
		customer->sendMessage(to_string(customer->id));

		chooseCashier(customer);
	}

	closesocket(sListen);
	WSACleanup();

	// Вычисляем итоговую статистику
	int total_customers = 0;
	int total_sum = 0;
	for (int i = 0; i < cashiers.size(); i++) {
		Cashier* cashier = cashiers.at(i);
		total_customers += cashier->total_customers;
		total_sum += cashier->total_sum;

		delete cashier;
	}
	cashiers.clear();

	printf("Всего магазин обслужил %d покупателей на сумму %d рублей\n", total_customers, total_sum);
	system("pause");
	exit(0);
}

string readFromSocket(SOCKET connection)
{
	char* buf = new char[BUFSIZE];
	int len = recv(connection, buf, sizeof(buf), NULL);
	return string(buf, 0, len);
}

void chooseCashier(Customer* customer)
{
	// если еще нет ни одного кассира за кассой зовем первого
	if (cashiers.size() == 0) {
		addCashier(customer);
		return;
	}

	// если кассиры есть, выбираем случайного
	srand(time(0));
	int cashierIndex = rand() % cashiers.size();
	Cashier* cashier = cashiers.at(cashierIndex);

	if (!cashier->lockQueue())
	{
		printf("Не удалось заблокировать очередь на кассе %d\n", cashier->id);
		system("pause");
		exit(1);
	}

	// если на кассе 3 и более человек в очереди, пробуем позвать нового кассира
	if (cashier->queue.size() > 2)
	{
		if (addCashier(customer))
		{
			cashier->unlockQueue();
			return;
		}
	}

	cashier->addToQueue(customer);

	// передаем клиенту номер кассы
	customer->sendMessage(to_string(cashier->id));

	cashier->unlockQueue();
}

bool addCashier(Customer* customer)
{
	// пытаемся добавить нового кассира
	if (cashiers.size() < cashiersMaxCount)
	{
		int cashier_id = cashiers.size() + 1;
		printf("Позвали нового кассира %d\n", cashier_id);

		// передаем клиенту номер кассы
		customer->sendMessage(to_string(cashier_id));

		Cashier* cashier = new Cashier(cashier_id, customer);
		cashiers.push_back(cashier);

		return true;
	}

	printf("Все кассиры заняты, зовем управляющего\n");
	withManager = true;

	return false;
}

DWORD WINAPI worker(LPVOID lpParam)
{
	Cashier* cashier = (Cashier*)lpParam;

	printf("Кассир %d встал за кассу и ждем покупателей\n", cashier->id);

	while (true)
	{
		if (cashier->waitForClosing)
		{
			break;
		}

		if (!cashier->lockQueue())
		{
			printf("Ошибка блокировки очереди на кассе %d\n", cashier->id);
			system("pause");
			exit(1);
		}

		if (cashier->queue.size() == 0)
		{
			cashier->unlockQueue();

			Sleep(100);
			continue;
		}

		Customer* customer = cashier->queue.front();

		int queueSize = cashier->queue.size();
		printf("Кассир %d начинает обслуживать следующего покупателя. Длина очереди - %d человек\n",
			cashier->id, queueSize);

		cashier->queue.pop();
		queueSize = cashier->queue.size();
		printf("Обслуживаем покупателя %d на кассе %d. Сумма покупки %d руб\n",
			customer->id, cashier->id, customer->sum);

		cashier->unlockQueue();

		// считаем, что время обслуживания зависит от суммы заказа
		// если смотрит управляющий, то скорость увеличивается в 2 раза
		int serveTime = withManager ? SERVE_TIME_IN_SEC / 2 : SERVE_TIME_IN_SEC;
		Sleep(serveTime * 1000);

		printf("Обслужили покупателя %d на кассе %d. В очереди осталось %d человек\n",
			customer->id, cashier->id, queueSize);
		customer->sendMessage("Done");
		delete customer;
	}

	printf("Завершили обслуживание на кассе %d\n", cashier->id);
	cashier->waitForClosing = false;
	return 0;
}


