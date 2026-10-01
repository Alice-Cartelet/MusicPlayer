# HTML 桌面背景：歌词与封面接口开发文档

面向编写 HTML 壁纸的开发者。当前协议版本为 `1`，本文的时间单位统一为毫秒。
在“设置 → HTML 桌面背景 → 歌词接口文档”中可以直接阅读本文，
程序目录中的 `HTML-WALLPAPER.md` 可用于编辑器查看、复制或分享。

## 1. 如何运行自己的 HTML

在播放器的“设置 → HTML 桌面背景”中，浏览选择本地 `.html` / `.htm`
文件，勾选“使用 HTML 作为桌面背景”，点击“确定”。
首次启动时默认选择程序目录内的 `music-wallpaper.html`；也可浏览选择自己的页面。
CSS、JavaScript、图片等文件可通过相对路径引用，请保留页面原有目录结构。

HTML 视图嵌入 Explorer 的背景 WorkerW，位于桌面图标下方；页面不会接管
桌面鼠标操作。多个屏幕使用一张跨越整个虚拟桌面的网页。
播放器其余功能保持原有行为。取消勾选或退出播放器后恢复原有系统壁纸；
启用状态和文件路径会保存，下次启动自动恢复。Explorer 重启后会尝试重新嵌入。

使用当前 Qt 6.11 / MinGW x64 工具链构建即可，无需 Qt WebEngine。
程序旁边需要 `WebView2Loader.dll`，qmake 构建时会自动复制它和微软许可证文件。
目标电脑需要安装 Microsoft Edge WebView2 Runtime。

WorkerW 是 Explorer 未公开的协议。如果系统版本无法提供背景宿主，
程序会等待并重试，不会显示覆盖桌面的普通窗口。

## 2. 通信方式与消息生命周期

接口通过 WebView2 的 `window.chrome.webview` 通信，无需开启 HTTP 服务、
指定端口、导入第三方 JavaScript 库或读取播放器的本地文件。
播放器发送的 `event.data` 已经是 JavaScript 对象，**不要再次 `JSON.parse(event.data)`**。
顶层 HTML 页面接收数据；如果界面位于 iframe，请由顶层页面转发数据。

基本顺序：

1. 创建页面元素，注册 `message` 监听器。
2. 可发送字符串 `musicplayer.requestState` 请求当前快照。
3. 接收 `musicplayer.track`，保存歌曲信息、全部歌词和封面。
4. 接收 `musicplayer.playback`，根据播放位置与状态更新页面。
5. 切歌时覆盖旧歌曲数据；缺少歌词或封面时清空相应元素。

自动补发与主动请求都按“歌曲消息 → 播放消息”的顺序返回快照。
歌曲切换、元数据更新或请求快照时，页面可能再次收到歌曲消息，因此处理应允许重复数据。
进度消息的频率取决于播放器通知，没有固定的帧率保证。
刷新页面、重新选择背景或 Explorer 重启后重新创建背景窗口，都会补发当前快照。
直接用 Chrome、Edge 等普通浏览器打开文件可查看样式，但不会收到播放器消息。

当前页面可发送的请求只有 `musicplayer.requestState`；此接口不提供播放、暂停、
切歌或读取任意本地文件的命令。请求的形式是字符串，不是 JSON 对象：

```javascript
window.chrome.webview.postMessage('musicplayer.requestState');
```

建议在初始化监听器之后请求一次，避免在每个进度消息或动画帧中反复请求。

## 3. 最小接入代码

播放器使用 WebView2 的 JSON 消息接口向所选 HTML 的顶层页面传递数据：

