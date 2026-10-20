# 迁移到 wxWidgets 的可行性分析（VBA-RR / Win32 前端）

分析对象：`C:/Users/zhang/vba-rerecording`（VBA 1.7.2 rerecording 分支）
候选库：`C:/Users/zhang/wxWidgets-3.3.4`
日期：2026-10-20

---

## 0. 结论

**技术上可行。而且有一个改变全局的事实：上游 `visualboyadvance-m/src/wx` 就是一套
wxWidgets 跨平台前端（Windows / Linux 含 Wayland / macOS / Android / 树莓派），
带 58 个 XRC 对话框、整棵菜单、配置体系、i18n，以及 macOS 专用的 `macsupport.mm` + Metal 渲染层。
所以这件事不是"从零重写 4.8 万行"，而是"移植 + 补 TAS 功能"。**（详见 §5.1）**

三条判断分开说：

| 问题 | 答案 |
|---|---|
| 能不能迁？ | 能。核心（`src/common`、`src/gba`、`src/gb`、`src/filters`）可移植，只有**两个**文件带平台头：`src/common/ImageWrite.cpp`（`windows.h`）、`src/gba/remote.cpp`（`winsock.h` + `SOCKET`）。 |
| 值不值得现在做？ | **值得，但要走 A′（移植上游前端）而不是从零写。** 从零写 110~169 人日；A′ 约 **66~109** 人日（含回归）。省下来的是"搭 UI"，不是"接核心"和"回归"。 |
| 跨平台的真正大头在哪？ | 不在 wx。6 个渲染后端里 4 个（GDI / DirectDraw / D3D9 / D3D11）纯 Windows，音频两个后端、输入（DirectInput）、AVI（VFW）、配置（注册表）、本地化（`.lng` 资源 DLL）全是 Windows 专属。**换 wx 只换壳。** |

**推荐顺序（修正后）：**

1. **A′ 路线：移植上游 wx 前端**——**≈ 66~109 人日**，一套代码三平台，macOS 上是**真 NSControl**。
   这是唯一同时满足"跨平台"和"真原生"的路线，也是你直觉想要的那条。
   第一刀建议 **A′1+A′2**（搬进来 + 核心接口对齐，13~22 人日），能立刻在 Windows 上看到可运行的 wx 版。
2. **B 路线：新增 SDL3 + ImGui 前端**——24~37 人日，Windows 零风险，**但 UI 完全不原生**
   （与你"要原生感"的诉求直接冲突，所以排第二）。它的第一步 B0（SDL 三后端，8~12 人日）仍有价值。
3. **C 路线：Qt 6**——85~130 人日，比 A′ 还贵，且 macOS 上不是真控件。**现在没有理由选它。**
4. **E 路线：手写 Cocoa**——90~140 人日，只覆盖 macOS，无参考实现。除非"最后 10% 原生感"对你值 90 人日。

**一个不在路线图里、但成本最低的动作**：把默认渲染方法从 `DIRECT_DRAW` 改掉（§4.7）。
你的默认值现在是 WINE 上最差的那条路，改一行就比接 SDL 更"对 WINE 友好"。

**明确不建议的三件事**：① 渲染器上 Vulkan（§5.3）；② 手写 Cocoa（§6 E，等 A′ 落地后再评估）；
③ 从零写 wx 前端（既然上游有，没必要）。

---

## 1. 基线（本次实测，非估算）

### 1.1 代码规模

| 类别 | 文件 | 行数 |
|---|---|---|
| 主窗口 / 菜单 / 命令 / 加速键（`MainWnd*`、`VBA.cpp`、`Commands.cpp`、`CmdAccelOb.cpp`、`AcceleratorManager.cpp`） | 14 | 10,415 |
| 对话框（`src/win32/Dialogs/`） | 91（.cpp/.h 成对） | 21,913 |
| 自绘控件（`src/win32/Controls/`） | 14 | 1,502 |
| 渲染后端（GDI / DDraw / D3D9 / D3D11 / GL / GL3.3） | 6 | 6,817 |
| 平台服务（输入 / 音频 / 配置 / 本地化 / 字体 / AVI / 杂项） | 22 | 5,654 |
| **合计（不含 upscalers 着色器头与 7zip）** | **158** | **48,203** |

### 1.2 MFC 耦合度（`src/win32` 全量 grep）

| 项目 | 数量 | 说明 |
|---|---|---|
| `afx_msg` 成员函数 | 1,111 | 每个都要改签名（`(UINT nID)` / `(CCmdUI*)` / 无参 → `(wxXxxEvent&)`） |
| `DECLARE_MESSAGE_MAP` / `BEGIN_MESSAGE_MAP` | 61 | 61 个类，含主窗、6 个 `MainWnd*.cpp` 分部、45 个对话框 |
| `ON_COMMAND` | 259 | → `Bind(wxEVT_MENU, ...)` |
| `ON_UPDATE_COMMAND_UI` | 251 | → `Bind(wxEVT_UPDATE_UI, ...)`，语义最接近的一块 |
| `CCmdUI` | 495 | 每个更新处理器里平均 2 次 `pCmdUI->Enable/SetCheck/SetRadio` |
| `CDialog` | 245 | → `wxDialog` |
| `CWnd` | 157 | → `wxFrame` / `wxWindow` / `wxControl` |
| `CString` | 439 | MBCS `CStringA` → `wxString`（UTF-16），`Format("%s", char*)` 全部要改 |
| `CDC` / `CBrush` / `CPen` / `CFont` | 31 / 37 | → `wxDC` / `wxBrush` / `wxPen` / `wxFont` |
| 菜单 / 对话框 / 字符串资源 | 51 个 `DIALOG(EX)`、501 个 `MENUITEM`、324 条加速键、202 条 `IDS_*` | 全在 `src/win32/vba.rc`（142 KB） |
| 运行时加载字符串 | 132 处 `winResLoadString()` | 走 `AfxFindResourceHandle` → 语言 DLL（`.lng`） |

### 1.3 构建与产物

- 只有 `win32/VBA_Latest.vcxproj`，8 个配置（V72D/V72R/V82D/V82R × Win32/x64），C++17，`SubSystem=Windows`。
- **`UseOfMfc=Static`**（`nafxcw.lib`），**`CharacterSet=MultiByte`**（不是 Unicode）。
- 产物 `VBA_VS_Latest_V82XR.exe` = 7.64 MB，`V72XR` = 7.85 MB。
- wxWidgets 3.3.4 目录里**没有任何编译好的库**（`lib/` 只有 `abicheck.sh` 和两个 VMS 选项文件），
  也就是说"用 wx"的第一天要先花时间把它编出来（`nmake -f build\msw\makefile.vc` 或 CMake）。

---

## 2. 现状架构：三层，只有最上面一层是要换的

```
┌──────────────────────────────────────────────────────────────┐
│ 表现层  MFC（CWinApp / CWnd / CDialog / CMenu / CCmdUI）      │  ← wx 只能替换这一层
│ 10.4k 行主窗+菜单  21.9k 行对话框  1.5k 行自绘控件            │
├──────────────────────────────────────────────────────────────┤
│ 抽象缝  IDisplay(6 实现) · ISound(2) · Input(DirectInput)     │  ← 已经是接口，不用动
│        Reg.cpp(注册表+ini 双写) · winResLoadString(本地化)     │
├──────────────────────────────────────────────────────────────┤
│ 平台后端  D3D9 / D3D11 / DDraw / GDI / WGL · DSound · XAudio2 │  ← 跨平台的真正工作量在这里
│          DirectInput · VFW AVI · 注册表 · .lng 资源 DLL        │
├──────────────────────────────────────────────────────────────┤
│ 模拟器核心  src/common · gba · gb · filters（可移植，零改动）  │
└──────────────────────────────────────────────────────────────┘
```

这张图就是本分析最重要的一句话：**wx 换掉的是最上面那一层，而被它挡住的下面两层才是"跨平台"的全部内容。**
所以"迁到 wx"和"能跨平台"之间没有因果关系。

已经存在的两个抽象缝（`Display.h` 的 `IDisplay`、`Sound.h` 的 `ISound`）是这次迁移最大的资产：
渲染后端不需要重写，只需要换一种方式把 HWND 递给它们。

---

## 3. "MFC 与 wx 语法很像"——像到什么程度

**像在形状，不像在语义。**逐项对照（左边是本仓库真实代码的形态）：

