#include "ARTrailUdpReceiver.h"
#include "Common/UdpSocketBuilder.h"

constexpr int32 BufferSize = 2 * 1024 * 1024; // 2MB

FARTrailUdpReceiver::~FARTrailUdpReceiver()
{
	Stop();
}

bool FARTrailUdpReceiver::Start()
{
	UE_LOG(LogTemp, Display, TEXT("ARTrailUdpReceiver::Start()"));
	
	if (bRunning.Load())
	{
		return true;
	}

	if (!CreateSocket())
	{
		return false;
	}

	SocketReceiver = MakeUnique<FUdpSocketReceiver>(Socket, FTimespan::FromMilliseconds(5), TEXT("Trail_UdpReceiver"));
	SocketReceiver->OnDataReceived().BindRaw(this, &FARTrailUdpReceiver::HandleDataReceived);
	SocketReceiver->Start();

	bRunning.Store(true);
	return true;
}

void FARTrailUdpReceiver::Stop()
{
	if (SocketReceiver)
	{
		SocketReceiver->Stop();
		SocketReceiver.Reset();
	}
	if (Socket)
	{
		DestroySocket();
	}

	bRunning.Store(false);
}

bool FARTrailUdpReceiver::DequeueMessage(FARTrailUdpMessage& OutMessage)
{
	return MessageQueue.Dequeue(OutMessage);
}

bool FARTrailUdpReceiver::CreateSocket()
{
	UE_LOG(LogTemp, Display, TEXT("ARTrailUdpReceiver::CreateSocket()"));
	
	const FIPv4Endpoint ListenEndpoint(FIPv4Address::Any, Port);
	Socket = FUdpSocketBuilder(TEXT("ARTrail_UDP_Socket"))
	         .AsReusable()
	         .AsNonBlocking()
	         .BoundToEndpoint(ListenEndpoint)
	         .WithReceiveBufferSize(BufferSize);

	return Socket != nullptr;
}

void FARTrailUdpReceiver::DestroySocket()
{
	Socket->Close();
	ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
	Socket = nullptr;
}

void FARTrailUdpReceiver::HandleDataReceived(const FArrayReaderPtr& ReaderPtr, const FIPv4Endpoint& Sender)
{
	// UE_LOG(LogTemp, Warning, TEXT("[Recv] receiver=%p bytes=%d from %s"), this, ReaderPtr->Num(), *Sender.ToString());

	if (!ReaderPtr || ReaderPtr->Num() <= 0 || !bRunning.Load())
	{
		return;
	}
	
	FARTrailUdpMessage Msg;
	Msg.Sender = Sender;
	Msg.Data.Append(ReaderPtr->GetData(), ReaderPtr->Num());
	
	MessageQueue.Enqueue(MoveTemp(Msg));
}
