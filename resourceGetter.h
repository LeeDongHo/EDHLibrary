#pragma comment(lib,"pdh.lib")
#include <Psapi.h>
#include <Pdh.h>
class resourceGetter
{
private:
	double	cpuValue;
	double	memValue;
	double	totalCPUValue;
	double	totalMemValue;
	// CPU HANDLE (Total)
	PDH_HQUERY		totalCpuQuery;
	PDH_HCOUNTER	totalCpuCounter;

	// CPU HANDLE (Process)
	HANDLE				thisProcessHandle;
	unsigned int		cpuCount;
	ULARGE_INTEGER			currentTotalTime, currentKernelTime, currentUserTime;
	ULARGE_INTEGER			prevTotalTime, prevKernelTime, prevUserTime;
	ULARGE_INTEGER			trashVal;

private:
	void calProcessCPU();
	void calProcessMEM();

	void calTotalCPU();
	void calTotalMEM();
public:
	resourceGetter();
	~resourceGetter() {};

	void		calProcessResourceValue(void);
	void		calTotalResourceValue(void);
	double	getProcessCPU(void) { return cpuValue; }
	double	getProcessMem(void) { return memValue/1024.0f/1024.0f; }
	double	getTotalCPU(void) { return totalCPUValue; }
	double	getTotalMem(void) { return totalMemValue/1024.0f/1024.0f; }
};