| MFC（本仓库） | wx 3.3 | 差异性质 |
|---|---|---|
| `BEGIN_MESSAGE_MAP(MainWnd, CWnd)` + `ON_COMMAND(ID, fn)` | `Bind(wxEVT_MENU, &fn, this, ID)` | **位置变了**：从类外宏改成构造/初始化时注册 |
| `afx_msg void OnFileOpen();` | `void OnFileOpen(wxCommandEvent&);` | **签名全变**，1111 个都要改 |
| `afx_msg void OnUpdateFilePause(CCmdUI* p)` + `p->Enable(b)` | `void OnUpdate(wxUpdateUIEvent& e)` + `e.Enable(b)` | 语义几乎 1:1，最省事的一块 |
| `CDialog` + `.rc` 模板 + `DDX_Text` + `UpdateData()` | `wxDialog` + sizer 代码 + `GetValue()/SetValue()` | **资源模板没有对应物**（见下） |
| `CString::Format("%s\\x.ini", p)` | `wxString::Format("%s", wxString(p))` | `%s` 不再接受 `char*`，最容易批量踩坑 |
| `CWinApp::OnIdle`（**仿真主循环在这里**） | `wxEVT_IDLE` + `evt.RequestMore()` | 语义接近，但**事件只发给顶级窗**，必须绑到 `wxApp` 上 |
| `CRect/CPoint/CSize`、`CMenu`、`CDC` | `wxRect/wxPoint/wxSize`、`wxMenu`、`wxDC` | 基本可用 |

最关键的一条差异不在语法表里：**wxWidgets 没有"把 `.rc` 的 DIALOGEX 模板变成一个可用的 wx 对话框"的能力。**

准确地说：`wxTopLevelWindowMSW::CreateDialog(const void* dlgTemplate, ...)`
（`wxWidgets-3.3.4/src/msw/toplevel.cpp:339`，`::CreateDialogIndirect`）是**存在**的，
但它是 protected、给 wx 自己内部用的，而且——

- 模板里的控件由 Win32 对话框管理器直接创建成**裸 HWND，没有对应的 wx 控件对象**，
  `GetDlgItem()` 拿到的是 HWND，只能用 `wxNativeWindow` 一个个包，等于自己重写一套 DDX；
- 这条路径**只在 MSW 上成立**，`#ifdef __WXMSW__` 一包，这块代码就永久 Windows-only，
  等于用"保留 `.rc`"换掉"跨平台"。

所以结论不变：51 个对话框、约 400 个控件（184 按钮 + 187 复选/单选 + 5 下拉 + 8 列表 + 8 SysListView）
必须**逐个用代码重新搭**，`.rc` 里 142 KB 的布局信息作废。
（若只想"先跑起来再慢慢换"，可以让遗留对话框以**裸 Win32 无模式窗**形式挂在新壳里，
但代价见 §7 的 P0.5。）

反过来，wx 也不是全无优势：`wxEVT_UPDATE_UI` 与 `ON_UPDATE_COMMAND_UI` 的对应关系已经很接近，
251 个菜单灰化逻辑可以近乎机械地搬；`wxFileDialog` 在 MSW 上就是 `IFileDialog`，
和现有 `ModernFileDialog.cpp`（554 行，`IFileDialogEvents`）行为天然对齐，这份自研代码可以删掉。

---

## 4. 六个硬点（按"被低估的程度"排序）

### 4.1 编码模型：MBCS → Unicode（最容易被低估）

现状：`CharacterSet=MultiByte`，全项目 `char*` + `CString`(A) + Win32 A 函数 + `.lng` 资源 DLL 里的 ANSI 字符串。
wx 3.x **只有 Unicode 构建**，没有 ANSI 版本可选。于是：

- 439 处 `CString` 要变成 `wxString`（UTF-16），`Format("%s", ...)` 的窄/宽混用逐个排查；
- 文件路径、ROM 名、`.vbm` 电影文件名、Lua 脚本路径、`.lng` 语言名，全在边界上；
- **本地化资源从 ANSI 到 UTF-16 的转换是行为变更**：现在依赖系统 ACP，迁移后必须显式指定代码页，
  否则日文/中文翻译会变乱码。

对策：内部仍保留 `char*`/`std::string`，只在 UI 边界转换（`wxString::FromUTF8` / `wxConvLocal`）。
这样改动面可控，但"完全统一成 wxString"的诱惑必须顶住，否则会波及核心。

**风险：高。** 这块不出问题则已，出问题就是"某些语言/某些路径下乱码"，回归很难覆盖全。

### 4.2 资源体系：`.rc` + `.lng` → wx 资源/翻译

- 51 个对话框模板作废（见 §3）。
- 501 个菜单项、324 条加速键要重建（`wxMenuBar` + `wxAcceleratorTable`，可行，但是纯手工）。
- 202 条 `IDS_*` + 132 处运行时取串，现在靠 `.lng` 资源 DLL（`AfxFindResourceHandle` 切模块句柄）实现
  热切换语言。wx 的对应物是 gettext（`.po/.mo`）+ `wxLocale`。
- **翻译流程会变**：现在译者拿到的是一个 `.lng` DLL（多半用 Resource Hacker 之类改）；
  换成 `.po` 后可以用 Poedit/Weblate/Crowdin——工具链其实更好，但**所有语言的存量翻译要重新灌一遍**，
  而且对话框布局改变后译文长度也需要重新校对。

**风险：高。**（不是技术风险，是社区/流程风险：这块会让现有译者全部返工。）

### 4.3 仿真主循环挂在 `OnIdle` 上

`VBA::OnIdle`（`VBA.cpp:1370`）里直接跑 `emulator.emuMain(emulator.emuCount)`——
**整个模拟器是跑在 MFC 空闲处理里的**，这是本项目的骨架，不是细节：

```cpp
else if (emulating && active && !paused) {
    emulator.emuMain(emulator.emuCount);
    saveRewindStateIfNecessary();
    ...
    return TRUE;      // 继续 idle
}
```

wx 能做等价的事（`wxEVT_IDLE` + `RequestMore()`），我核对了 3.3.4 的源码，四个事实：

1. **分发路径**：`wxAppBase::ProcessIdle()`（`src/common/appcmn.cpp:408`）**先**把 idle 发给
   `wxTheApp` 本身，**然后**遍历 `wxTopLevelWindows` 逐个 `SendIdleEvents()`（并递归到子窗）。
   所以绑在 frame 上也能收到，但每帧要过一遍所有顶级窗——VBA 同时开 10 个调试窗时这笔开销是白付的。
   ⇒ **绑在 `wxApp` 上**，一次到位。
2. **循环条件**：`wxEventLoopManual::Run()`（`src/common/evtloopcmn.cpp:275`）是
   `while (!m_shouldExit && !Pending() && !HasPendingEvents() && ProcessIdle())`。
   也就是说 "只有消息队列空了才跑 idle"——**这一点与 MFC 的 `OnIdle` 语义一致**
   （`VBA.cpp:1378` 那边也是 `PeekMessage(PM_NOREMOVE)` 判空），但 wx 多一个 `HasPendingEvents()` 条件。
3. **模态对话框期间仿真是否继续**：`wxDialog::ShowModal` 用的是 `wxModalEventLoop`
   （派生自 `wxEventLoopManual`），**同样走上面那个 Run 循环**，所以 idle 会被继续分发，
   仿真不会停。这一点与现版行为一致（现在打开内存查看器时游戏照跑），**但仍需 T3 实测**，
   因为"父窗被 disable 时 idle 是否照发"属于实现细节。
4. **固定开销**：`RequestMore()` 会让上面那个 while 每帧再转一圈，含一次全量 idle 分发。
   60 fps 下影响小，但**节流（throttle）与音频同步靠墙钟**，抖动会体现在音画同步上，必须实测。
5. TAS 的**确定性不受影响**（回放是逐帧喂输入，不依赖墙钟），但"手感/速度"会变，
   这是用户能立刻感知的回归面。

**风险：中。** 技术上等价，工程上需要重排一次启动/关闭/暂停状态机。

### 4.4 渲染：6 个后端 + HWND + 全屏 + 模式切换

