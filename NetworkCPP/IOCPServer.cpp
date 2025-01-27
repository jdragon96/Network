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

	// TCP ���� ���
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
	// ��� �ҽ����� ������ ���
	stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	// ��Ĺ�� �������� �ο�
	int nRet = bind(mListenSocket, (SOCKADDR*)&stServerAddr, sizeof(SOCKADDR_IN));
	if (0 != nRet)
		return false;
	// ���� ���
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
		// 1. IOCP �ڵ鷯 ����
		mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, mMaxClientNumber);
		if (NULL == mIOCPHandle)
		{
			return false;
		}
	}
	{
		// 2. ���� ����
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

		// Ŀ�ؼ� �޼��� ���� �� ���� ���
		auto socket = WSAAccept(mListenSocket, (SOCKADDR*)&stClientAddr, &nAddrLen, NULL, NULL);
		if (INVALID_SOCKET == socket)
		{
			continue;
		}

		// IOC Port�� ��Ĺ ����
		stClientInfo* clientPtr = new stClientInfo();
		clientPtr->m_socketClient = socket;
		if (!BindCompletionKey(clientPtr))
		{
			delete clientPtr;
			continue;
		}

		// ���� IOCP �۾� ��û
		char clientIP[32] = { 0, };
		inet_ntop(AF_INET, &(stClientAddr.sin_addr), clientIP, 32 - 1);
		clientPtr->m_ip = clientIP;
		clientPtr->m_port = stClientAddr.sin_port;
		mClientInfos.push_back(clientPtr);
		connectObserver.Broadcast(clientPtr);

		// ���� �۾� ��û
		WaitRecv(clientPtr);
	}
}

void IOCPServer::MsgMonitoring()
{
	//CompletionKey�� ���� ������ ����
	stClientInfo* pClientInfo = NULL;
	//�Լ� ȣ�� ���� ����
	BOOL bSuccess = TRUE;
	//Overlapped I/O�۾����� ���۵� ������ ũ��
	DWORD dwIoSize = 0;
	//I/O �۾��� ���� ��û�� Overlapped ����ü�� ���� ������
	LPOVERLAPPED lpOverlapped = NULL;
	stOverlappedEx buffer;

	while (isRunning)
	{
		bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
			&dwIoSize,					// ������ ���۵� ����Ʈ
			(PULONG_PTR)&pClientInfo,		// CompletionKey
			&lpOverlapped,				// Overlapped IO ��ü
			INFINITE);						// ����� �ð�

		if (NULL == lpOverlapped)
		{
			continue;
		}

		// ���� ���д� ��Ĺ ����� ó���Ѵ�.
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
	* I/O �Ϸ� ��Ʈ�� �����, ���� �ڵ鿡 �����Ѵ�.
	* - pClientInfo->m_socketClient: I/O �Ϸ� �˶��� ������ ���
	* - mIOCPHandle : IOCP
	* - CompletionKey: I/O �Ϸ� ��Ŷ�� ���Ե� ����� ���� �Ϸ�Ű
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

	struct linger stLinger = { 0, 0 };	// SO_DONTLINGER�� ����
	// bIsForce�� true�̸� SO_LINGER, timeout = 0���� �����Ͽ� ���� ���� ��Ų��. ���� : ������ �ս��� ������ ���� 
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

	//���۵� �޼����� ����
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
		// ���� �����ϱ�
		closesocket(clientPtr->m_socketClient);
		std::remove(mClientInfos.begin(), mClientInfos.end(), clientPtr);
		std::cout << "[����] WaitSend" << " ���� �߻�" << std::endl;
	}
}

void IOCPServer::WaitRecv(stClientInfo* clientPtr)
{
	DWORD dwFlag = 0;
	DWORD dwRecvNumBytes = 0;

	// Overlapped I/O�� ���� �� ������ ������ �ش�.
	// 1024����Ʈ �����͸� ���Ź޴´�.
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

	//socket_error�̸� client socket�� �������ɷ� ó���Ѵ�.
	if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
	{
		// ���� �����ϱ�
		closesocket(clientPtr->m_socketClient);
		std::remove(mClientInfos.begin(), mClientInfos.end(), clientPtr);
		std::cout << "[����] WaitRecv" << " ���� �߻�" << std::endl;
	}
}