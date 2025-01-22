using NetworkCS.Base;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;
using System.Windows.Input;

namespace NetworkCS.View
{
    public class MainWindowVM: VMBase
    {
        public MainWindowVM()
        {
            InitCommand();
            Controls = new Dictionary<int, UserControl>();
        }

        private UserControl _Content { get; set; }
        public UserControl Content
        {
            get { return _Content; }
            set
            {
                _Content = value;
                OnPropertyChanged("Content");
            }
        }

        Dictionary<int, UserControl> Controls;

        public ICommand ViewChangeCommand { get; set; }
        void InitCommand()
        {
            ViewChangeCommand = new RelayCommand(ExecuteViewChange);
        }

        private void ExecuteViewChange(object parameter)
        {
            int viewType = parameter.ToInt();
            UserControl control = null;
            if (Controls.ContainsKey(viewType))
                control = Controls[viewType];
            if (control == null)
            {
                control = CreateContent(viewType);
                Controls[viewType] = control;
            }
            if (control == null)
                return;
            Content = control;
        }

        private UserControl CreateContent(int Type)
        {
            switch (Type)
            {
                case 0:
                    return new TCPServerView();
                case 1:
                    return new TCPClientView();
            }
            return null;
        }
    }
}