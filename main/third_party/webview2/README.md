Microsoft.Web.WebView2 SDK **1.0.2903.40**, downloaded from the official NuGet package:
https://www.nuget.org/packages/Microsoft.Web.WebView2/1.0.2903.40

Vendored files: the unmodified `build/native/include/WebView2.h`, the x64
`build/native/x64/WebView2Loader.dll`, `LICENSE.txt`, and `NOTICE.txt`.
The loader is loaded dynamically, so the project retains its Qt/MinGW toolchain.
Microsoft Edge WebView2 Runtime must be installed on the target machine.

Distribute LICENSE.txt and NOTICE.txt with the application.
