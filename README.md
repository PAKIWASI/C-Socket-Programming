# C TCP Chat Application 


### Server Capabilities

1. **Multi-Client Support**
   - Handles up to 100 concurrent clients using `select()` multiplexing
   - Thread-safe client management with `Client` struct array
   - Efficient single-threaded architecture (no race conditions)

2. **User Management**
   - Automatic username assignment (`User<socket_fd>`)
   - Username change capability via `/name` command
   - Duplicate username prevention
   - User list display via `/list` command

3. **Message Broadcasting (Publish-Subscribe)**
   - Messages sent to all connected clients except sender
   - Join/leave notifications broadcast to all users
   - Username change notifications

4. **Unicast (Private Messaging)**
   - Direct messages via `/msg <username> <message>`
   - Confirmation sent to both sender and recipient
   - User validation before message delivery

5. **Robust Error Handling**
   - Graceful client disconnection handling
   - Network error detection and recovery
   - Server full notification
   - Invalid command feedback

### Client Capabilities

1. **Connection Management**
   - Connect to server using IP address
   - Graceful disconnection with `quit` command
   - Connection status feedback

2. **Dual Input Monitoring**
   - Simultaneous monitoring of server messages and user input
   - Uses `select()` for non-blocking I/O
   - Real-time message display

3. **Command Support**
   - `/name <username>` - Change username
   - `/msg <user> <message>` - Send private message
   - `/list` - View online users
   - `/help` - Display command help
   - `quit` - Exit application

4. **User Experience**
   - Clear message formatting with sender identification
   - Private message indicators `[PM from/to]`
   - System notifications with `***` markers

---

## Usage Instructions

### Starting the Server

```bash
./chatserver
```

**Output:**
```
Chat server listening on port 6969
Max clients: 100
```

### Connecting Clients

```bash
# Connect to localhost
./chatclient 127.0.0.1

# Connect to remote server
./chatclient 192.168.1.100
```

### Example Session

**Client 1:**
```
Connected to chat server at 127.0.0.1:6969
Type /help for commands, or just type to chat!
Type 'quit' to exit

Welcome to the chat! Your username is: User4
Type /help for commands
*** User4 has joined the chat ***

/name Alice
*** User4 is now known as Alice ***
Username changed successfully

Hello everyone!
[Alice]: Hello everyone!

/msg Bob Hi there!
[PM to Bob]: Hi there!
```

**Client 2:**
```
Connected to chat server at 127.0.0.1:6969
Welcome to the chat! Your username is: User5

*** User4 has joined the chat ***
*** User4 is now known as Alice ***

/name Bob
Username changed successfully

[Alice]: Hello everyone!

[PM from Alice]: Hi there!

/list

=== Online Users ===
[Alice]
[Bob]  (you)
====================
```

---

## Technical Architecture

### Server Architecture

```
┌─────────────────────────────────────┐
│       Main Server Loop              │
│  (select() multiplexing)            │
├─────────────────────────────────────┤
│                                     │
│  ┌──────────────┐  ┌─────────────┐ │
│  │  Listen FD   │  │ Client FDs  │ │
│  │  (new conn)  │  │  (messages) │ │
│  └──────────────┘  └─────────────┘ │
│         │                  │        │
│         ▼                  ▼        │
│  ┌──────────────┐  ┌─────────────┐ │
│  │ accept() new │  │   recv()    │ │
│  │   clients    │  │   messages  │ │
│  └──────────────┘  └─────────────┘ │
│         │                  │        │
│         ▼                  ▼        │
│  ┌──────────────────────────────┐  │
│  │   Client Management System   │  │
│  │  - add_client()              │  │
│  │  - remove_client()           │  │
│  │  - broadcast_message()       │  │
│  │  - handle_command()          │  │
│  └──────────────────────────────┘  │
└─────────────────────────────────────┘
```

### Client Architecture

```
┌─────────────────────────────────────┐
│       Main Client Loop              │
│  (select() multiplexing)            │
├─────────────────────────────────────┤
│                                     │
│  ┌──────────────┐  ┌─────────────┐ │
│  │  Server FD   │  │  STDIN FD   │ │
│  │ (messages)   │  │ (user input)│ │
│  └──────────────┘  └─────────────┘ │
│         │                  │        │
│         ▼                  ▼        │
│  ┌──────────────┐  ┌─────────────┐ │
│  │   recv()     │  │  fgets()    │ │
│  │   from       │  │  from user  │ │
│  │   server     │  │             │ │
│  └──────────────┘  └─────────────┘ │
│         │                  │        │
│         ▼                  ▼        │
│  ┌──────────────┐  ┌─────────────┐ │
│  │   Display    │  │   send()    │ │
│  │   to user    │  │  to server  │ │
│  └──────────────┘  └─────────────┘ │
└─────────────────────────────────────┘
```

### Key Design Decisions

1. **select() vs threads:**
   - Single-threaded with `select()` for I/O multiplexing
   - More efficient than thread-per-client model
   - No need for mutex locks or thread synchronization
   - Lower memory overhead

2. **Client Management:**
   - Fixed-size array for O(1) access
   - Active flag for efficient iteration
   - Username stored per client for fast lookups

3. **Message Protocol:**
   - Newline-terminated messages
   - Command prefix: `/`
   - Private message format: `/msg <username> <message>`
   - System notifications: `*** message ***`

4. **Error Handling:**
   - Graceful degradation on client disconnect
   - Network error logging without server crash
   - Input validation for all commands

---

## Command Reference

| Command | Description | Example |
|---------|-------------|---------|
| `/name <username>` | Change your username | `/name Alice` |
| `/msg <user> <msg>` | Send private message | `/msg Bob Hello!` |
| `/list` | List all online users | `/list` |
| `/help` | Show command help | `/help` |
| `quit` | Exit the chat | `quit` |
| `<message>` | Broadcast to all users | `Hello everyone!` |

---

## Performance Characteristics

- **Concurrency Model:** Single-threaded with I/O multiplexing
- **Maximum Clients:** 100 (configurable via `MAX_CLIENTS`)
- **Buffer Size:** 4096 bytes per message
- **Latency:** Near-instant message delivery (LAN)
- **Memory Usage:** ~50KB per client (minimal overhead)
- **CPU Usage:** Idle when no activity (event-driven)

---

## Limitations and Future Enhancements

### Current Limitations
- No persistent message history
- No encryption (plaintext communication)
- No file transfer support
- Single server (no distributed architecture)

### Potential Enhancements
1. Add SSL/TLS encryption
2. Implement message history storage
3. Add file transfer capability
4. Support for chat rooms/channels
5. Implement rate limiting
6. Add user authentication
7. Create GUI client interface

---
