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
#include <string>

#define BUFSIZE 1024

using namespace std;

DWORD WINAPI worker(LPVOID lpParam);
HANDLE createPipe();
void waitingForCustomer(HANDLE& pipe);
void writeToPipe(HANDLE hPipe, string str);

// Покупатель
struct Customer
{
	int id;
	HANDLE hPipe;
	int sum;

	Customer(int id, HANDLE hPipe, int sum) {
		this->id = id;
		this->hPipe = hPipe;
		this->sum = sum;
	};

	~Customer()
	{
		CloseHandle(hPipe);
	}

	void send(string message)
	{
		writeToPipe(hPipe, message);
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

	Cashier(int id, Customer* customer) {
		this->id = id;
		this->queue.push(customer);
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
		CloseHandle(this->mutex);
		CloseHandle(this->thread);
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
string readFromPipe(HANDLE hPipe);

vector<Cashier*> cashiers;
int cashiersMaxCount;
int customerCount = 0;
bool withManager = false;

int main()
{
	setlocale(LC_ALL, "Russian");
	srand(time(NULL));

	printf("Введите количество кассиров X: ");
	cin >> cashiersMaxCount;

	printf("Магазин начал свою работу. Ждем покупателей.\n");

	while (true)
	{
		// создаем канал и ждем очередного покупателя
		HANDLE hPipe = createPipe();
		waitingForCustomer(hPipe);

		// Читаем информацию по сумме
		string msg = readFromPipe(hPipe);
		int sum = atoi(msg.c_str());

		Customer* customer = new Customer(++customerCount, hPipe, sum);

		// Пишем информацию о номере покупателя
		customer->send(to_string(customer->id));

		chooseCashier(customer);
	}
}

HANDLE createPipe()
{
	// Создаём именованный канал для следующего покупателя
	HANDLE pipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\shop"), PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES, BUFSIZE, BUFSIZE, 0, NULL);

	if (pipe == INVALID_HANDLE_VALUE) {
		printf("Ошибка при создании именованного канала: %d\n", GetLastError());
		system("pause");
		exit(1);
	}

	return pipe;
}

void waitingForCustomer(HANDLE& pipe)
{
	while (true) {
		if (ConnectNamedPipe(pipe, NULL))
		{
			printf("Новый покупатель подошел к кассам\n");
			break;
		}
	}
}

string readFromPipe(HANDLE hPipe)
{
	char* buf = new char[BUFSIZE];
	DWORD dwBytes;
	if (!ReadFile(hPipe, buf, BUFSIZE, &dwBytes, NULL)) {
		printf("Ошибка получения данных от клиента.\n");
		system("pause");
		exit(1);
	}

	string msg(buf, 0, dwBytes);
	return msg;
}

void writeToPipe(HANDLE hPipe, string str)
{
	DWORD dwBytes;
	if (!WriteFile(hPipe, str.c_str(), size(str), &dwBytes, NULL)) {
		printf("Ошибка при передаче данных на сервер.\n");
		system("pause");
		exit(1);
	}
}

void chooseCashier(Customer* customer)
{
	// если еще нет ни одного кассира за кассой зовем первого
	if (cashiers.size() == 0) {
		addCashier(customer);
		return;
	}

	// если кассиры есть, выбираем случайного
	int cashierIndex;
	cashierIndex = rand() % cashiers.size();
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

	cashier->queue.push(customer);

	// передаем клиенту номер кассы
	customer->send(to_string(cashier->id));

	cashier->unlockQueue();
}

bool addCashier(Customer* customer)
{
	if (cashiers.size() < cashiersMaxCount)
	{
		int cashier_id = cashiers.size() + 1;
		printf("Позвали нового кассира %d\n", cashier_id);

		// передаем клиенту номер кассы
		customer->send(to_string(cashier_id));

		Cashier* cashier = new Cashier(cashier_id, customer);
		cashiers.push_back(cashier);

		return true;
	}

	printf("Все кассиры заняты, зовем управляющего");
	withManager = true;

	return false;
}

DWORD WINAPI worker(LPVOID lpParam)
{
	Cashier* cashier = (Cashier*)lpParam;

	printf("Кассир %d встал за кассу и ждем покупателей\n", cashier->id);

	while (true)
	{
		if (!cashier->lockQueue())
		{
			printf("Ошибка блокировки очереди на кассе %d\n", cashier->id);
			system("pause");
			exit(1);
		}

		if (cashier->queue.size() == 0) {
			cashier->unlockQueue();
			Sleep(1000);
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
		int serveTime = withManager ? customer->sum / 2 : customer->sum;
		Sleep(serveTime);
		cashier->total_sum += customer->sum;

		printf("Обслужили покупателя %d на кассе %d. В очереди осталось %d человек\n",
			customer->id, cashier->id, queueSize);
		customer->send("Done");
		delete customer;
	}

	printf("Завершаем обслуживание на кассе %d\n", cashier->id);
	system("pause");
	ExitThread(0);
}


