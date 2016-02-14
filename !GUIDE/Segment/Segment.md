## 播放分段的 FLV\MP4

### 0. 功能说明

- 混合播放多个分段视频源，**无缝过度** 到下一个分段，Timeline 自动同步。
- 支持**动态更新**每个分段的URL地址，比如在Seek时、切换下一个分段的时候。
- 自定义网络参数、用户代理、缓冲长度和超时设置等。

***

### 1. 初始化播放器核心 SYEngine 模块

- 安装 VSIX **[软件包](https://coding.net/u/amamiya/p/SYEngine_VSIX/git)**，然后引用这个扩展。

- 添加初始化代码（在**播放开始前**的任意位置，比如App::OnLaunched或者Page的构造方法）
  
  ``` c#
   SYEngine.Core.Initialize(); //执行一次就行了
  ```

- > ![](https://raw.githubusercontent.com/amamiya/SYEngine/master/!GUIDE/Segment/0.png)

- 然后，我们在MainPage.xaml添加一个MediaElement控件，命名为 **MediaPlayer**；再添加一个按钮，命名为 **Play**。
  
- > ![](https://raw.githubusercontent.com/amamiya/SYEngine/master/!GUIDE/Segment/1.png)

***

### 2. 播放多个网络分段源

- 首先准备几个网络源的地址，支持度说明如下：
  - 协议：本地文件(可访问的)、**HTTP** 网络源。
  - 容器：仅限于 **MP4** 或 **FLV**。
  - 编码：仅限于 **H264** 的视频，以及 **AAC-LC\HE** 的音频。
  - 备注：必须保证每个分段源的**容器和编码都是相同的**，不能出现上一个分段和下一个分段的编码配置方案不同的情况。


- 使用播放列表：
  
  - 实例化新的 **`SYEngine.Playlist`** 对象；
  - 使用 **`Append`** 方法添加每个分段的地址，如果能提供**文件大小和时长**，则最好提供，不能提供，可以给 **0** 即可；可以只提供时长信息；如果**时长信息**不能提供，SYEngine **可能需要**连接所有URL地址去获取这个信息；
  - 使用 **`SYEngine.PlaylistNetworkConfigs`** 结构体来设置网络参数。这个结构体的相关解释请拉到页面底部的 **[网络选项说明](https://github.com/amamiya/SYEngine/blob/master/!GUIDE/Segment/Segment.md#4-网络选项说明)** 来查看；
  - 使用 **`SaveAndGetFileUriAsync`** 异步方法返回一个Uri对象，然后设置为 MediaElement 的 **Source** 属性即可。
  
- 最后，使用 MediaElement 的 **Play** 方法来播放，或者设置 **AutoPlay** 属性为true，下面是完整的代码截图。
  
- > ![](https://raw.githubusercontent.com/amamiya/SYEngine/master/!GUIDE/Segment/2.png)

***

### 3. 支持动态更新网络分段源的地址 (高级)

- 如果有需求，需要在播放时，动态的更新下一个分段的源地址，或者在用户拖进度条的时候更新地址，以及解析出来的视频源的地址**持续时间短**、可能失效的情况下，可以使用动态更新源地址的功能。如果你没有这样的需求，可以不需要看这一段。
  
- 订阅相关事件：
  
  - **`SYEngine.Core.PlaylistSegmentUrlUpdateEvent`** 提供一个基本的更新URL的**事件**通知，如果**返回一个新的地址**，则更新URL为返回的新地址，如果不需要更新当前地址，则可以返回 **string.Empty**。
  - **`SYEngine.Core.PlaylistSegmentDetailUpdateEvent`** 提供更详细的选项用于更新分段URL的信息，可以设置连接分段时，新的HTTP报文头参数等。
  
- 事件参数说明：
  
  - **uniqueId** 即为在设置 **`SYEngine.PlaylistNetworkConfigs`** 时的参数。
  - **curIndex** 指示当前需要更新的分段在 **Playlist** 中从0开始的索引下标。
  - **opType** 提供本次回调事件通知时的操作类型，指示当前的内部状态：
    
    | opType |                 说明                  |       建议        |
    | :---------- | :---------------------------------: | :-------------: |
    | OPEN        |        连接第一个分段，curIndex一定=0         | **可以** 发起长时间的连接 |
    | NEXT        | 正在播放当前的分段，预先缓冲下一个分段，curIndex=下一个分段  |                 |
    | SEEK        | 用户拖动进度条，请求跳播到指定的时间，curIndex=用户请求的分段 | **可以** 发起长时间的连接 |
    | GAP         |       当播放器被用户暂停超过一分钟，又恢复播放的时候        |                 |
    | ERROR       |  连接当前的URL错误，必须提供新的URL，如果不提供，则播放结束   | **可以** 发起长时间的连接 |
    | RECONNECT   |             当触发自动重连的时候              |                 |
  
- 处理事件相关注意：
  
  - 如果使用的是 **`SYEngine.Core.PlaylistSegmentDetailUpdateEvent`** 来监听事件的话，请使用参数 **info** 来读写相关信息  ([示例代码](https://github.com/amamiya/SYEngine/blob/master/Demo/SYEngineRuntime/SYEngineRuntime.Shared/App.xaml.cs))。
  - 两个事件通知都在不知名的**系统线程池**中运行，如果在事件代码内要**访问UI**，则需要用分发调用的方式。
  - 在事件中的代码，执行的时间请不要超过 **5秒**，因为 SYEngine 的默认缓冲大小是 5 秒，如果事件代码块的执行时间超过 5 秒，则MediaElement可能会出现进入 **Buffering** 的状态。建议的使用方式是，根据 **opType参数** 来决定是否发起 HTTP 的 **长时间连接** 去更新分段源地址，比如 opType 为 SEEK、ERROR 这样的状态的时候，可以放心的发起可能超过5秒钟的长时间连接。
  - SYEngine 的默认缓冲时间可以通过属性 `SYEngine.Core.NetworkBufferTimeInSeconds` 来更改，单位是秒，但是不能设置低于 2 秒。不修改的话，默认是 **5 秒**。
  
- 其他备注：
  
  - 因为是可以动态更新视频源的地址的，所以在初始化的  `Append` 方法中，第一个URL参数可以添加一个 **虚假的地址**，即那种不需要能被打开的地址也行，然后在事件通知中，根据 **curIndex** 参数来更新每个分段的源地址。但是必须添加正确的 **文件大小和时长** 信息，并且不能打开 **DetectDurationForParts** 选项，因为虚假的地址，引擎无法自动连接并获取准确的文件大小和时长信息。
  - 在不推荐发起超过 **5 秒** 的 opType 触发的情况下，比如 **NEXT** 事件。可以使用一些变通的方法来处理，比如在 App 中设置一个定时器，监听 MediaElement 播放的进度，到当前分段剩余 60 秒的时候，就自动开启获取下一个分段地址的任务，然后在触发 NEXT 事件的时候，即可快速拿到地址。
  
- 下面是示例的代码截图。
  
- > ![](https://raw.githubusercontent.com/amamiya/SYEngine/master/!GUIDE/Segment/3.png)

***

### 4. 网络选项说明

- 可以通过 `SYEngine.PlaylistNetworkConfigs` 中的值来定义本次播放时，网络连接的参数，必须在执行 `SaveAndGetFileUriAsync` 方法前设置。
  - **HttpUserAgent**：HTTP连接时的用户代理，如果使用系统默认的代理字符串，则设置为 string.Empty。
  - **HttpReferer**：HTTP连接时的引用网页地址，不需要设置为 string.Empty。
  - **HttpCookie**：HTTP连接时的Cookie字符串，不需要设置为 string.Empty。
  - **BufferBlockCount**：最大下载缓冲在内存中的Block总数。**不设置**的话，默认是80，即为80 * 64K = 5MB。
  - **BufferBlockSizeKB**：每个Block的大小，单位为KB。**不设置**的话，默认每个Block是64K。
  - **DownloadRetryOnFail**：是否启用断线自动重连，推荐**启用**。
  - **FetchNextPartThresholdSeconds**：设置一个值，单位是秒，当现在的分段播放到剩余多少秒的时候，就开始预先缓冲下一个分段，**不设置**的话，默认是 30 秒。
  - **DetectDurationForParts**：默认不开启，如果开启的话，SYEngine 会自动去连接所有添加在 Playlist 中的分段视频地址来获取准确的 **时长和文件大小** 信息。如果在 `Append`的时候已经写入准确的 **时长和大小** 信息，则**不要开启**这个选项（会浪费流量）。如果开启这个选项，则 ExplicitTotalDurationSeconds  的设置会被忽略。
  - **ExplicitTotalDurationSeconds**：设置时长，单位为秒，在 DetectDurationForParts 不开启的情况下，设置这个值即为 MediaElement 返回的媒体时长。如果在 `Append` 方法中已经**为每个项目添加了时长信息**，也可以**不需要设置**这个值，时长信息会被自动计算。
  - **NotUseCorrectTimestamp**：如果播放您的视频源出现音视频同步不正确的问题，打开这个选项即可。（一般不需要打开）
  - **UniqueId**：如果需要支持 **动态更新分段源地址**，则必须设置这个值，用于在事件通知中的第一个同名参数，不需要的话，则设置为 string.Empty。如果是 string.Empty，则不会触发事件通知。

- 相关建议：
  - 一般，不要设置更改 **BufferBlockCount** 和 **BufferBlockSizeKB** 选项，除非明确知道当前的网络情况。
  - 建议开启 **DownloadRetryOnFail** 选项。
  - 如果开启 **DetectDurationForParts** 选项，则 **ExplicitTotalDurationSeconds** 和 `Append` 方法提供的文件大小和时长信息会被忽略。
  - 如果 `Append` 方法不提供时长信息，又不打开 **DetectDurationForParts** 选项，则必须设置 **ExplicitTotalDurationSeconds** 总时长信息。