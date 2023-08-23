using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;

namespace PlayerUWP.Controls
{
    public class EditorWidthCalc : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            double val = (double)value;
            if (val > 200)
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
}
