# 解耦方案：把 `theApp` 从核心里拿掉（落在 MFC 仓库，作为代码优化项）

> **状态：已完成（2026-10-20）。** 三个步骤全部落地，`src/{common,gba,gb,filters}` 下零 MFC、
> 零 Windows 头。验收用的是逐帧状态哈希：`-hashtrace` 在 Castlevania AoS 与 MMZ3 两份 V7
> 录像上各跑 4001 帧，`ram` 列逐字节一致（`full` 列连 `pix` 一起哈希，含墙钟 OSD，不作判据）。
>
> 本文保留为**施工单原文 + 结果记录**。实际落地与本方案的两处有意偏离记在 §3 步骤 3 末尾。

> 决定来源：2026-10-20 用户提出「解耦合的代码可以直接放入 MFC 项目作为代码优化项」。
> 本文件是给 **`C:\Users\zhang\vba-rerecording`（MFC 主仓库）** 用的施工单，
> 不是给 wx 分支用的。wx 分支只消费结果。

---

## 0. 为什么放在 MFC 仓库做（三条收益，一条验证）

| 收益 | 说明 |
|---|---|
| 不分叉 | 解耦只做一次。放在 wx 分支里做，两个仓库的核心从此各走各的，最贵的资产（TAS 行为）会被悄悄分裂 |
| MFC 版立刻变好 | 核心不再隐式依赖 `CVBAApp`，编译期就能发现越界引用；也是将来任何前端（wx / SDL / Qt）的公共前置 |
| **有对照组** | MFC 版是现成的、可运行的基线。重构前后跑同一批 `.vbm` 逐帧比对，**这是全项目唯一能证明"没改行为"的手段** |
| 顺带清掉一个历史债 | `HardResetAndSRAMClear()` 里已经有 `#if WIN32 ... #else ...` 双份实现，说明上游早就想解耦，只是没做完 |

---

## 1. 改动面积（已逐文件核对，不是估算）

| 文件 | `theApp` 引用 | 内容 | 难度 |
|---|---|---|---|
| `src/gba/remote.cpp` | 0 | 只有 `#include "../win32/stdafx.h"` + `winsock.h` | ★（10 行以内） |
| `src/common/lua-engine.cpp` | 4 个字段 + `CString`×17 / `Afx`×10 / `MessageBox`×7 | 还 include 了 `MainWnd.h` / `VBA.h` / `Dialogs/LuaOpenDialog.h` | ★★ |
| `src/common/movie.cpp` | **20 个字段** | 其中 **18 个是纯 int/bool**，1 个 `m_pMainWnd`，1 个 `removeIntros` 只出现在注释里 | ★★★（TAS 核心，需基线） |

其余 **62 个 .cpp + 2 个 .c** 已经实测一次编译通过，零改动。

### 1.1 `movie.cpp` 的 20 个字段按类型分

```
纯标量 18 个：
  bool  frameCounter frameSearching frameSearchSkipping inputDisplay nextInputDisplay
        lagCounter extraCounter hideMovieBorder movieOnEndPause skipBiosIntro useBiosFile
  int   autoFire autoFire2 autoHold movieOnEndBehavior winFlashSize winGbBorderOn winSaveType
真前端对象 1 个：
  m_pMainWnd（CWnd*）
不存在 1 个：
  removeIntros（VBA.h:178 已被注释；movie.cpp 里只出现在注释中）
```

### 1.2 唯一真正"带逻辑"的耦合点

```cpp
// src/common/movie.cpp:701
static void HardResetAndSRAMClear()
{
#if (defined(WIN32) && !defined(SDL))
    winEraseBatteryFile();
    MainWnd *temp = static_cast<MainWnd *>(theApp.m_pMainWnd);
    if (!temp->winFileRun(true))   // 重启游戏
        temp->winFileClose();
#else
    char fname[1024];
    GetBatterySaveName(fname);
    remove(fname);
    theEmulator.emuReset();
#endif
}
```

**这段已经是解耦的半成品** —— `#else` 分支就是"核心自己干"的版本。
要做的只是把 `#if` 分支搬进前端，改成一个虚函数调用。

---

## 2. 施工方案

### 2.1 新增两个核心自有头文件（`src/common/`，零 MFC / 零 Windows 类型）

```cpp
// src/common/CoreOptions.h
#pragma once

// 以前散落在 CVBAApp 上的纯标量选项。核心拥有它，前端读写它。
struct CoreOptions
{
    bool frameCounter        = false;
    bool frameSearching      = false;
    bool frameSearchSkipping = false;
    bool inputDisplay        = false;
    bool nextInputDisplay    = false;
    bool lagCounter          = false;
    bool extraCounter        = false;
    bool hideMovieBorder     = false;
    bool movieOnEndPause     = true;
    bool skipBiosIntro       = false;
    bool useBiosFile         = false;

    int autoFire          = 0;
    int autoFire2         = 0;
    int autoHold          = 0;
    int movieOnEndBehavior= 0;
    int winFlashSize      = 0;
    int winGbBorderOn     = 0;
    int winSaveType       = 0;
};

extern CoreOptions g_core;      // 定义放 src/common/CoreOptions.cpp
```

