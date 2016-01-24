using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Microsoft.Win32;

namespace SYEngineWPF
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private async void BtnPlayFile_OnClick(object sender, RoutedEventArgs e)
        {
            tbPlayStatus.Text = string.Empty;

            //player.IsLooping = (bool)cboxLoop.IsChecked;
           
            var openFileDialog = new OpenFileDialog();
            openFileDialog.InitialDirectory = "c:\\";
            openFileDialog.Filter = "video files (*.mp4)|*.mp4|All files (*.*)|*.*";
            openFileDialog.FilterIndex = 2;
            openFileDialog.RestoreDirectory = true;
            if (openFileDialog.ShowDialog()==true)
            {
                player.Source=new Uri(openFileDialog.FileName);
            }
        }

     

        private void btnPlayNetwork_Click(object sender, RoutedEventArgs e)
        {
            var url = tboxNetworkUri.Text;
            if (string.IsNullOrEmpty(url))
            {
                return;
            }

            tbPlayStatus.Text = string.Empty;
            //player.IsLooping = (bool)cboxLoop.IsChecked;
            player.Source = new Uri(url);
        }

        private async void btnMultiFiles_Click(object sender, RoutedEventArgs e)
        {
         
        }

        private async void btnMultiUrl_Click(object sender, RoutedEventArgs e)
        {
          
        }

        private void player_CurrentStateChanged(object sender, RoutedEventArgs e)
        {
         //   tbPlayStatus.Text = player.is .ToString();
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
