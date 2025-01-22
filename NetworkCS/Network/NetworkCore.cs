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
                using (Stream s = new NetworkStream(soc))
                {
                    StreamWriter writer = new StreamWriter(s);
                    writer.AutoFlush = true;
                    string json = JsonSerializer.Serialize(packet);
                    writer.WriteLine(json);
                    return true;
                }
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
            using (Stream s = new NetworkStream(soc))
            {
                s.ReadTimeout = timeout;
                var reader = new StreamReader(s);
                var msg = reader.ReadLine();
                return JsonSerializer.Deserialize<T>(msg);
            }
        }
    }
}
