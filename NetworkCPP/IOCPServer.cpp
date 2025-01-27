#include "./IOCPServer.h"
#include <iostream>

IOCPServer::IOCPServer(int maxClientNumber) : mMaxClientNumber(maxClientNumber), isRunning(false)
{

}

bool IOCPServer::Open()
{
	WSADATA wsaData;

	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (0 != nRet)
		return false;

	// TCP 연결 허용
	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == mListenSocket)
		return false;
	return true;
}

bool IOCPServer::Bind(int listenPort)
{
	SOCKADDR_IN		stServerAddr;
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(listenPort);
	// 모든 소스에서 접속을 허용
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	// 소캣에 서버정보 부여
	int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (0 != nRet)
		return false;
	// 접속 허용
	nRet = listen(mListenSocket, 5);
	if (0 != nRet)
		return false;
	return true;
}

void IOCPServer::Close()
{
	if (!isRunning)
		return;
	isRunning = false;
	CloseHandle(mIOCPHandle);
	closesocket(mListenSocket);
	if (mConnectionThread.joinable())
		mConnectionThread.join();
	for (auto& t : mMsgMonitorThread)
	{
		if (t.joinable())
			t.join();
	}
	for (auto cloent : mClientInfos)
	{
		delete cloent;
	}
	WSACleanup();
}

bool IOCPServer::Start(int numOfThread)
{
	{
		// 1. IOCP 핸들러 생성
		mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mMaxClientNumber);
		if (NULL == mIOCPHandle)
		{
			return false;
		}
	}
	{
		// 2. 설정 변경
		isRunning = true;
	}
	mConnectionThread = std::thread([this]() {ConnMonitoring(); });
	for (int i = 0; i < numOfThread; ++i)
	{
		mMsgMonitorThread.push_back(std::thread([this]() {MsgMonitoring(); }));
	}
	return true;
}

void IOCPServer::ConnMonitoring()
{
	SOCKADDR_IN		stClientAddr;
	int nAddrLen = sizeof(SOCKADDR_IN);

	while (isRunning)
	{
		if (mClientInfos.size() > mMaxClientNumber)
		{
			std::this_thread::sleep_for(std::chrono::microseconds(1000));
			continue;
		}

		// 커넥션 메세지 수신 전 까지 대기
		auto socket = WSAAccept(mListenSocket, (SOCKADDR*)&stClientAddr, &nAddrLen, NULL, NULL);
		if (INVALID_SOCKET == socket)
		{
			continue;
		}

		// IOC Port와 소캣 연결
		stClientInfo* clientPtr = new stClientInfo();
		clientPtr->m_socketClient = socket;
		if (!BindCompletionKey(clientPtr))
		{
			delete clientPtr;
			continue;
		}

		// 수신 IOCP 작업 요청
		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
		clientPtr->m_ip = clientIP;
		clientPtr->m_port = stClientAddr.sin_port;
		mClientInfos.push_back(clientPtr);
		connectObserver.Broadcast(clientPtr);

		// 수신 작업 요청
		WaitRecv(clientPtr);
	}
}

void IOCPServer::MsgMonitoring()
{
	//CompletionKey를 받을 포인터 변수
	stClientInfo* pClientInfo = NULL;
	//함수 호출 성공 여부
	BOOL bSuccess = TRUE;
	//Overlapped I/O작업에서 전송된 데이터 크기
	DWORD dwIoSize = 0;
	//I/O 작업을 위해 요청한 Overlapped 구조체를 받을 포인터
	LPOVERLAPPED lpOverlapped = NULL;
	stOverlappedEx buffer;

	while (isRunning)
	{
		bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
			&dwIoSize,					// 실제로 전송된 바이트
			(PULONG_PTR)&pClientInfo,		// CompletionKey
			&lpOverlapped,				// Overlapped IO 객체
			INFINITE);						// 대기할 시간

		if (NULL == lpOverlapped)
		{
			continue;
		}

		// 수신 실패는 소캣 종료로 처리한다.
		if (FALSE == bSuccess)
		{
			CloseSocket(pClientInfo);
			continue;
		}

		stOverlappedEx* pOverlappedEx = (stOverlappedEx*)lpOverlapped;
		if (pOverlappedEx->m_eOperation == IOOperation::RECV)
		{
			auto packet = SendPacket::Parser(pOverlappedEx->m_wsaBuf.buf);
			recvObserver.Broadcast(packet);
		}
		else if (pOverlappedEx->m_eOperation == IOOperation::SEND)
		{
			auto packet = SendPacket::Parser(pOverlappedEx->m_wsaBuf.buf);
			sendObserver.Broadcast(packet);
		}
		WaitRecv(pClientInfo);
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}
}

