// MQ2Grid.cpp : Defines the entry point for the DLL application.
//

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup.

#include <mq/Plugin.h>

PreSetup("MQ2Grid");
PLUGIN_VERSION(0.1);


static inline PSPAWNINFO SpawnMe() {
	return pControlledPlayer;
}

static inline int InGame() {
	return (gbInZone && gGameState == GAMESTATE_INGAME && SpawnMe() && GetPcProfile() && GetCharInfo() && GetCharInfo()->Stunned != 3);
}


namespace utils {
	int GetSkillLevel(const int skill) {
		return GetPcProfile()->Skill[skill];
	}

	std::string GetArgString(const char* args, const int offset) {
		char arg[MAX_STRING];
		GetArg(arg, args, offset);
		return std::string{ arg };
	}

	float GetArgFloat(const char* args, const int offset) {
		char arg[MAX_STRING];
		GetArg(arg, args, offset);
		return (float)atof(arg);
	}

	int GetArgInt(const char* args, const int offset) {
		char arg[MAX_STRING];
		GetArg(arg, args, offset);
		return atoi(arg);
	}

	static char* sprintf(const char* szFormat, ...) {
		va_list vaList;
		va_start(vaList, szFormat);

		const int len = _vscprintf(szFormat, vaList) + 1 + 32;
		const auto out = std::make_unique<char[]>(len);
		char* szOutput = out.get();

		vsprintf_s(szOutput, len, szFormat, vaList);
		return szOutput;
	}

	static std::string sprintf(const std::string szFormat, ...) {
		va_list vaList;
		va_start(vaList, szFormat.c_str());

		const int len = _vscprintf(szFormat.c_str(), vaList) + 1 + 32;
		const auto out = std::make_unique<char[]>(len);
		char* szOutput = out.get();

		vsprintf_s(szOutput, len, szFormat.c_str(), vaList);
		return std::string{ szOutput };
	}

	static inline void EzCommandf(const char* szFormat, ...) {
		va_list vaList;
		va_start(vaList, szFormat);

		const int len = _vscprintf(szFormat, vaList) + 1 + 32;
		const auto out = std::make_unique<char[]>(len);
		char* szOutput = out.get();

		vsprintf_s(szOutput, len, szFormat, vaList);
		EzCommand(szOutput);
	}

	static EQZoneInfo* GetZoneByID(const EQZoneIndex id) { return pWorldData->ZoneArray[id]; }
	static EQZoneInfo* GetZoneByShortName(const char* name) { return GetZoneByID(GetZoneID(name)); }
	static EQZoneInfo* GetZoneByShortName(const std::string& name) { return GetZoneByID(GetZoneID(name.c_str())); }
	static EQZoneInfo* GetCurrentZone() { return GetZoneByID(GetCharInfo()->zoneId); }
}

struct Box {
	int row = 0;
	int col = 0;
	int width = 8;
	int height = 8;

	Box() = default;
	Box(const int row, const int col, const int width, const int height) : row(row), col(col), width(width), height(height) {}
};

struct Screen {
	Box box;
	int focus = 0;
};

struct Grid {
	int width = 160;
	int height = 87;

	Box focus;

	void apply(const Box& box) const {
		const HWND window = GetEQWindowHandle();
		WINDOWPLACEMENT placement{};
		GetWindowPlacement(window, &placement);

		const int top = box.row * height;
		const int left = box.col * width;

		const int right = left + box.width * width;
		const int bottom = top + box.height * height;

		if (placement.rcNormalPosition.left == left
			&& placement.rcNormalPosition.right == right
			&& placement.rcNormalPosition.top == top
			&& placement.rcNormalPosition.bottom == bottom) {
			return;
		}

		placement.rcNormalPosition.left = left;
		placement.rcNormalPosition.right = right;

		placement.rcNormalPosition.top = top;
		placement.rcNormalPosition.bottom = bottom;

		SetWindowPlacement(window, &placement);
	}
};

Grid grid;
Screen screen;

void apply() {
	const HWND window = GetEQWindowHandle();
	WINDOWPLACEMENT placement{};
	GetWindowPlacement(window, &placement);

	const Box& box = GetFocus() == window && screen.focus ? grid.focus : screen.box;

	const int top = box.row * grid.height;
	const int left = box.col * grid.width;

	const int right = left + box.width * grid.width;
	const int bottom = top + box.height * grid.height;

	if (placement.rcNormalPosition.left == left
		&& placement.rcNormalPosition.right == right
		&& placement.rcNormalPosition.top == top
		&& placement.rcNormalPosition.bottom == bottom) {
		return;
	}

	placement.rcNormalPosition.left = left;
	placement.rcNormalPosition.right = right;

	placement.rcNormalPosition.top = top;
	placement.rcNormalPosition.bottom = bottom;

	SetWindowPlacement(window, &placement);
}