好消息：`IDisplay` 接口已经把渲染隔开了（`Display.h`），6 个后端 6.8k 行**基本不用改**——
它们要的只是一个 HWND（`OpenGL.cpp:128` 的 `ChoosePixelFormat`、`Direct3D11.cpp:629` 的 `CreateDevice`
都是拿主窗 HWND/DC 干活）。wx 里 `wxWindow::GetHWND()` 直接给。

坏消息在两处：

- **窗口重建逻辑**：`VBA::initDisplay()`（`VBA.cpp:1780`）在"窗口化 ↔ 全屏"切换时会
  `DestroyWindow()` + `new MainWnd` 重建整个主窗（因为要换 `WS_POPUP` 风格）。
  在 wx 里这么做等于在事件处理中途销毁 `wxFrame`，而 idle 里还在跑仿真 → 悬垂指针。
  **必须改成 `ShowFullScreen()`**，这是对 `MainWnd` 生命周期的一次真重构，不是搬代码。
- 全屏模式切换（`ChangeDisplaySettingsEx`、`SystemParametersInfo(SPI_GETWORKAREA)`、
  `AdjustWindowRectEx`）与 wx 的 `wxDisplay`/`ShowFullScreen` 两套机制会打架，
  建议**保留原生那套**，只在 wx 里把主窗当哑容器。

**风险：中。** 工作量不大，但这是"必崩"级别的坑，必须先做原型验证。

补一条非 Windows 的账：**6 个后端里能过到 Linux 的只有 `OPENGL` 和 `OPENGL33`**
（GDI/DDraw/D3D9/D3D11 全得砍，Windows 侧原样保留）。而这两个后端的"现成"程度是：
`src/win32/upscalers/libretro` 下的 GLSL 着色器与整条采样/上采样管线**可直接复用**，
`wglChoosePixelFormatARB` / `wglCreateContextAttribsARB` / `wglSwapIntervalEXT`
（`OpenGL3.cpp:191-226`）这一圈引导要换成 `GLX_ARB_create_context` 或 EGL，
再加上 `glXSwapIntervalEXT/MESA/SGI` 的探测——**约 150~250 行胶水，不是重写后端**。
所以"OpenGL 现成可用"这个判断我认为成立，只需要把"全屏/分辨率切换"从 `MainWnd` 里摘出来。

### 4.5 输入 / 音频 / AVI / 配置：wx 一件都帮不上

- **输入**：`DirectInput.cpp`（1,248 行）用 `LPDIRECTINPUT` + `GetDeviceState` **每帧轮询**，
  不走窗口消息。这是好事（换成 wx 完全不用改），但也意味着**换框架对输入毫无收益**。
- **音频**：DirectSound / XAudio2 各有自己的线程（`XAudio2Sound.cpp:478` 回调）。
  wxSound 是给"放个 wav"用的，低延迟仿真音频完全不能用 → 两个后端原样保留。
  ⚠️ 但这里有一条**比换框架重要得多**的事实：`DirectSound::write()`（`DirectSound.cpp:449-470`）
  在 `!theApp.useOldSync` 时是**忙等 `GetCurrentPosition()` 来给仿真器配速**的——
  也就是说**音频设备就是整个模拟器的节流主时钟**，A/V 同步靠"音频播到哪了就放下一帧"。
  这条决定了：换任何音频后端都不是"接上就能用"，而是要**重新设计配速回路**
  （OpenAL 的 `AL_BUFFERS_PROCESSED` 粒度是"整块播完"，没有 DS 那样的播放游标，
  照抄会从"精确配速"退化成"整块粒度配速"，表现为音画漂移）。这也是 TAS 项目最不能糊弄的地方。
- **AVI 录制**：VFW（`vfw32.lib`），Windows 专属，原样保留。
- **配置**：`Reg.cpp` 已经是"注册表 + `vba.ini` 双写"的自研薄封装，25 个 API 调用。
  可以映射到 `wxConfig`，也可以原样保留——**这是全项目最不用动的一块**。

结论：**这 5.6k 行里 95% 在 wx 下原样编译通过。** 它们既是"wx 帮不上忙"的证据，
也是"迁 wx 不会更跨平台"的证据。

### 4.6 手写的深色模式与 DPI 会与新框架打架

- `DarkMode.cpp`（349 行）是纯 Win32 手艺活：`SetWindowSubclass()` 挂窗口过程
  （`DarkMode.cpp:298`）+ `DwmSetWindowAttribute()` 让标题栏变暗（`:386`）+
  **手工绘制菜单栏**——靠的是 `WM_UAHDRAWMENU` / `WM_UAHDRAWMENUITEM` / `WM_UAHINITMENU` /
  `WM_UAHMEASUREMENUITEM` 这组**未文档化**的 UAH 消息。
  `vba.rc` 里还有 16 处 `OWNERDRAW`/`MENUBREAK`/`CHECKED`/`GRAYED` 之类的定制标记。
  ⇒ **这段代码的价值恰恰在"菜单栏画得好不好看"，而它是最不可移植的 349 行。**
- wx 3.3 **确实自带 MSW 深色模式**（`include/wx/msw/darkmode.h`，`wxDarkModeSettings`），
  这是选 3.3 而不是 3.2 的最实质理由——但**3.3 是开发分支**（3.2 才是稳定版），
  意味着你在一个会变 API 的分支上做长期项目。
- 两套深色机制只能二选一：留自己的，就得关掉 wx 的；用 wx 的，`DarkMode.cpp` 白写，
  且 wx 的深色覆盖范围（菜单/静态框/滚动条）未必达到你现在手调的观感。

**风险：中低，但心理成本高**——这是最近才做的工作，迁一次等于扔掉。

### 4.7 WINE 友好性：瓶颈不在 DirectX，在默认渲染方法

你提到"接 SDL 至少对 WINE 更友好"。这条我认为**方向对、手段错**，而且真实原因是别的东西：

- `VBA::loadSettings()` 的默认值是 **`renderMethod = DIRECT_DRAW`**（`VBA.cpp:495`）——
  DirectDraw 恰恰是 WINE 上最容易出问题的一条路：它是 1996 年的 API，涉及独占协作级别
  （`SetCooperativeLevel(DSSCL_EXCLUSIVE)` 这类）、显示模式切换、以及 `ddrawEmulationOnly` 这种
  兼容开关（作者自己都知道它挑环境）。
- 反过来，WINE 对 **D3D11（→ vkd3d → Vulkan）**、DirectSound、XAudio2、DirectInput 的实现都相当成熟。
  也就是说：**这四条路的 WINE 兼容性不靠 SDL 才有**，它们本来就行。
- 而 SDL3 自己建窗口时，Windows 上的默认 renderer **也是 D3D11**——你绕一圈又回到同一条路，
  只是多了一层纹理拷贝。

**所以对 WINE 最友好的一步是：把默认渲染方法从 `DIRECT_DRAW` 换成 `DIRECT_3D11`（或 `OPENGL`），
并保留 `updateRenderMethod()` 现有的失败回退链（`VBA.cpp:1876-1901` 已经有 `D3D11→D3D9→DDraw→GDI` 的降级）。**
这大概是一行改动 + 一轮回归，收益远大于接一套 SDL。

真要更进一步，就把"全屏"从独占模式改成无边框窗口式全屏——那是 WINE 上第二个高频坑。

---

## 5. 选型：wx / Qt / SDL3+ImGui

先校准你暂定 wx 的两个前提：

- **"语法像 MFC"**：形状像，语义不像（§3），而且像的那部分只占工作量三成。
- **"轻量"**：比 Qt 轻是真的。但要对齐一个量级——SDL3 ≈ 1 MB、Dear ImGui ≈ 1 MB、
  **wx（base+core 静态）≈ 数 MB 量级、你现在的 MFC 静态 exe 是 7.6 MB，换 wx 大概落在 10~13 MB**、
  Qt Widgets 部署包 ≈ 40 MB DLL。所以 wx 在"轻量"上是中间档，不是最轻档。

先明确一件事：**下面三个方案，§4.4 和 §4.5 的工作量是完全一样的。**
区别只在"51 个对话框 + 501 个菜单项 + 202 条翻译 + 配置"这四件事谁更省力。

