using NetworkCS.Base;
using NetworkCS.Network;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace NetworkCS.View
{
    public class TCPChattingVM : VMBase
    {
        public TCPChattingVM()
        {
            InitCommand();
        }

        /** Property Start **/
        private string _Status { get; set; }
        public string Status
        {
            get { return _Status; }
            set
            {
                _Status = value;
                OnPropertyChanged("Status");
            }
        }

        private string _Port { get; set; }
        public string Port
        {
            get { return _Port; }
            set
            {
                _Port = value;
                OnPropertyChanged("Port");
            }
        }

        private string _IP { get; set; }
        public string IP
        {
            get { return _IP; }
            set
            {
                _IP = value;
                OnPropertyChanged("IP");
            }
        }

        private Server server { get; set; }
        private Task _updateTask;
        private Task _listenTask;
        private bool _isRunning = true;

        /** Property end **/

        /** Functions end **/
        private void Update()
        {
            while (_isRunning)
            {
                Thread.Sleep(5);
            }
        }
        /** Functions end **/

        //////////////////////////////// Command Start
        public ICommand RunCommand { get; set; }
        public ICommand StopCommand { get; set; }

        private void InitCommand()
        {
            RunCommand = new AsyncCommand(Run);
            StopCommand = new AsyncCommand(Stop);
        }

        private async Task Run()
        {
            Status = "시작";
            int socketPort = 0;
            var isValidPort = int.TryParse(Port, out socketPort);

            {
                server = new Server(System.Net.IPAddress.Any, socketPort);
                server.Open();
            }
            //_listenTask = Task.Run(() => server.Start());
            //_updateTask = Task.Run(() => Update());
        }

        private async Task Stop()
        {
            Status = "연결 종료";
        }
        //////////////////////////////// Command End
    }
}