```html
<img id="cover" alt="歌曲封面" hidden>
<div id="lyric"></div>
<script>
const cover = document.getElementById('cover');
const lyric = document.getElementById('lyric');
window.chrome.webview.addEventListener('message', ({ data }) => {
  if (data.version !== 1) return;
  if (data.type === 'musicplayer.track') {
    const track = data.track;
    cover.hidden = !track.coverDataUrl;
    if (track.coverDataUrl) cover.src = track.coverDataUrl;
    else cover.removeAttribute('src');
    // track.lyrics 为完整时间轴，可自行实现滚动/逐字动画。
  }
  if (data.type === 'musicplayer.playback') {
    const playback = data.playback;
    lyric.textContent = playback.lyric.text;
    // playback.positionMs、durationMs 和 state 可用于同步动画。
  }
});
window.chrome.webview.postMessage('musicplayer.requestState');
</script>
```

## 4. 歌曲消息：musicplayer.track

完整消息结构示例（封面字符串在此省略实际 Base64 内容）：

```json
{
  "type": "musicplayer.track",
  "version": 1,
  "track": {
    "title": "示例歌曲",
    "artist": "示例歌手",
    "album": "示例专辑",
    "coverDataUrl": "data:image/png;base64,...",
    "lyrics": [
      {
        "startMs": 1000,
        "text": "你好世界",
        "translation": "Hello world",
        "hasTiming": true,
        "chars": [
          { "offsetMs": 0, "charIndex": 0 },
          { "offsetMs": 500, "charIndex": 1 },
          { "offsetMs": 1000, "charIndex": 2 },
          { "offsetMs": 1500, "charIndex": 3 },
          { "offsetMs": 2000, "charIndex": 4 }
        ]
      },
      {
        "startMs": 3000,
        "text": "下一句",
        "translation": "",
        "hasTiming": false,
        "chars": []
      }
    ]
  }
}
```

`track` 字段：

| 字段 | 内容 |
| --- | --- |
| `title`, `artist`, `album` | 歌名、艺术家、专辑；未知值为空字符串 |
| `coverDataUrl` | 内嵌封面转换后的 `data:image/png;base64,...`，可直接赋给 `img.src`；无封面为空字符串 |
| `lyrics` | 全部歌词，按时间排序；无歌词为 `[]` |

`title` 优先使用媒体元数据，缺失时使用播放器中的歌曲标题。
MP3 封面优先从 ID3 内嵌图片提取，缺失时尝试媒体元数据中的封面或缩略图。
图片转换为 PNG 后传输，因此网页不需要知道原始图片格式或本地路径。
`coverDataUrl` 是完整图片数据，保留完整字符串即可；不要拼接 `file://` 或额外 Base64 编码。

### 歌词时间轴字段

| 字段 | 类型 | 含义 |
| --- | --- | --- |
| `startMs` | number | 本句开始时刻，相对于整首歌曲 |
| `text` | string | 已去除时间标记的歌词文本 |
| `translation` | string | 本句译文；无译文为 `""` |
| `hasTiming` | boolean | 是否有字符级时间信息 |
| `chars` | array | 字符时间点；普通 LRC 通常为空 |
| `chars[].offsetMs` | number | 相对于本句开始时刻的偏移 |
| `chars[].charIndex` | number | 此时间点对应的字符边界，从 0 开始 |

每条 `lyrics` 为 `{startMs, text, translation, hasTiming, chars}`。
`chars` 中每项为 `{offsetMs, charIndex}`，时间相对本句 `startMs`，
`charIndex` 按 Unicode 码点计数（JavaScript 中使用 `Array.from(text)`）。
有逐字信息时可在相邻两个时间点之间插值高亮；普通 LRC 可按句的起止时间动画。

例如 `startMs = 1000`、字符时间点 `<500,1>` 表示整首歌曲播放到 `1500ms`
时到达第 1 个字符边界。末尾时间点可以是 `charIndex = 字符总数`，用于描述最后一个字符的结束。
JavaScript 的 `text.length` 按 UTF-16 码元计数，包含 emoji 时可能与字符边界不同；
对本接口使用 `Array.from(text)` 生成字符数组。

普通歌词的句尾使用下一句的 `startMs`，最后一句使用歌曲的 `durationMs`。
时间点是数据，不会自动更改页面颜色；需要开发者根据它们绘制自己的滚动、淡入或高亮效果。

