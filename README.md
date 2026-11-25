# Đề tài 15: POP3 v5 — Thống kê và sao lưu hộp thư (C++ / WinSock2)

Chương trình Client–Server thực thi giao thức **POP3 (RFC 1939)** bằng **C++ (MinGW / Code::Blocks)**, kế thừa bộ thư viện OOP WinSock2 (`Socket`, `TcpSocket`, `TcpServerSocket`, `TCPServer`, `Session`, `ServerConfig`, `Account`, `TcpClient`, `CmdLineInterface`).

## Chức năng chính
- **POP3 Server (`Pop3Server/`)**:
  - Hỗ trợ đa luồng (`std::thread`) phục vụ đồng thời nhiều client, khóa hộp thư theo phiên (`lockMailbox`/`unlockMailbox`), xử lý chuẩn 3 pha `AUTHORIZATION` → `TRANSACTION` → `UPDATE` và dot-stuffing RFC 1939.
  - Các lệnh CLI:
    - `start [port]` : Khởi động server và lắng nghe kết nối (mặc định cổng `110`).
    - `stop` : Tắt server.
    - `status`, `help`, `quit`.
- **POP3 Client (`Pop3Client/`)**:
  - Kết nối và đăng nhập bằng `USER`/`PASS`, thống kê tổng số email và tổng dung lượng (byte) qua `STAT`, tải toàn bộ email qua `LIST`/`RETR` (kèm dot-unstuffing) và **ghi nối tiếp (append)** vào một file văn bản duy nhất mà **không xóa mail trên server**.
  - Các lệnh CLI:
    - `login <IP server> <username> <password> [port]` : Kết nối và đăng nhập (mặc định cổng `110`).
    - `stat` : Xem tổng số email và tổng dung lượng (byte) của hộp thư.
    - `backup <tên file lưu>` : Tải toàn bộ email hiện có và ghi nối tiếp vào `<tên file lưu>`.
    - `quit` : Gửi lệnh `QUIT`, kết thúc phiên làm việc và đóng kết nối.
    - Lệnh bổ trợ: `list`, `logout`, `help`.

## Tài khoản mẫu (`Pop3Server/accounts.txt`)
| Username | Password | Hộp thư mẫu |
| :--- | :--- | :--- |
| `bao456` | `happy` | 3 email (`mailboxes/bao456/`) |
| `user1` | `123456` | 2 email (`mailboxes/user1/`) |
| `empty` | `123456` | 0 email (`mailboxes/empty/`) |

## Biên dịch và chạy

### Cách 1: Code::Blocks
Mở lần lượt `Pop3Server/Pop3Server.cbp` và `Pop3Client/Pop3Client.cbp`, nhấn **Build & Run (F9)**.

### Cách 2: Dòng lệnh `g++` (MinGW / MSYS2 trên Windows)
```bash
# Biên dịch Pop3Server
cd Pop3Server
g++ -std=c++11 -Wall -fexceptions main.cpp cli.cpp socket.cpp tcpsocket.cpp tcpserversocket.cpp tcpserver.cpp session.cpp serverconfig.cpp pop3config.cpp pop3session.cpp pop3server.cpp pop3servercli.cpp -o Pop3Server.exe -lws2_32

# Biên dịch Pop3Client
cd ../Pop3Client
g++ -std=c++11 -Wall -fexceptions main.cpp cli.cpp socket.cpp tcpsocket.cpp tcpclient.cpp pop3client.cpp pop3clientcli.cpp -o Pop3Client.exe -lws2_32
```

### Ví dụ sử dụng
```text
# Terminal 1 (Pop3Server)
Pop3Server> start

# Terminal 2 (Pop3Client)
Pop3Client> login 127.0.0.1 bao456 happy
Pop3Client> stat
Pop3Client> backup backup.txt
Pop3Client> quit
```

## Biểu đồ Lớp mức thực thi (Draw.io)
- **Mở trực tiếp trên Draw.io (3 trang chỉnh sửa được: `Pop3Server`, `Pop3Client`, `Overview Client-Server`)**:
  - Tải file `docs/Pop3_v5_ClassDiagram.drawio`, vào https://app.diagrams.net/ → **File → Open from → Device...** (hoặc kéo thả file vào trình duyệt).
- **Import bằng mã Mermaid trong Draw.io**:
  - Mở `docs/Pop3_v5_ClassDiagram_Mermaid.mmd`, copy nội dung và dán vào **Arrange → Insert → Advanced → Mermaid...**.

