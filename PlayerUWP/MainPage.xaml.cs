using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Devices.Enumeration;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage;
using Windows.Storage.Pickers;
using Windows.Storage.Streams;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// Документацию по шаблону элемента "Пустая страница" см. по адресу https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x419

namespace PlayerUWP
{

    public class EditorWidthCalc : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            double val = (double)value;
            if(val > 200)
            {
                return val - 200;
            }
            return 0;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }

    public class TimeSpanConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value is double position)
            {
                TimeSpan timeSpan = TimeSpan.FromTicks((long)position / 100);

                string hours = timeSpan.Hours.ToString("D2");
                string minutes = timeSpan.Minutes.ToString("D2");
                string seconds = timeSpan.Seconds.ToString("D2");

                return $"{hours}:{minutes}:{seconds}";
            }

            return "00:00:00"; // Default value if conversion fails
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }

    /// <summary>
    /// Пустая страница, которую можно использовать саму по себе или для перехода внутри фрейма.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
            menu.DataContext = viewport;
            this.DataContext = viewport;
            instance = this;
        }
        public void EditPanel()
        {
            if(IsEditOpened)
                editPanel.Visibility = Visibility.Collapsed;
            else 
                editPanel.Visibility = Visibility.Visible;
            IsEditOpened = !IsEditOpened;
        }
        public static MainPage instance { get; private set; }
        public static StorageFile videoFile { get; set; }
        public static IRandomAccessStream videoStream { get; set; }
        public static IRandomAccessStream outp { get; set; }
        private bool IsEditOpened = false;

        private void editPanel_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (editPanel.ActualWidth > 200)
            {
                editGrid.Width = editPanel.ActualWidth - 200;
            }
        }

        private async void SaveCopyClick(object sender, RoutedEventArgs e)
        {
            FileSavePicker savePicker = new FileSavePicker();
            savePicker.SuggestedStartLocation = PickerLocationId.VideosLibrary;
            savePicker.FileTypeChoices.Add("MP4 Video", new List<string>() { ".mp4" });

            StorageFile file = await savePicker.PickSaveFileAsync();
            if (file != null)
            {
                MainPage.outp = await file.OpenAsync(FileAccessMode.ReadWrite);
                VideoPanel.FX.VideoCutter t = new VideoPanel.FX.VideoCutter(MainPage.videoStream, MainPage.outp, (ulong)videoRangeSelector.RangeStart, (ulong)videoRangeSelector.RangeEnd);
                t.SaveCopy();
            }
        }
    }
}