| | **wx 3.3** | **Qt 6 Widgets** | **SDL3 + Dear ImGui** |
|---|---|---|---|
| 与 MFC 的思维距离 | 最近（都是 90 年代 C++ 控件传统） | 中（信号槽 ≠ 消息映射） | 远（立即模式，没有控件对象） |
| **原生控件**（你最在意的） | **真控件**：macOS 上是真 `NSButton`/`NSTextField`，Windows 上是真 HWND 控件 | 自绘 `QStyle`：外观像，**但 macOS 上没有真 NSControl** | 全自绘，无原生可言 |
| 51 个对话框 | 代码搭 sizer，无设计器依赖，**无 `.rc` 导入** | Qt Designer `.ui` + `uic`，`QDialogButtonBox` 现成 | **没有对话框概念**，要自己写窗口/输入框/表格 |
| 501 菜单项 + 251 灰化逻辑 | `wxEVT_UPDATE_UI` 语义几乎 1:1 | `QAction::setEnabled` 手动调，或每帧轮询 | 菜单要手搓，逐帧重建 |
| 大表格（内存查看器 / RAM Search / 金手指） | `wxListCtrl` + `wxLC_VIRTUAL` 手写回调 | `QAbstractTableModel` + `QSortFilterProxyModel`，**天然虚拟化+排序过滤** | `ImGui::BeginTable` + `clipper`，很顺手 |
| 本地化 | gettext `.po/.mo`，Weblate/Crowdin/Poedit 原生支持（**社区翻译最顺**） | `.ts/.qm` + Linguist（专业但工具少） | 无，自己搞 |
| 配置 | `wxConfig` | `QSettings` | 自己搞 |
| 深色模式 | 3.3 自带（**但是开发分支**） | 6.x 有，成熟度高 | 主题就是几个颜色变量，最自由 |
| 原生 HWND 给渲染器 | `GetHWND()` | `winId()` / `QWindow::winId()` | `SDL_GetWindowWMInfo` |
| 交付体积 | 静态约 +3~5 MB（估），**单 exe** | `windeployqt` 约 +40 MB DLL | +2~3 MB，单 exe |
| 许可 | wxWindows（LGPL+例外），**最宽松** | LGPLv3（GPL 项目可用，静态链接有额外义务） | zlib，最宽松 |
| 构建 | 自带 vcxproj / CMake，可与现有 `.vcxproj` 共存 | **基本要求 CMake**，工程要重写 | CMake 或手写 |

**我的判断：**

- **"语法像 MFC"这一点不足以支撑选 wx。** 像的部分（消息映射、更新 UI）只占工作量的三成；
  不像的部分（资源、编码、翻译）恰恰是最贵的三成。
- **wx 的真正优点是"单 exe + 零运行时 + 许可最宽松 + Windows 观感不变"**，这四条对你这个
  TAS 工具（社区下载一个 exe 就能跑）价值很高。**如果你看重这四条，wx 就是正确选择。**
- **如果目标是"最省力地统一 UI"，Qt 6 更省**：省在 `.ui` 设计器、`QAbstractTableModel`
  （内存查看器/RAM Search 那两个 1,992 / 1,122 行的表格窗口会明显轻）、`QSettings`、
  以及 `.ts` 的完整工具链。代价是 40 MB 依赖 + CMake 改造 + LGPL 静态链接义务。
- **如果目标只是"Linux/macOS 能跑"，两者都不该选**——见 §6 的 B 路线。

一个反直觉但重要的点：**你现在的 MFC 前端已经调过 70 个提交（深色模式、现代文件对话框、
现代字体、DPI、分组金手指……），这些"手感"是 wx/Qt 迁移里最容易被悄悄丢掉的部分**，
而且它们不会出现在任何 diff 里。

### 5.1 一个能改写结论的事实：上游已经有一份完整的 wx 前端

`C:/Users/zhang/visualboyadvance-m/src/wx` —— **VBA-M 自己就是用 wxWidgets 做的跨平台前端**，
而且覆盖 Windows / Linux（含 Wayland）/ **macOS** / Android / 树莓派。

| 证据 | 内容 | 对我们的意义 |
|---|---|---|
| `xrc/` **58 个 `.xrc`** | `MainMenu.xrc`(30 KB 整个菜单栏)、`MemViewer` `MapViewer` `OAMViewer` `PaletteViewer` `TileViewer` `Disassemble` `IOViewer`（GBA/GB 两套）、`CheatList/CheatEdit/CheatAdd`、`SoundConfig*`、`DisplayConfig*`、`GameBoy*Config*`、`JoypadConfig`、`AccelConfig`… | **我说的"51 个对话框要重写"不再成立**：wx 有 XRC（XML 资源），上游已经把同类对话框做完了 |
| `dialogs/` 19 个 `.cpp` | `display-config.cpp` 89 KB、`sound-config`、`game-boy-config`、`speedup-config`、`accel-config`… | 对话框逻辑也有现成实现 |
| `widgets/` | `render-plugin`、`sdl-poller`、**`dpi-support.cpp` + `dpi-support-mac.mm`**、`on-screen-controller`、`shortcut-menu-bar`、`checkedlistctrl`、`keyboard-input-handler`、`user-input-ctrl` | 连 DPI（含 macOS 专用 `.mm`）、渲染插件、快捷键菜单栏都写好了 |
| `macsupport.mm` 48 KB、`macsandbox.mm`、`VKMetalLayer.mm`、`AAPLShaders.metal` | macOS 沙盒、原生窗口胶水、**Metal 渲染层** | 上游对"macOS 要原生"的答案是：**用 wx + 少量 `.mm` 打补丁**，不是手写 Cocoa |
| `panel.cpp:2587` `emusys->emuMain(emusys->emuCount)`，注释写"Called from OnIdle" | 主循环同样挂在 idle 上，`EmulatedSystem*` 结构与本仓库同源 | **接口契约几乎可以直接对齐**，不是重写 |
| 已有 `rewind_mem` / `REWIND_SIZE` | 上游 wx 前端已经带 rewind | 与本仓库的 rewind 概念一致 |

**这直接推翻了我上一轮的 A 路线估算**：110~169 人日是"从零写 51 个对话框"的价格，
而上游已经把 58 个对话框、整棵菜单、配置体系、i18n、macOS 胶水都写完了。
**A 路线应该重估为"移植"：≈ 66~109 人日（含回归，见 §6 A′）**——省的是"搭 UI"，
不是"接核心"和"回归"。缺口也很清楚：**TAS 专属功能上游没有**
（`ram_search` / `ramwatch` / 电影编辑 / 帧搜索 / 输入显示 / 延迟计数器，grep 全空），
这些正是本仓库的立身之本，必须自己补。

**顺带纠正一个常见误解**：Qt Widgets **不是** ImGui 那种自渲染——它用 `QStyle` 画，
并且调用平台主题 API（Windows 上是 `windows11`/`vista` 样式，macOS 上读 `NSAppearance`），
观感很接近原生；但它**确实不创建真的 `NSButton`/`NSTextField`**（Qt Quick 才是全自绘）。
所以你的直觉在 macOS 上是对的：**要"真原生控件"，wx 是三者里唯一的选择。**

### 5.2 减少表现层面的面积，仍然是最省的一步

即使有了上游前端当骨架，**减少表现层面的面积**依然是收益最大的一步——因为要移植的对话框越少，
A′ 的 45~75 人日还能再往下走。

如果"主要问题就是表现层"，那么**减少表现层面面积**的收益，比在 wx / Qt 之间挑一个的收益大得多。
现在的 51 个对话框，按功能其实只有六类：

| 类别 | 现有 | 为什么会这么多 | 合并后 |
|---|---|---|---|
| 设置类单值输入 | `Throttle` `MaxScale` `VideoMode` `Drivers` `Directories` `Config` `TextOptions` `LangSelect` `Associations` `RewindInterval` `Joypad` `MotionConfig` `AccelEditor` ≈ 13 | 2005 年的习惯：每个选项组开一个小模态窗，MFC 建窗几乎零成本 | **1 个设置窗**（6 页：显示/声音/输入/路径/语言/快捷键） |
| 调试器视图 | `MemoryViewer`(+address/size) `OamView` `PaletteView` `TileView` `MapView` `IOViewer` `Disassemble`，GBA/GB 各一套 ≈ 12 | 每套是独立模态窗，靠 `winAddUpdateListener` 每帧刷 | **3~4 个停靠/标签视图**（内存 / 图形视图 / 反汇编 / IO），GBA/GB 用同一套 UI 换数据源 |
| 金手指 | `GBACheatsDlg` `GBCheatsDlg` `add-cheat` `cheat list` ≈ 3 | GBA/GB 拆两套 | **1 个**（分组 + 内联编辑） |
| 电影 | `MovieCreate` `MovieOpen` `ModeConfirm` ≈ 3 | 录制/回放/转换各一窗 | **1 个 + 3 步向导** |
| 小工具 | `Logging` `LuaOpen` `ram_search` `ramwatch` `ExportGSASnapshot` `GSACodeSelect` `RomInfo` `ResizeDlg` ≈ 8 | 各自独立 | 开 ROM 信息 1 个 + RAM Search/Watch 合成 1 个 + 其余并入设置 |
| 杂项 | `About` `BugReport` `Prompt` `DialogBox`(archive chooser) + 其他 | — | 保留 4~5 个 |

