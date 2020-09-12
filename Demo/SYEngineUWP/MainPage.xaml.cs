using System;
using System.IO;
using Windows.Storage.Pickers;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using SYEngine;

// https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x804 上介绍了“空白页”项模板

namespace SYEngineUWP
{
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();

            this.Loaded += MainPage_Loaded;
        }

        private void MainPage_Loaded(object sender, RoutedEventArgs e)
        {
            this.SoftDecodeCheckbox.IsChecked = SYEngine.Core.ForceSoftwareDecode;
        }

        private void Player_OnCurrentStateChanged(object sender, RoutedEventArgs e)
        {
            this.PlayStatusBlock.Text = this.Player.CurrentState.ToString();
        }

        private void Player_OnBufferingProgressChanged(object sender, RoutedEventArgs e)
        {
            if (Player.BufferingProgress < 1.0)
            {
                PlayStatusBlock.Text = $"Buffering... {(int)(Player.BufferingProgress * 100)}%";
            }
        }

        private void Player_OnDownloadProgressChanged(object sender, RoutedEventArgs e)
        {
            DownloadProgressBlock.Text = $"Downloaded {(int)(Player.DownloadProgress * 100)}%";
        }

        private async void PlayFileBtn_OnClick(object sender, RoutedEventArgs e)
        {
            PlayStatusBlock.Text = string.Empty;

            var op = new FileOpenPicker();
            op.ViewMode = PickerViewMode.List;
            op.FileTypeFilter.Add(".flv");
            op.FileTypeFilter.Add(".f4v");
            op.FileTypeFilter.Add(".mkv");
            op.FileTypeFilter.Add(".mp4");
            var file = await op.PickSingleFileAsync();
            if (null != file)
            {
                Player.SetSource(await file.OpenReadAsync(), file.ContentType);
            }
        }

        private async void PlayNetworkBtn_OnClick(object sender, RoutedEventArgs e)
        {
            PlayStatusBlock.Text = string.Empty;

            var url = NetworkUrlTextBox.Text;
            if (Uri.TryCreate(url, UriKind.RelativeOrAbsolute, out var uri))
            {
                Player.Source = uri;
            }
            else
            {
                await new MessageDialog("Not a valid uri format.", "Error").ShowAsync();
            }
        }

        private async void MultiFilesBtn_OnClick(object sender, RoutedEventArgs e)
        {
            var installPath = Windows.ApplicationModel.Package.Current.InstalledLocation.Path;
            var plist = new SYEngine.Playlist(PlaylistTypes.LocalFile);
            plist.Append(installPath + "\\Assets\\MultipartStreamMatroska\\0.mp4", 0, 0);
            plist.Append(installPath + "\\Assets\\MultipartStreamMatroska\\1.mp4", 0, 0);

            PlayStatusBlock.Text = string.Empty;
            Player.Source = await plist.SaveAndGetFileUriAsync();
        }

        private async void MultiUrlBtn_OnClick(object sender, RoutedEventArgs e)
        {
            var url = NetworkUrlTextBox.Text;
            if (!Uri.TryCreate(url, UriKind.RelativeOrAbsolute, out var uri))
            {
                await new MessageDialog("Not a valid uri format.", "Error").ShowAsync();
                return;
            }

            var plist = new SYEngine.Playlist(PlaylistTypes.NetworkHttp);
            var cfgs = default(SYEngine.PlaylistNetworkConfigs);
            cfgs.DownloadRetryOnFail = true;
            cfgs.DetectDurationForParts = true;
            cfgs.HttpUserAgent = string.Empty;
            cfgs.HttpReferer = string.Empty;
            cfgs.HttpCookie = string.Empty;
            cfgs.UniqueId = Path.GetFileNameWithoutExtension(NetworkUrlTextBox.Text);
            cfgs.BufferBlockSizeKB = 64; //one block is 64KB
            cfgs.BufferBlockCount = 160; //160 * 64K = 10M network io buf.
            plist.NetworkConfigs = cfgs;
            plist.Append(NetworkUrlTextBox.Text, 0, 0);
#if DEBUG
            var debugFile = Path.Combine(Windows.Storage.ApplicationData.Current.TemporaryFolder.Path, "DebugFile.mkv");
            plist.SetDebugFile(debugFile);
#endif

            PlayStatusBlock.Text = string.Empty;
            Player.Source = await plist.SaveAndGetFileUriAsync();
        }

        private async void RemuxPlayBtn_OnClick(object sender, RoutedEventArgs e)
        {
            PlayStatusBlock.Text = string.Empty;

            var op = new FileOpenPicker();
            op.ViewMode = PickerViewMode.List;
            op.FileTypeFilter.Add(".flv");
            op.FileTypeFilter.Add(".f4v");
            op.FileTypeFilter.Add(".mkv");
            op.FileTypeFilter.Add(".mp4");
            var file = await op.PickSingleFileAsync();
            if (file != null)
            {
                var plist = new SYEngine.Playlist(PlaylistTypes.LocalFile);
                plist.Append(file.Path, 0, 0);
                Player.Source = await plist.SaveAndGetFileUriAsync();
            }
        }

        private void SoftDecodeCheckbox_OnChecked(object sender, RoutedEventArgs e)
        {
            SYEngine.Core.ForceSoftwareDecode = this.SoftDecodeCheckbox.IsChecked ?? false;
        }
    }
}