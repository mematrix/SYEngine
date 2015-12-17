# Guide: How to Use It (WinRT 8.1)
### My Environment
 - Windows 8.1
 - Visual Studio 2013 with Update 3

### Features
 - **FLV\MKV** Playback (File or NetworkStream).
 - MP4\FLV Segment Mixed Playback.
 - RTMP over HTTP Live Stream.
 - Hardware accelerated.

***
### 1. Install VSIX Packages and...
 - Download and Install:
   - Windows 8.1: `SYEngine_WinRT81.vsix`
   - Windows Phone 8.1: `SYEngine_WinPhone81.vsix`
 - Create a new UAP Project
   - **Windows 8.1 or Windows Phone 8.1 only**
   - Visual C#

### 2. Add References
![](https://raw.githubusercontent.com/amamiya/SYEngine/master/!GUIDE/0.png)

 - Windows 8.1: `ShanYe MediaPlayer Engine (Windows 8.1)`
 - Windows Phone 8.1: `ShanYe MediaPlayer Engine (Windows Phone 8.1)`
 - **Change build target to: x86\x64\ARM.**

### 3. Open `App.xaml.cs` and Add Code...
![](https://raw.githubusercontent.com/amamiya/SYEngine/master/!GUIDE/1.png)

```
In App::OnLaunched:
  SYEngineCore.Core.Initialize();
```

### 4. Build and Run
 - use the **MediaElement** Control to Play a Video (File or Network).
 - use the **Windows.Media.Transcoding.MediaTranscoder** API.