**对 §6 估算的影响：**

| 工作包 | 原估 | 砍完 51→12 后 | 依据 |
|---|---|---|---|
| A2 菜单（501 项） | 12~18 | **8~12** | 纯设置型单选组（节流 14、跳帧 10、存档类型 8、GB 机型 8、优先级 4、采样率 3、音频 API 2、语言 3 ≈ 52 项）收进设置窗；13 个"打开设置"入口并成 1 个。**注意：菜单总量只降到 ~330 项，不会降到 150**——缩放、渲染方法这些高频切换必须留在菜单里 |
| A3 对话框（51 个） | 40~60 | **18~28** | 12 个窗，其中 4 个是"一页设置"，重头戏只有内存/图形视图和设置窗 |
| A7 本地化 | 10~15 | **6~10** | 待翻译字符串总量随 UI 面积下降 |
| **合计（其余工作包不变）** | **110~169** | **≈ 80~126** | 降幅约 25~30%，不是腰斩 |

**一个反直觉的结论**：砍完面积之后，**wx 与 Qt 的差距会明显缩小**（见 §6 C 路线的重算）——
因为 Qt 的优势主要在"搭 51 个布局"和"大表格"上，窗只剩 12 个时这笔优势只剩 15~20 人日。
也就是说：

- **面积大时**，选型很重要（差 25~30 人日）；
- **面积小时**，选型不重要（差 15~20 人日，且可以被"单 exe / 40MB 依赖"这类硬约束一票决定）。

这正是为什么我建议**先砍面积、再选库**：砍完之后，选型问题会自己消失一半。

⚠️ 但**走 A′ 时这条的收益变小了**：上游 58 个 XRC 已经把布局写完了，"砍面积"省的主要是
"挑哪些 XRC 搬过来"（A′4，6~10 人日），不再是从 40~60 天砍到 18~28 天。
**所以：A′ 路线下，"砍面积"从"最大的杠杆"降级为"可选的整理动作"。**
（上表的 80~126 是 A 路线从零写的账，保留作对照。）

代价不是技术风险，是**产品决策**：TAS 老用户对菜单位置有肌肉记忆（"Options → Emulator → …"），
砍菜单会挨骂，而且要配套写迁移说明。这件事必须在动第一行代码之前拍板。

### 5.3 Vulkan 值不值：不值，除非你要的是"一个后端通吃"

先说结论：**把渲染器直接推到 Vulkan 是这个项目里性价比最差的一个决定**，理由五条，都可验证：

| 维度 | 事实 | 结论 |
|---|---|---|
| 成本 | Vulkan 要自己写 instance/device/swapchain/同步/pipeline/descriptor + SPIR-V 编译（glslang）+ 验证层 | 3,000~6,000 行；对比 `OpenGL3.cpp` 已有 2,593 行且能跑 |
| 收益 | GBA 输出 240×160，放大到 4K 也只是"一次全屏 blit + 一次上采样" | 瓶颈在 CPU 仿真，GPU 端**无可测收益** |
| 着色器资产 | NIS 是 HLSL 头（`upscalers/NIS_Scaler.h` 918 行）、FSR1 是 HLSL 头（`ffx_fsr1.h` 1,198 行）、xBRZ/libretro 是 GLSL（27 个 `.glsl`） | 上 Vulkan 要把最值钱的资产 HLSL→SPIR-V 重做一遍 |
| 跨平台 | macOS 只能靠 MoltenVK 再包一层；GL 3.3 在 macOS 上限 4.1，对本项目够用 | Vulkan 反而多一层 |
| 维护 | swapchain/HDR/多显示器/驱动差异的坑比 GL 多一个量级 | 单人维护不划算 |

**如果真实诉求是"想要一个后端同时覆盖 D3D12/Vulkan/Metal"**，正确解法不是写 Vulkan，而是：
- **sokol_gfx**（单头文件，D3D11/GL/Vulkan/WebGPU/Metal 一套 API）；
- 或 **SDL3 的 GPU API**（`SDL_GPUDevice`，抽象 D3D12/Vulkan/Metal）。

这两个都是"想要 Vulkan 但不想写 Vulkan"。**建议顺序：先把 `OPENGL33` 的 WGL 引导换成 GLX/EGL（§4.4，150~250 行），
Vulkan 永远排最后。**

---

## 6. 各条路线与工作量

假设：单人、熟悉本仓库、有 AI 辅助编码、不含翻译校对、按"能达到今天功能对等"计。

### A′ 路线：以上游 wx 前端为骨架移植（**修正后推荐**）

不是从零写，而是把 `visualboyadvance-m/src/wx` 拿过来，按本仓库的核心重新接线。
依据见 §5.1：58 个 XRC 对话框、整棵菜单、配置体系、i18n、macOS 胶水（`macsupport.mm` + Metal 层）都已存在，
而且它的主循环同样是 `OnIdle` → `emusys->emuMain()`。

| # | 工作包 | 工作量（人日） | 说明 |
|---|---|---|---|
| A′1 | 把上游 wx 前端搬进 `src/wx`，配 CMake，本机编出 Windows 版 | 5~8 | 上游 CMake 已支持 Win/Linux/macOS |
| A′2 | **核心接口对齐**：把 73 处 `system*()` 调用面、`EmulatedSystem` 成员顺序、`config` 结构对本仓库核 | 8~14 | 这是最大的一块，也是最容易出隐性 bug 的地方 |
| A′3 | **补 TAS 功能**（上游全空）：`ram_search` `ramwatch` 电影编辑/帧搜索/输入显示/延迟计数器/rewind UI | 15~25 | 本仓库的立身之本，必须重写；但可复用本仓库现有的算法（`ramwatch.cpp` 等） |
| A′4 | 对话框裁剪/合并（§5.2，51 → 12~20） | 6~10 | 上游 58 个 XRC 里挑需要的 |
| A′5 | 编码 MBCS→Unicode（439 处 `CString`） | 8~12 | 与 A 路线相同，无法规避 |
| A′6 | 本地化（可用上游 gettext 体系，省一大截） | 4~7 | 比 A 路线的 10~15 天少，因为 `.po` 管线现成 |
| A′7 | 渲染：保留本仓库 6 个后端（`IDisplay`），或改用上游 `render-plugin` | 5~8 | 两条路都行，见 §4.4 |
| A′8 | 回归：8 配置 × 渲染后端 × TAS 确定性 × 多语言 | 15~25 | 不可压缩 |
| | **合计** | **≈ 66~109**（其中回归 15~25；不含回归 ≈ 50~84） | |

**比 A 路线省在哪**：A2（501 菜单）→ 上游 `MainMenu.xrc` 现成；A3（51 对话框）→ 上游 58 个 XRC 现成；
A7（i18n）→ 上游 `.po` 管线现成。**省的是"搭 UI"，不是"接核心"和"回归"。**

**许可**：VBA-M 是 GPLv2，本仓库 `COPYING` 也是 GPLv2，兼容；但要保留版权头并在 `AUTHORS`/README 里署名。

### A 路线：wx 全量替换（从零写，**已被 A′ 取代**，留作对照）

