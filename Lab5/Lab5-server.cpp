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

DWORD WINAPI worker(LPVOID lpParam);
HANDLE createPipe();
void waitingForCustomer(HANDLE& pipe);

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

	void send(string message)
	{

	}
};

// Кассир
struct Cashier
{
	int id;
	queue<Customer> queue;
	HANDLE mutex = NULL;
	HANDLE thread = NULL;
	int total_sum = 0;

	Cashier(int id, Customer customer) {
		this->id = id;
		this->queue.push(customer);
		this->mutex = CreateMutex(NULL, 0, NULL);
		if (this->mutex) {
			printf("Ошибка при создании мьютекса: %d\n", GetLastError());
			system("pause");
			exit(1);
		}
	};

	~Cashier()
	{
		CloseHandle(this->mutex);
		CloseHandle(this->thread);
		this->mutex = NULL;
	}

	bool lockQueue()
	{
		DWORD dwWaitResult = WaitForSingleObject(this->mutex, INFINITE);
		return (dwWaitResult == WAIT_OBJECT_0);
	}

	void unlockQueue()
	{
		ReleaseMutex(this->mutex);
	}

	void startWork()
	{
		thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)worker, (LPVOID)this, 0, NULL);
		if (thread == NULL)
		{
			printf("Ошибка при создании потока: %d\n", GetLastError());
			system("pause");
			exit(1);
		}
	}
};

Cashier chooseCashier(Customer customer);
bool addCashier(Customer customer);

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

	printf("Магазин начал свою работу. Ждем покупателей.");

	while (true)
	{
		// создаем канал и ждем очередного покупателя
		HANDLE hPipe = createPipe();
		waitingForCustomer(hPipe);

		// Читаем информацию по сумме
		if (!ReadFile(hPipe, buf, BUFSIZE, &dwBytes, NULL)) {
			printf("Ошибка получения данных от клиента.\n");
			system("pause");
			exit(1);
		}

		string msg(buf, 0, dwBytes);
		int sum = atoi(msg.c_str());

		Customer customer(++customerCount, hPipe, sum);
		Cashier cashier = chooseCashier(customer);
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
			printf("Новый покупатель\n");
			break;
		}
	}
}

Cashier chooseCashier(Customer customer)
{
	// если еще нет ни одного кассира за кассой зовем первого
	if (cashiers.size() == 0) {
		addCashier(customer);
		return cashiers.at(cashiers.size() - 1);
	}

	// если кассиры есть, выбираем случайного
	int cashierIndex;
	cashierIndex = rand() % cashiers.size();
	Cashier cashier = cashiers.at(cashierIndex);

	if (cashier.lockQueue())
	{

		if (cashier.queue.size() < 3 || !addCashier(customer))
		{
			cashier.queue.push(customer);
			return cashier;
		}

		cashier.unlockQueue();
	}
}

bool addCashier(Customer customer)
{
	if (cashiers.size() < cashiersMaxCount)
	{
		int cashier_id = cashiers.size() + 1;
		Cashier cashier(cashier_id, customer);
		cashiers.push_back(cashier);
		return true;
	}

	return false;
}

DWORD WINAPI worker(LPVOID lpParam)
{
	Cashier* cashier = (Cashier*)lpParam;

	while (true)
	{
		if (cashier->lockQueue())
		{
			if (cashier->queue.size() == 0) {
				Sleep(1000);
				continue;
			}
			Customer* customer = &cashier->queue.front();
			cashier->queue.pop();

			cashier->unlockQueue();


			printf("Обслуживаем покупателя %d на кассе %d", customer->id, cashier->id);
			Sleep(2000);
			cashier->total_sum += customer->sum;

			printf("Обслужили покупателя %d на кассе %d", customer->id, cashier->id);
			customer->send("Done");
			delete customer;
		}
	}
	printf("Завершаем обслуживание на кассе %d", cashier->id);
	system("pause");
	ExitThread(0);
}


