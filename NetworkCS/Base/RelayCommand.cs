using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace NetworkCS.Base
{
    public class RelayCommand : ICommand
    {
        #region Fields

        readonly Action<object> _excute;
        readonly Predicate<object> _canExecute;
        private RelayCommand commandRecordFolder;
        #endregion

        #region 생성자

        public RelayCommand(Action<object> execute) : this(execute, null) { }

        public RelayCommand(Action<object> execute, Predicate<object> canExecute)
        {
            if (execute == null)
                throw new ArgumentNullException("excute");
            _excute = execute;
            _canExecute = canExecute;
        }

        public RelayCommand(RelayCommand commandRecordFolder)
        {
            this.commandRecordFolder = commandRecordFolder;
        }

        #endregion

        public bool CanExecute(object parameter)
        {
            return _canExecute == null ? true : _canExecute(parameter);
        }

        public event EventHandler CanExecuteChanged
        {
            add { CommandManager.RequerySuggested += value; }
            remove { CommandManager.RequerySuggested -= value; }
        }

        public void Execute(object parameter)
        {
            _excute(parameter);
        }
    }
}
