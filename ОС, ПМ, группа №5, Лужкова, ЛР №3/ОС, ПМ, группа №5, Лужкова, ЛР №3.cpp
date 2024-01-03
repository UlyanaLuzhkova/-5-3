// ОС, ПМ, группа №5, Лужкова, ЛР №3.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <Windows.h>
#include <iostream>

using namespace std;

int arraySize = 0;
int* arr = nullptr;
CRITICAL_SECTION cs; //объект критической секции для синхронизации потоков
HANDLE* handleThreads;
HANDLE* handleThreadsAreStarted;
HANDLE* handleThreadsAreStopped;
HANDLE* handleThreadsAreExited; //указатели на массивы дескр пот и соб
HANDLE handleMutex; //дескриптор мьютекса

DWORD WINAPI marker(LPVOID threadIndex) //указатель на данные, передаваемые в поток
{
    WaitForSingleObject(handleThreadsAreStarted[(int)threadIndex], INFINITE); //заменить и сделать ручной сброс

    int markedNumbersCounter = 0;
    srand((int)threadIndex);

    while (true) {
        EnterCriticalSection(&cs); //вход в критическую секцию

        //здесь я генерирую случайное число, если оно равно 0 в массиве то помечаю его и увелисиваю счетчик пом чисел. выхожу из кр секции
        int randomNumber = rand() % arraySize;
        if (arr[randomNumber] == 0) {
            Sleep(5);
            arr[randomNumber] = (int)threadIndex + 1;
            markedNumbersCounter++;
            Sleep(5);
            LeaveCriticalSection(&cs);
        }

        //если число не равно 0 то вывожу информацию, выхожу из кр секции 
        else {
            cout << "\tNumber of threads: " << (int)threadIndex + 1 << "\n";
            cout << "\tAmount of marked element: " << markedNumbersCounter << "\n";
            cout << "\tIndex of an element that cannot be marked: " << randomNumber << "\n";
            cout << "\n";
            LeaveCriticalSection(&cs);

            SetEvent(handleThreadsAreStopped[(int)threadIndex]); //установка
            ResetEvent(handleThreadsAreStarted[(int)threadIndex]); //сброс

            HANDLE handleThreadStartedExitedPair[]{ handleThreadsAreStarted[(int)threadIndex], handleThreadsAreExited[(int)threadIndex] };

            if (WaitForMultipleObjects(2, handleThreadStartedExitedPair, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) { //если событие .. раньше, то
                EnterCriticalSection(&cs);
                for (size_t i = 0; i < arraySize; i++) {
                    if (arr[i] == (int)threadIndex + 1) {
                        arr[i] = 0;
                    }
                }
                LeaveCriticalSection(&cs);

                ExitThread(NULL); //обнуляю и завершаю поток
            }
            else {
                ResetEvent(handleThreadsAreStopped[(int)threadIndex]); //сбрасываю и цикл повторяю снова
                continue;
            }
        }
    }
}

int main()
{

    int amountOfThreads = 0;
    cout << "Enter the size of the element array: ";
    cin >> arraySize;
    arr = new int[arraySize] {};
    cout << "Enter the number of threads : ";
    cin >> amountOfThreads;

    InitializeCriticalSection(&cs);

    handleThreads = new HANDLE[amountOfThreads];
    handleThreadsAreStarted = new HANDLE[amountOfThreads];
    handleThreadsAreStopped = new HANDLE[amountOfThreads];
    handleThreadsAreExited = new HANDLE[amountOfThreads];
    handleMutex = CreateMutex(NULL, FALSE, NULL);

    //Здесь я создаю потоки и события и "назначаю" каждому потоку свое событие
    for (int i = 0; i < amountOfThreads; i++) {
        handleThreads[i] = CreateThread(NULL, 1, marker, (LPVOID)i, NULL, NULL);
        handleThreadsAreStarted[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
        handleThreadsAreStopped[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
        handleThreadsAreExited[i] = CreateEvent(NULL, TRUE, FALSE, NULL);

    }

    for (int i = 0; i < amountOfThreads; i++) {
        SetEvent(handleThreadsAreStarted[i]);
    }

    int amount_of_completed_threads = 0;
    bool* is_thread_exited = new bool[amountOfThreads] {};
    while (amount_of_completed_threads < amountOfThreads) { //пока не завершатся все потоки
        WaitForMultipleObjects(amountOfThreads, handleThreadsAreStopped, TRUE, INFINITE); //ожидание завершения событий

        handleMutex = OpenMutex(NULL, FALSE, NULL);

        cout << "The resulting array: ";
        for (int i = 0; i < arraySize; i++) {
            cout << arr[i] << " ";
        }
        cout << "\n";

        ReleaseMutex(handleMutex); //"освобождаю" мьютекс

        int stopMarkerId;
        cout << "Enter the number of the thread you want to stop:\n";
        cin >> stopMarkerId;
        stopMarkerId--;

        if (!is_thread_exited[stopMarkerId]) { //увеличиваю счетчик если поток не завершился
            amount_of_completed_threads++;
            is_thread_exited[stopMarkerId] = true;

            //уст событие для завершения потока. ожидаю завершение и закрываются дескрипторы
            SetEvent(handleThreadsAreExited[stopMarkerId]);
            WaitForSingleObject(handleThreads[stopMarkerId], INFINITE);
            CloseHandle(handleThreads[stopMarkerId]);
            CloseHandle(handleThreadsAreExited[stopMarkerId]);
            CloseHandle(handleThreadsAreStarted[stopMarkerId]);
        }

        handleMutex = OpenMutex(NULL, FALSE, NULL); //открываю мьютекс

        cout << "Array: ";
        for (int i = 0; i < arraySize; i++) {
            cout << arr[i] << " ";
        }
        cout << "\n";

        ReleaseMutex(handleMutex);

        for (int i = 0; i < amountOfThreads; i++) {
            if (!is_thread_exited[i]) {
                ResetEvent(handleThreadsAreStopped[i]);
                SetEvent(handleThreadsAreStarted[i]);
            }
        }
    }
    for (int i = 0; i < amountOfThreads; i++) {
        CloseHandle(handleThreadsAreStopped[i]);
    }
    DeleteCriticalSection(&cs);
    return 0;
}