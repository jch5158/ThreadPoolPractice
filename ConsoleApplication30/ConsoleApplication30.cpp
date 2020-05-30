#include <iostream>
#include <Windows.h>

#define WORK_MAX 100
#define THREAD_MAX 10

typedef void (*WORK) ();

DWORD AddWorkToPool(WORK work);
WORK GetWorkFromPool();
DWORD MakeThreadToPool(DWORD numOfThread);
void WorkerThreadFunction(LPVOID pParam);

struct WorkerThread
{
	HANDLE hThread;
	DWORD idThread;
};

struct ThreadPool 
{
	WORK workList[WORK_MAX]; // 추가된 함수들입니다.

	WorkerThread workerThreadList[THREAD_MAX]; // WorkerThread의 배열 리스트입니다.
	HANDLE workerEventList[THREAD_MAX]; // 쓰레드 이벤트 핸들 리스트입니다.

	DWORD idxOfCurrentWork; // 쓰레드의 front
	DWORD idxOfLastAddedWork; // 쓰레드의 rear

	DWORD threadIdx; // 풀에 존재하는 쓰레드 개수
};

static HANDLE mutex = NULL;
ThreadPool threadPool;

void InitMutex();
void DeInitMutex();
void AcquireMutex();
void ReleaseMutex();
void TestFunction();


int wmain(int argc, WCHAR* argv[]) 
{
	MakeThreadToPool(3);

	for (int iCnt = 0; iCnt < 100; iCnt++)
	{
		AddWorkToPool(TestFunction);

	}


	return -1;
}


DWORD AddWorkToPool(WORK work) {
	AcquireMutex(); // 뮤텍스를 wait함

	// 일의 최대치를 넘었을 경우
	if (threadPool.idxOfLastAddedWork >= WORK_MAX)  
	{
		wprintf_s(L"AddWorkToPool fail\n");
		return NULL;
	}

	// 일 리스트에 실행시키고자 하는 work를 추가한다. 
	threadPool.workList[threadPool.idxOfLastAddedWork] = work;

	// 일 추가 후 증가시킴
	threadPool.idxOfLastAddedWork++;

	for (int iCnt = 0; iCnt < threadPool.threadIdx; iCnt++) 
	{
		// 이벤트 오브젝트 객체가 Signaled 상태가 된다.
		SetEvent(threadPool.workerEventList[iCnt]); 
	}

	ReleaseMutex();
}


WORK GetWorkFromPool() 
{
	WORK work = NULL;

	AcquireMutex(); // 뮤텍스를 wait하는 함수입니다.

	if (!(threadPool.idxOfCurrentWork < threadPool.idxOfLastAddedWork))
	{
		ReleaseMutex();
		return NULL;
	}

	// fornt의 work값을 return한다.
	work = threadPool.workList[threadPool.idxOfCurrentWork++];

	ReleaseMutex();

	return work;
}

DWORD MakeThreadToPool(DWORD numOfThread)  // 3이 전달된다.
{
	InitMutex();
	
	DWORD capacity = WORK_MAX - (threadPool.threadIdx);  

	if (capacity < numOfThread) 
	{
		numOfThread = capacity;
	}

	for(DWORD iCnt = 0; iCnt < numOfThread; iCnt++)
	{
		DWORD idThread;
		HANDLE hThread;

		//자동 리셋 이벤트를 생성합니다. Non-signaled 상태로 생성된다.
		threadPool.workerEventList[threadPool.threadIdx] = CreateEvent(NULL, FALSE, FALSE, NULL);

		hThread = CreateThread
		(
			//threadPool.threadIdx == argv 매개변수라고 생각하면 된다.
			NULL, 0, (LPTHREAD_START_ROUTINE)WorkerThreadFunction, (LPVOID)threadPool.threadIdx, 0, &idThread
		);

		threadPool.workerThreadList[threadPool.threadIdx].hThread = hThread;
		threadPool.workerThreadList[threadPool.threadIdx].idThread = idThread;

		threadPool.threadIdx++;
	}

	return numOfThread;
}


void WorkerThreadFunction(LPVOID pParam)
{
	WORK workFunction;

	// threadIdx 번째 인덱스에 event 핸들을 저장했기 때문에 pParam으로 확인이 가능하다.
	HANDLE event = threadPool.workerEventList[(DWORD)pParam];

	while (1)
	{

		// 일을 받는다.
		workFunction = GetWorkFromPool();

		if (workFunction == NULL)
		{
			// 이벤트 오브젝트가 signaled가 되도록 기다린다.
			WaitForSingleObject(event, INFINITE);
			continue;
		}

		workFunction();
	}

}


void TestFunction()
{
	AcquireMutex();

	// 자꾸 동시 접근해서 ㅈㄹ 되는거구나 ㅋㅋㅋ
	static int I = 0;

	I++;

	wprintf_s(L"GoodTest : %d, Processing thread : %d \n", I, GetCurrentThreadId());

	ReleaseMutex();
}



void InitMutex() 
{
	mutex = CreateMutex(NULL, FALSE, NULL); // 보안속성, 꼭 뮤텍스 생성한 쓰레드가 아니여도 점유 가능, 뮤텍스 이름
}

void DeInitMutex() 
{
	CloseHandle(mutex);
}

void AcquireMutex() 
{
	DWORD ret = WaitForSingleObject(mutex,INFINITE);

	if (ret == WAIT_FAILED) {
		wprintf_s(L"Error Occur\n"); // 오류입니당.!
	}

}

void ReleaseMutex()
{
	DWORD ret = ReleaseMutex(mutex);

	if (ret == 0) {
		wprintf_s(L"Error Occur\n"); // 오류입니당.!
	}
}
