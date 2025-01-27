//#include <windows.h>
//#include <stdio.h>
//#include <string>
//#include <fstream>
#include <iostream>
#include <string>

#include "IOCPServer.h"
#include "NetworkUtility.h"

IOCPServer server(100);

void ReceiveMsgEvent(SendPacket& packet)
{
	if (packet.Type == MessageType::MESSAGE)
	{
		std::cout << "[����]" << packet.ToString() << std::endl;
		server.Broadcast(packet);
	}
}

void ConnectEvent(stClientInfo* client)
{
	SendPacket packet;
	packet.GuidId = NetworkUtility::generateGUID();
	packet.Message = "���� �õ�";
	packet.Type = MessageType::OPEN;
	packet.Source = IOOperation::SEND;
	server.WaitSend(client, packet);
}

int main(int argc, char** argv) {
	server.Open();
	server.Bind(11345);
	server.Start(4);

	server.OnReceiveMsg(ReceiveMsgEvent);
	server.OnConnectClient(ConnectEvent);

	printf("�ƹ� Ű�� ���� ������ ����մϴ�\n");
	getchar();

	server.Close();
	return 0;
}

