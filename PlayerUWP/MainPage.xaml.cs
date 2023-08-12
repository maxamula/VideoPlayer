using System;
using System.Collections.Generic;
using System.ComponentModel;
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
            IsEditMode = false;
        }
        public static MainPage instance { get; private set; }
        public static StorageFile videoFile { get; set; }
        public static IRandomAccessStream videoStream { get; set; }
        public static IRandomAccessStream outp { get; set; }

        public bool IsEditMode
        {
            get => _isEditMode;

            set
            {
                if(_isEditMode != value)
                {
                    _isEditMode = value;
                    if (_isEditMode)
                        bottomPresenter.Content = new Controls.EditBar() { DataContext = this.DataContext };
                    else
                        bottomPresenter.Content = new Controls.VideoProgressBar() { DataContext = this.DataContext };
                }
            }
        }
        private bool _isEditMode = true;
        public bool IsBusy
        {
            get => _isBusy;

            set
            {
                _isBusy = value;
                Loading.IsLoading = value;
            }
        }
        private bool _isBusy = false;
    }
}
