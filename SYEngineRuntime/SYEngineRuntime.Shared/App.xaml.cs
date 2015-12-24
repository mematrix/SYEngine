using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.ApplicationModel;
using Windows.ApplicationModel.Activation;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Animation;
using Windows.UI.Xaml.Navigation;

namespace SYEngineRuntime
{
    public sealed partial class App : Application
    {
#if WINDOWS_PHONE_APP
        private TransitionCollection transitions;

        public static EventHandler<IContinuationActivatedEventArgs> ContinuationEventArgsChanged;
        public static IContinuationActivatedEventArgs ContinuationActivatedEventArgs { get; private set; }
#endif

        public App()
        {
            this.InitializeComponent();
            this.Suspending += this.OnSuspending;
        }

        private string OnUrlSegmentUpdateEvent(string uniqueId, int curIndex, int totalCount, string curUrl)
        {
            System.Diagnostics.Debug.WriteLine(uniqueId);
            System.Diagnostics.Debug.WriteLine(curUrl);
            //return "http://ws.acgvideo.com/f/dc/3425953-1.flv";
            return string.Empty;
        }

        protected override void OnLaunched(LaunchActivatedEventArgs e)
        {
            SYEngineCore.Core.Initialize();
            SYEngineCore.Core.ChangeNetworkPreloadTime(2.0);
            //SYEngineCore.Core.ChangeNetworkIOBuffer(32 * 1024);
            SYEngineCore.Core.UrlSegmentUpdateEvent += OnUrlSegmentUpdateEvent;

            Frame rootFrame = Window.Current.Content as Frame;
            if (rootFrame == null)
            {
                rootFrame = new Frame();
                rootFrame.CacheSize = 1;
                Window.Current.Content = rootFrame;
            }

            if (rootFrame.Content == null)
            {
#if WINDOWS_PHONE_APP
                if (rootFrame.ContentTransitions != null)
                {
                    this.transitions = new TransitionCollection();
                    foreach (var c in rootFrame.ContentTransitions)
                    {
                        this.transitions.Add(c);
                    }
                }

                rootFrame.ContentTransitions = null;
                rootFrame.Navigated += this.RootFrame_FirstNavigated;
#endif
            }

            rootFrame.Navigate(typeof(MainPage), e.Arguments);
            Window.Current.Activate();
        }

#if WINDOWS_PHONE_APP
        private void RootFrame_FirstNavigated(object sender, NavigationEventArgs e)
        {
            var rootFrame = sender as Frame;
            rootFrame.ContentTransitions = this.transitions ?? new TransitionCollection() { new NavigationThemeTransition() };
            rootFrame.Navigated -= this.RootFrame_FirstNavigated;
        }
#endif

        private void OnSuspending(object sender, SuspendingEventArgs e)
        {
            var deferral = e.SuspendingOperation.GetDeferral();
            deferral.Complete();
        }

        /// <summary>
        /// Windows Phone 文件选择器导航
        /// </summary>

        protected override void OnActivated(IActivatedEventArgs e)
        {
#if WINDOWS_PHONE_APP
            ContinuationActivatedEventArgs = e as IContinuationActivatedEventArgs;
            if (ContinuationEventArgsChanged != null)
                ContinuationEventArgsChanged(this, ContinuationActivatedEventArgs);
#endif
        }
    }
}