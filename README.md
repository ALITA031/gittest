# 俄罗斯方块 / Terminal Tetris Pro

一个用 **C 语言** 编写的终端版俄罗斯方块。它不是单文件 demo，而是一个可继续维护和扩展的小型游戏项目：游戏引擎、终端输入、渲染、测试和 CI 分离。

本项目主要面向 macOS / Linux 终端。核心不依赖 SDL、ncurses 或其他图形库，使用 ANSI 终端转义序列和 POSIX `termios` 实现实时输入与绘制。

## 当前定位

Terminal Tetris Pro 的目标不是只做一个能跑的俄罗斯方块，而是做成一个结构清楚、可测试、可扩展的 C 语言游戏项目：

- 终端客户端可以直接玩；
- 核心游戏引擎保持独立，方便测试和未来移植；
- 项目有 CI、sanitizer、单元测试和文档；
- 后续可以继续扩展排行榜、配置、回放、AI 和图形前端。

## 主要特性

### 玩法

- 标准 10 × 20 可见棋盘，额外 2 行隐藏出生区
- 七种经典方块：I、O、T、S、Z、J、L
- 7-bag 随机系统，降低连续极端坏块概率
- 5 个 Next 队列预览
- Hold 暂存块
- Ghost Piece 幽灵落点
- Soft Drop / Hard Drop
- Lock Delay：落地后有短暂调整时间
- 限制 Lock Reset，避免无限拖延
- SRS 风格旋转与 wall kick，包括 I 块独立 kick 表
- 基础 T-Spin 判断
- Single / Double / Triple / Tetris / T-Spin 计分
- Combo 连击加分
- Back-to-Back 加分
- 等级提升后自动加速
- 本地高分保存

### 工程结构

- `src/game.c`：纯游戏逻辑，不依赖终端，方便测试和移植
- `src/render.c`：ANSI 终端渲染
- `src/terminal.c`：非阻塞输入、raw mode、计时
- `src/main.c`：主循环、高分读写、输入分发
- `tests/test_game.c`：游戏引擎单元测试
- `.github/workflows/ci.yml`：GitHub Actions 自动编译和测试

## 编译运行

### macOS / Linux

```bash
make
./tetris
```

或者：

```bash
make run
```

### 运行测试

```bash
make test
```

### 运行 sanitizer 测试

```bash
make sanitize
```

### 完整本地检查

```bash
make check
```

### 发布构建

```bash
make release
```

### 清理构建产物

```bash
make clean
```

## 操作方式

| 按键 | 功能 |
|---|---|
| ← / → 或 A / D | 左右移动 |
| ↓ 或 S | 软降 |
| ↑ 或 W / X | 顺时针旋转 |
| Z | 逆时针旋转 |
| Space | 硬降 |
| C | Hold 暂存 |
| P | 暂停 / 继续 |
| R | 重新开始 |
| Q | 退出 |

## 高分保存位置

优先使用：

```text
$XDG_DATA_HOME/terminal-tetris-pro/highscore
```

如果没有设置 `XDG_DATA_HOME`，则使用：

```text
~/.local/share/terminal-tetris-pro/highscore
```

## 项目结构

```text
.
├── Makefile
├── README.md
├── LICENSE
├── CONTRIBUTING.md
├── ROADMAP.md
├── .gitignore
├── .github
│   └── workflows
│       └── ci.yml
├── docs
│   └── ARCHITECTURE.md
├── src
│   ├── game.c
│   ├── main.c
│   ├── render.c
│   ├── render.h
│   ├── terminal.c
│   ├── terminal.h
│   └── tetris.h
└── tests
    └── test_game.c
```

## 开发文档

- [ROADMAP.md](ROADMAP.md)：项目路线图，说明如何从终端小游戏扩展成可维护的开源项目。
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)：架构说明，解释 engine、terminal、render 和 application 层的边界。
- [CONTRIBUTING.md](CONTRIBUTING.md)：贡献指南，包含本地检查命令、代码风格和 PR 检查项。

## 兼容性说明

- 推荐 macOS Terminal、iTerm2、Linux GNOME Terminal、Kitty、Alacritty 等现代终端。
- Windows 建议使用 WSL。原生 Windows 需要额外适配 Console API。
- 终端窗口建议至少 82 列 × 26 行。

## 后续可扩展方向

- 加入可配置按键和速度曲线
- 加入排行榜文件
- 加入 AI 自动玩俄罗斯方块
- 加入回放系统
- 加入 SDL2 / raylib 图形前端，复用现有 `src/game.c` 引擎
- 加入更严格的 Guideline T-Spin Mini 与 Perfect Clear 规则

## License

MIT
