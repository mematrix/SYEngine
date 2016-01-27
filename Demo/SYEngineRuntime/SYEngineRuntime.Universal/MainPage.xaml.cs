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
            player.IsLooping = (bool)cboxLoop.IsChecked;
            SYEngine.Core.ForceSoftwareDecode = (bool)cboxSoftDecode.IsChecked;

            var op = new FileOpenPicker();
            op.ViewMode = PickerViewMode.List;
            op.FileTypeFilter.Add(".flv");
            op.FileTypeFilter.Add(".f4v");
            op.FileTypeFilter.Add(".mkv");
            op.FileTypeFilter.Add(".mp4");
            var file = await op.PickSingleFileAsync();
            if (file != null)
                player.SetSource(await file.OpenReadAsync(), file.ContentType);
        }

        private void btnPlayNetwork_Click(object sender, RoutedEventArgs e)
        {
            tbPlayStatus.Text = string.Empty;
            player.IsLooping = (bool)cboxLoop.IsChecked;
            SYEngine.Core.ForceSoftwareDecode = (bool)cboxSoftDecode.IsChecked;

            player.Source = new Uri(tboxNetworkUri.Text);
        }

        private async void btnMultiFiles_Click(object sender, RoutedEventArgs e)
        {
            var plist = new SYEngine.Playlist(SYEngine.PlaylistTypes.LocalFile);
            plist.Append(Windows.ApplicationModel.Package.Current.InstalledLocation.Path + "\\MultipartStreamMatroska\\0.mp4", 0, 0);
            plist.Append(Windows.ApplicationModel.Package.Current.InstalledLocation.Path + "\\MultipartStreamMatroska\\1.mp4", 0, 0);

            tbPlayStatus.Text = string.Empty;
            player.IsLooping = (bool)cboxLoop.IsChecked;
            SYEngine.Core.ForceSoftwareDecode = (bool)cboxSoftDecode.IsChecked;

            player.Source = await plist.SaveAndGetFileUriAsync();
        }

        private async void btnMultiUrl_Click(object sender, RoutedEventArgs e)
        {
            var test = new Uri(tboxNetworkUri.Text);

            var plist = new SYEngine.Playlist(SYEngine.PlaylistTypes.NetworkHttp);
            var cfgs = default(SYEngine.PlaylistNetworkConfigs);
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
#if DEBUG
            var debugFile = System.IO.Path.Combine(Windows.Storage.ApplicationData.Current.TemporaryFolder.Path, "DebugFile.mkv");
            plist.SetDebugFile(debugFile);
#endif

            tbPlayStatus.Text = string.Empty;
            player.IsLooping = (bool)cboxLoop.IsChecked;
            SYEngine.Core.ForceSoftwareDecode = (bool)cboxSoftDecode.IsChecked;

            player.Source = await plist.SaveAndGetFileUriAsync();
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
