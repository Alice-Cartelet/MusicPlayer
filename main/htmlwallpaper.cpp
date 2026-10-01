#include "htmlwallpaper.h"

#include <QDir>
#include <QCoreApplication>
#include <QFileInfo>
#include <QLibrary>
#include <QPointer>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QJsonDocument>
#include <functional>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include "third_party/webview2/WebView2.h"

namespace {
// Explicit IIDs also work with MinGW, where MSVC's __uuidof is unavailable.
const IID environmentHandlerId = {0x4e8a3389, 0xc9d8, 0x4bd2, {0xb6, 0xb5, 0x12, 0x4f, 0xee, 0x6c, 0xc1, 0x4d}};
const IID controllerHandlerId = {0x6c4819f3, 0xc9b7, 0x4260, {0x81, 0x27, 0xc9, 0xf5, 0xbd, 0xe7, 0xf6, 0x8c}};
const IID navigationHandlerId = {0xd33a35bf, 0x1c49, 0x4f98, {0x93, 0xab, 0x00, 0x6e, 0x05, 0x33, 0xfe, 0x1c}};
const IID messageHandlerId = {0x57213f19, 0x00e6, 0x49fa, {0x8e, 0x07, 0x89, 0x8e, 0xa0, 0x1e, 0xcb, 0xd2}};

template<class Interface, class A, class B>
class Callback final : public Interface
{
public:
    Callback(const IID &id, std::function<HRESULT(A, B)> fn) : m_id(id), m_fn(std::move(fn)) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void **object) override
    {
        if (!object) return E_POINTER;
        *object = nullptr;
        if (IsEqualIID(id, IID_IUnknown) || IsEqualIID(id, m_id)) {
            *object = static_cast<Interface *>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refs); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG refs = InterlockedDecrement(&m_refs);
        if (!refs) delete this;
        return refs;
    }
    HRESULT STDMETHODCALLTYPE Invoke(A a, B b) override { return m_fn(a, b); }
private:
    IID m_id;
    std::function<HRESULT(A, B)> m_fn;
    LONG m_refs = 1;
};
using EnvironmentCallback = Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, HRESULT, ICoreWebView2Environment *>;
using ControllerCallback = Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, HRESULT, ICoreWebView2Controller *>;
using NavigationCallback = Callback<ICoreWebView2NavigationCompletedEventHandler, ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *>;
using MessageCallback = Callback<ICoreWebView2WebMessageReceivedEventHandler, ICoreWebView2 *, ICoreWebView2WebMessageReceivedEventArgs *>;

HWND findBackgroundWindow()
{
    HWND background = nullptr;
    EnumWindows([](HWND window, LPARAM data) -> BOOL {
        if (FindWindowExW(window, nullptr, L"SHELLDLL_DefView", nullptr)) {
            HWND worker = FindWindowExW(nullptr, window, L"WorkerW", nullptr);
            if (worker) {
                *reinterpret_cast<HWND *>(data) = worker;
                return FALSE;
            }
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&background));
    // Newer Explorer versions can place the background WorkerW inside Progman.
    if (!background) {
        HWND progman = FindWindowW(L"Progman", nullptr);
        HWND worker = nullptr;
        while (progman && (worker = FindWindowExW(progman, worker, L"WorkerW", nullptr))) {
            if (!FindWindowExW(worker, nullptr, L"SHELLDLL_DefView", nullptr)) {
                background = worker;
                break;
            }
        }
    }
    return background;
}

HWND createBackgroundWindow()
{
    HWND progman = FindWindowW(L"Progman", nullptr);
    if (!progman) return nullptr;
    DWORD_PTR result = 0;
    // Explorer's wallpaper host protocol is undocumented: validate discovery
    // and never fall back to an ordinary window over the user's desktop.
    SendMessageTimeoutW(progman, 0x052c, 0, 0, SMTO_ABORTIFHUNG, 1000, &result);
    HWND background = findBackgroundWindow();
    if (!background) {
        SendMessageTimeoutW(progman, 0x052c, 0x0d, 0, SMTO_ABORTIFHUNG, 1000, &result);
        SendMessageTimeoutW(progman, 0x052c, 0x0d, 1, SMTO_ABORTIFHUNG, 1000, &result);
        background = findBackgroundWindow();
    }
    return background;
}

LRESULT CALLBACK wallpaperWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    if (message == WM_NCHITTEST) return HTTRANSPARENT;
    return DefWindowProcW(window, message, wParam, lParam);
}
}
#endif