| # | 工作包 | 工作量（人日） | 风险 |
|---|---|---|---|
| A1 | 骨架：`wxApp`/`wxFrame` 替 `CWinApp`/`MainWnd`，idle 循环、全屏、DPI、拖放、关窗 | 12~18 | 中 |
| A2 | 菜单栏 501 项 + 324 加速键 + 259 命令 + 251 更新处理器 | 12~18 | 中 |
| A3 | 51 个对话框重写（21.9k 行，含 DDX→sizer、Tab 顺序、校验） | 40~60 | 高 |
| A4 | 7 个自绘控件（`PaletteViewControl`/`ZoomControl`/`KeyboardEdit`…） | 5~8 | 低 |
| A5 | 6 个渲染后端接 wx HWND + 全屏重构（§4.4） | 5~8 | 中 |
| A6 | 编码 MBCS→Unicode 全量（439 处 `CString`） | 8~12 | 高 |
| A7 | 本地化换血（202 串 + 51 对话框 + 菜单 + 多语言文件） | 10~15 | 高 |
| A8 | 构建：编译 wx（静态 /MT）+ 工程改造 | 3~5 | 低 |
| A9 | 回归：8 配置 × 6 渲染后端 × TAS 确定性 × 多语言 | 15~25 | 中 |
| | **合计** | **110~169**（约 5.5~8.5 人月） | |

### B 路线：Win32 前端不动，新增 SDL3 + ImGui 精简前端（推荐）

保留 `src/win32` 作为 Windows/TAS 主力（零风险），新增 `src/sdl`，共用
`src/common|gba|gb|filters` 和 `EmulatedSystem` 回调。

必须做的：窗口 + GL 纹理上屏、SDL 音频、键盘/手柄、ROM 载入、即时存档、电影录制/回放、
帧/延迟计数器、OSD、RAM Search/Watch（ImGui 表格）、输入显示。

| 工作包 | 工作量 | 说明 |
|---|---|---|
| 主循环 + 视频 + 音频 + 输入 | 8~12 | SDL3 的 `while(running)` 比模拟 MFC 的 idle 更贴近仿真器需求 |
| 电影/存档/rewind/计数器（复用 `src/common`） | 6~10 | 逻辑都在核心里，只需菜单入口 |
| ImGui 调试器/RAM Search/Watch | 6~10 | 立即模式做这种"每帧刷新的大表"反而最省力 |
| 构建（CMake，Linux 交叉验证） | 3~5 | |
| **合计** | **24~37** | **Windows 侧改动 = 0** |

代价：两套 UI 要各自维护；Linux 版没有金手指/地图查看器等 45 个对话框里的长尾功能。

**附：把老 SDL 代码找回来值不值？**

先给事实：**这套核心至今还带着 SDL 的条件编译残留，共约 65 处**
（`common/movie.cpp` 25、`gba/V7|V8/GBAMemory.cpp` 30、`gba/V8/GBA.cpp` 5、
`gba/remote.cpp` 5、`common/lua-engine.cpp` 12、`win32/WinSystem.cpp` 1），
例如 `movie.cpp:463` 的 `#if defined(SDL) systemClearJoypads();`、
`V8/GBA.cpp:3830` 的 `cpuBreakLoop`、`DirectSound.cpp:444` 的 `#if (defined(WIN32) && !defined(SDL))`。

所以结论分三层：

1. **核心不用找**——它本来就是"能脱离 Win32 编译"的，残留证据就在上面这些 `#ifdef` 里。
   真要找也不会找回核心，只会找回前端。
2. **前端找回来，帮助落在"你不缺的那一半"**：老 SDL 前端能给的是主循环、视频上屏、音频队列、
   输入映射、简易菜单——这四样你已经判断"OpenGL 现成、OpenAL 不难"。
3. **有三样东西仍然值得从老代码里抄**（这三样不是"接上就能用"的部分）：
   - **音频配速回路**：现在 `DirectSound::write()` 是忙等播放游标来给仿真器配速（§4.5），
     老 SDL 版本必然有等价实现，抄它比重新设计 OpenAL 配速快；
   - **`systemDrawScreen` / `systemSoundWrite` 的调用契约**（谁在哪个线程调、帧内什么时机调），
     这是决定"要不要引入渲染线程"的关键；
   - **按键映射表**（`DIK_*` → VBA 按钮的 60 行表，`DirectInput.cpp` 顶部）——直接可用。
4. **期望值要压低**：如果那套代码是 SDL 1.2 时代的（1.7.2 那个年代基本都是），
   窗口/GL 上下文/音频 API 全部要按 SDL3 重写，可复用率不到三成。

**净判断：值得找回来当"参考实现"，不值得当成"起点代码"。**
它省的是 3~5 天，不是 30 天。

### B0 路线：先只接 SDL 三后端（B 的前置步，不发布）

> ⚠️ 我没读 `sdl/master`（本项目规矩：我不跑任何 git 命令，本地也没有该分支的 worktree）。
> 下面的评估基于**同源的维护中实现**：`C:/Users/zhang/visualboyadvance-m/src/sdl`
> （`SDL.cpp` 2,605 行、`audio_sdl.cpp` 221、`inputSDL.cpp` 704、`filters.cpp` 568、`ConfigManager.cpp` 974，
> 已支持 SDL2 **和 SDL3**）。要按你自己的分支核对，见本节末尾。

**为什么技术上很顺：三个接缝都已经现成。**

| 接缝 | 现状 | 加一个 SDL 实现要写的 |
|---|---|---|
| `IDisplay`（`Display.h`，6 个纯虚函数 + 3 个可选） | 6 个实现 | SDL 窗口/纹理上屏 ≈ 300~500 行（`SDL_Texture` STREAMING + `SDL_UpdateTexture` + `SDL_RenderCopy`） |
| `ISound`（`Sound.h`，8 个纯虚函数） | 2 个实现 | ≈ 250 行，**配速模型可以直接抄**（见下） |
| `Input`（`Input.h`，9 个纯虚函数） | 1 个实现（DirectInput） | ≈ 400~600 行 |

**主循环不用动**，这是最关键的一点：VBA-M 的 SDL 前端是
`while (emulating) { emulator.emuMain(...); sdlPollEvents(); }`（`SDL.cpp:2458-2486`），
**与本项目 `OnIdle` 的"跑一帧再泵消息"完全同构**。所以不存在"SDL 音频回调驱动仿真"那种反转控制问题。

**三样东西值得直接抄，而且抄了就是赚：**

1. **音频配速**：`SoundSDL::write()`（`audio_sdl.cpp:110-156`）是"环形缓冲满了就
   `SDL_WaitSemaphore` 等音频回调消费"，这与本项目 `DirectSound::write()`（`DirectSound.cpp:449-470`）
   的忙等播放游标**在语义上是同一个东西**——音频设备做主时钟。也就是说**你担心的 OpenAL 配速问题，
   SDL 实现里已经有答案了**，而且是经过实战的。
2. **手柄枚举**：`inputSDL.cpp` 用 `SDL_Joystick*` + `NumButtons/NumAxes/NumHats`
   （`inputSDL.cpp:400-512`），与本项目 `pDevices[]` + `GetCapabilities()` 的结构**一一对应**，
   连"每设备轴数/帽数"这个概念都一样。移植基本是改名。
3. **`systemDrawScreen` 的调用契约**：谁在帧内什么时机把 `g_pix` 交给显示层
   （`SDL.cpp:2571-2650`），决定了渲染线程的边界——这段逻辑本项目的 `WinSystem.cpp` 里也有对应物。

**别抄的**：`SDL.cpp` 那 2,605 行的主程序（配置/命令行/菜单）、`filters.cpp`（本仓库 `src/filters` 更全）。

**成本与收益：**

| 项 | 量 |
|---|---|
| 三个后端接线 | 1,200~2,000 行，8~12 人日 |
| 收益 1 | **同机 A/B**：同一 ROM、同一 `.vbm`，MFC+D3D11 vs MFC+SDL 各跑一遍，比对画面/音画同步/帧计数——这是最便宜的验证手段，也符合本项目"两份真实二进制"的规矩 |
| 收益 2 | 证明"核心 + 三后端"不依赖 MFC，为 B 路线铺好 60% 的路 |
| 收益 3 | 这套 `InputSDL` 在 Linux 上直接可用，不用再写一遍 |

**风险（必须提前定死）：**

- **不要用 `SDL_CreateWindowFrom(MainWnd 的 HWND)` 往 MFC 主窗里塞子窗**——那会变成两套事件循环、
  两套重绘、两个 DPI 处理在同一个 HWND 上打架。**让 SDL 自己建窗（接管模式）**，
  MFC 主窗退化成"菜单壳 + 参数源"，这样才和 Linux 形态一致，也才能真做 A/B。
