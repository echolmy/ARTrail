#pragma once
#include "Common/UdpSocketReceiver.h"

struct FARTrailUdpMessage
{
	FIPv4Endpoint Sender;
	TArray<uint8> Data;
};

class FARTrailUdpReceiver
{
public:
	FARTrailUdpReceiver(int32 InPort) : Port(InPort)
	{
	}

	FARTrailUdpReceiver() = delete;

	~FARTrailUdpReceiver();

	bool Start();

	void Stop();

	bool FORCEINLINE IsRunning() const { return bRunning.Load(); }
	
	bool DequeueMessage(FARTrailUdpMessage& OutMessage);

private:
	bool CreateSocket();

	void DestroySocket();

	// OnDataReceived callback
	void HandleDataReceived(const FArrayReaderPtr& ReaderPtr, const FIPv4Endpoint& Sender);

private:
	int32 Port;
	FSocket* Socket = nullptr;
	TUniquePtr<FUdpSocketReceiver> SocketReceiver;

	TQueue<FARTrailUdpMessage> MessageQueue;

	TAtomic<bool> bRunning{false};
};
