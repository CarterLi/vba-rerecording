# 核心重构规范（借 wx 迁移做的那一次）

> 来源：2026-10-20 用户提出「本 MFC 项目底子是 10 多年前的代码，希望借这次迁移将核心重构一下」
> 「之后跨平台时可补 ImageIO 等后端，复用现有代码、省 exe 体积，支持的格式还比 wx 多」。

---

## 0. 两条铁律（先立规矩，再谈重构）

**铁律一：每次重构都必须能证明"行为没变"。**

手段是刚做好的 `-hashtrace`：逐帧对 `emuWriteMemState()` 的完整状态 blob 取 crc32。

```bat
vba.exe -rom game.gba -play movie.vbm 1 -hashtrace D:\before.csv
:: 改代码
vba.exe -rom game.gba -play movie.vbm 1 -hashtrace D:\after.csv
fc /b before.csv after.csv
```

一致性判据：**除首行注释外逐字节相同**。任何一个十六进制位不同，就说明这一版重构改变了行为，
不允许以"应该没影响"为理由放行。

**铁律二：本轮不改 savestate 布局。**

`emuWriteMemState` 的 blob 就是 savestate 的格式。一旦布局变了：
老存档（.sav/.sgm）全部失效、老录像的 rewind/frame-search 数据全部对不上、哈希基线也全部作废。
如果确实要改格式，**单独开一轮**，先把格式改完并单独验证，再叠行为重构。两件事绝不能混在一个提交里。

---

## 1. 重构清单（按「收益 / 风险」分层）

### A 类：纯机械，零行为风险，可批量做

| # | 事项 | 现状证据 | 收益 |
|---|---|---|---|
| **A1** | `#include "V7/GBA.cpp"` 这种"用 include 拼 CPU 核"改成真实的编译单元选择 | `src/gba/GBA.cpp:5` `#include "V7/GBA.cpp"` / `V8/GBA.cpp`；`bios.cpp`、`GBA-arm.cpp`、`GBA-thumb.cpp`、`GBAMemory.cpp`、`GBACheats.cpp` 同款 | IDE 能跳转、能增量编译、obj 不再互相覆盖；V7/V8 可做成两个独立 target |
| **A2** | 核心与 `theApp` 解耦 | `movie.cpp` 20 处、`lua-engine.cpp` 4 处；施工单见 `decouple-core-from-mfc.md` | 核心不再依赖前端；wx/SDL/Qt 共用同一份核心 |
| **A3** | `gba/remote.cpp` 去掉 `../win32/stdafx.h` | 只需 `<winsock2.h>` + `<io.h>` | 核心最后一个 Winsock 侧的 MFC 依赖消失 |
| **A4** | 清理 65 处 `#ifdef SDL` 死代码 | `common/movie.cpp`(25)、`gba/V7\|V8/GBAMemory.cpp`(30)、`lua-engine.cpp`(12) 等 | 这些分支永远编不到，却让人误判"核心已支持 SDL"。删掉或改成正式的 `IEmuFrontend` 分支 |
| **A5** | 头文件自包含 | `System.h:6` 直接 `#include "zlib.h"`、`Util.h:6` 同 | 减少隐式依赖，任何一个 .cpp 单独编译不再靠运气 |

### B 类：低风险、收益明显

| # | 事项 | 说明 |
|---|---|---|
| **B1** | `System.h` 从 72 个声明瘦到实际被引用的 40 个 | 链接器实测证明另外 32 个核心根本不调用（`systemGetFrameRate`、`systemSetTitle`、`systemClonePixBuffer`…）。留着只会诱导前端去实现没人用的函数 |
| **B2** | 全局变量按模块收拢到 `struct` + 匿名 namespace | ⚠️ **必须保持声明顺序与类型完全不变**——这些全局的地址布局就是 savestate 的布局。只能"换写法"，不能"换排列" |
| **B3** | `enum` → `enum class`、`NULL` → `nullptr`、`#define` 常量 → `constexpr` | 纯语法层，不改变任何数值 |
| **B4** | 图像后端抽象（见 §2） | 一次改造，同时解决"跨平台"和"exe 体积"两件事 |

### C 类：高风险，必须单独一轮 + 完整基线，且**本次迁移不做**

| # | 事项 | 为什么不做 |
|---|---|---|
| C1 | 重写 `movie.cpp` 的 TAS 状态机 | 收益是"代码好看"，风险是"所有 .vbm 失效"。性价比最差的一项 |
| C2 | 动 CPU 内核（V7/V8 的时序、宏、`arm-new.h`） | **明确禁止**。那是另一个项目；改一个条件码就可能让所有 TAS 录像漂移 |
| C3 | savestate 格式版本化 | 违反铁律二。要做就单独开一轮，先把格式改完并单独验证 |

---

## 2. 图像后端：WIC / ImageIO / 平台后端（回应「复用现有代码 + 省体积 + 格式更多」）

### 2.1 现状比我原以为的好

`common/ImageWrite.cpp` **已经是 WIC 实现**，而且第 20~26 行特意 `DEFINE_GUID` 了
`IID_IWICImagingFactory`，注释写明是为了"不把 `windowscodecs.dll` 写进 import table，
好让 XP 上还能启动"——**WIC 是运行时依赖，不是链接依赖**。

更关键的是 `ImageWrite.cpp:325` 那段：

```
// Collects the containers the local WIC can encode.  PNG is moved to the front
```