## 5. 播放消息：musicplayer.playback

对应上述歌曲，播放至 `1250ms` 时的消息示例：

```json
{
  "type": "musicplayer.playback",
  "version": 1,
  "playback": {
    "positionMs": 1250,
    "durationMs": 180000,
    "state": "playing",
    "lyric": {
      "index": 0,
      "previous": "",
      "text": "你好世界",
      "next": "下一句",
      "translation": "Hello world",
      "startMs": 1000,
      "endMs": 3000,
      "progress": 0.125,
      "highlightedCharacters": 0.5,
      "hasTiming": true
    }
  }
}
```

`playback` 字段：

| 字段 | 内容 |
| --- | --- |
| `positionMs`, `durationMs` | 当前播放位置、总时长，单位毫秒 |
| `state` | `playing`、`paused` 或 `stopped` |
| `lyric` | 当前歌词快照，结构如下 |

`lyric` 包含 `index`、`previous`、`text`、`next`、`translation`、
`startMs`、`endMs`、`progress`（0–1）、`highlightedCharacters`（可为小数）
和 `hasTiming`。首句开始前或没有歌词时 `index = -1`，`text` 为空；
最后一句的 `endMs` 使用歌曲时长，时长尚未知时进度为 0。

| lyric 字段 | 类型 | 含义 |
| --- | --- | --- |
| `index` | number | 当前句在 `track.lyrics` 中的索引；无当前句为 -1 |
| `previous` | string | 上一句；不存在为空 |
| `text` | string | 当前句；首句开始前或无歌词时为空 |
| `next` | string | 下一句；首句开始前为第一句，最后一句之后为空 |
| `translation` | string | 当前句译文 |
| `startMs`, `endMs` | number | 当前句在整首歌曲中的起止时刻 |
| `progress` | number | 按句起止时间计算的进度，范围 0–1 |
| `highlightedCharacters` | number | 已高亮的字符数量，可为小数；与 `progress` 不一定等价 |
| `hasTiming` | boolean | 当前句是否具有字符级时间信息 |

例如 `highlightedCharacters = 2.5` 表示前两个字符已经高亮，第三个字符高亮一半。
有字符时间点时该值按相邻时间点插值，无字符时间点时按句进度估算。
`progress` 适合整句进度条；逐字效果应使用 `highlightedCharacters` 或 `chars`。
`state = paused` 或 `stopped` 时应停止自行推进动画；播放进度跳转后直接使用新位置，
不要假定时间只会增加。媒体尚未报告时长时 `durationMs` 可能为 0。

## 6. 完整可运行 HTML：封面、歌词和逐字高亮

将以下代码保存为 UTF-8 的 `index.html`，在播放器中选择它作为 HTML 背景即可。
这个示例直接使用播放快照绘制，较复杂的平滑插值示例见程序旁边的 `music-wallpaper.html`。

