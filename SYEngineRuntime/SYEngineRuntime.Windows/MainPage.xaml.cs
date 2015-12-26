using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Windows.Storage.Streams;
using Windows.Storage.Pickers;

namespace SYEngineRuntime
{
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
        }

        private async void btnPlayFile_Click(object sender, RoutedEventArgs e)
        {
            tbPlayStatus.Text = string.Empty;

            var op = new FileOpenPicker();
            op.ViewMode = PickerViewMode.List;
            op.FileTypeFilter.Add(".flv");
            op.FileTypeFilter.Add(".mkv");
            op.FileTypeFilter.Add(".mp4");
            var file = await op.PickSingleFileAsync();
            if (file != null)
                player.SetSource(await file.OpenReadAsync(), file.ContentType);
        }

        private void btnPlayNetwork_Click(object sender, RoutedEventArgs e)
        {
            tbPlayStatus.Text = string.Empty;
            player.Source = new Uri(tboxNetworkUri.Text);
        }

        private async void btnMultiFiles_Click(object sender, RoutedEventArgs e)
        {
            SYEngineCore.Playlist plist = new SYEngineCore.Playlist(SYEngineCore.PlaylistTypes.LocalFile);
            plist.Append(Windows.ApplicationModel.Package.Current.InstalledLocation.Path + "\\MultipartStreamMatroska\\0.mp4", 0, 0);
            plist.Append(Windows.ApplicationModel.Package.Current.InstalledLocation.Path + "\\MultipartStreamMatroska\\1.mp4", 0, 0);
            var s = "plist://WinRT-TemporaryFolder_" + System.IO.Path.GetFileName(await plist.SaveAndGetFileUriAsync());

            tbPlayStatus.Text = string.Empty;
            player.Source = new Uri(s);
        }

        private async void btnMultiUrl_Click(object sender, RoutedEventArgs e)
        {
            var test = new Uri(tboxNetworkUri.Text);

            SYEngineCore.Playlist plist = new SYEngineCore.Playlist(SYEngineCore.PlaylistTypes.NetworkHttp);
            SYEngineCore.PlaylistNetworkConfigs cfgs = default(SYEngineCore.PlaylistNetworkConfigs);
            cfgs.DownloadRetryOnFail = true;
            cfgs.DetectDurationForParts = true;
            cfgs.HttpUserAgent = string.Empty;
            cfgs.HttpReferer = string.Empty;
            cfgs.HttpCookie = string.Empty;
            cfgs.UniqueId = System.IO.Path.GetFileNameWithoutExtension(tboxNetworkUri.Text);
            cfgs.BufferBlockSizeKB = 64; //one block is 64KB
            cfgs.BufferBlockCount = 160; //160 * 64K = 10M network io buf.
            plist.NetworkConfigs = cfgs;
            plist.Append(tboxNetworkUri.Text, 0, 0);
            var s = "plist://WinRT-TemporaryFolder_" + System.IO.Path.GetFileName(await plist.SaveAndGetFileUriAsync());
            
            tbPlayStatus.Text = string.Empty;
            player.Source = new Uri(s);
        }

        private void player_CurrentStateChanged(object sender, RoutedEventArgs e)
        {
            tbPlayStatus.Text = player.CurrentState.ToString();
        }

        private void player_BufferingProgressChanged(object sender, RoutedEventArgs e)
        {
            if (player.BufferingProgress < 1.0)
                tbPlayStatus.Text = string.Format("Buffering... {0}%", (int)(player.BufferingProgress * 100));
        }

        private void player_DownloadProgressChanged(object sender, RoutedEventArgs e)
        {
            tbDownloadProgress.Text = string.Format("Downloaded {0}%", (int)(player.DownloadProgress * 100));
        }
    }
}