⇒ **格式列表是运行时从 WIC 枚举出来的**，PNG 只是因为"优先"被提到最前面，并不是唯一格式。
所以你今天能存的不止 PNG：系统装了哪些 codec 就能存哪些（JPEG / BMP / GIF / TIFF / HEIF / WebP…）。
**这一点上现有代码已经比 wx 内嵌的那 4 种（PNG/JPEG/TIFF/WebP）强，而且是靠 Windows 自己升级。**

### 2.2 抽象成后端，是"顺手"而不是"另起"

```
common/ImageWrite.h            公共接口（不含任何平台类型）
   ├─ ImageWriteWIC.cpp        Windows：现有 ImageWrite.cpp 原样搬进来
   ├─ ImageWriteImageIO.mm     macOS：CGImageDestination + UTType 枚举
   └─ ImageWriteStb.cpp        Linux：stb_image_write，或 libpng/libjpeg
```

接口只需要三个函数（现有调用点就这么多）：

```cpp
bool imageWriteIsAvailable();
int  imageWriteEnumerateFormats(const char **names, const char **exts, int max); // 运行时枚举
bool imageWrite(const char *path, const char *formatHint,
                const unsigned char *pixels, int w, int h, int stride, int bpp);
```

收益：

1. **`#include <windows.h>` 从核心里彻底消失**（A 类清单里的最后一个平台头）；
2. macOS 上 ImageIO 的格式列表同样是运行时枚举的（`CGImageDestinationCopyTypeIdentifiers`），
   **"格式比 wx 多"这条在三个平台上都成立**；
3. **wx 的 `wxUSE_LIBPNG / LIBJPEG / LIBTIFF / LIBWEBP` 可以关掉**，exe 直接小几 MB
   —— 因为我们的截图/存档缩略图根本不走 wx 的图像栈。

### 2.3 关掉 wx 的 PNG 之前，需要先查的坑（已查完）

**结论：XRC 不受影响，只有 8 处"保存图片"的调用需要换掉。**

| 检查 | 结果 |
|---|---|
| `frontend-wx/xrc/` 里的 `<bitmap>` | **9 处，全部是 `stock_id`**（`wxART_FILE_OPEN`/`wxART_FILE_SAVE`/`wxART_DELETE`/`gtk-*` 等 wxArtProvider 内建图标），**没有任何 `.png` 文件引用** |
| 真正用到 PNG 的地方 | `cmdevents.cpp:836,865`（截图存盘）、`gfxviewers.cpp:1695,1713,1718,1719,1862,1880`（Tile/Palette/OAM 视图导出，`wxBITMAP_TYPE_PNG`/`BMP`） |
| 性质 | **全部是"把像素存成文件"**，正是 §2.2 那个 `imageWrite()` 接口要接管的事 |

⇒ 把这 8 处换成 `imageWrite()`，就可以把 `wxUSE_LIBPNG/LIBJPEG/LIBTIFF/LIBWEBP` 全关掉，
exe 少几 MB，而格式支持由 WIC / ImageIO 提供（反而更多）。

⚠️ 一个例外要确认：`wxArtProvider` 在 MSW 上从 `wx.rc` 资源取图标（BMP/ICO），不走 PNG；
但在 GTK/macOS 上它可能从内嵌 PNG 资源取图 —— 那是**另一个平台**的事，届时按平台决定开关，
不影响 Windows 侧先关。

---

## 3. 实施顺序（每步一次独立提交 + 一次基线比对）

| 序 | 步骤 | 预估 | 验收 |
|---|---|---|---|
| 0 | ✅ 已完成：`-hashtrace`（`common/HashTrace.cpp`，`systemFrameBoundaryWork` 里每帧一次） | — | 编译通过；契约仍 40 符号 |
| 1 | 用现有 exe 跑出 6~10 个 `.vbm` 的基线 CSV，入库 | 0.5 天 | `# end frames=N` 数量对齐 |
| 2 | A3 `remote.cpp` | 0.5 天 | 哈希逐字节一致 |
| 3 | A2 `lua-engine.cpp` | 1.5~2 天 | 同上 |
| 4 | A1 CPU 核编译单元化 | 1~2 天 | 同上（这一步纯编译期，风险低但影响面大） |
| 5 | A2 `movie.cpp` | 1.5~2 天 | 同上；硬复位路径手测 |
| 6 | B1 瘦身 `System.h` | 0.5 天 | 链接通过 + 哈希一致 |
| 7 | B4 图像后端抽象（WIC 先搬，macOS/Linux 后端到 M5 再做） | 1~1.5 天 | 截图三种格式各存一张、能打开 |
| 8 | B2/B3 语法现代化（分批，每批一次比对） | 2~3 天 | 每批哈希一致 |

合计约 **9~14 人日**，其中第 2~5 步（解耦）已经包含在 D5 的 3~5 人日里。

---

## 4. 明确不做的事

1. **不重写 CPU 内核**（C2）——这是 TAS 项目的地基，动它等于换一个模拟器。
2. **不改 savestate 布局**（铁律二）。
3. **不做"现代化"表演**：不引入 `std::filesystem`、不改成 C++20 modules、不把全局换成单例、
   不为了"架构好看"给热路径套虚函数。这个项目的性能与确定性优先于代码美学。
4. **不为了重构而重构**：上表里没有列出、且说不清收益的改动，一律不做。