```html
<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>音乐壁纸</title>
  <style>
    html, body { height: 100%; margin: 0; overflow: hidden; }
    body {
      display: grid; place-items: center;
      background: #151d2d; color: #eef2ff;
      font-family: "Microsoft YaHei", sans-serif;
    }
    main { width: min(900px, 80vw); text-align: center; }
    #cover { width: 160px; height: 160px; object-fit: cover; border-radius: 20px; }
    #cover[hidden] { display: none; }
    h1 { font-size: 22px; }
    #artist, #previous, #next, #time { color: #a6b4cc; }
    #current { min-height: 60px; font-size: 36px; line-height: 1.6; }
    #current span {
      background-clip: text; -webkit-background-clip: text; color: transparent;
    }
    #translation { min-height: 26px; color: #c5d8f4; }
  </style>
</head>
<body>
  <main>
    <img id="cover" alt="歌曲封面" hidden>
    <h1 id="title">等待播放</h1><p id="artist"></p>
    <p id="previous"></p><div id="current"></div>
    <p id="translation"></p><p id="next"></p><p id="time"></p>
  </main>
  <script>
    const $ = id => document.getElementById(id);
    let lastText = null;
    let lastCover = '';
    let hasLyrics = false;

    function showTrack(track) {
      hasLyrics = Boolean(track.lyrics?.length);
      $('title').textContent = track.title || '等待播放';
      $('artist').textContent = [track.artist, track.album].filter(Boolean).join(' · ');
      const url = track.coverDataUrl || '';
      if (url !== lastCover) {
        lastCover = url;
        $('cover').hidden = !url;
        if (url) $('cover').src = url;
        else $('cover').removeAttribute('src');
      }
      if (!track.lyrics?.length) {
        lastText = null;
        $('current').textContent = '暂无歌词';
        $('previous').textContent = '';
        $('next').textContent = '';
        $('translation').textContent = '';
      }
    }

    function showPlayback(playback) {
      const lyric = playback.lyric || {};
      const text = lyric.text || (hasLyrics ? '' : '暂无歌词');
      if (text !== lastText) {
        lastText = text;
        $('current').replaceChildren(...Array.from(text, character => {
          const span = document.createElement('span');
          span.textContent = character;
          return span;
        }));
      }
      const lit = lyric.highlightedCharacters || 0;
      Array.from($('current').children).forEach((span, index) => {
        const percent = Math.max(0, Math.min(1, lit - index)) * 100;
        span.style.backgroundImage =
          `linear-gradient(to right, #a6d3ff ${percent}%, #ffffff55 ${percent}%)`;
      });
      $('previous').textContent = lyric.previous || '';
      $('next').textContent = lyric.next || '';
      $('translation').textContent = lyric.translation || '';
      const seconds = Math.floor((playback.positionMs || 0) / 1000);
      $('time').textContent =
        `${Math.floor(seconds / 60)}:${String(seconds % 60).padStart(2, '0')} · ${playback.state}`;
    }

    if (window.chrome?.webview) {
      window.chrome.webview.addEventListener('message', ({ data }) => {
        if (!data || data.version !== 1) return;
        if (data.type === 'musicplayer.track') showTrack(data.track);
        if (data.type === 'musicplayer.playback') showPlayback(data.playback);
      });
      window.chrome.webview.postMessage('musicplayer.requestState');
    } else {
      $('title').textContent = '请在播放器的 HTML 桌面背景中打开此页面';
    }
  </script>
</body>
</html>
```

## 7. 更平滑的动画与暂停、跳转处理

需要每帧动画时，在收到播放消息后保存 `positionMs`、`state`、`durationMs`
和 `performance.now()`。只在 `playing` 状态下估算帧内播放位置：

```javascript
let playback = { positionMs: 0, durationMs: 0, state: 'stopped' };
let receivedAt = performance.now();

function onPlayback(data) {
  playback = data;
  receivedAt = performance.now();
}