void saveGrid() {
	const std::string file = INIFileName;
	const std::string section = GetServerShortName();

	const std::string key = "GRID";
	const std::string value = utils::sprintf("GRID %i %i %i %i %i %i", grid.width, grid.height, grid.focus.width, grid.focus.height, grid.focus.row, grid.focus.col);

	WritePrivateProfileString(section, key, value, file);
	WriteChatf("save grid %s", value.c_str());
}

void loadGrid() {
	const std::string file = INIFileName;
	const std::string section = GetServerShortName();

	const std::string key = "GRID";
	const std::string value = GetPrivateProfileString(section, key, "", file);

	if (value == "") {
		return;
	}

	const char* args = value.c_str();

	grid.width = utils::GetArgInt(args, 2);
	grid.height = utils::GetArgInt(args, 3);
	grid.focus.width = utils::GetArgInt(args, 4);
	grid.focus.height = utils::GetArgInt(args, 5);
	grid.focus.row= utils::GetArgInt(args, 6);
	grid.focus.col = utils::GetArgInt(args, 7);
}

void saveScreen() {
	const std::string file = INIFileName;
	const std::string section = GetServerShortName();

	const std::string key = GetCharInfo()->Name;
	const std::string value = utils::sprintf("%s %i %i %i %i %i", GetCharInfo()->Name, screen.box.width, screen.box.height, screen.box.row, screen.box.col, screen.focus);

	WritePrivateProfileString(section, key, value, file);
	WriteChatf("save box %s", value.c_str());
}

void loadScreen() {
	const std::string file = INIFileName;
	const std::string section = GetServerShortName();

	const std::string key = GetCharInfo()->Name;
	const std::string value = GetPrivateProfileString(section, key, "", file);

	if (value == "") {
		return;
	}

	const char* args = value.c_str();

	screen.box.width = utils::GetArgInt(args, 2);
	screen.box.height = utils::GetArgInt(args, 3);
	screen.box.row = utils::GetArgInt(args, 4);
	screen.box.col = utils::GetArgInt(args, 5);
	screen.focus = utils::GetArgInt(args, 6);
}

PLUGIN_API VOID GridCommand(PSPAWNINFO pChar, PCHAR args) {
	loadGrid();
	loadScreen();
	apply();
}

PLUGIN_API VOID BoxSizeCommand(PSPAWNINFO pChar, PCHAR args) {
	loadGrid();

	screen.box.width = utils::GetArgInt(args, 1);
	screen.box.height = utils::GetArgInt(args, 2);

	apply();

	saveScreen();
}

PLUGIN_API VOID BoxCellCommand(PSPAWNINFO pChar, PCHAR args) {
	loadGrid();

	screen.box.row = utils::GetArgInt(args, 1);
	screen.box.col = utils::GetArgInt(args, 2);

	apply();

	saveScreen();
}

std::map<std::string, void(*)(PSPAWNINFO pChar, PCHAR args)> commands{
	{"/grid", GridCommand},
	{"/size", BoxSizeCommand},
	{"/cell", BoxCellCommand},
};

void addCommands() {
	for (const auto& it : commands) {
		AddCommand(it.first.c_str(), it.second);
	}
}

void removeCommands() {
	for (const auto& it : commands) {
		RemoveCommand(it.first.c_str());
	}
}

class Timer {
	std::chrono::steady_clock::time_point point = std::chrono::steady_clock::now();
	std::chrono::milliseconds duration;

public:
	Timer(const std::chrono::milliseconds duration) : duration(duration) {}

	bool ready() {
		const auto now = std::chrono::steady_clock::now();
		if (now < point) {
			return false;
		}

		point = now + duration;

		return true;
	}
};

using namespace std::chrono_literals;

Timer pulse{ 250ms };

PLUGIN_API void InitializePlugin()
{
	addCommands();

	if (InGame()) {
		loadGrid();
		loadScreen();
		apply();
	}
}

PLUGIN_API void ShutdownPlugin()
{
	removeCommands();
}

PLUGIN_API void OnPulse()
{
	if (pulse.ready()) {
		if (InGame()) {
			apply();
		}
	}
}

PLUGIN_API void OnZoned()
{
	if (InGame()) {
		loadGrid();
		loadScreen();
		apply();
	}
}