class HtmlWallpaper::Private
{
public:
    explicit Private(HtmlWallpaper *owner) : q(owner)
    {
        timer.setInterval(2000);
        QObject::connect(&timer, &QTimer::timeout, q, [this] { maintainHost(); });
#ifdef Q_OS_WIN
        comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        loader.setLoadHints(QLibrary::PreventUnloadHint);
        loader.setFileName(QCoreApplication::applicationDirPath() + "/WebView2Loader.dll");
#endif
    }
    ~Private()
    {
        stop();
#ifdef Q_OS_WIN
        if (SUCCEEDED(comResult)) CoUninitialize();
#endif
    }
    void stop()
    {
        ++generation; // Invalidate every outstanding asynchronous callback.
        timer.stop();
        starting = false;
        pageLoaded = false;
#ifdef Q_OS_WIN
        if (controller) {
            controller->put_IsVisible(FALSE);
            controller->Close();
        }
        if (webview) { webview->Release(); webview = nullptr; }
        if (controller) { controller->Release(); controller = nullptr; }
        if (host && IsWindow(host)) DestroyWindow(host);
        host = nullptr;
        background = nullptr;
#endif
    }
    void fail(const QString &message)
    {
        enabled = false;
        stop();
        emit q->enabledChanged(false);
        emit q->errorOccurred(message);
    }
    void maintainHost()
    {
#ifdef Q_OS_WIN
        if (!enabled) return;
        if (!host || !IsWindow(host) || !IsWindow(background) || GetParent(host) != background) {
            stop();
            timer.start();
            // Explorer may still be restarting. Wait for its desktop to return.
            if (FindWindowW(L"Progman", nullptr)) start();
            return;
        }
        resizeHost();
#endif
    }
    void start()
    {
#ifdef Q_OS_WIN
        if (starting || controller || !enabled) return;
        if (FAILED(comResult)) {
            fail(QStringLiteral("无法初始化 HTML 背景的浏览器线程。"));
            return;
        }
        background = createBackgroundWindow();
        if (!background) return; // Retry while Explorer is unavailable.
        WNDCLASSW windowClass = {};
        windowClass.lpfnWndProc = wallpaperWindowProc;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpszClassName = L"MusicPlayerHtmlWallpaper";
        RegisterClassW(&windowClass);
        host = CreateWindowExW(WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
                              windowClass.lpszClassName, L"MusicPlayer HTML Wallpaper",
                              WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                              0, 0, 1, 1, background, nullptr, windowClass.hInstance, nullptr);
        if (!host) {
            fail(QStringLiteral("无法创建桌面背景窗口。"));
            return;
        }
        resizeHost();
        using CreateEnvironment = HRESULT (STDAPICALLTYPE *)(PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions *, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *);
        const auto createEnvironment = reinterpret_cast<CreateEnvironment>(loader.resolve("CreateCoreWebView2EnvironmentWithOptions"));
        if (!createEnvironment) {
            fail(QStringLiteral("无法加载 WebView2Loader.dll，请将它放在播放器程序旁边。"));
            return;
        }
        const QString profile = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/HtmlWallpaper/WebView2";
        if (!QDir().mkpath(profile)) {
            fail(QStringLiteral("无法创建 HTML 背景的浏览器缓存目录。"));
            return;
        }
        starting = true;
        const quint64 request = generation;
        QPointer<HtmlWallpaper> guard(q);
        auto *callback = new EnvironmentCallback(environmentHandlerId,
            [guard, request](HRESULT result, ICoreWebView2Environment *environment) -> HRESULT {
                if (!guard || !guard->d->enabled || guard->d->generation != request) return S_OK;
                if (FAILED(result) || !environment) {
                    guard->d->fail(QStringLiteral("无法启动 WebView2。请安装 Microsoft Edge WebView2 Runtime。"));
                    return S_OK;
                }
                guard->d->createController(environment, request);
                return S_OK;
            });
        const HRESULT result = createEnvironment(nullptr, reinterpret_cast<PCWSTR>(profile.utf16()), nullptr, callback);
        callback->Release();
        if (FAILED(result)) fail(QStringLiteral("启动 HTML 背景失败，请检查 WebView2 运行时。"));
#else
        fail(QStringLiteral("HTML 桌面背景目前仅支持 Windows。"));
#endif
    }
    void postData(const QString &type, const QString &key, const QJsonObject &data)
    {
#ifdef Q_OS_WIN
        if (!enabled || !pageLoaded || !webview) return;
        LPWSTR source = nullptr;
        if (FAILED(webview->get_Source(&source))) return;
        const bool selectedPage = QUrl(QString::fromWCharArray(source)).matches(
            QUrl::fromLocalFile(path), QUrl::RemoveQuery | QUrl::RemoveFragment);
        CoTaskMemFree(source);
        if (!selectedPage) return;
        // Serialize as JSON, never concatenate lyric text into executable code.
        const QString json = QString::fromUtf8(QJsonDocument(QJsonObject{
            {"type", type}, {"version", 1}, {key, data}}).toJson(QJsonDocument::Compact));
        webview->PostWebMessageAsJson(reinterpret_cast<PCWSTR>(json.utf16()));
#else
        Q_UNUSED(type); Q_UNUSED(key); Q_UNUSED(data);
#endif
    }
    void sendState()
    {
        postData("musicplayer.track", "track", track);
        postData("musicplayer.playback", "playback", playback);
    }
#ifdef Q_OS_WIN
    void createController(ICoreWebView2Environment *environment, quint64 request)
    {
        QPointer<HtmlWallpaper> guard(q);
        auto *callback = new ControllerCallback(controllerHandlerId,
            [guard, request](HRESULT result, ICoreWebView2Controller *created) -> HRESULT {
                if (!guard || !guard->d->enabled || guard->d->generation != request) {
                    if (created) created->Close();
                    return S_OK;
                }
                auto *d = guard->d.get();
                d->starting = false;
                if (FAILED(result) || !created) {
                    d->fail(QStringLiteral("无法创建 HTML 背景的网页视图。"));
                    return S_OK;
                }
                d->controller = created;
                created->AddRef();
                created->put_IsVisible(FALSE);
                if (FAILED(created->get_CoreWebView2(&d->webview))) {
                    d->fail(QStringLiteral("无法获取 HTML 背景的网页引擎。"));
                    return S_OK;
                }
                ICoreWebView2Settings *settings = nullptr;
                if (SUCCEEDED(d->webview->get_Settings(&settings))) {
                    settings->put_AreDefaultContextMenusEnabled(FALSE);
                    settings->put_AreDevToolsEnabled(FALSE);
                    settings->put_IsStatusBarEnabled(FALSE);
                    settings->put_IsWebMessageEnabled(TRUE);
                    settings->Release();
                }
                auto *navigation = new NavigationCallback(navigationHandlerId,
                    [guard, request](ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *args) -> HRESULT {
                        if (!guard || !guard->d->enabled || guard->d->generation != request) return S_OK;
                        BOOL success = FALSE;
                        args->get_IsSuccess(&success);
                        if (!success) {
                            guard->d->fail(QStringLiteral("HTML 背景加载失败，请检查文件及其引用的资源。"));
                            return S_OK;
                        }
                        auto *d = guard->d.get();
                        d->pageLoaded = true;
                        d->sendState();
                        d->resizeHost();
                        d->controller->put_IsVisible(TRUE);
                        ShowWindow(d->host, SW_SHOWNOACTIVATE);
                        emit guard->ready();
                        return S_OK;
                    });
                EventRegistrationToken token = {};
                const HRESULT registered = d->webview->add_NavigationCompleted(navigation, &token);
                navigation->Release();
                if (FAILED(registered)) {
                    d->fail(QStringLiteral("无法监听 HTML 背景的加载状态。"));
                    return S_OK;
                }
                auto *message = new MessageCallback(messageHandlerId,
                    [guard, request](ICoreWebView2 *, ICoreWebView2WebMessageReceivedEventArgs *args) -> HRESULT {
                        if (!guard || !guard->d->enabled || guard->d->generation != request) return S_OK;
                        LPWSTR text = nullptr;
                        LPWSTR source = nullptr;
                        const HRESULT readText = args->TryGetWebMessageAsString(&text);
                        const HRESULT readSource = args->get_Source(&source);
                        if (SUCCEEDED(readText) && SUCCEEDED(readSource) &&
                            QString::fromWCharArray(text) == "musicplayer.requestState" &&
                            QUrl(QString::fromWCharArray(source)).matches(QUrl::fromLocalFile(guard->d->path),
                                QUrl::RemoveQuery | QUrl::RemoveFragment))
                            guard->d->sendState();
                        CoTaskMemFree(text);
                        CoTaskMemFree(source);
                        return S_OK;
                    });
                const HRESULT messageRegistered = d->webview->add_WebMessageReceived(message, &token);
                message->Release();
                if (FAILED(messageRegistered)) {
                    d->fail(QStringLiteral("无法初始化 HTML 背景的歌曲数据接口。"));
                    return S_OK;
                }
                d->resizeHost();
                const QString url = QUrl::fromLocalFile(d->path).toString(QUrl::FullyEncoded);
                if (FAILED(d->webview->Navigate(reinterpret_cast<PCWSTR>(url.utf16()))))
                    d->fail(QStringLiteral("无法打开选中的 HTML 文件。"));
                return S_OK;
            });
        const HRESULT result = environment->CreateCoreWebView2Controller(host, callback);
        callback->Release();
        if (FAILED(result)) fail(QStringLiteral("无法初始化 HTML 背景视图。"));
    }
    void resizeHost()
    {
        if (!host || !IsWindow(host)) return;
        POINT origin = {GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN)};
        MapWindowPoints(nullptr, background, &origin, 1);
        const int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        const int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        SetWindowPos(host, HWND_BOTTOM, origin.x, origin.y, width, height, SWP_NOACTIVATE);
        if (controller) {
            RECT bounds = {};
            GetClientRect(host, &bounds);
            controller->put_Bounds(bounds);
            controller->NotifyParentWindowPositionChanged();
        }
    }
    HRESULT comResult = E_FAIL;
    QLibrary loader;
    HWND background = nullptr;
    HWND host = nullptr;
    ICoreWebView2Controller *controller = nullptr;
    ICoreWebView2 *webview = nullptr;
