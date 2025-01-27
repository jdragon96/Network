#pragma once
#pragma comment(lib, "ws2_32")

#include <winsock2.h>
#include <Ws2tcpip.h>

#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <functional>
#include <algorithm>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

#include "Observer.h"

#define MAX_SOCKBUF 1024	//��Ŷ ũ��

enum class IOOperation
{
	RECV,
	SEND
};

//WSAOVERLAPPED����ü�� Ȯ�� ���Ѽ� �ʿ��� ������ �� �־���.
struct stOverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;		//Overlapped I/O����ü
	SOCKET		m_socketClient;			//Ŭ���̾�Ʈ ����
	WSABUF		m_wsaBuf;				//Overlapped I/O�۾� ����
	char		m_szBuf[MAX_SOCKBUF]; //������ ����
	IOOperation m_eOperation;			//�۾� ���� ����
};

//Ŭ���̾�Ʈ ������ ������� ����ü
struct stClientInfo
{
	SOCKET			m_socketClient;			//Cliet�� ����Ǵ� ����
	std::string m_ip;
	USHORT m_port;
	stOverlappedEx	m_stRecvOverlappedEx;	//RECV Overlapped I/O�۾��� ���� ����
	stOverlappedEx	m_stSendOverlappedEx;	//SEND Overlapped I/O�۾��� ���� ����

	char			mRecvBuf[MAX_SOCKBUF]; //������ ����
	char			mSendBuf[MAX_SOCKBUF]; //������ ����

	stClientInfo()
	{
		ZeroMemory(&m_stRecvOverlappedEx, sizeof(stOverlappedEx));
		m_stRecvOverlappedEx.m_eOperation = IOOperation::RECV;
		ZeroMemory(&m_stSendOverlappedEx, sizeof(stOverlappedEx));
		m_stSendOverlappedEx.m_eOperation = IOOperation::SEND;
		m_socketClient = INVALID_SOCKET;
	}
};

enum class MessageType
{
	OPEN, CLOSE, MESSAGE, HEARTBEAT
};

struct SendPacket
{
	std::string GuidId;
	MessageType Type;
	std::string Message;
	IOOperation Source = IOOperation::RECV;

	static SendPacket Parser(const char* str)
	{
		SendPacket packet;
		try {
			rapidjson::Document doc;
			doc.Parse(str);
			if (!doc.HasParseError())
			{
				if (doc.HasMember("Message"))
				{
					packet.Message = doc["Message"].GetString();
				}
				if (doc.HasMember("GuidId"))
				{
					packet.GuidId = doc["GuidId"].GetString();
				}
				if (doc.HasMember("Type"))
				{
					packet.Type = static_cast<MessageType>(doc["Type"].GetInt());
				}
				if (doc.HasMember("Source"))
				{
					packet.Source = static_cast<IOOperation>(doc["Source"].GetInt());
				}
			}
		}
		catch (std::exception e)
		{

		}
		return packet;
	}

	std::string ToString()
	{
		rapidjson::StringBuffer sb;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
		writer.StartObject();
		writer.String("GuidId");
		writer.String(GuidId.c_str());
		writer.String("Type");
		writer.Int(static_cast<int>(Type));
		writer.String("Message");
		writer.String(Message.c_str());
		writer.EndObject();
		return sb.GetString();
	}
};

class IOCPServer
{
public:
	IOCPServer(int maxClientNumber);

	/**
	* �ش� ���α׷��� ��Ĺ �ν��Ͻ��� �����Ѵ�.
	*
	*/
	bool Open();

	/**
	* �ش� ���α׷��� ��Ĺ�� ���ε��Ѵ�.
	*
	* @params listenPort: ������ ��Ʈ��ȣ
	*/
	bool Bind(int listenPort);

	/**
	* �޼��� ó�� ����
	*/
	bool Start(int numOfThread);

	/**
	* ���� ����
	*/
	void Close();

	void Broadcast(SendPacket packet);

	void OnConnectClient(std::function<void(stClientInfo* client)> func)
	{
		connectObserver.Subscribe(func);
	}

	void OnReceiveMsg(std::function<void(SendPacket&)> func)
	{
		recvObserver.Subscribe(func);
	}

	void OnSendMsg(std::function<void(SendPacket&)> func)
	{
		sendObserver.Subscribe(func);
	}

	void WaitSend(stClientInfo* clientPtr, SendPacket packet);

	void WaitRecv(stClientInfo* clientPtr);

private:
	/**
	* Ŀ�ؼ� �޼����� ����͸��Ѵ�.
	*/
	void ConnMonitoring();

	/**
	* �޼����� ����͸��Ѵ�.
	*/
	void MsgMonitoring();

	/**
	* IOCP �ڵ鷯�� Completion key�� ����Ѵ�.
	*
	* @params pClientInfo: Overlapped I/O�� ��û�� Ŭ���̾�Ʈ
	*/
	bool BindCompletionKey(stClientInfo* clientPtr);

	/**
	* IOCP �޼����� �д´�.
	*/
	SendPacket RecvParser(stClientInfo* pClientInfo, stOverlappedEx& overlap, stOverlappedEx* recvOverlap);

	SendPacket MsgParser(stClientInfo* clientPtr);

	void CloseSocket(stClientInfo* pClientInfo);

private:
	bool isRunning;
	int mMaxClientNumber;
	std::vector<stClientInfo*> mClientInfos;

	SOCKET		mListenSocket = INVALID_SOCKET;
	HANDLE		mIOCPHandle = INVALID_HANDLE_VALUE;

	std::thread	mConnectionThread;
	std::vector<std::thread>	mMsgMonitorThread;

	Observer<SendPacket&> recvObserver;
	Observer<SendPacket&> sendObserver;
	Observer<stClientInfo*> connectObserver;
};