bool IOCPServer::BindCompletionKey(stClientInfo* pClientInfo)
{
	/**
	* I/O 완료 포트를 만들고, 파일 핸들에 연결한다.
	* - pClientInfo->m_socketClient: I/O 완료 알람을 감지할 대상
	* - mIOCPHandle : IOCP
	* - CompletionKey: I/O 완료 패킷에 포함된 사용자 정의 완료키
	*/
	auto hIOCP = CreateIoCompletionPort(
		(HANDLE)pClientInfo->m_socketClient,
		mIOCPHandle,
		(ULONG_PTR)(pClientInfo),
		0);
	if (NULL == hIOCP || mIOCPHandle != hIOCP)
	{
		return false;
	}
	return true;
}

void IOCPServer::CloseSocket(stClientInfo* pClientInfo)
{
	if (NULL == pClientInfo)
		return;

	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER로 설정
	// bIsForce가 true이면 SO_LINGER, timeout = 0으로 설정하여 강제 종료 시킨다. 주의 : 데이터 손실이 있을수 있음 
	stLinger.l_onoff = 1;
	shutdown(pClientInfo->m_socketClient, SD_BOTH);
	setsockopt(pClientInfo->m_socketClient, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));
	closesocket(pClientInfo->m_socketClient);
	pClientInfo->m_socketClient = INVALID_SOCKET;
}

std::string replace_all(
	__in const std::string& message,
	__in const std::string& pattern,
	__in const std::string& replace) {
	std::string result = message;
	std::string::size_type pos = 0;
	std::string::size_type offset = 0;
	while ((pos = result.find(pattern, offset)) != std::string::npos) {
		result.replace(result.begin() + pos, result.begin() + pos + pattern.size(), replace);
		offset = pos + replace.size();
	}
	return result;
}

void IOCPServer::Broadcast(SendPacket packet)
{
	for (auto client : mClientInfos)
	{
		WaitSend(client, packet);
		//WaitRecv(client);
	}
}

void IOCPServer::WaitSend(stClientInfo* clientPtr, SendPacket packet)
{
	DWORD dwRecvNumBytes = 0;
	DWORD	dwFlags = 0;

	//전송될 메세지를 복사
	auto str = packet.ToString();
	str = replace_all(str, "\n", "");
	str = replace_all(str, " ", "");
	ZeroMemory(clientPtr->m_stSendOverlappedEx.m_szBuf, MAX_SOCKBUF);
	CopyMemory(clientPtr->m_stSendOverlappedEx.m_szBuf, str.c_str(), str.size());
	clientPtr->mSendBuf[str.size()] = '\0';
	clientPtr->m_stSendOverlappedEx.m_wsaBuf.len = str.size();
	clientPtr->m_stSendOverlappedEx.m_wsaBuf.buf = clientPtr->m_stSendOverlappedEx.m_szBuf;
	clientPtr->m_stSendOverlappedEx.m_eOperation = IOOperation::SEND;

	int nRet = WSASend(
		clientPtr->m_socketClient,
		&(clientPtr->m_stSendOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		dwFlags,
		(LPWSAOVERLAPPED) & (clientPtr->m_stSendOverlappedEx),
		NULL);

	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		// 연결 해제하기
		closesocket(clientPtr->m_socketClient);
		std::remove(mClientInfos.begin(), mClientInfos.end(), clientPtr);
		std::cout << "[오류] WaitSend" << " 오류 발생" << std::endl;
	}
}

void IOCPServer::WaitRecv(stClientInfo* clientPtr)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	// Overlapped I/O을 위해 각 정보를 셋팅해 준다.
	// 1024바이트 데이터를 수신받는다.
	ZeroMemory(clientPtr->m_stRecvOverlappedEx.m_szBuf, MAX_SOCKBUF);
	clientPtr->m_stRecvOverlappedEx.m_wsaBuf.len = MAX_SOCKBUF;
	clientPtr->m_stRecvOverlappedEx.m_wsaBuf.buf = clientPtr->m_stRecvOverlappedEx.m_szBuf;
	clientPtr->m_stRecvOverlappedEx.m_eOperation = IOOperation::RECV;

	int nRet = WSARecv(clientPtr->m_socketClient,
		&(clientPtr->m_stRecvOverlappedEx.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		&dwFlag,
		(LPWSAOVERLAPPED) & (clientPtr->m_stRecvOverlappedEx),
		NULL);

	//socket_error이면 client socket이 끊어진걸로 처리한다.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		// 연결 해제하기
		closesocket(clientPtr->m_socketClient);
		std::remove(mClientInfos.begin(), mClientInfos.end(), clientPtr);
		std::cout << "[오류] WaitRecv" << " 오류 발생" << std::endl;
	}
}