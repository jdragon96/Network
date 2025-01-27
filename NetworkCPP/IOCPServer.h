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

#define MAX_SOCKBUF 1024	//패킷 크기

enum class IOOperation
{
	RECV,
	SEND
};

//WSAOVERLAPPED구조체를 확장 시켜서 필요한 정보를 더 넣었다.
struct stOverlappedEx
{
	WSAOVERLAPPED m_wsaOverlapped;		//Overlapped I/O구조체
	SOCKET		m_socketClient;			//클라이언트 소켓
	WSABUF		m_wsaBuf;				//Overlapped I/O작업 버퍼
	char		m_szBuf[MAX_SOCKBUF]; //데이터 버퍼
	IOOperation m_eOperation;			//작업 동작 종류
};

//클라이언트 정보를 담기위한 구조체
struct stClientInfo
{
	SOCKET			m_socketClient;			//Cliet와 연결되는 소켓
	std::string m_ip;
	USHORT m_port;
	stOverlappedEx	m_stRecvOverlappedEx;	//RECV Overlapped I/O작업을 위한 변수
	stOverlappedEx	m_stSendOverlappedEx;	//SEND Overlapped I/O작업을 위한 변수

	char			mRecvBuf[MAX_SOCKBUF]; //데이터 버퍼
	char			mSendBuf[MAX_SOCKBUF]; //데이터 버퍼

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
	* 해당 프로그램의 소캣 인스턴스를 생성한다.
	*
	*/
	bool Open();

	/**
	* 해당 프로그램의 소캣을 바인딩한다.
	*
	* @params listenPort: 서버측 포트번호
	*/
	bool Bind(int listenPort);

	/**
	* 메세지 처리 시작
	*/
	bool Start(int numOfThread);

	/**
	* 서버 종료
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
	* 커넥션 메세지를 모니터링한다.
	*/
	void ConnMonitoring();

	/**
	* 메세지를 모니터링한다.
	*/
	void MsgMonitoring();

	/**
	* IOCP 핸들러에 Completion key를 등록한다.
	*
	* @params pClientInfo: Overlapped I/O를 요청할 클라이언트
	*/
	bool BindCompletionKey(stClientInfo* clientPtr);

	/**
	* IOCP 메세지를 읽는다.
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