- 这条路**不要发布成 Windows 版的新渲染器**：SDL 的 renderer 在 Windows 上就是 D3D11，
  你等于用 1,500 行胶水换来一条比 `Direct3D11.cpp` 更弱（没有 NIS/FSR/xBRZ 管线）、多一次纹理拷贝的路径。
  它的价值在"迁移"，不在"功能"。

**要按你自己的分支核对，把分支摆出来就行（你来执行）：**

```sh
git worktree add ../vba-sdl sdl/master
```
之后我可以逐文件比对：`src/sdl` 是否存在、用的是 SDL1.2 还是 3、`systemDrawScreen` 的签名、
音频是回调还是信号量、`Input` 用的是 `SDL_Joystick` 还是 `SDL_GameController`。
**若是 1.7.2 时代的 SDL 1.2 实现，上面"直接抄"的三样里有两样要按 SDL3 重写，成本翻倍。**

### C 路线：Qt 6 统一

在 A 路线基础上：A2 减 3~5 天（`QAction` + Designer），A3 减 10~15 天（`.ui` + `QDialogButtonBox`
+ `QAbstractTableModel`），A7 减 3~5 天（`lupdate` 自动抽取），A8 加 3~5 天（CMake 全量改造）。
**净计 85~130 人日**，但分布参数更差（依赖体积、部署脚本、许可合规）。
若同时采用 §5.2 的"51 窗 → 12 窗"：**≈ 66~113 人日**——
与 wx 的 80~126 只差 15~20 人日，**差距被面积缩小吞掉了**。

### E 路线：macOS 原生 Cocoa 前端（只做 mac，最原生，也最贵）

**优点是真的**：只有这一条能拿到 100% 平台集成——`NSVisualEffectView` 毛玻璃、统一标题栏、
`NSToolbar`、原生菜单校验（`validateMenuItem:`）、Touch Bar、Metal 渲染、`NSDocument` 体系。
wx 与 Qt 都只能"接近"。

**但要认清三件事：**

1. **没有参考实现**。上游 VBA-M 面对同样的需求，选择的是 **wx + `macsupport.mm`（48 KB）+
   `VKMetalLayer.mm` + `AAPLShaders.metal`**，而不是手写 Cocoa。也就是说：
   "macOS 要原生"这件事，上游认为 wx 够用。你要越过他们的判断，就得自己承担全部风险。
2. **只能覆盖一个平台**。Windows 还是 MFC，Linux 还是一片空白 ⇒ 三套 UI 三份维护。
   如果你是"为了 macOS 用户"，先问一句：**macOS 上现在有多少潜在用户？** 若答案是"很少"，
   这条路的单位收益是全项目最低的。
3. **成本最高**：ObjC++/AppKit 从零到功能对等，含 6 个渲染后端里的 1~2 个（Metal 或 GL 4.1）、
   音频（CoreAudio）、输入（IOKit HID / GameController）、51 个对话框（`NSWindowController` + xib/storyboard）、
   菜单（`NSMenu` 全手写）、本地化（`.strings`/`.xcstrings`）。
   **估算 90~140 人日**，而且 AppKit 的坑（坐标系左下原点、retina 缩放、`NSOpenGLView` 弃用、
   事件循环与 `CVDisplayLink` 的时序）几乎全是**运行期**才知道的。

### F 路线：如果只是"想要 macOS 原生感"，其实是 A′ + 三个 `.mm`

上游的做法值得抄：**主 UI 用 wx（真 NSControl），只在需要"更原生"的地方补 Objective-C++**：
`dpi-support-mac.mm`（DPI）、`macsandbox.mm`（沙盒/权限）、`VKMetalLayer.mm` + `AAPLShaders.metal`
（Metal 渲染层，绕开 macOS 已弃用的 OpenGL）。
**这是"90% 原生感、10% 成本"的解**，而且和 A′ 是同一条路，不是分叉。

### 判定矩阵

| 你的真实目标 | 选它 |
|---|---|
| **一套代码三平台 + 真原生控件（macOS 要真 NSControl）** | **A′（移植上游 wx 前端）**，66~109 人日 |
| 一套代码三平台、单 exe、许可最宽松 | **A′**（wx 静态链接，仍是单 exe） |
| 一套代码三平台、最省人力、能接受 40MB 依赖 | **C（Qt 6）**，85~130 人日 |
| Linux/macOS 能跑就行、Windows 产品不能有任何风险 | **B（SDL3+ImGui）**，24~37 人日，但 UI 完全不原生 |
| **只做 macOS、且愿意为最后 10% 原生感付 90 人日** | **E（手写 Cocoa）** |
| 只是想让 UI 更现代、没打算真的发布 Linux 版 | **不做迁移**，继续在 Win32 上做（`DarkMode.cpp` + `ModernFileDialog.cpp` 这条路已经在走） |
| 想先"把后端从 MFC 里切出来"、在 Windows 上就能 A/B | **B0（SDL 并行后端）**，8~12 人日，但**别发布** |
| 只想要 WINE 上更稳 | **不改架构**：默认渲染 `DIRECT_DRAW` → `DIRECT_3D11`/`OPENGL`（§4.7），一行 + 回归 |
| 想要"现代图形 API" | **不写 Vulkan**，先修 `OPENGL33` 的 GLX/EGL 引导；真要通吃用 sokol_gfx / SDL_GPU（§5.3） |

---

## 7. 如果确定走 wx：分阶段、每阶段可回滚

**走 A′ 时，阶段就是 §6 的 A′1~A′8**：先把上游前端搬进来编出 Windows 版（A′1），
再做核心接口对账（A′2），此时就已经是一个"能开 ROM、能跑、菜单和对话框都在"的可运行版本——
比从零写快得多。TAS 功能（A′3）在有了可运行版本之后再加，每加一个都能 A/B 验证。

**但下面这组 P0 探针仍然要先做**（3~5 天）——它们验证的是"wx 能不能承载这个仿真器"，
和"从哪里抄 UI"是两件事：

- **P0 探针（3~5 天，产出物是"能不能"的答案，不是代码）**
  1. 编译 wx 3.3.4 静态 `/MT`（`nmake -f build\msw\makefile.vc BUILD=release SHARED=0`）。
     （若走 A′，也可以先用上游 CMake 把它依赖的 wx 编出来，两件事合并。）
  2. 最小 `wxApp` + `wxFrame`，把现有 `GDIDisplay` 挂上去跑一个 ROM，测帧率/CPU。
  3. 同一份 `wxFrame` 里开一个模态 `wxDialog`，验证 **仿真是否继续跑**（§4.3 第 3 条）。
  4. 切一次全屏，验证 §4.4 的窗口重建问题。
  **这四步任一失败，就回到选型阶段，不要继续。**

- **P0.5 逃生路线（可选，只在"必须一次性切过去"时用）**：把 51 个 `.rc` 对话框先降级成
  **裸 Win32 无模式对话框**（`CreateDialogIndirectParam` + 自己的 `DLGPROC`，不需要 MFC），
  由新 wx 主窗托管，逐个换成 wx 原生实现。这条路能保住布局、把迁移摊成 51 个小步，
  但有两个明确代价：
  1. `DialogBoxParam` 的**系统模态循环会挡住 wx 的 idle**（不走 `wxEventLoopManual`），
     仿真会在对话框打开期间停住 → 必须做成无模式 + 手动 `EnableWindow(false)` 父窗；
  2. 这块代码永久 `#ifdef __WXMSW__`，跨平台版里没有它。
  **走 A′ 时不需要这条**（上游 XRC 已经把布局写好了）。

- **P1 骨架（A1+A5+A8）**：主窗、idle、6 个渲染后端、全屏。菜单先用最小集（File/Exit/Pause）。
  产物：能开 ROM 能跑能出画面，其余功能全部缺失。
- **P2 菜单（A2）**：501 项 + 251 个更新处理器。这一步工作量确定、风险低，可并行。
- **P3 对话框（A3+A4）**：按使用频率排序，先 `Throttle`/`VideoMode`/`Joypad`/`Directories`
  这类小窗，再 `ram_search`/`GBACheatsDlg`/`MemoryViewer` 这几个大的。
- **P4 编码与本地化（A6+A7）**：放最后做，因为前三个阶段要用旧 ANSI 路径跑通才有对照。
- **P5 回归（A9）**：必须用**两份真实二进制**做 A/B（这是本项目的既有规矩）：
  同一 `.vbm` 在旧 exe 与新 exe 上各回放一遍，比对帧计数/结束状态；
  6 个渲染后端各截一张图；至少中/日/英三种语言各开一轮所有对话框。

