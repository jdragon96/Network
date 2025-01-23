using NetworkCS.Network;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Runtime.Serialization.Formatters.Binary;
using System.Security.Policy;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace NetworkCS.Network
{
    public class Client
    {
        public delegate void ClientListenEventHandler(SendPacket packet);

        public Guid ClientId { get; private set; }
        public Socket Socket { get; private set; }
        public IPEndPoint EndPoint { get; private set; }
        public IPAddress Address { get; private set; }
        public bool IsRunning { get; private set; }

        public event ClientListenEventHandler OnReceivePacket;
        private Task _monitoring;

        public int ReceiveBufferSize
        {
            get { return Socket.ReceiveBufferSize; }
            set { Socket.ReceiveBufferSize = value; }
        }

        public int SendBufferSize
        {
            get { return Socket.SendBufferSize; }
            set { Socket.SendBufferSize = value; }
        }

        public Client(string address, int port)
        {
            IPAddress ipAddress;
            var validIp = IPAddress.TryParse(address, out ipAddress);
            if (!validIp)
                ipAddress = Dns.GetHostAddresses(address)[0];
            Address = ipAddress;
            EndPoint = new IPEndPoint(ipAddress, port);
            Socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            ReceiveBufferSize = 8000;
            SendBufferSize = 8000;
            IsRunning = true;
        }

        public Client(Socket socket)
        {
            Socket = socket;
            var endPoint = ((IPEndPoint)Socket.LocalEndPoint);
            EndPoint = endPoint;
            ClientId = Guid.NewGuid();
            ReceiveBufferSize = 8000;
            SendBufferSize = 8000;
            IsRunning = true;
        }

        public Client(Socket socket, string guid)
        {
            Socket = socket;
            IsRunning = true;
            ClientId = Guid.Parse(guid);
            _monitoring = Task.Run(() => MonitorStreams());
        }

        private async Task MonitorStreams()
        {
            while (IsRunning)
            {
                Thread.Sleep(10);
                var packet = await ReadMessage();
                if (packet != null)
                {
                    OnReceivePacket.Invoke(packet);
                }
            }
        }

        public async Task<bool> SendMessage<T>(T packet)
        {
            return NetworkCore.SendMessage(Socket, packet);
        }

        public async Task<SendPacket?> ReadMessage()
        {
            if (Socket == null)
                return null;
            if (Socket.Available == 0)
                return null;
            return NetworkCore.ReadMessage<SendPacket>(Socket, 1000);
        }

        //https://stackoverflow.com/questions/2661764/how-to-check-if-a-socket-is-connected-disconnected-in-c
        public bool IsSocketConnected()
        {
            try
            {
                bool part1 = Socket.Poll(5000, SelectMode.SelectRead);
                bool part2 = (Socket.Available == 0);
                if (part1 && part2)
                    return false;
                else
                    return true;
            }
            catch (ObjectDisposedException e)
            {
                Console.WriteLine("IsSocketConnected " + e.Message);
                return false;
            }
        }
        public void Disconnect()
        {
            if (Socket != null)
                Socket.Close();
            IsRunning = false;
        }
    }
}
