# 俄罗斯方块 / Terminal Tetris

一个用 **C 语言** 编写的终端版俄罗斯方块小游戏。

本项目不依赖 SDL、ncurses 或其他图形库，核心实现基于 ANSI 终端转义序列与 POSIX `termios`，适合在 macOS / Linux 终端中编译运行。

## 功能

- 10 × 20 标准棋盘
- 7-bag 随机方块生成
- 七种经典方块：I、O、T、S、Z、J、L
- 左右移动、软降、硬降、顺/逆时针旋转
- 简单 wall kick，靠墙旋转更自然
- 下一块预览
- 幽灵落点提示
- 消行、得分、等级、速度递增
- 暂停、重新开始、退出

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
| P | 暂停 / 继续 |
| R | 游戏结束后重新开始 |
| Q | 退出 |

## 项目结构

```text
.
├── Makefile
├── README.md
├── LICENSE
├── .gitignore
├── .github
│   └── workflows
│       └── ci.yml
└── src
    └── main.c
```

## 说明

这是一个单文件核心实现，适合学习 C 语言、终端 UI、游戏主循环、碰撞检测、旋转逻辑、消行和计分。

注意：本项目使用 POSIX 终端接口，主要面向 macOS / Linux。Windows 需要 WSL、MSYS2、Cygwin 或自行适配终端输入输出。