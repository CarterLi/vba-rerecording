#ifndef VBA_LUA_H
#define VBA_LUA_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

enum LuaCallID
{
	LUACALL_BEFOREEMULATION,
	LUACALL_AFTEREMULATION,
	LUACALL_BEFOREEXIT,
	LUACALL_AFTERPOWERON,
	LUACALL_BEFOREPOWEROFF,
	LUACALL_BEFORESTATELOAD,
	LUACALL_AFTERSTATELOAD,
	LUACALL_BEFORESTATESAVE,
	LUACALL_AFTERSTATESAVE,

	LUACALL_COUNT
};
void CallRegisteredLuaFunctions(LuaCallID calltype);

enum LuaMemHookType
{
	LUAMEMHOOK_WRITE,
	LUAMEMHOOK_READ,
	LUAMEMHOOK_EXEC,
	LUAMEMHOOK_WRITE_SUB,
	LUAMEMHOOK_READ_SUB,
	LUAMEMHOOK_EXEC_SUB,

	LUAMEMHOOK_COUNT
};
void CallRegisteredLuaMemHook(unsigned int address, int size, unsigned int value, LuaMemHookType hookType);

enum LuaJoypadType
{
	LUAJOYPAD_USER,
	LUAJOYPAD_PLAYBACK,
	LUAJOYPAD_RECORD,
	LUAJOYPAD_LUA,
	LUAJOYPAD_GAME,

	LUAJOYPAD_COUNT
};

// Just forward function declarations

void VBALuaFrameBoundary();
int VBALoadLuaCode(const char *filename);
int VBAReloadLuaCode();
void VBALuaStop();
int VBALuaRunning();

int VBALuaUsingJoypad(int);
int VBALuaReadJoypad(int);
int VBALuaRemappingInputDisplay(int);
int VBALuaRemapInputDisplay(int, int, enum LuaJoypadType);
int VBALuaSpeed();
bool8 VBALuaRerecordCountSkip();

void VBALuaGui(uint8 *screen, int ppl, int width, int height);
void VBALuaClearGui();

char* VBAGetLuaScriptName();

// The engine scripts run on and the file it was loaded from.  The version is
// read out of the engine itself rather than out of the headers the emulator was
// built against, so a lua51.dll swapped in next to the exe is reported as it is.
char* VBALuaEngineVersion();
char* VBALuaEnginePath();

// And some interesting REVERSE declarations!
char *VBAGetFreezeFilename(int slot);

#endif // VBA_LUA_H
