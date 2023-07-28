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
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;

// Документацию по шаблону элемента "Пользовательский элемент управления" см. по адресу https://go.microsoft.com/fwlink/?LinkId=234236

namespace PlayerUWP.Controls
{
    public class VideoStateToImageConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            if (value is VideoPanel.PlayerState state)
            {
                string imagePath = ""; // Set the path to the image based on the enum value.
                switch (state)
                {
                    case VideoPanel.PlayerState.Playing:
                        imagePath = "Assets/pause.png";
                        break;
                    case VideoPanel.PlayerState.Paused:
                        imagePath = "Assets/play.png";
                        break;
                }

                if (!string.IsNullOrEmpty(imagePath))
                {
                    return new BitmapImage(new Uri("ms-appx:///" + imagePath));
                }
            }

            return null;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
    public sealed partial class PlaybackControl : UserControl
    {
        public PlaybackControl()
        {
            this.InitializeComponent();
        }

        private void Image_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            VideoPanel.VideoPanel panel = (VideoPanel.VideoPanel)DataContext;
            if(panel.State == VideoPanel.PlayerState.Playing)
                panel.State = VideoPanel.PlayerState.Paused;
            else if(panel.State == VideoPanel.PlayerState.Paused)
                panel.State = VideoPanel.PlayerState.Playing;
        }
    }
}
