using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage.Pickers;
using Windows.Storage.Streams;
using Windows.Storage;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;
using Windows.ApplicationModel.Core;

// Документацию по шаблону элемента "Пользовательский элемент управления" см. по адресу https://go.microsoft.com/fwlink/?LinkId=234236

namespace PlayerUWP.Controls
{
    public class VideoStateToIconConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value is VideoPanel.PlayerState state)
            {
                switch (state)
                {
                    case VideoPanel.PlayerState.Playing:
                        return Symbol.Pause;
                    case VideoPanel.PlayerState.Paused:
                        return Symbol.Play;
                    default:
                        return Symbol.Play;
                }
            }
            return Symbol.Play;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }

    public class VideoStateToTextConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value is VideoPanel.PlayerState state)
            {
                switch (state)
                {
                    case VideoPanel.PlayerState.Playing:
                        return "Pause";
                    case VideoPanel.PlayerState.Paused:
                        return "Play";
                }
            }
            return "-";
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
    public sealed partial class ApplicationMenu : UserControl
    {
        public ApplicationMenu()
        {
            this.InitializeComponent();
        }

        private async void PlayPauseClick(object sender, RoutedEventArgs e)
        {
            VideoPanel.VideoPanel panel = (VideoPanel.VideoPanel)DataContext;
            if (panel.State == VideoPanel.PlayerState.Playing)
                panel.State = VideoPanel.PlayerState.Paused;
            else if (panel.State == VideoPanel.PlayerState.Paused)
                panel.State = VideoPanel.PlayerState.Playing;
            else if(panel.State == VideoPanel.PlayerState.Idle)
            {
                panel.StopRenderingAsnyc();
                FileOpenPicker openPicker = new FileOpenPicker();
                openPicker.ViewMode = PickerViewMode.Thumbnail;
                openPicker.SuggestedStartLocation = PickerLocationId.VideosLibrary;
                openPicker.FileTypeFilter.Add(".mp4");

                MainPage.videoFile = await openPicker.PickSingleFileAsync();
                if(MainPage.videoFile != null)
                {
                    MainPage.videoStream = await MainPage.videoFile.OpenAsync(FileAccessMode.Read);
                    panel.Open(MainPage.videoStream);
                    panel.StartRenderingAsnyc();
                }
            }
        }

        private void StopClick(object sender, RoutedEventArgs e)
        {
            VideoPanel.VideoPanel panel = (VideoPanel.VideoPanel)DataContext;
            panel.State = VideoPanel.PlayerState.Idle;
        }

        private void EditClick(object sender, RoutedEventArgs e)
        {
            MainPage.instance.EditPanel();
        }
    }
}