#endif
    HtmlWallpaper *q;
    QTimer timer;
    QString path;
    bool enabled = false;
    bool starting = false;
    bool pageLoaded = false;
    QJsonObject track;
    QJsonObject playback;
    quint64 generation = 0;
};

HtmlWallpaper::HtmlWallpaper(QObject *parent) : QObject(parent), d(std::make_unique<Private>(this)) {}
HtmlWallpaper::~HtmlWallpaper() = default;
bool HtmlWallpaper::isEnabled() const { return d->enabled; }
QString HtmlWallpaper::filePath() const { return d->path; }
void HtmlWallpaper::updateTrack(const QJsonObject &track)
{
    if (d->track == track) return;
    d->track = track;
    d->postData("musicplayer.track", "track", track);
    // A metadata/cover change can happen while paused, with no new position
    // signal. Supply playback too so pages can rebuild without losing state.
    d->postData("musicplayer.playback", "playback", d->playback);
}
void HtmlWallpaper::updatePlayback(const QJsonObject &playback)
{
    if (d->playback == playback) return;
    d->playback = playback;
    d->postData("musicplayer.playback", "playback", playback);
}

void HtmlWallpaper::setWallpaper(const QString &filePath, bool enabled)
{
    const QFileInfo file(filePath);
    const QString path = file.absoluteFilePath();
    if (enabled && (!file.isFile() || !file.isReadable() ||
        (file.suffix().compare("html", Qt::CaseInsensitive) != 0 && file.suffix().compare("htm", Qt::CaseInsensitive) != 0))) {
        d->fail(QStringLiteral("请选择可读取的 HTML 文件（.html 或 .htm）。"));
        return;
    }
    if (d->enabled == enabled && d->path == path) return;
    d->stop();
    d->path = path;
    d->enabled = enabled;
    emit enabledChanged(enabled);
    if (enabled) {
        d->timer.start();
        d->start();
    }
}
