using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Runtime.Serialization.Formatters.Binary;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace NetworkCS.Network
{
    public delegate void PacketEventHandler(SendPacket packet);
    public delegate void ConnectEventHandler(Client client);

    public class Server
    {
        public IPAddress Address { get; private set; }
        public int Port { get; private set; }

        public IPEndPoint EndPoint { get; private set; }
        public Socket Socket { get; private set; }

        public bool IsRunning { get; private set; }
        public List<Client> Connections { get; private set; }

        private Task _receivingTask;

        public event PacketEventHandler OnReceivePacket;
        public event ConnectEventHandler OnConnectClient;

        public Server(IPAddress address, int port)
        {
            Address = address;
            Port = port;
            EndPoint = new IPEndPoint(address, port);
            Socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            Socket.ReceiveTimeout = 5000;
            Connections = new List<Client>();
        }

        public bool Open()
        {
            Socket.Bind(EndPoint);
            Socket.Listen(10);
            return true;
        }

        public async Task<bool> Start()
        {
            _receivingTask = Task.Run(() => MonitorStreams());
            IsRunning = true;
            await MonitorNewConnect();
            await _receivingTask;
            return true;
        }

        public bool Close()
        {
            IsRunning = false;
            Connections.Clear();
            if (Socket != null)
            {
                Socket.Close();
            }
            return true;
        }

        public async Task Broadcast<T>(T packet)
        {
            foreach (var client in Connections)
            {
                NetworkCore.SendMessage(Socket, packet);
            }
        }

        public async Task Broadcast<T>(T packet, Client except)
        {
            foreach (var client in Connections)
            {
                if (except == client)
                    continue;
                if (!client.Socket.Connected)
                    continue;
                NetworkCore.SendMessage(client.Socket, packet);
            }
        }

        public async Task<bool> MonitorNewConnect()
        {
            while (IsRunning)
            {
                if (Socket.Poll(100000, SelectMode.SelectRead))
                {
                    var newConnection = Socket.Accept();
                    if (newConnection != null)
                    {
                        var client = new Client(newConnection);
                        Connections.Add(client);
                        SendPacket packet = new SendPacket();
                        packet.Type = MessageType.OPEN;
                        packet.GuidId = client.ClientId.ToString();
                        await client.SendMessage(packet);
                        OnConnectClient.Invoke(client);
                    }
                }
            }
            return true;
        }

        private void MonitorStreams()
        {
            while (IsRunning)
            {
                foreach (var client in Connections.ToList())
                {
                    // 1. 연결 끊어지면 커넥션 제거
                    if (!client.IsSocketConnected())
                    {
                        Connections.Remove(client);
                        client.Disconnect();
                        continue;
                    }
                    var packet = ReacPacket(client.Socket);
                    if (packet == null)
                        continue;
                    OnReceivePacket.Invoke(packet);
                }
            }
        }

        private SendPacket? ReacPacket(Socket clientSocket)
        {
            if (clientSocket.Available == 0)
                return null;
            return NetworkCore.ReadMessage<SendPacket>(clientSocket);
        }
    }
}
