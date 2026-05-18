# 🎵 本地音乐播放器（Qt C++）
![C++](https://img.shields.io/badge/C++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-%23217346.svg?style=for-the-badge&logo=Qt&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-blue.svg?style=for-the-badge)

基于 Qt 6 开发的简洁本地音乐播放器，透明，功能极简，只有桌面歌词显示功能。
<img width="1204" height="993" alt="屏幕截图 2026-05-18 200635" src="https://github.com/user-attachments/assets/304b7efd-e0f2-4fa2-bec6-31a9a28565a7" />

---
# 🎵 MusicPlayer

一个基于 Qt 开发的现代化桌面音乐播放器，支持歌词逐字高亮、专辑封面显示、毛玻璃 UI、美化动画与播放进度记忆等功能，兼顾视觉体验与日常使用。

---

## ✨ 核心特性 (Features)

* 🎶 **本地音乐自动扫描**

  * 自动递归扫描音乐目录，识别常见音频格式并生成播放列表。
  * 支持多级文件夹读取，无需手动逐个添加歌曲。
  * 智能排序：优先英文、中文，其余字符自动排列。

* 📝 **高级歌词系统**

  * 支持传统 `.lrc` 歌词。
  * 支持逐字歌词高亮动画（卡拉 OK 风格）。
  * 兼容多种逐字格式：

    * `[00:37.040]<00:37.04>歌词`
    * `[00:00.00]对 [00:00.36]但 [00:00.90]我`
  * 无歌词时自动显示 “暂无歌词”。

* 🎨 **现代化视觉设计**

  * 无边框窗口设计。
  * 半透明亚克力 / 毛玻璃背景效果。
  * 动态模糊背景（基于当前歌曲封面生成）。
  * 自定义 CSS 绘制播放器按钮与进度条。
  * 大尺寸歌词阴影与柔和发光效果，提高可读性。

* 💿 **专辑封面支持**

  * 自动读取音频文件内嵌封面。
  * 背景可自动使用当前歌曲封面生成模糊氛围图。
  * 无封面时自动显示默认图片。

* ⏯️ **播放控制**

  * 播放 / 暂停 / 上一曲 / 下一曲。
  * 进度拖动与音量调节。
  * 自动播放下一首歌曲。
  * 单实例运行检测，避免重复打开播放器。

* 💾 **播放记忆功能**

  * 自动保存上次播放歌曲。
  * 自动记录播放进度与音量。
  * 下次启动后可继续上次播放位置。

* ⚡ **细节优化**

  * Marquee 滚动长标题。
  * 平滑动画与透明渐变。
  * UTF-8 多语言歌词兼容（支持中文 / 日文 / 梵文等）。
  * 高 DPI 显示优化。

---

## 📸 界面展示 (Screenshots)

| 主播放界面 | 修改界面 |
| :---: | :---: |
| <img width="1204" height="993" alt="" src="https://github.com/user-attachments/assets/304b7efd-e0f2-4fa2-bec6-31a9a28565a7" /> | <img width="1203" height="993" alt="" src="https://github.com/user-attachments/assets/cb8d241f-7d03-420b-a9a2-12a0fe993c73" /> |
| *播放器主界面截图* | *修改界面* |

---

## 🛠️ 技术栈 (Tech Stack)

* **编程语言**：C++
* **GUI 框架**：Qt Widgets
* **音频模块**：Qt Multimedia
* **构建工具**：qmake
* **标准支持**：C++17

---

## 🚀 快速开始 (Getting Started)

### 直接下载

* 在 Release 页面下载对应平台的可执行文件并运行。

### 手动编译

#### 环境依赖

* Qt 5.15+ 或 Qt 6.x
* C++17 编译器（MSVC / MinGW / GCC）
* Qt Multimedia 模块

#### 编译方法

```bash
qmake
make
```

或直接使用 Qt Creator 打开 `.pro` 文件编译运行。

---

## 💡 使用指南 (Usage)

### 添加音乐

* 在设置中选择本地音乐目录。
* 软件会自动扫描目录中的音频文件。

### 歌词支持

将歌词文件与歌曲放置在同一目录，也可分别置于不同目录：

```text
song.mp3
song.mp3 - .lrc
```

播放器会自动匹配歌词。

### 播放控制

* 空格键：播放 / 暂停
* 鼠标拖动：调整进度
* 点击按钮：切换歌曲

### 个性化

* 支持修改播放器主题样式。
* 支持自定义背景与透明度。
* 可调整歌词字体与阴影效果。

---

## 📂 项目结构 (Project Structure)

```text
MusicPlayer/
├── main.cpp
├── mainwindow.cpp
├── mainwindow.h
├── playlist.cpp
├── playlist.h
├── lrcxparser.cpp
├── lrcxparser.h
├── lyricsoverlay.cpp
├── lyricsoverlay.h
├── settingsdialog.cpp
├── settingsdialog.h
├── resources.qrc
└── MusicPlayer.pro
```

---

## 🔧 支持的音频格式 (Supported Formats)

* MP3
* FLAC
* WAV
* OGG
* AAC
* M4A

（具体取决于 Qt Multimedia 后端支持）

---

## 📄 许可证 (License)

* 本项目基于 Qt 开发，遵循 LGPL v3 协议。
* 项目本体采用 MIT License 开源。

```text
MIT License

Copyright (c) 2026

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.
```

---

## ❤️ 致谢 (Acknowledgements)

* Qt Framework
* Qt Multimedia
* 所有测试与使用本项目的用户

---

