#include "process.h"
#include "netvars.h"

#define STRINGIFY(s)	#s

std::unique_ptr<Process> process = std::make_unique<Process>();
IBaseClientDLL* g_CHLClient = nullptr;

//Create shortcut
HRESULT CreateLink(LPCWSTR lpszPathObj, LPCSTR lpszPathLink, LPCWSTR lpszDesc)
{
	HRESULT hres;
	IShellLink* psl;

	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
	if (SUCCEEDED(hres))
	{
		IPersistFile* ppf;

		psl->SetPath(lpszPathObj);
		psl->SetDescription(lpszDesc);

		hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

		if (SUCCEEDED(hres))
		{
			WCHAR wsz[MAX_PATH];
			MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, wsz, MAX_PATH);
			hres = ppf->Save(wsz, TRUE);
			ppf->Release();
		}
		psl->Release();
	}
	return hres;
}

void Process::PreInit()
{
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	std::wstring modulePath;
	modulePath.resize(MAX_PATH);
	GetModuleFileNameW(NULL, modulePath.data(), MAX_PATH);

	std::wstring dir = modulePath.c_str();
	dir = dir.erase(dir.find_last_of('\\') + 1, dir.back());
	SetCurrentDirectoryW(dir.c_str());

	std::ifstream csgo("csgo.exe");
	if (!csgo.good())
	{
		MessageBoxA(nullptr, "Please place dumper in the same folder as csgo.exe", "Error", MB_ICONERROR | MB_OK);
		ExitProcess(0);
	}
	csgo.close();

	std::ifstream lnk("C:\\PP-Multi\\Dumper.lnk");
	if (!lnk.good())
		CreateLink(modulePath.c_str(), "C:\\PP-Multi\\Dumper.lnk", nullptr);
	lnk.close();

	CoUninitialize();
}

