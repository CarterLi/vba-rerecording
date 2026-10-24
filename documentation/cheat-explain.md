# 金手指说明（Cheat list 的 Explain 按钮）

## 它做什么

金手指列表里选中一行（分组模式下选一个组也可以），按 **Explain**，弹出的窗口把这一行
（或整组）的每条代码翻译成一句话：哪个地址、多少位、写成什么值；如果是条件码，写出
什么条件成立才执行；如果是按键码，写出要按哪些键。

```
74000130 03FB
    如果 0x03000130 处的 16 位值等于 0x03FB，则执行后面的代码
82037368 0000
    把 0x02037368 处的 16 位值设为 0x0000
```

## 数据从哪来

不需要重新解密任何东西。代码加入列表时引擎已经解析过一遍，结果就在
`CheatsData`（`src/gba/GBACheats.h`）里：

| 字段 | 含义 |
|---|---|
| `size` | 指令类型（`INT_16_BIT_WRITE`、`CBA_IF_TRUE`、`GSA_8_BIT_FILL` …） |
| `address` | 操作的目标地址（条件码是「读哪个地址」，按键码只保留比较模式） |
| `value` | 写入值 / 比较值 / 打包的计数与步长，具体含义由 `size` 决定 |

这些类型编号以前是 V7 和 V8 各自 `#define` 一份（编号相同、但 V7 只用到前 37 个）。
现在统一放在 `src/gba/GBACheatTypes.h`，两个核心和 win32 一侧都 include 它，
避免第三份副本漂移。

逐条的真实语义以 `V8/GBACheats.cpp` 的 `cheatsCheckKeys()` 为准，解释器里的每组
case 都是照着那个 switch 写的；写文案时如果两边对不上，以引擎为准。

## 文案策略

每条模板最多两个 `%s`，**值先格式化成字符串再传**：

```
IDS_CHEAT_EXPLAIN_WRITE   "set %s to %s"    "把 %s设为 %s"
```

这样做有两个原因：

1. 中文和英文的语序不同，模板里的 `%s` 可以按各自语言挪位置，不需要按参数顺序
   硬凑句子；
2. 不会重复当年 RAM Search 标题的错——`%d` 后面跟复数后缀 `%s`，译文一删后缀，
   参数就串位，`%X` 会把指针当数字打印出来。**模板里只放数字或整段字符串，
   不要把占位符当拼接零件用。**

值本身按指令位宽补零（`0x0000` 而不是 `0x0`），跟原始代码里的数字长度一致，
方便左右对照。

多行指令（slide、ROM patch 的 `2C/2D/2E/2F`、`GS_WRITE2`、`CBA_SUPER`、
`ADD2/SUB2`）的第二行是**数据**，不是指令：说明合并写在第一行下面，第二行单独
显示原始码并标「上一行的数据」。判断方法是对照 `isMultilineWithData()`。

## 已知取舍

- **有符号与无符号比较共用一套文案。** `IF_LOWER_S*` 和 `IF_LOWER_U*` 都写成
  「小于」。两者的区别只在实际比较时对负数有效，说明用途时意义不大；
  真要区分，得再加 6 条模板。
- **`GSA_16_BIT_MIF_*` 不写出跳过多少行。** 跳行数存在 `rawaddress` 里，而 V7 的
  `CheatsData` 没有这个字段，写了会在 V7 构建下编译失败。文案改成「否则跳过若干行」。
- **`CBA_SUPER` 只把第一行数据标成数据**，后面还有多少行要看 `super` 计数，
  `CheatsData` 里没有记录，所以剩余行会显示成「未识别的代码类型」。不影响理解。
- **`UNKNOWN_CODE` 有两种来源**：真正不支持的指令，以及超级码的数据行。文案把两种
  情况都覆盖了；`value == 0x1DC0DE` 的 GS 行另外认成「识别码」。
- 按键码的 `0x10` / `0x20` 两种比较模式分开写（全部按住 / 任意一个），`0x00` 那种
  「没按的键恰好等于 value」的写法太绕，按「任意一个」处理。

## 加一种类型

1. 在 `win32/Dialogs/CheatExplain.cpp` 的 `winExplainOne()` 里加 case：
   - 纯写入 → `winExplainWrite(address, bits, value)`
   - 条件 → `winExplainCondition(IDS_..., address, bits, value)`，会关掉后续全部代码的
     用 `winExplainConditionTail()`
   - 需要自己的句子 → 新增 `IDS_CHEAT_EXPLAIN_*`，英文写进 `vba.rc`，
     中文写进 `lang/chs/strings.tsv`，然后 `python make_rc.py`
2. 如果下一行是它的数据，把 `extraLines` 设成 1。
3. 占位符只能是 `%s`（整段文本）或数字；改模板时**同时**改 `strings.tsv` 的
   源列和译文列，只改一边会让译文里的 `%X` 去读一个字符串指针。

## 验证办法

`winExplainCheats()` 只依赖 `cheatsList` 和三个外部符号，可以脱离模拟器单独跑：
把 `CheatExplain.cpp` 编成 obj，配一个提供 `cheatsList` / `winResLoadString` /
`winWideFromAnsi` 桩的程序，用 `rc.exe` + `link /DLL /NOENTRY` 把 `vba.rc` 和
`vba_chs.rc` 各打成资源模块，就能在同一台机器上打印两种语言的输出。
