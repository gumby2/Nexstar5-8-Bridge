# Functional and block diagrams

These diagrams describe the modular v1.0.0 architecture. They are logical
diagrams: module ownership is more stable than individual function names, and
some application state still resides in the Arduino sketch during the
modularization work.

## Overall system block diagram

```mermaid
flowchart LR
    subgraph Clients[External clients]
        SkyWiFi[SkySafari / LX200 Wi-Fi]
        SkyBT[SkySafari / LX200 Bluetooth]
        Alpaca[Alpaca clients]
        Stellarium[Stellarium clients]
        Browser[Web browser]
        Console[Serial / Telnet console]
    end
    subgraph Transport[Network and device transports]
        WiFi[Wi-Fi TCP / UDP]
        BT[Bluetooth SPP]
        HTTP[Async HTTP compatibility layer]
        UARTConsole[USB serial / Telnet service]
    end
    subgraph Firmware[ESP32 firmware]
        Router[LX200 / Alpaca / Stellarium / Web routers]
        Queue[GOTO queue and async request state]
        Cache[Position cache and polling scheduler]
        Proto[NexStar protocol encoding]
        Tx[Mount transport lock and recovery]
    end
    Mount[Original Celestron NexStar mount]
    SkyWiFi --> WiFi
    SkyBT --> BT
    Alpaca --> HTTP
    Stellarium --> WiFi
    Browser --> HTTP
    Console --> UARTConsole
    WiFi --> Router
    BT --> Router
    HTTP --> Router
    UARTConsole --> Router
    Router --> Cache
    Router --> Queue
    Queue --> Proto
    Cache --> Proto
    Proto --> Tx
    Tx <-->|9600 baud UART| Mount
    Cache -. cached / estimated positions .-> Router
```

## Main loop and service ordering

```mermaid
flowchart TD
    Start[loop begins] --> PollTop[Prioritized mount poll check]
    PollTop --> ConsoleSvc[Console service]
    ConsoleSvc --> Poll1[Mount poll check]
    Poll1 --> Protocols[HTTP / Alpaca / LX200 / discovery]
    Protocols --> Poll2[Mount poll check]
    Poll2 --> Stellarium[Stellarium service]
    Stellarium --> Poll3[Mount poll check]
    Poll3 --> Telnet[Telnet service]
    Telnet --> Poll4[Mount poll check]
    Poll4 --> GotoWatch[GOTO completion watcher]
    GotoWatch --> GotoQueue[GOTO queue service]
    GotoQueue --> Async[Async reads / slews]
    Async --> Time[Apply staged time/site updates]
    Time --> FinalPoll[Final mount poll check]
    FinalPoll --> Heartbeat[Heartbeat, diagnostics, yield]
    Heartbeat --> Start
    PollTop -. blocked by mountBusy, GOTO, or recovery backoff .-> PollTop
```

## Client request path

```mermaid
sequenceDiagram
    participant C as Client
    participant R as Router
    participant Q as Queue/cache
    participant N as NexStar protocol
    participant T as Mount transport
    participant M as Mount
    C->>R: Position read or GOTO request
    alt Position read
        R->>Q: Read cache or enqueue async E/Z read
        Q->>N: Request transaction when idle
    else GOTO
        R->>Q: Queue target and acknowledge client
        Q->>N: Start when mount is idle
    end
    N->>T: Acquire single-command lock
    T->>M: ?
    M-->>T: #
    T->>M: E, Z, or R + payload
    alt Fixed position response
        M-->>T: Fixed payload
    else Movement response
        M-->>T: @ completion
        T->>T: Bounded trailing-byte drain
    end
    T-->>Q: Success or bounded failure
    Q-->>R: Cache, slewing state, or recovery status
    R-->>C: Protocol response / JSON / UI update
```

## Mount transaction and recovery state machine

```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> Handshake: command due and lock available
    Handshake --> Handshake: unexpected byte / record stale input
    Handshake --> Command: receive #
    Handshake --> Recovery: handshake timeout
    Command --> PayloadRead: E or Z sent
    Command --> CompletionWait: R and payload sent
    PayloadRead --> Idle: exact payload received
    PayloadRead --> Recovery: payload timeout / invalid framing
    CompletionWait --> CompletionWait: unexpected byte / record anomaly
    CompletionWait --> Drain: receive @
    CompletionWait --> Recovery: completion timeout
    Drain --> Idle: quiet interval reached
    Recovery --> Backoff: record fault and release bounded transaction
    Backoff --> Idle: recovery pause expires
```

The state machine is the contract for future refactors: no new mount command
may bypass the lock, and normal E/Z polling must not restart before a GOTO
completion and drain boundary is complete.
