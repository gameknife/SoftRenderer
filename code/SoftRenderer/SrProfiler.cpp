/**
  @file SrProfiler.cpp
  
  @author Kaiming

  更改日志 history
  ver:1.0
   
 */
#include "stdafx.h"
#include "SrProfiler.h"

#include "mmgr/mmgr.h"
#if defined(__APPLE__)
#include <sys/sysctl.h>
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#elif defined(__linux__)
#include <fstream>
#endif

static std::string TrimCpuModelName(const std::string& value)
{
	const std::string::size_type first = value.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return std::string();
	const std::string::size_type last = value.find_last_not_of(" \t\r\n");
	return value.substr(first, last - first + 1);
}

static std::string DetectCpuModelName()
{
#if defined(__APPLE__)
	size_t size = 0;
	if (sysctlbyname("machdep.cpu.brand_string", NULL, &size, NULL, 0) != 0 || size == 0)
		return "Apple Silicon";
	std::vector<char> model(size);
	if (sysctlbyname("machdep.cpu.brand_string", model.data(), &size, NULL, 0) != 0)
		return "Apple Silicon";
	return TrimCpuModelName(model.data());
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
	int cpuInfo[4] = {};
	__cpuid(cpuInfo, 0x80000000);
	const unsigned int maxExtendedLeaf = static_cast<unsigned int>(cpuInfo[0]);
	if (maxExtendedLeaf < 0x80000004)
		return "Unknown CPU";

	int brandData[12] = {};
	for (int leaf = 0x80000002; leaf <= 0x80000004; ++leaf)
		__cpuid(brandData + (leaf - 0x80000002) * 4, leaf);

	char brand[49] = {};
	memcpy(brand, brandData, sizeof(brandData));

	const std::string model = TrimCpuModelName(brand);
	return model.empty() ? "Unknown CPU" : model;
#elif defined(_WIN32)
	const char* processor = std::getenv("PROCESSOR_IDENTIFIER");
	return processor && processor[0] ? TrimCpuModelName(processor) : "Unknown CPU";
#elif defined(__linux__)
	std::ifstream cpuInfo("/proc/cpuinfo");
	std::string line;
	std::string fallback;
	while (std::getline(cpuInfo, line))
	{
		const std::string::size_type colon = line.find(':');
		if (colon == std::string::npos)
			continue;
		const std::string key = TrimCpuModelName(line.substr(0, colon));
		const std::string value = TrimCpuModelName(line.substr(colon + 1));
		if (value.empty())
			continue;
		if (key == "model name")
			return value;
		if (fallback.empty() && (key == "Hardware" || key == "Processor" || key == "Model"))
			fallback = value;
	}
	return fallback.empty() ? "Unknown CPU" : fallback;
#else
	return "Unknown CPU";
#endif
}

const std::string& SrGetCpuModelName()
{
	static const std::string cpuModelName = DetectCpuModelName();
	return cpuModelName;
}

SrProfiler::SrProfiler()
	: m_tileBusyRatioSum(0.f), m_tileBalanceSum(0.f), m_tileBatchCount(0)
{
	m_profileElements.assign(ePe_Count, SrProfilerElement());
	m_profileElements[ePe_ZError].dontClear = true;
}

SrProfiler::~SrProfiler()
{
}


void SrProfiler::setBegin( EProfilerElement element )
{
	m_profileElements[element].prevTime = gEnv->timer->getRealTime();
}

void SrProfiler::setEnd( EProfilerElement element )
{
	SrProfilerElement& elementInst = m_profileElements[element];
	float now = gEnv->timer->getRealTime();
	elementInst.elapsedTime = now - elementInst.prevTime;
}


void SrProfiler::IncreaseTime( EProfilerElement element, float time )
{
	m_profileElements[element].elapsedTime += time;
}


void SrProfiler::setIncrement( EProfilerElement element, int count /*= 1 */ )
{
	m_profileElements[element].count += count;
}

float SrProfiler::getTime( EProfilerElement element )
{
	return m_profileElements[element].elapsedTime * 1000.f;
}

float SrProfiler::getAverageTime( EProfilerElement element )
{
	return m_profileElements[element].avgElapsedTime * 1000.f;
}

int SrProfiler::getCount( EProfilerElement element )
{
	return m_profileElements[element].count;
}

void SrProfiler::addWorkerStats(uint32 workerIndex, uint32 jobs, float busyMilliseconds)
{
	if (m_workerStats.size() <= workerIndex)
		m_workerStats.resize(workerIndex + 1);
	m_workerStats[workerIndex].jobs += jobs;
	m_workerStats[workerIndex].busyMilliseconds += busyMilliseconds;
}

void SrProfiler::addTileBatchStats(float busyRatio, float balance)
{
	m_tileBusyRatioSum += busyRatio;
	m_tileBalanceSum += balance;
	++m_tileBatchCount;
}

void SrProfiler::Update()
{
	for (int i = 0; i < ePe_Count; i++)
	{
		m_profileElements[i].Clear();
	}
	m_workerStats.clear();
	m_tileBusyRatioSum = 0.f;
	m_tileBalanceSum = 0.f;
	m_tileBatchCount = 0;
}
