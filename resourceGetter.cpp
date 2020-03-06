#include "stdafx.h"

resourceGetter::resourceGetter()
{
	// Total CPU
	PdhOpenQuery(NULL, NULL, &totalCpuQuery);
	PdhAddCounter(totalCpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &totalCpuCounter);
	PdhCollectQueryData(totalCpuQuery);

	// Process CPU
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	cpuCount = systemInfo.dwNumberOfProcessors;

	GetSystemTimeAsFileTime((LPFILETIME)&currentTotalTime);
	prevTotalTime = currentTotalTime;

	thisProcessHandle = GetCurrentProcess();
	GetProcessTimes(thisProcessHandle, (LPFILETIME)&trashVal, (LPFILETIME)&trashVal, (LPFILETIME)&currentKernelTime, (LPFILETIME)&currentUserTime);
	prevKernelTime = currentKernelTime;
	prevUserTime = currentUserTime;
}

void resourceGetter::calProcessCPU()
{
	GetSystemTimeAsFileTime((LPFILETIME)&currentTotalTime);
	GetProcessTimes(thisProcessHandle, (LPFILETIME)&trashVal, (LPFILETIME)&trashVal, (LPFILETIME)&currentKernelTime, (LPFILETIME)&currentUserTime);

	double Now, Kernel, User;

	Now = currentTotalTime.QuadPart - prevTotalTime.QuadPart;
	Kernel = currentKernelTime.QuadPart - prevKernelTime.QuadPart;
	User = currentUserTime.QuadPart - prevUserTime.QuadPart;
	
	cpuValue = (((Kernel + User) / Now) / cpuCount) * 100.0f;

	prevTotalTime = currentTotalTime;
	prevKernelTime = currentKernelTime;
	prevUserTime = currentUserTime;
}

void resourceGetter::calProcessMEM()
{
	PROCESS_MEMORY_COUNTERS_EX memoryCounter;
	K32GetProcessMemoryInfo(thisProcessHandle, (PROCESS_MEMORY_COUNTERS*)&memoryCounter, sizeof(memoryCounter));
	memValue = memoryCounter.WorkingSetSize;
}

void resourceGetter::calTotalCPU()
{
	PDH_FMT_COUNTERVALUE counterVal;

	PdhCollectQueryData(totalCpuQuery);
	PdhGetFormattedCounterValue(totalCpuCounter, PDH_FMT_DOUBLE, NULL, &counterVal);
	totalCPUValue = counterVal.doubleValue;
}

void resourceGetter::calTotalMEM()
{
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);
	totalMemValue = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
}

void resourceGetter::calProcessResourceValue()
{
	calProcessCPU();
	calProcessMEM();
}

void resourceGetter::calTotalResourceValue()
{
	calTotalCPU();
	calTotalMEM();
}