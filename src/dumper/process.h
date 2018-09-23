#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define DATE_BUILD_LIB
#define NOMINMAX
#include <Windows.h>
#include <deque>
#include <thread>
#include <memory>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <map>
#include <fstream>
#include <sstream>
#include <WinNls.h>
#include <ShObjIdl.h>
#include <objbase.h>
#include <ObjIdl.h>
#include <ShlGuid.h>

#include "Valve\IBaseClientDll.h"
#include "Date\date.h"
#include "timer.h"

class Process
{
public:
	void PreInit();
	void Init();
	void Signatures();
	void NetVars();
	void Output();

private:
	uint8_t * PatternScan(LPVOID module, const char* signature);
	CreateInterfaceFn GetModuleFactory(HMODULE module)
	{
		return reinterpret_cast<CreateInterfaceFn>(GetProcAddress(module, "CreateInterface"));
	}

	template<typename T>
	T* GetInterface(CreateInterfaceFn f, const char* szInterfaceVersion)
	{
		auto result = reinterpret_cast<T*>(f(szInterfaceVersion, nullptr));
		return result;
	}

	HMODULE			m_hEngine = nullptr;
	HMODULE			m_hPanorama = nullptr;

	uintptr_t		dwPlayerResource;
	uintptr_t		dwViewMatrix;
	uintptr_t		dwEntityList;
	uintptr_t		dwLocalPlayer;
	uintptr_t		dwBoneMatrix;
	uintptr_t		dwClientState;
	uintptr_t		dwLocalPlayerIndex;
	uintptr_t		dwViewAngle;
	uintptr_t		dwRadarBase;
	uintptr_t		dwRadarBasePointer;
	uintptr_t		dwRadarStructSize;
	uintptr_t		dwRadarStructPos;
	uintptr_t		dwEntitySize;

	uintptr_t		m_iCompetitiveRanking;
	uintptr_t		m_iKills;
	uintptr_t		m_iDeaths;
	uintptr_t		m_bHasDefuser;
	uintptr_t		m_iPlayerC4;
	uintptr_t		m_iHealth;
	uintptr_t		m_vecOrigin;
	uintptr_t		m_iTeamNum;
	uintptr_t		m_lifeState;
	uintptr_t		m_fFlags;
	uintptr_t		m_nForceBone;
	uintptr_t		m_aimPunchAngle;
	uintptr_t		m_bDormant;
	uintptr_t		m_iCrosshairId;
	uintptr_t		m_iShotsFired;
	uintptr_t		m_vecVelocity;
	uintptr_t		m_vecViewOffset;

	std::vector<std::pair<std::string, uintptr_t>> settings;
	std::unique_ptr<Timer> timer = std::make_unique<Timer>();
};

extern std::unique_ptr<Process> process;
extern IBaseClientDLL* g_CHLClient;