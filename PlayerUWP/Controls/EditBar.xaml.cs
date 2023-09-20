using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage.Pickers;
using Windows.Storage;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using System.Threading.Tasks;
using Windows.UI.Core;

// Документацию по шаблону элемента "Пользовательский элемент управления" см. по адресу https://go.microsoft.com/fwlink/?LinkId=234236

namespace PlayerUWP.Controls
{
    public sealed partial class EditBar : UserControl
    {
        public EditBar()
        {
            this.InitializeComponent();
        }
        private void editPanel_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (editPanel.ActualWidth > 200)
            {
                editGrid.Width = editPanel.ActualWidth - 200;
            }
        }
        private async void SaveCopyClick(object sender, RoutedEventArgs e)
        {
            if(MainPage.videoStream != null)
            {
                FileSavePicker savePicker = new FileSavePicker();
                savePicker.SuggestedStartLocation = PickerLocationId.VideosLibrary;
                savePicker.FileTypeChoices.Add("MP4 Video", new List<string>() { ".mp4" });

                StorageFile file = await savePicker.PickSaveFileAsync();
                if (file != null)
                {
                    MainPage.outp = await file.OpenAsync(FileAccessMode.ReadWrite);
                    VideoPanel.FX.VideoCutter cutter = new VideoPanel.FX.VideoCutter(MainPage.videoStream, MainPage.outp, (ulong)videoRangeSelector.RangeStart, (ulong)videoRangeSelector.RangeEnd);
                    MainPage.instance.IsBusy = true;
                    cutter.SaveCopy(() =>
                    {
                        CoreDispatcher dispatcher = Windows.ApplicationModel.Core.CoreApplication.MainView.CoreWindow.Dispatcher;
                        dispatcher.RunAsync(CoreDispatcherPriority.Normal, () => { MainPage.instance.IsBusy = false; });
                    });  
                }
            }
        }
    }
}
