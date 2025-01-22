using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NetworkCS.Network
{
    public enum MessageType
    {
        OPEN, CLOSE, MESSAGE, HEARTBEAT
    }

    [Serializable]
    public class SendPacket
    {
        public string GuidId { get; set; } = "";
        public MessageType Type { get; set; } = MessageType.MESSAGE;
        public string Message { set; get; } = "";
    }

}