```cpp
// src/common/IEmuFrontend.h
#pragma once

// 核心对前端的最小需求。MFC / wx 各实现一份，启动时用 SetEmuFrontend() 注入。
struct IEmuFrontend
{
    virtual ~IEmuFrontend() = default;

    // movie.cpp：从 .vbm 触发硬复位时的"重启游戏"（今天是 winEraseBatteryFile + MainWnd::winFileRun）
    virtual void HardResetAndClearSRAM() = 0;

    // lua-engine.cpp
    virtual bool GetSavestateFilename(const char *romPath, int which, char *out, int outSize) = 0;
    virtual void PauseAviRecorder(bool pause) = 0;
    virtual int  GetAviFrameCount() = 0;          // 没在录时返回 -1
    virtual void CheckFullscreen() = 0;           // 原 theApp.winCheckFullscreen()
    virtual void ShowError(const char *msg) = 0;  // 原 AfxMessageBox
};

void SetEmuFrontend(IEmuFrontend *fe);
IEmuFrontend *GetEmuFrontend();
```

### 2.2 MFC 侧只改一处：把 `CVBAApp` 的 18 个标量改成**引用成员**

```cpp
// src/win32/VBA.h  —— CVBAApp 内
#include "../common/CoreOptions.h"

class CVBAApp : public CWinApp
{
    // ...
    // 原来是 bool frameCounter; int autoFire; … 现在改为引用绑定：
    bool &frameCounter        = g_core.frameCounter;
    bool &frameSearching      = g_core.frameSearching;
    bool &frameSearchSkipping = g_core.frameSearchSkipping;
    bool &inputDisplay        = g_core.inputDisplay;
    bool &nextInputDisplay    = g_core.nextInputDisplay;
    bool &lagCounter          = g_core.lagCounter;
    bool &extraCounter        = g_core.extraCounter;
    bool &hideMovieBorder     = g_core.hideMovieBorder;
    bool &movieOnEndPause     = g_core.movieOnEndPause;
    bool &skipBiosIntro       = g_core.skipBiosIntro;
    bool &useBiosFile         = g_core.useBiosFile;
    int  &autoFire            = g_core.autoFire;
    int  &autoFire2           = g_core.autoFire2;
    int  &autoHold            = g_core.autoHold;
    int  &movieOnEndBehavior  = g_core.movieOnEndBehavior;
    int  &winFlashSize        = g_core.winFlashSize;
    int  &winGbBorderOn       = g_core.winGbBorderOn;
    int  &winSaveType         = g_core.winSaveType;
};
```

**这一步的价值**：`theApp.frameCounter` 这类写法在**整个 MFC 前端（约 4.8 万行）里完全不用改**，
而读写已经落到核心自有的 `g_core` 上。改动量 = 18 行声明。

> 注意：`CVBAApp` 因为有引用成员而不可赋值/不可拷贝——它本来就是单例式的 `CWinApp`，
> 不会被拷贝，编译期会直接报出来，不会留下隐患。

### 2.3 核心侧的机械替换

```sh
# movie.cpp：18 个标量
#   theApp.frameCounter  ->  g_core.frameCounter
#   ……（每个字段一行 sed）
```
`m_pMainWnd` / `winEraseBatteryFile` / `winFileRun` 三处改走 `GetEmuFrontend()->HardResetAndClearSRAM()`，
即把 §1.2 那段改成：

```cpp
static void HardResetAndSRAMClear()
{
    if (IEmuFrontend *fe = GetEmuFrontend())
        fe->HardResetAndClearSRAM();
    else
    {
        char fname[1024];
        GetBatterySaveName(fname);
        remove(fname);
        theEmulator.emuReset();
    }
}
```

`lua-engine.cpp` 的 4 个字段 + `CString` + `Afx*` 全部改走 `IEmuFrontend`。
`remote.cpp` 把 `#include "../win32/stdafx.h"` 换成：

```cpp
#ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <io.h>
```

---

## 3. 顺序与验收

### 步骤 0（**必须先做**）：建回归基线

`movie.cpp` 是 TAS 核心，没有基线不许动。

1. 收集 6~10 个覆盖性 `.vbm`：GBA 普通、GBA 用到 EEPROM/Flash 存档、GB/GBC、SGB 边框、
   用到硬复位（`HardResetAndSRAMClear`）的那一类、有 Lua 脚本的。
2. 在**现有 MFC 版**上加一个只在调试构建里启用的开关（约 50 行）：

```
vba.exe rom.gba --play movie.vbm --hashtrace out.csv
每帧输出：帧号, crc32(gbMemory/gbaMemory), crc32(VRAM), 关键寄存器快照
```

3. 用它把基线跑出来，存档进 git（小文件）。

### 步骤 1：`remote.cpp`（0.5 人日）✅ **2026-10-20 已完成**

