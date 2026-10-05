# LAN Peer-to-Peer File Transfer

A simple peer-to-peer file transfer application written in C using TCP sockets. It allows two devices on the same local network to transfer files directly without relying on a central file-storage server.

## Features

- TCP-based communication using IPv4 sockets.
- Direct file transfer between two devices.
- Transfers files in chunks instead of loading the entire file into memory.
- Sends file metadata, including filename and file size, before transferring file contents.
- Handles partial TCP sends and receives.
- Displays transfer progress.
- Saves received files in the `downloads/` directory.

## Project Structure

```text
.
├── src/
│   ├── client.c
│   └── server.c
├── downloads/
├── Makefile
├── README.md
├── LICENSE
└── .gitignore
```

## Requirements

- Linux
- GCC
- GNU Make

## Build

Clone the repository:

```bash
git clone <YOUR_REPOSITORY_URL>
cd <YOUR_REPOSITORY_NAME>
```

Compile the project:

```bash
make
```

## Usage

### 1. Start the receiver

On the device that will receive the file:

```bash
mkdir -p downloads
./server
```

The server listens for incoming TCP connections on port `9001`.

### 2. Send a file

On the device that will send the file:

```bash
./client <RECEIVER_IP> <FILE_PATH>
```

Example:

```bash
./client 192.168.1.105 sample.txt
```

The received file will be saved in the receiver's `downloads/` directory.

For testing on the same machine, use:

```bash
./client 127.0.0.1 sample.txt
```

Both devices must be connected to the same network and the receiver must allow incoming TCP connections on port `9001`.

## How It Works

1. The receiver creates a TCP socket and binds it to port `9001`.
2. The receiver listens for incoming connections.
3. The sender creates a TCP connection to the receiver's IP address.
4. The sender transmits the file size and filename as metadata.
5. The file contents are transferred in chunks over TCP.
6. The receiver writes the incoming data to a file in `downloads/`.
7. The receiver sends a status response after attempting to save the file.

## Current Limitations

- The receiver handles one file transfer per server execution.
- The receiver's IP address must be provided manually.
- Both devices must be reachable on the same network.
- Automatic peer discovery is not implemented.
- Encryption and authentication are not implemented.
- A received file with the same name as an existing file may overwrite it.

## Clean Build Files

To remove the generated executables:

```bash
make clean
```

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).