void Process::Init()
{
	timer->StartTimer();
	SetConsoleTitleA("PP-Multi Dumper v2");
	auto load = [&](std::string path) -> HMODULE
	{
		auto mod = [&]() -> HMODULE {
			if (strstr(path.c_str(), "icui18n.dll"))//problematic dll; load w/o calling DllMain
				return LoadLibraryExA(path.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
			else
				return LoadLibraryA(path.c_str());
		};
		auto handle = mod();
		std::string modName = path.erase(0, path.find_last_of('\\') + 1);
		if (!handle)
			return nullptr;
		printf_s("%-38s: 0x%p\n", modName.c_str(), handle);
		return handle;
	};

	std::deque<std::string> modules;
	//too lazy to find dependency of client_panorama.dll; just gonna load everything
	for (const auto& it : std::experimental::filesystem::directory_iterator("./bin/"))
	{
		std::string fileName = it.path().string();
		if (strstr(fileName.c_str(), ".dll"))
			modules.push_back(fileName);
	}

	std::cout << std::setfill('=') << std::setw(50) << "" << std::endl;
	std::cout << "[Loading DLLs]" << std::endl;

	//keep loading dlls until client_panorama.dll can be loaded
	while (!m_hPanorama)
	{
		for (auto& it : modules)
		{
			if (!load(it))
			{
				modules.push_back(it);
				modules.pop_front();
				continue;
			}
			modules.pop_front();
		}
		m_hPanorama = LoadLibraryA("./csgo/bin/client_panorama.dll");
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	m_hEngine = GetModuleHandleA("engine.dll");

	auto clientFactory = GetModuleFactory(m_hPanorama);
	g_CHLClient = GetInterface<IBaseClientDLL>(clientFactory, "VClient018");
	netvars->Initialize();
}

//signatures provided by hazedumper
void Process::Signatures()
{
	std::cout << std::setfill('=') << std::setw(50) << "" << std::endl;
	std::cout << "[Signatures]" << std::endl;

	dwPlayerResource = (uintptr_t)PatternScan(m_hPanorama, "8B 3D ? ? ? ? 85 FF 0F 84 ? ? ? ? 81 C7");
	dwPlayerResource = *(uintptr_t*)(dwPlayerResource + 2);
	dwPlayerResource -= (uintptr_t)m_hPanorama;
	printf_s("%-38s: 0x%p\n", STRINGIFY(dwPlayerResource), dwPlayerResource);

	dwViewMatrix = (uintptr_t)PatternScan(m_hPanorama, "0F 10 05 ? ? ? ? 8D 85 ? ? ? ? B9");
	dwViewMatrix = *(uintptr_t*)(dwViewMatrix + 3);
	dwViewMatrix -= (uintptr_t)m_hPanorama;
	dwViewMatrix += 176;
	printf_s("%-38s: 0x%p\n", STRINGIFY(dwViewMatrix), dwViewMatrix);

	dwEntityList = (uintptr_t)PatternScan(m_hPanorama, "BB ? ? ? ? 83 FF 01 0F 8C ? ? ? ? 3B F8");
	dwEntityList = *(uintptr_t*)(dwEntityList + 1);
	dwEntityList -= (uintptr_t)m_hPanorama;
	printf_s("%-38s: 0x%p\n", STRINGIFY(dwEntityList), dwEntityList);

	dwLocalPlayer = (uintptr_t)PatternScan(m_hPanorama, "A3 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 59 C3 6A ?");
	dwLocalPlayer = *(uintptr_t*)(dwLocalPlayer + 1);
	dwLocalPlayer -= (uintptr_t)m_hPanorama;
	dwLocalPlayer += 16;
	printf_s("%-38s: 0x%p\n", STRINGIFY(dwLocalPlayer), dwLocalPlayer);

	dwClientState = (uintptr_t)PatternScan(m_hEngine, "A1 ? ? ? ? 33 D2 6A 00 6A 00 33 C9 89 B0");
	dwClientState = *(uintptr_t*)(dwClientState + 1);
	dwClientState -= (uintptr_t)m_hEngine;
	printf_s("%-38s: 0x%p\n", STRINGIFY(dwClientState), dwClientState);

	dwLocalPlayerIndex = (uintptr_t)PatternScan(m_hEngine, "8B 80 ? ? ? ? 40 C3");
	dwLocalPlayerIndex = *(uintptr_t*)(dwLocalPlayerIndex + 2);
	printf_s("%-38s: 0x%p\n", STRINGIFY(dwLocalPlayerIndex), dwLocalPlayerIndex);

	dwViewAngle = (uintptr_t)PatternScan(m_hEngine, "F3 0F 11 80 ? ? ? ? D9 46 04 D9 05");
	dwViewAngle = *(uintptr_t*)(dwViewAngle + 4);
	printf_s("%-38s: 0x%p\n", STRINGIFY(dwViewAngle), dwViewAngle);

	dwRadarBase = (uintptr_t)PatternScan(m_hPanorama, "A1 ? ? ? ? 8B 0C B0 8B 01 FF 50 ? 46 3B 35 ? ? ? ? 7C EA 8B 0D");
	dwRadarBase = *(uintptr_t*)(dwRadarBase + 1);
	dwRadarBase -= (uintptr_t)m_hPanorama;
	printf_s("%-38s: 0x%p\n", STRINGIFY(dwRadarBase), dwRadarBase);
}

void Process::NetVars()
{
	std::cout << std::setfill('=') << std::setw(50) << "" << std::endl;
	std::cout << "[NETVARS]" << std::endl;

	auto GetOffset = [&](const std::string& table, const std::string& prop) {
		uintptr_t offset = netvars->GetOffset(table, prop);
		printf_s("%-38s: 0x%p\n", prop.c_str(), offset);
		return offset;
	};

	m_iCompetitiveRanking		= GetOffset("DT_CSPlayerResource", STRINGIFY(m_iCompetitiveRanking));
	m_iKills					= GetOffset("DT_PlayerResource", STRINGIFY(m_iKills));
	m_iDeaths					= GetOffset("DT_PlayerResource", STRINGIFY(m_iDeaths));
	m_bHasDefuser				= GetOffset("DT_CSPlayer", STRINGIFY(m_bHasDefuser));
	m_iPlayerC4					= GetOffset("DT_CSPlayerResource", STRINGIFY(m_iPlayerC4));
	m_iHealth					= GetOffset("DT_BasePlayer", STRINGIFY(m_iHealth));
	m_vecOrigin					= GetOffset("DT_BaseEntity", STRINGIFY(m_vecOrigin));
	m_iTeamNum					= GetOffset("DT_BaseEntity", STRINGIFY(m_iTeamNum));
	m_lifeState					= GetOffset("DT_BasePlayer", STRINGIFY(m_lifeState));
	m_fFlags					= GetOffset("DT_BasePlayer", STRINGIFY(m_fFlags));
	m_nForceBone				= GetOffset("DT_BaseAnimating", STRINGIFY(m_nForceBone));
	m_aimPunchAngle				= GetOffset("DT_BasePlayer", STRINGIFY(m_aimPunchAngle));
	m_bDormant					= 0xE9;
	m_iCrosshairId				= m_bHasDefuser + 0x5C;
	m_iShotsFired				= GetOffset("DT_CSPlayer", STRINGIFY(m_iShotsFired));
	m_vecVelocity				= GetOffset("DT_BasePlayer", "m_vecVelocity[0]");
	m_vecViewOffset				= GetOffset("DT_BasePlayer", "m_vecViewOffset[0]");
	dwBoneMatrix				= m_nForceBone + 28;

	dwRadarBasePointer			= 0x6C;
	dwRadarStructSize			= 0x168;
	dwRadarStructPos			= 0x18;
	dwEntitySize				= 0x10;
}

void Process::Output()
{
	auto pair = [&](const std::string& str, uintptr_t var) {
		return std::pair<std::string, uintptr_t>(str, var);
	};

	std::cout << std::setfill('=') << std::setw(50) << "" << std::endl;
	std::cout << "[Final Output]" << std::endl;

	settings.push_back(pair(STRINGIFY(m_iCompetitiveRanking),	m_iCompetitiveRanking));
	settings.push_back(pair(STRINGIFY(m_iKills),				m_iKills));
	settings.push_back(pair(STRINGIFY(m_iDeaths),				m_iDeaths));
	settings.push_back(pair(STRINGIFY(m_bHasDefuser),			m_bHasDefuser));
	settings.push_back(pair(STRINGIFY(m_iPlayerC4),				m_iPlayerC4));
	settings.push_back(pair(STRINGIFY(dwPlayerResource),		dwPlayerResource));
	settings.push_back(pair(STRINGIFY(dwViewMatrix),			dwViewMatrix));
	settings.push_back(pair(STRINGIFY(dwEntityList),			dwEntityList));
	settings.push_back(pair(STRINGIFY(dwLocalPlayer),			dwLocalPlayer));
	settings.push_back(pair(STRINGIFY(m_iHealth),				m_iHealth));
	settings.push_back(pair(STRINGIFY(m_vecOrigin),				m_vecOrigin));
	settings.push_back(pair(STRINGIFY(m_iTeamNum),				m_iTeamNum));
	settings.push_back(pair(STRINGIFY(m_lifeState),				m_lifeState));
	settings.push_back(pair(STRINGIFY(m_fFlags),				m_fFlags));
	settings.push_back(pair(STRINGIFY(dwBoneMatrix),			dwBoneMatrix));
	settings.push_back(pair(STRINGIFY(m_aimPunchAngle),			m_aimPunchAngle));
	settings.push_back(pair(STRINGIFY(m_bDormant),				m_bDormant));
	settings.push_back(pair(STRINGIFY(dwClientState),			dwClientState));
	settings.push_back(pair(STRINGIFY(dwLocalPlayerIndex),		dwLocalPlayerIndex));
	settings.push_back(pair(STRINGIFY(m_iCrosshairId),			m_iCrosshairId));
	settings.push_back(pair(STRINGIFY(dwViewAngle),				dwViewAngle));
	settings.push_back(pair(STRINGIFY(m_iShotsFired),			m_iShotsFired));
	settings.push_back(pair(STRINGIFY(m_vecVelocity),			m_vecVelocity));
	settings.push_back(pair(STRINGIFY(m_vecViewOffset),			m_vecViewOffset));
	settings.push_back(pair(STRINGIFY(dwRadarBase),				dwRadarBase));
	settings.push_back(pair(STRINGIFY(dwRadarBasePointer),		dwRadarBasePointer));
	settings.push_back(pair(STRINGIFY(dwRadarStructSize),		dwRadarStructSize));
	settings.push_back(pair(STRINGIFY(dwRadarStructPos),		dwRadarStructPos));
	settings.push_back(pair(STRINGIFY(dwEntitySize),			dwEntitySize));

	std::string filePath;
	filePath.resize(36);
	sprintf_s(filePath.data(), 36, "C:\\PP-Multi\\Offsets[%s].txt", date::format("%F", std::chrono::system_clock::now()).c_str());

	std::ofstream file;
	file.open(filePath);

	std::string timestamp = date::format("%F %T", std::chrono::system_clock::now()) + " UTC";
	file << std::setfill('=') << std::setw(50) << "" << std::endl;
	file << timestamp << std::endl;
	file << "**Offset names may differ from others**" << std::endl;
	file << "**However it does not affect the offset it self**" << std::endl;
	file << std::setfill('=') << std::setw(50) << "" << std::endl;

	for (const auto& it : settings)
	{
		printf_s("Offsets.%-30s: 0x%0X\n", it.first.c_str(), it.second);
		file << "Offsets." << it.first << " = 0x" << std::uppercase << std::hex << it.second << std::endl;
	}
	file.close();
	
	std::cout << std::setfill('=') << std::setw(50) << "" << std::endl;
	std::cout << timestamp << std::endl;
	std::cout << filePath << std::endl;

	timer->StopTimer();
	std::cout << "Elapsed time: " << timer->GetElapsedTime() << "ms" << std::endl;
	std::cout << std::setfill('=') << std::setw(50) << "" << std::endl;
	system("pause");
}

//c+p'd from csgosimple
uint8_t* Process::PatternScan(LPVOID module, const char* signature)
{
	static auto pattern_to_byte = [](const char* pattern) {
			auto bytes = std::vector<int>{};
			auto start = const_cast<char*>(pattern);
			auto end = const_cast<char*>(pattern) + strlen(pattern);
			for (auto current = start; current < end; ++current) 
			{
				if (*current == '?') 
				{
					++current;
					if (*current == '?')
						++current;
					bytes.push_back(-1);
				}
				else
					bytes.push_back(strtoul(current, &current, 16));
			}
		return bytes;
	};

		auto dosHeader = (PIMAGE_DOS_HEADER)module;
		auto ntHeaders = (PIMAGE_NT_HEADERS)((uint8_t*)module + dosHeader->e_lfanew);
		auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
		auto patternBytes = pattern_to_byte(signature);
		auto scanBytes = reinterpret_cast<uint8_t*>(module);
		auto s = patternBytes.size();
		auto d = patternBytes.data();
		for (auto i = 0ul; i < sizeOfImage - s; ++i) 
		{
			bool found = true;
			for (auto j = 0ul; j < s; ++j) {
				if (scanBytes[i + j] != d[j] && d[j] != -1) 
				{
					found = false;
					break;
				}
			}
			if (found)
				return &scanBytes[i];
		}
	return nullptr;
}