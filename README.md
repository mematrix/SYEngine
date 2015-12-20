# ShanYe MediaFoundation Engine (Windows)

**[ > 快速整合指南 | Quick Start Guide](https://github.com/amamiya/SYEngine/blob/master/!GUIDE/GUIDE.md)**

### 开发信息
 - 开发者：**ShanYe**
 - 完成度：98% [实现跟系统播放MP4(本地或网络)一样的性能、稳定性、效果、API兼容性。]
 
### 许可协议
 - Licensed under ***LGPLv3*** or later.
 - 在**LGPLv3**许可请求外的情况，请联系`rukino.saki@outlook.com`，并说明目的。
 - 下面的库依据原本的协议：`ffcodecs` (ffmpeg)、`stagefright` (AOSP)

### 使用方式
 - **直接下载VSIX扩展包，安装后在VS中引用即可使用。**
 - 使用git submodule进行源码级别的整合。
 - 下载最新的zip压缩包进行源码级别的整合。

### 版本历史 (简单信息)
 - 2015-12-18：已经不需要在 `Package.appxmanifest` 中添加相关对象信息，真正实现了**一行代码初始化**即可完美整合。
 - 2015-12-16：修复MediaElement的DownloadProgressChanged事件在播放FLV的时候不触发下载进度的问题，现在**FLV整体播放效果已跟系统播放MP4近乎100%相同**！
 - 2015-12-14：移除CoreMFCommon工程；修复播放网络流的时候，Seek没缓冲进度的问题。
 - **2015-12-11**：修复关键bug，即花屏、网络差环境下的缓冲逻辑等，已经可以投入产品使用。不过仍有细小的不稳定性。
 - 2015-12-09：重构demuxers，合并到一个工程，原来的工程删除。 
 - 2015-12-07：第一次提交到git进行托管。

### 下一步 (预计)
 - **移植兼容UWP**。
 - 使用Windows.Web.Http.HttpClient来提供Http下载服务。

***
### 组件1：跨平台的解复用器
 - **CoreCommon**：此为核心模块，组件1中的所有其他的项目都会直接**依赖于此项目**，此项目为静态库，提供基础的抽象、内存管理、解码器标准化定义等。
 - **CoreDemuxers**：包含自行编写的`FLV\MKV\MP4`解封装器。

### 组件2：Win32的MF分离器和混流器
 - **CoreMFSource**：此为最**核心**的实现，其直接依赖CoreCommon，间接依赖FLV、MKV、MP4这三个Demuxer。其提供了高层封装，使组件可以在Win32的MF管道线模型中运行，其也是WinRT应用的MediaElement控件播放时调用的核心模块。
 - **MultipartStreamMatroska**：直接依赖ffcodecs。此库提供多切片分段的视频流重新混合为一个分段并且输出为MKV容器，然后CoreMFSource即可播放这个混合后的单个视频，实现**多分段播放无缝过度**。其可以无缝混合本地文件以及HTTP网络URL流。

***
### 已知问题
 - MultipartStreamMatroska 存在一点点的**内存泄漏**，多一个分P泄漏20字节左右，不是很严重。
 - MultipartStreamMatroska 无法提供整体的下载进度。