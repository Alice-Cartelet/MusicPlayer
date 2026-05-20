# 🎵 极简本地桌面歌词音乐播放器
![C++](https://img.shields.io/badge/C++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-%23217346.svg?style=for-the-badge&logo=Qt&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-blue.svg?style=for-the-badge)

基于 Qt 6 开发的简洁本地音乐播放器，有半透明磨砂效果，功能极简，只有附加桌面歌词显示功能。
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

  * 支持传统 `.lrcx` 歌词。
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
  * 可以对桌面歌词颜色进行修改，在设置界面新增修改歌词颜色功能
  * 播放界面可以强制置顶，添加钉子功能。
  * 新增最小化到托盘功能，可在设置界面打开或关闭。
  * 新增当鼠标处于歌词上方自动隐藏歌词功能，可在设置界面打开，当开启时，鼠标会穿过歌词，因此拖拽歌词将不再可用。
  * 新增记忆歌词位置功能，当下次打开时，无需重新拖拽歌词到适当位置。
  * 新增添加可以改变桌面歌词大小的功能。
  * 新增设置创建一个小控制窗，这个窗口将永远处于最上层避免被覆盖，并且可以修改其透明度。
  * 新增添加歌词编辑功能，在主界面最右侧✏按钮可编辑歌词，没有歌词文件时将创建。
  * 高 DPI 显示优化。

---

## 📸 界面展示 (Screenshots)

| 主播放界面 | 修改界面 |
| :---: | :---: |
| <img width="1204" height="993" alt="" src="https://github.com/user-attachments/assets/304b7efd-e0f2-4fa2-bec6-31a9a28565a7" /> | <img width="1232" height="1078" alt="屏幕截图 2026-05-20 101247" src="https://github.com/user-attachments/assets/75134249-fca6-49fe-aa0e-1967ac4749dd" /> |
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
song.mp3 - .lrcx
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
* 使用Qt开发，遵循LGPL v3许可。
* 并遵循本项目采用 MIT License 协议。
```
MIT 许可证
版权所有 (c) 2026 Alice-Cartelet
特此免费授予任何获得本软件及相关文档文件（以下简称“软件”）副本的人员不受限制地处理本软件
的权利，包括但不限于使用、复制、修改、合并、发布、分发、再许可和/或出售本软件副本的权利，
并允许向其提供本软件的人员这样做，但须符合以下条件：
上述版权声明和本许可声明应包含在本软件的所有副本或实质性部分中。
本软件按“原样”提供，不提供任何形式的明示或暗示的保证，包括但不限于对适销性、特定用途适用
性和非侵权性的保证。在任何情况下，作者或版权持有人均不对因本软件或本软件的使用或其他交易
而产生、引起或与之相关的任何索赔、损害或其他责任负责，无论是在合同诉讼、侵权行为还是其他
方面。
MIT License
Copyright (c) 2026 Alice-Cartelet
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
