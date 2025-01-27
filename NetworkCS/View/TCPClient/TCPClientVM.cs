using NetworkCS.Base;
using NetworkCS.Network;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Net;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows.Input;
using System.Collections.ObjectModel;
using System.Collections;
using System.Windows.Interop;

namespace NetworkCS.View
{
    public class TCPClientVM : VMBase
    {
        public TCPClientVM()
        {
            InitCommand();
            Outputs = new ObservableCollection<string>();
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


        private string _ClientGUID { get; set; }
        public string ClientGUID
        {
            get { return _ClientGUID; }
            set
            {
                _ClientGUID = value;
                OnPropertyChanged("ClientGUID");
            }
        }

        private string _Message { get; set; }
        public string Message
        {
            get { return _Message; }
            set
            {
                _Message = value;
                OnPropertyChanged("Message");
            }
        }

        private string _Ping { get; set; }
        public string Ping
        {
            get { return _Ping; }
            set
            {
                _Ping = value;
                OnPropertyChanged("Ping");
            }
        }
        private string _MyPort { get; set; } = "";
        public string MyPort
        {
            get { return _MyPort; }
            set
            {
                _MyPort = value;
                OnPropertyChanged("MyPort");
            }
        }

        public ObservableCollection<string> Outputs { get; set; }

        private Server server { get; set; }
        private Task _pingTask;
        private Task _listenTask;
        private bool _isRunning = false;
        private Socket _serverSocket = null;
        private Client _client;

        /** Property end **/

        /** Functions end **/
        private async Task PingCheck()
        {
            if (_serverSocket == null)
                return;

            SendPacket pingPacket = new SendPacket();
            pingPacket.GuidId = ClientGUID;
            pingPacket.Type = MessageType.HEARTBEAT;
            pingPacket.Message = String.Empty;

            while (_isRunning)
            {
                Thread.Sleep(5000);
                if (await _client.SendMessage(pingPacket))
                    Ping = DateTime.Now.ToString();
            }
        }

        private void ReceiveMessage(SendPacket message)
        {
            if (message.Type == MessageType.MESSAGE)
            {
                App.Current.Dispatcher.BeginInvoke(new Action(() =>
                {
                    Outputs.Add(message.Message);
                }));
            }
        }

        //public Tuple<int, byte[]> Receive()
        //{

        //    try
        //    {
        //        byte[] ReadBuffer = new byte[2048];
        //        var nRecv = Sock.Receive(ReadBuffer, 0, ReadBuffer.Length, SocketFlags.None);

        //        if (nRecv == 0)
        //        {
        //            return null;
        //        }

        //        return Tuple.Create(nRecv, ReadBuffer);
        //    }
        //    catch (SocketException se)
        //    {
        //        LatestErrorMsg = se.Message;
        //    }

        //    return null;
        //}
        /** Functions end **/

        //////////////////////////////// Command Start
        public ICommand RunCommand { get; set; }
        public ICommand StopCommand { get; set; }
        public ICommand SendMessageCommand { get; set; }

        private void InitCommand()
        {
            RunCommand = new AsyncCommand(ExecuteRun);
            StopCommand = new AsyncCommand(ExecuteStop);
            SendMessageCommand = new AsyncCommand(ExecuteSendMessage);
        }

        private async Task ExecuteRun()
        {
            if (_serverSocket != null)
                _serverSocket.Close();
            _serverSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            int port = Port.ToInt();

            IPAddress ipAdr;
            IPAddress.TryParse(IP, out ipAdr);
            IPEndPoint End = new IPEndPoint(ipAdr, port);
            _serverSocket.Connect(End);
            byte[] ReadBuffer = new byte[2048];
            await _serverSocket.ReceiveAsync(ReadBuffer).ContinueWith((recvByteLength) =>
            {
                string text = Encoding.UTF8.GetString(ReadBuffer, 0, recvByteLength.Result);
                var packet = JsonSerializer.Deserialize<SendPacket>(text);
                if (packet != null)
                {
                    ClientGUID = packet.GuidId;
                    if (_client == null)
                    {
                        _client = new Client(_serverSocket, ClientGUID);
                        _client.OnReceivePacket += ReceiveMessage;
                    }
                }

                _isRunning = true;
                _pingTask = Task.Run(() => PingCheck());
            });
        }
        private async Task ExecuteStop()
        {
            if (_serverSocket != null)
                _serverSocket.Close();
            if (_client != null)
                _client.Disconnect();
            _isRunning = false;
        }

        private async Task ExecuteSendMessage()
        {
            SendPacket packet = new SendPacket();
            packet.Type = MessageType.MESSAGE;
            packet.Message = Message;
            packet.GuidId = ClientGUID;
            await _client.SendMessage(packet);
            Message = "";
        }
    }
}
