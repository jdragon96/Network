using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

namespace NetworkCS.Network
{
    public static class NetworkCore
    {
        static public bool SendMessage<T>(Socket soc, T packet)
        {
            try
            {
                if (soc != null && soc.Connected)
                {
                    byte[] ReadBuffer = new byte[2048];
                    string json = JsonSerializer.Serialize(packet);
                    var bytes = Encoding.UTF8.GetBytes(json);
                    soc.Send(bytes, 0, bytes.Length, SocketFlags.None);
                    return true;
                }
                return false;

                // 바이트 위치가 고정되어 있을 떄
                //var packetSize = bodyDataSize + PacketDef.PACKET_HEADER_SIZE;
                //List<byte> dataSource = new List<byte>();
                //dataSource.AddRange(BitConverter.GetBytes((UInt16)packetSize));
                //dataSource.AddRange(BitConverter.GetBytes((UInt16)packetID));
                //dataSource.AddRange(new byte[] { (byte)0 });
                //dataSource.AddRange(bodyData);

                //using (Stream s = new NetworkStream(soc))
                //{
                //    StreamWriter writer = new StreamWriter(s);
                //    writer.AutoFlush = true;
                //    string json = JsonSerializer.Serialize(packet);
                //    writer.WriteLine(json);
                //    return true;
                //}
            }
            catch (IOException e)
            {
                Console.WriteLine("TrySendMessage " + e.Message);
                return false;
            }
        }
        static public bool SendMessage(Socket soc, string packetStr)
        {
            try
            {
                using (Stream s = new NetworkStream(soc))
                {
                    StreamWriter writer = new StreamWriter(s);
                    writer.AutoFlush = true;
                    writer.WriteLine(packetStr);
                    return true;
                }
            }
            catch (IOException e)
            {
                Console.WriteLine("TrySendMessage " + e.Message);
                return false;
            }
        }
        static public T ReadMessage<T>(Socket soc, int timeout = 1000)
        {
            byte[] ReadBuffer = new byte[2048];
            var rev = soc.Receive(ReadBuffer);
            string text = Encoding.UTF8.GetString(ReadBuffer, 0, rev);
            return JsonSerializer.Deserialize<T>(text);

            using (Stream s = new NetworkStream(soc))
            {
                //s.ReadTimeout = timeout;
                //var reader = new StreamReader(s);
                //var msg = reader.ReadLine();
                //return JsonSerializer.Deserialize<T>(msg);
            }
        }
    }
}