function positionAt(now) {
  const advance = playback.state === 'playing'
    ? Math.min(1500, Math.max(0, now - receivedAt)) : 0;
  const position = playback.positionMs + advance;
  return playback.durationMs > 0 ? Math.min(position, playback.durationMs) : position;
}
```

这里最多向前估算 1500ms，避免通知中断时画面无限前进。这是示例策略，不是协议要求。
暂停、继续、拖动进度条时都以新消息重设基准；暂停时保留当前位置，停止自动估算。
每帧按时间轴选择最后一个 `startMs <= position` 的句子，找不到时使用索引 -1。
随后按本句的 `chars` 插值：

```javascript
const fraction = (position - line.startMs - a.offsetMs) / (b.offsetMs - a.offsetMs);
const highlighted = a.charIndex + (b.charIndex - a.charIndex) * fraction;
```

此计算仅适用于当前位置位于 `a`、`b` 两个时间点之间，且 `b.offsetMs > a.offsetMs`。
首个字符时间点之前使用 0，最后一个时间点之后使用字符总数；没有逐字时间时按句进度估算。
无需每帧重新解码封面或重建全部歌词元素；歌词换句时更新文本，平时只更新高亮比例。

## 8. 歌词文件准备

接口使用播放器当前加载的歌词文件，独立于悬浮歌词及壁纸歌词开关。
HTML 页面不需要自己读取 `.lrc` / `.lrcx`。
例如音乐名为 `song.mp3`，播放器先在设置的歌词目录中查找，再在音乐所在目录中查找：

1. `song.mp3 - .lrcx`
2. `song.mp3 - .lrc`
3. `song.lrcx`
4. `song.lrc`

文件建议使用 UTF-8；当前解析器的时间戳需要小数部分，例如 `[00:01.000]` 或 `[00:01.00]`。

普通歌词示例：

```text
[00:01.000]你好世界
[00:03.000]下一句
```

逐字歌词与译文示例（UTF-8 的 `song.lrcx`）：

```text
[00:01.000]你好世界
[00:01.000][tt]<0,0><500,1><1000,2><1500,3><2000,4>
[00:01.000]Hello world[00:03.000]
[00:03.000]下一句
```

`[tt]` 时间点的单位是毫秒，相对于本句开始时刻；使用明确的 `<偏移,字符边界>`。
译文与主歌词使用相同开始时间，并带结束时间标记。
也支持每段文字前带时间戳的逐字形式，例如：

```text
[00:01.000]你[00:01.500]好[00:02.000]世[00:02.500]界[00:03.000]
```

## 9. 常见问题与开发检查

| 现象 | 检查方式 |
| --- | --- |
| `window.chrome.webview` 不存在 | 使用播放器启用 HTML 背景；普通浏览器没有该接口 |
| 页面只在开始时有数据 | 检查监听器是否仍存在，框架组件卸载后是否误移除了监听器 |
| 晚加载的脚本错过初始数据 | 注册监听器后发送 `musicplayer.requestState` |
| 有歌名但没有歌词 | 检查播放器歌词目录、文件命名、UTF-8 编码和带小数的时间戳 |
| 没有译文或逐字时间 | 并非所有文件包含这些数据；检查 `translation` 和 `hasTiming` |
| 中文正常，emoji 的高亮错位 | 使用 `Array.from(text)` 而不是 `text.length` 或 `text.split('')` |
| 暂停后高亮还在走 | 只在 `state === 'playing'` 时向前估算位置 |
| 切歌后仍显示上一首封面 | `coverDataUrl === ''` 时移除 `src` 并隐藏图片 |
| 歌词显示成标签或触发脚本 | 使用 `textContent`，不要把歌词放进 `innerHTML` |
| iframe 没收到数据 | 在顶层页面监听并转发，接口不会自动向 iframe 广播 |
| 壁纸上的按钮点不到 | 壁纸处于桌面图标下方，桌面输入交由 Windows 处理 |

注册监听器时若使用命名函数，可在组件卸载或页面销毁时移除它：

```javascript
function onMessage(event) {
  // 根据 event.data.type 处理消息。
}
window.chrome.webview.addEventListener('message', onMessage);
// 组件卸载时：
window.chrome.webview.removeEventListener('message', onMessage);
```

## 10. 消息发送与打包行为

切歌、歌词目录变更、封面修改和元数据更新时发送歌曲消息并补发播放快照；进度、跳转、
时长和播放/暂停状态变化时发送播放消息。封面及完整歌词不会随每个进度包重复传输。
页面加载完成、背景窗口重新创建时会自动补发两条当前快照；
晚注册监听器的页面可发送 `musicplayer.requestState` 主动请求快照。
接口独立于悬浮歌词及壁纸歌词开关，使用播放器当前加载的 `.lrc` / `.lrcx` 文件。
无歌词或无封面的新歌曲会发送空值，让页面清除上一首的内容。

`music-wallpaper.html` 已实现封面、歌名、译文、上下句歌词和逐字高亮。
传入的文本应使用 `textContent` 显示，不要把歌词拼接成 HTML 或 JavaScript。
在普通浏览器中打开页面不会收到播放器数据，需在播放器的 HTML 背景中使用。
