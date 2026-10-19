# 魔兽争霸3冰封王座 / Switch

《魔兽争霸 III》OpenRealm 引擎的 Nintendo Switch 原生移植测试版。

应用显示名称：**魔兽争霸3冰封王座**；移植版显示作者：**小帆**。

**状态：已完成 Switch 交叉编译并生成 NSP/NRO。尚未通过 Switch 实机验证，不能视为稳定可玩版本。**

基于 [corepunch/open-realm](https://github.com/corepunch/open-realm)，固定上游提交
`8853d069c21c94a981121066e1d78e967676fae4`。原项目采用 MIT 许可，版权和许可证保留在仓库中。

## 下载与安装

从本仓库的 [Releases](https://github.com/zqzhuhai-commits/Warcraft3-switch/releases)
下载 `魔兽争霸3冰封王座-Switch-0.1.4.nsp`。这是包含原生引擎的完整应用包，不是 NRO 前端。
也提供 NRO 作为备用运行方式；NRO 需使用完整应用内存模式，不支持相册 Applet 模式。

1. 将你拥有的 **Warcraft III Classic 的 MPQ 文件**放到 SD 卡目录：
   `sd:/switch/warcraft3/data/`。
2. 必需资源通常包括 `War3.mpq` 和 `War3Local.mpq`；冰封王座还需
   `War3x.mpq` 和 `War3xLocal.mpq`。可选的 `Maps/` 文件夹也放在该目录。
3. 升级旧版时，把 SD 卡的 `switch/openrealm` 文件夹重命名为 `switch/warcraft3`，资源和存档可一并保留。若新目录已存在，请先备份，再合并所需文件。
4. 在支持自制软件的 Switch 环境安装 NSP 后，从主界面启动。
5. 没有 `War3.mpq` 时会显示资源说明页，按 B 返回。存在 `War3x.mpq` 时默认进入 TFT 模式。

游戏资源和密钥均不随仓库或安装包提供；Reforged 的 CASC 资源不在此测试版的支持范围内。

## 操作

| 按键 | 功能 |
|---|---|
| 左摇杆 | 移动鼠标指针 |
| L + 左摇杆 | 精细移动指针 |
| A / ZR | 左键选择；按住拖动框选 |
| R | 右键 / 移动、攻击等智能指令 |
| ZL | Shift / 追加选择、队列指令 |
| 右摇杆 | 移动视角 |
| B | 取消 / Escape |
| X / Y | 第一个英雄 / 空闲工人 |
| ＋ / － | 游戏菜单 / 任务 |
| 十字键上 / 下 | 缩放 |
| 十字键左 / 右 | 切换子组 / 快速镜头 |
| 按下左 / 右摇杆 | 快速保存 / 截图 |
| 触屏点按 / 单指滑动 | 鼠标左键 / 按住左键拖动框选 |
| 触屏原地长按 0.5 秒 / 双指触碰 | 鼠标右键 / 智能指令 |

## 存档与故障信息

- 配置、战役进度、存档：`sd:/switch/warcraft3/user/`
- 日志：`sd:/switch/warcraft3/warcraft3.log`
- 截图：`sd:/switch/warcraft3/screenshots/`
- 默认设置：1280×720、30 FPS 上限、垂直同步。这是初始设置，不代表已经实测达到 30 FPS。

若发生黑屏或退出，请保留日志，并记录系统/Atmosphère 版本、使用的 MPQ 版本和触发步骤。

## 已完成的检查

- devkitA64 / libnx / SDL2 / switch-mesa 原生交叉编译，静态链接完整引擎。
- NSP 容器与内容文件摘要检查；NCA 解密及哈希验证。
- 原生测试套件：34 份报告、6,952 个测试、0 个失败；Switch 手柄、触屏事件与文件系统适配器测试通过。
- 中文段落横向换行、英文长词、颜色控制、显式换行的生产渲染布局测试通过（调试及优化构建）。
- 未改动网络消息布局、实体/玩家状态或存档格式。

## 尚未验证与限制

- 尚无 Switch 实机安装、GPU 渲染、地图加载、音频、存档恢复、休眠恢复的验证结果。
- 此构建保留原生存档实现，但实机存档可靠性尚未验证。
- 不含可选 FFmpeg，预渲染影片和依赖 FFmpeg 的背景音乐暂未启用；普通音效与语音代码已编入。
- 联机、自定义大型地图、战役完整通关均未认证；会继承上游引擎的功能缺口。
- 文件系统兼容层面向单实例、引擎线程串行保存，不支持其他进程同时修改其存档目录。

## 从源码构建

使用 devkitPro 安装 `devkitA64 libnx switch-tools switch-sdl2 switch-mesa switch-zlib`，然后：

```sh
make font
python3 port/switch/build.py
# 本地安装 hacbrewpack 并准备自己的私有 keyset 后：
python3 port/switch/patch-hacbrewpack.py /path/to/hacBrewPack
make -C /path/to/hacBrewPack
python3 port/switch/package.py --keyset /private/path/prod.keys
```

产物位于 `build/switch/`；`package/` 中包含 NSP 打包输入。打包使用私有的本地 keyset，
不得把 keyset、原版资源或包含密钥的工具日志上传到仓库。GitHub Actions 只编译无密钥的 NRO/NSP 输入。

技术说明见 [Switch 移植文档](docs/platform/switch.md)，上游介绍见 [README.upstream.md](README.upstream.md)。