---

## 8. 必须实测的探针（不要靠读代码下结论）

| # | 探针 | 判定标准 |
|---|---|---|
| T1 | wx 3.3.4 静态编译（/MT、Unicode、无 PCH） | 编译通过 + 生成的 `.lib` 能链进现有 vcxproj |
| T2 | `wxEVT_IDLE` 里跑 `emuMain`，60 fps | CPU 占用不高于现版 1.2 倍；无掉帧 |
| T3 | 模态 `wxDialog` 打开时仿真是否继续、节奏是否抖动 | 帧计数器在对话框打开期间仍增长；帧间隔标准差与现版相当 |
| T4 | 全屏切换 10 次 | 不崩、不闪白、仿真不停 |
| T5 | `Direct3D11` 后端在 `wxFrame` 的 HWND 上建 swapchain | `CreateDevice`+`CreateSwapChain` 成功，resize 正常 |
| T6 | `DirectInput` 轮询在 wx 窗口失焦后仍然有效 | 背景输入选项行为与现版一致 |
| T7 | 非 ASCII 路径（日文 ROM 名）+ `.vbm` 保存/载入 | 与现版行为一致（或明确记录差异） |
| T8 | 同一 `.vbm` 在新旧 exe 上回放 | 帧计数、结束状态、内存哈希一致 |
| T9 | `.lng` → `.po` 的转换保真度 | 抽 3 种语言，比对字符串数量与内容 |
| T10 | **OpenAL Soft 配速回路**（替代 `DirectSound.cpp:449` 的忙等游标） | 连续跑 3 分钟：音画漂移 < 1 帧、无欠载爆音；`useOldSync` 两种模式都测 |
| T11 | **GLX/EGL 引导**（把 `OPENGL33` 后端搬到 Linux） | 上下文创建成功、glsl 着色器编译通过、vsync 可切、全屏 resize 正常 |
| T12 | **上游 wx 前端整体编译**（按它自己的 CMake，Windows 优先） | 能编出 exe、能开 ROM、能跑满 60 fps |
| T13 | **`EmulatedSystem` 成员级对账**（上游 vs 本仓库） | 列出签名/顺序差异清单，逐项给出映射或适配层 |
| T14 | **TAS 缺口清单**（上游 wx 缺哪些本仓库功能） | 逐项列出：ram_search / ramwatch / movie 编辑 / 帧搜索 / 输入显示 / 延迟计数器 / rewind UI |

---

## 9. 未验证项（诚实列出）

1. **没有编译过 wxWidgets 3.3.4**（本机 `lib/` 为空），也没有跑通 T1~T9 任何一条。
   §6 的工作量是**基于代码规模与耦合点数的估算**，不是实测工时。
2. **`wxEVT_IDLE` 在模态对话框下是否仍然分发**：源码上已确认会（§4.3 第 3 条，
   `wxModalEventLoop` 派生自 `wxEventLoopManual`，同一条 `Run` 循环），
   但没有实测过 `wxDialog` disable 父窗之后的分发节奏，仍需 T3 确认。
3. **没有验证 wx 3.3 的深色模式覆盖范围**能否达到 `DarkMode.cpp` 现在的观感。
4. **`.lng` 资源 DLL 的实际数量与语言清单没有盘点**（仓库里没有 `.lng`，是运行时产物）。
5. Qt 方案**没有在本机验证**（未安装 Qt），体积/许可结论来自公开资料。
6. `src/win32/upscalers/` 下的着色器头（约 5.3k 行）已从统计中排除，它们与 UI 框架无关。
7. **OpenGL 后端的 Linux 可移植性没有实测**：管线与着色器可复用这点是从代码结构判断的
   （`OpenGL3.cpp` 走 `wglGetProcAddress` + `libretro/*.glsl`），GLX/EGL 引导未落地。
8. **老 SDL 前端的实际版本与内容未知**：仓库里没有残留（只有 `VBA.cpp:279` 一句注释说
   `parseCommandLine` 抄自 `SDL_main.c`），65 处 `#ifdef SDL` 只能证明"核心曾支持 SDL"，
   不能证明"老前端能直接跑 SDL3"。我没跑任何 git 命令，历史由你自己查。
9. 🔴 **`sdl/master` 分支我一行都没读到**（规矩：不跑 git）。§6 B0 路线的评估是拿
   `C:/Users/zhang/visualboyadvance-m/src/sdl`（同源、维护到 SDL3 的实现）当代理做的，
   **不能代替**对你那条分支的核对。工作树摆出来（`git worktree add ../vba-sdl sdl/master`）后
   我可以逐文件比对。
10. **默认渲染 `DIRECT_DRAW` 的改动没做**（只是记录结论），也没在 WINE 下实测过
    `D3D11 / OPENGL / DDraw` 三条路的实际表现差异。§4.7 是代码证据 + 常识推断，不是 WINE 实测。
11. **上游 wx 前端只做到"目录级 + 少量代码级"的核查**：我数了 `xrc/` 的 58 个文件、看了
    `panel.cpp:2587` 的 `emuMain` 调用、`CMakeLists.txt` 的平台分支，但**没有编译过它**，
    也没有逐行比对它的 `EmulatedSystem` 与本仓库的成员差异（§6 A′2 的 8~14 人日就是给这块留的）。
12. **Cocoa 我无法在本机验证**（实测）：`clang -x objective-c++ -fsyntax-only` 能过纯语法
    （只有一个 `-Wobjc-root-class` 警告），但 `#import <Cocoa/Cocoa.h>` 直接
    `fatal error: 'Cocoa/Cocoa.h' file not found`。⇒ 我能写 ObjC++，**但一行 AppKit 代码都编不了**。
    这条决定了 E 路线不能由我主导（见 §6 E）。
13. **macOS 上 wx 的原生控件覆盖度没实测**（`NSTextField`/`NSPopUpButton`/`NSButton` 是真控件这点
    来自 wx 文档与 `wx/msw`+`src/osx` 的实现结构，没有 mac 机器验证观感与 HiDPI）。

---

## 10. 一句话建议

**我改主意了：不要从零写，去搬上游的 wx 前端。** 这是本次分析里最重要的一条更正。

- 上游 `visualboyadvance-m/src/wx` 已经有 58 个 XRC 对话框、整棵菜单、配置体系、i18n、
  macOS 胶水（`macsupport.mm` + Metal 层），主循环同样挂在 `OnIdle` 上调 `emusys->emuMain()`。
  ⇒ **A 路线从 110~169 降到 A′ 的 66~109 人日**，而且三平台一套代码、macOS 上是真 NSControl。
- **它省的是"搭 UI"，不是"接核心"和"回归"**：A′2（核心接口对齐，8~14 天）和 A′8（回归，15~25 天）
  一分都少不了，TAS 专属功能（ram_search/ramwatch/电影编辑/帧搜索）上游完全没有，得自己写。
- **C（Qt）现在没有理由选了**：85~130 人日比 A′ 还贵，且 macOS 上不是真控件。
  **B（SDL+ImGui）与你"要原生感"的诉求直接冲突**，只在"Windows 零风险"这条上仍有价值。
- **E（手写 Cocoa）我不建议现在做**：90~140 人日、只覆盖一个平台、无参考实现，
  而且**我无法在本机验证任何 AppKit 代码**（实测 `Cocoa/Cocoa.h` 不存在）。
  先做 A′，把 wx 版跑起来，再决定要不要为最后 10% 的原生感付这 90 人日。
- 后端不再是主要风险：表现层 ≈ 75~80%、平台胶水 ≈ 20~25%。两个例外是**音频配速**
  （`DirectSound.cpp:449` 忙等游标）和**窗口/全屏/上下文生命周期**（与 `MainWnd` 重建纠缠）。
- 成本最低的动作仍然是把默认渲染从 `DIRECT_DRAW` 改掉（§4.7）——一行，比接 SDL 更 WINE 友好。

**下一步（3~5 天，能出结论的那种）**：跑 T12（把上游 wx 前端按它自己的 CMake 编出来）+ T13
（`EmulatedSystem` 成员级对账）+ T14（TAS 缺口清单）。
这三件做完，A′ 的 66~109 就能收敛成一个能写进计划的数字。
