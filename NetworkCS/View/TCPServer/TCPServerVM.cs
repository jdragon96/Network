using NetworkCS.Base;
using NetworkCS.Network;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace NetworkCS.View
{
    public class TCPServerVM : VMBase
    {
        public TCPServerVM()
        {
            InitCommand();
            Outputs = new ObservableCollection<string>();
            ClientHash = new Dictionary<string, Client>();
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

        private string _Port { get; set; } = "8000";
        public string Port
        {
            get { return _Port; }
            set
            {
                _Port = value;
                OnPropertyChanged("Port");
            }
        }

        private string _IP { get; set; } = "127.0.0.1";
        public string IP
        {
            get { return _IP; }
            set
            {
                _IP = value;
                OnPropertyChanged("IP");
            }
        }

        public ObservableCollection<string> Outputs { get; set; }
        public Dictionary<string, Client> ClientHash { get; set; } 

        private Server server { get; set; }
        private Task _listenTask;
        private bool _isRunning = true;

        /** Property end **/

        /** Functions end **/

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
                _listenTask = Task.Run(() => server.Start());
            }
            server.OnReceivePacket += Server_OnReceivePacket;
            server.OnConnectClient += Server_OnConnectClient;
        }

        private void Server_OnConnectClient(Client client)
        {
            ClientHash[client.ClientId.ToString()] = client;
        }

        private void Server_OnReceivePacket(SendPacket packet)
        {
            if(packet.Type == MessageType.MESSAGE)
            {
                App.Current.Dispatcher.Invoke(delegate
                {
                    Outputs.Add(packet.Message);
                });
                server.Broadcast(packet, ClientHash[packet.GuidId]);
            }
        }

        private async Task Stop()
        {
            Status = "연결 종료";
            server.Close();
        }
        //////////////////////////////// Command End
    }
}