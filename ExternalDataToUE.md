# Design of sending JSON from external CPP to Unreal Engine

## 1. Summary
For a real-time video/live-streaming scenario, UDP is used, where an external C++ program acts as the message sender (client) and the UE application acts as the receiver (server). 

Goals:
1. Low-latency packet reception.
2. Avoid blocking the game thread.
3. Transfer data from the network thread to the game thread.
4. Tolerate UDP packet loss or out-of-order delivery.

## 2. Class

## 2.1 `FARTrailUdpReceiver`

```cpp 
// FARTrailUdpReceiver.h

struct FARTrailUdpMessage;

class FARTrailUdpReceiver
{
public:
    // Constructor
    // Initialize configuration parameters（Port、Packet size, etc）
    FARTrailUdpReceiver(int32 ListenPort);
    FARTrailUdpReceiver() = delete;

    // Destructor
    ~FARTrailUdpReceiver();

    // Start receiving messages
    // 1) CreateSocket()
    // 2) Construct FUdpSocketReceiver
    // 3) Bind OnDataReceived callback
    // 4) Receiver->Start()
    bool Start();

    // Stop receiving messages and release resources.
    // 1) Receiver->Stop()
    // 2) DestroySocket()
    // 3) Update bRunning
    void Stop();

    // Read messages
    bool DequeueMessage(FARTrailUdpMessage& OutMessage);

    bool IsRunning() const { return bRunning.Load(); }

private:
    // Create UDP Socket and initialization
    bool CreateSocket();

    // Close + DestroySocket
    void DestroySocket();

    // OnDataReceived callback
    void HandleDataReceived(const FArrayReaderPtr&, const FIPv4Endpoint& Sender);

private:
    int32 ListenPort;
    FSocket* Socket = nullptr; // UDP Socket
    TUniquePtr<FUdpSocketReceiver> Receiver; // UDP Socket receiver
    TQueue<FARTrailUdpMessage> MessageQueue;
    TAtomic<bool> bRunning{false}; // Marker：True if start sucessfully
};
```

## 3. Design Explanation

The worker thread must not access UObjects for data operations, and the game thread must not directly operate on `Socket` or `Receiver`. `MessageQueue` is the only cross-thread exchange channel. This decouples the two threads through a queue instead of allowing direct reads/writes of each other's internal state.

This implementation is based on Unreal Engine 5.7 networking source codes, including `FUdpMessageProcessor.h/.cpp`, `UdpSocketBuilder.h`, `UdpSocketReceiver.h`, `UdpMessageTransport.h/cpp` and `Sockets.h/cpp`, etc.

### 3.1 Variables

- `FSocket* Socket`: Low-level UDP port binding handle.
- `TUniquePtr<FUdpSocketReceiver> Receiver`: Executes the receive callback when `OnDataReceived` is triggered.
- `TQueue<FARTrailUdpMessage> MessageQueue`: Lets the game thread pull messages safely, avoiding direct UObject/state mutation from the worker thread.
- `TAtomic<bool> bRunning`: Exposes running state safely across threads and avoids data races.

Reference UE 5.7 source code:
```cpp 
// Source/Runtime/Networking/Public/Common/UdpSocketReceiver.h

// Delegate type for received data.
// The first parameter is the received data. The second parameter is sender's IP endpoint.
DECLARE_DELEGATE_TwoParams(FOnSocketDataReceived, const FArrayReaderPtr&, const FIPv4Endpoint&);

// Asynchronously receives data from an UDP socket.
class FUdpSocketReceiver : public FRunnable , private FSingleThreadRunnable{
    // Returns a delegate that is executed when data has been received.
    // This delegate must be bound before the receiver thread is started with the Start() method. It cannot
    // be unbound while the thread is running.
    // Returns:
    // The delegate.
    FOnSocketDataReceived& OnDataReceived();
}
```

### 3.2 Functions

- `HandleDataReceived(const FArrayReaderPtr&, const FIPv4Endpoint&)`: Network-thread callback entry point that packs and enqueues received messages.
- `DequeueMessage(FARTrailUdpMessage& OutMessage)`: Game-thread consumption entry point.

## 4. Data Workflow Pipeline

1. The external C++ program sends UDP messages.
2. `Socket` receives data, and `FUdpSocketReceiver` triggers the callback on the worker thread.
3. `HandleDataReceived` packs the message and enqueues it.
4. The game thread consumes messages in `Tick` by repeatedly calling `DequeueMessage`.
5. The data is parsed into UE data structures and applied to runtime state updates.
