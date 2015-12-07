# ShanYe MediaFoundation Engine (Windows)
***
### 开发信息
 - 开发者：**ShanYe**
 - 应用于：BiliBili 客户端 for Windows (8.1)
 - 完成度：90%
### 许可协议
 - Licensed under ***LGPLv3*** or later.
 - 在**LGPLv3**许可请求外的情况，请联系`rukino.saki@outlook.com`，并说明目的。
 - 建议直接下载vsix整合使用。
 - 下面的库依据原本的协议：`ffcodecs` (ffmpeg)、`stagefright` (AOSP)
### 版本历史 (简单信息)
 - 2015-12-07：第一次提交到git进行托管，将与**暗影吉他手**合作维护。
### 下一步 (预计)
 - **移植兼容UWP**。
 - 使用Windows.Web.Http.HttpClient来提供Http下载服务。
***
### 组件1：跨平台的解复用器
 - **CoreCommon**：此为核心模块，组件1中的所有其他的项目都会直接**依赖于此项目**，此项目为静态库，提供基础的抽象、内存管理、解码器标准化定义等。
 - **FLV**Demuxer：Adobe FLV格式的解封装器，兼容的编码有H264、AAC\MP3。
 - **MKV**Demuxer：Matroska 格式的解封装器，兼容的编码有H264\HEVC\MPEG2\MPEG4\VC1\WMV9\、AAC\MP3\DTS\AC3\FLAC\ALAC\TTA\WavPack等。
 - **MP4**Demuxer：ISO标准化定义的MP4容器格式解封装器，兼容的编码有H264\HEVC\MPEG4、AAC\MP3\ALAC\DTS\AC3等。
### 组件2：Win32的MF分离器
 - **CoreMFCommon**：提供MF相关的API简单化包装类，比如异步回调、任务队列等。此为静态库。
 - **CoreMFSource**：此为最**核心**的实现，其直接依赖CoreCommon、CoreMFCommon，间接依赖FLV、MKV、MP4这三个Demuxer。其提供了高层封装，使组件可以在Win32的MF管道线模型中运行，其也是WinRT应用的MediaElement控件播放时调用的核心模块。
### 组件3：Win32的MF多流重混合器
 - ffcodecs：提供ffmpeg的高层函数导出，并且经过**一定的优化**（注意，此库会有link警告，可以忽略）。
 - **MultipartStreamMatroska**：直接依赖ffcodecs。此库提供多分P的视频流重新混合为一个分P并且输出为MKV容器，然后CoreMFSource即可播放这个混合后的单分P，实现**多分P播放无缝过度**。其可以无缝混合本地文件以及HTTP网络URL流。
***
### 待修复的已知问题
 - MultipartStreamMatroska 存在一点点的**内存泄漏**，多一个分P泄漏20字节左右，不是很严重。
 - CoreMFSource 目前还无法比较完美的兼容网络流播，可能会出现虽然进入缓冲模式了，但是MediaElement控件的State没有切到缓存模式的情况。也可能出现在网络较差的情况下，花屏的情况。
### 待增强开发的功能
 - MultipartStreamMatroska 无法提供整体的下载进度。