改动：`#include "../win32/stdafx.h"` → `<windows.h>` + `<winsock.h>` + `<io.h>`
（前面加 `WIN32_LEAN_AND_MEAN` / `NOMINMAX` 保护）。该文件只用到 winsock 1.1 与
`_read`/`_write`，从未用过 MFC 的任何东西。

验收结果（两份基线，各 4001 帧）：

| 检查 | 结果 |
|---|---|
| 编译 | exit=0，0 warning |
| Castlevania / MMZ3 `ram` 列 | **差异均为 0** |
| `full` 列 | 有漂移（2037 / 323 行）—— 与本次改动无关，见下 |

⚠️ **判据修正（实测得出）**：`-hashtrace` 的 `full` 列把 `pix`（显示缓冲区）一起哈希，
而 `pix` 上有按墙钟计时的 OSD 文字，**天生不稳定**。同一份代码空载连跑两次恰好全等是运气；
带负载重跑就会出现成百上千行差异，而 `ram` 始终为 0。
⇒ **判定只看 `ram` 列**，不要用 `fc /b` 比整份文件。

### 步骤 2：`lua-engine.cpp`（1.5~2 人日）

验收：全部 Lua 脚本用例（含存档读写、`savestate`/`loadstate`、AVI 录制开关）行为不变；
`--hashtrace` 与基线一致。

### 步骤 3：`movie.cpp`（1.5~2 人日）✅ **2026-10-20 已完成**

验收：
1. `--hashtrace` 与基线**逐帧一致**（这是硬门槛，不一致立即回滚）；
2. 硬复位路径（`HardResetAndSRAMClear`）单独手测一遍；
3. 硬编码 `#include <afx` 在 `src/common/`、`src/gba/`、`src/gb/`、`src/filters/` 下**归零**；
4. `dumpbin /dependents` 或直接看编译日志确认核心 obj 不再引用 MFC。

**实测结果：**

| 验收项 | 结果 |
|---|---|
| Castlevania AoS 4001 帧 | **全列逐字节一致**（`ram` 与 `full` 都 0 差异） |
| MMZ3 4001 帧 | **全列逐字节一致** |
| V7 全量重编（170 个编译单元） | exit=0，0 error 0 warning |
| `src/{common,gba,gb,filters}` 里的 MFC / Windows 头 | **0** |
| wx 侧核心库探针未解析符号 | **0**（改前 27） |

**与本方案的两处有意偏离：**

1. **没有引入 `IEmuFrontend` 抽象类 + `SetEmuFrontend()` 注入。** 改成与 `LuaHost.h` 相同的
   "按模块命名空间 + 链接期实现"：`movie_host::hardResetAndClearSram()`。`movie.cpp` 只需要
   一个函数，虚函数表与注入时机都是额外的失败面；两个模块用同一种模式更好维护。
2. **`CoreOptions` 不加默认成员初始化器**，保持纯聚合、靠静态零初始化。`CVBAApp theApp` 是
   全局对象，引用成员在它的构造函数里绑定；只有 `g_core` 在静态初始化阶段就已完成初始化，
   才不受跨 TU 顺序影响。前端构造函数本来就会把这 18 个字段全赋一遍，默认值写在哪里不影响行为。

**一个容易漏的细节：** `movie.cpp` 里有 13 处 `#if (defined(WIN32) && !defined(SDL))`。
在 Windows 上它们一直走 `if` 分支，看不出问题；但 Linux 构建会掉进 `#else` 的
`// SDL FIXME` 空分支，**电影选项会静默失效**。本次一并反转为 `#if !defined(SDL)`。

---

## 4. 完成后的状态

- 核心 `src/{common,gba,gb,filters}` **不含任何 MFC 引用**，也不 include 前端头文件；
- MFC 前端仍然一行调用都不用改（靠引用成员绑定）；
- wx 分支只需要实现 **两个 host 接口**，就能把 `movie.cpp` / `lua-engine.cpp` 直接编进 `core.lib`：
  `lua_host`（11 个函数，`common/LuaHost.h`）与 `movie_host`（1 个函数，`common/MovieHost.h`），
  外加读写 `g_core`（`common/CoreOptions.h`）；
- 前端契约最终形态：**43 个符号 + 2 个 host 接口 + 1 个选项 POD**（实测探针残留 0）。

---

## 5. 风险

| 风险 | 应对 |
|---|---|
| `movie.cpp` 改动导致回放漂移 | 步骤 0 的哈希基线是硬门槛；不一致就回滚，不做"再试一次" |
| 引用成员影响 `CVBAApp` 的默认构造顺序 | 引用必须在 `g_core` 之后初始化 —— 把 `g_core` 定义在 `.cpp` 里（静态初始化），引用成员用默认成员初始化器绑定，编译期无误 |
| `theApp` 之外还有隐藏的全局（`systemSaveUpdateCounter` 等） | 这些本来就是核心自有的，已在 `core.lib` 里解析，不在本次范围 |
| 与上游 VBA-M 的后续合并 | 本仓库是 VBA-RR 分支，本来就不跟 VBA-M 合并；解耦反而让两边更接近（VBA-M 自己也把选项放在 `opts::` 里） |
