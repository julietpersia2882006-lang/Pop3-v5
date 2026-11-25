#include "pop3client.h"
#include <fstream>
#include <iostream>
#include <sstream>

Pop3Client::Pop3Client() : TcpClient()
{
    loggedIn = false;
    currentPort = 110;
}

Pop3Client::~Pop3Client()
{
    if (isConnected())
    {
        quitSession();
    }
}

bool Pop3Client::isOkResponse(const string& resp)
{
    return (resp.size() >= 3 && resp.compare(0, 3, "+OK") == 0);
}

bool Pop3Client::sendCommand(const string& cmd)
{
    if (!isConnected())
        return false;

    string full = cmd + "\r\n";
    int sent = sendStringRequest(full);
    return (sent == (int)full.size());
}

bool Pop3Client::readResponseLine(string& line)
{
    char buf[8192];
    int bytes = recvGetLine(buf, sizeof(buf));
    if (bytes <= 0)
    {
        line.clear();
        return false;
    }
    line = buf;
    return true;
}

bool Pop3Client::readMultiLine(vector<string>& lines)
{
    lines.clear();
    string line;
    while (true)
    {
        if (!readResponseLine(line))
            return false;

        // Ket thuc phan hoi da dong POP3: dong chi chua 1 dau cham "."
        if (line == ".")
            break;

        // Dot-unstuffing theo RFC 1939: ".." o dau dong -> "."
        if (line.size() >= 2 && line[0] == '.' && line[1] == '.')
        {
            line.erase(0, 1);
        }
        lines.push_back(line);
    }
    return true;
}

bool Pop3Client::login(const string& serverIP, const string& username, const string& password, unsigned short port)
{
    if (isConnected())
    {
        cout << "Loi: Da co ket noi. Hay dung lenh logout hoac quit truoc." << endl;
        return false;
    }

    if (!open(serverIP, port))
    {
        cout << "Loi: Khong the ket noi den POP3 Server " << serverIP << ":" << port << endl;
        return false;
    }

    string resp;
    if (!readResponseLine(resp) || !isOkResponse(resp))
    {
        cout << "Loi: Khong nhan duoc loi chao hop le tu server." << endl;
        if (!resp.empty())
            cout << "Server: " << resp << endl;
        close();
        return false;
    }
    cout << "Server: " << resp << endl;

    // Gui lenh USER
    if (!sendCommand("USER " + username) || !readResponseLine(resp))
    {
        cout << "Loi: Gui lenh USER that bai." << endl;
        close();
        return false;
    }
    cout << "Server: " << resp << endl;
    if (!isOkResponse(resp))
    {
        close();
        return false;
    }

    // Gui lenh PASS
    if (!sendCommand("PASS " + password) || !readResponseLine(resp))
    {
        cout << "Loi: Gui lenh PASS that bai." << endl;
        close();
        return false;
    }
    cout << "Server: " << resp << endl;
    if (!isOkResponse(resp))
    {
        close();
        return false;
    }

    loggedIn = true;
    currentUser = username;
    currentHost = serverIP;
    currentPort = port;
    return true;
}

bool Pop3Client::stat(int& mailCount, long long& totalBytes)
{
    mailCount = 0;
    totalBytes = 0;

    if (!isLoggedIn())
    {
        cout << "Loi: Ban chua dang nhap vao POP3 Server." << endl;
        return false;
    }

    if (!sendCommand("STAT"))
    {
        cout << "Loi: Khong gui duoc lenh STAT." << endl;
        return false;
    }

    string resp;
    if (!readResponseLine(resp))
    {
        cout << "Loi: Mat ket noi khi nhan phan hoi STAT." << endl;
        close();
        loggedIn = false;
        return false;
    }

    if (!isOkResponse(resp))
    {
        cout << "Server tra loi loi: " << resp << endl;
        return false;
    }

    // Dinh dang phan hoi STAT: +OK <so_luong> <tong_dung_luong>
    istringstream iss(resp);
    string statusTag;
    if (!(iss >> statusTag >> mailCount >> totalBytes))
    {
        cout << "Loi: Dinh dang phan hoi STAT khong hop le: " << resp << endl;
        return false;
    }

    return true;
}

bool Pop3Client::listMails(vector<pair<int, long long> >& mails)
{
    mails.clear();

    if (!isLoggedIn())
    {
        cout << "Loi: Ban chua dang nhap vao POP3 Server." << endl;
        return false;
    }

    if (!sendCommand("LIST"))
    {
        cout << "Loi: Khong gui duoc lenh LIST." << endl;
        return false;
    }

    string resp;
    if (!readResponseLine(resp))
    {
        cout << "Loi: Mat ket noi khi nhan phan hoi LIST." << endl;
        close();
        loggedIn = false;
        return false;
    }

    if (!isOkResponse(resp))
    {
        cout << "Server tra loi loi: " << resp << endl;
        return false;
    }

    vector<string> lines;
    if (!readMultiLine(lines))
    {
        cout << "Loi: Doc danh sach email that bai." << endl;
        close();
        loggedIn = false;
        return false;
    }

    for (size_t i = 0; i < lines.size(); i++)
    {
        istringstream iss(lines[i]);
        int id = 0;
        long long sz = 0;
        if (iss >> id >> sz)
        {
            mails.push_back(make_pair(id, sz));
        }
    }
    return true;
}

bool Pop3Client::retrMail(int mailId, string& content)
{
    content.clear();

    if (!isLoggedIn())
    {
        cout << "Loi: Ban chua dang nhap vao POP3 Server." << endl;
        return false;
    }

    ostringstream cmd;
    cmd << "RETR " << mailId;
    if (!sendCommand(cmd.str()))
    {
        cout << "Loi: Khong gui duoc lenh RETR " << mailId << "." << endl;
        return false;
    }

    string resp;
    if (!readResponseLine(resp))
    {
        cout << "Loi: Mat ket noi khi nhan phan hoi RETR " << mailId << "." << endl;
        close();
        loggedIn = false;
        return false;
    }

    if (!isOkResponse(resp))
    {
        cout << "Server tra loi loi cho RETR " << mailId << ": " << resp << endl;
        return false;
    }

    vector<string> lines;
    if (!readMultiLine(lines))
    {
        cout << "Loi: Doc noi dung email #" << mailId << " that bai." << endl;
        close();
        loggedIn = false;
        return false;
    }

    ostringstream oss;
    for (size_t i = 0; i < lines.size(); i++)
    {
        oss << lines[i] << "\n";
    }
    content = oss.str();
    return true;
}

bool Pop3Client::backup(const string& outputFile)
{
    if (!isLoggedIn())
    {
        cout << "Loi: Ban chua dang nhap vao POP3 Server." << endl;
        return false;
    }

    int mailCount = 0;
    long long totalBytes = 0;
    if (!stat(mailCount, totalBytes))
        return false;

    cout << "Bat dau sao luu hop thu (" << mailCount << " email, "
         << totalBytes << " byte) vao file \"" << outputFile << "\"..." << endl;

    // Mo file o che do append (noi tiep) theo yeu cau de bai
    ofstream ofs(outputFile.c_str(), ios::app | ios::binary);
    if (!ofs.is_open())
    {
        cout << "Loi: Khong mo duoc file \"" << outputFile << "\" de ghi." << endl;
        return false;
    }

    if (mailCount == 0)
    {
        ofs.close();
        cout << "Hop thu rong (0 email). Khong co email de sao luu." << endl;
        return true;
    }

    vector<pair<int, long long> > mails;
    if (!listMails(mails))
    {
        ofs.close();
        return false;
    }

    int savedCount = 0;
    for (size_t i = 0; i < mails.size(); i++)
    {
        int id = mails[i].first;
        long long sz = mails[i].second;
        string content;

        if (retrMail(id, content))
        {
            ofs << "==================== EMAIL #" << id
                << " (" << sz << " bytes) ====================\n";
            ofs << content;
            ofs << "==============================================================\n\n";
            savedCount++;
            cout << "  -> Da tai va ghi email #" << id << " (" << sz << " byte)" << endl;
        }
        else
        {
            cout << "  -> Canh bao: Khong tai duoc email #" << id << endl;
        }
    }

    ofs.flush();
    ofs.close();

    cout << "Sao luu hoan tat: " << savedCount << "/" << mails.size()
         << " email da duoc ghi noi tiep vao \"" << outputFile
         << "\" (khong xoa mail tren server)." << endl;
    return (savedCount == (int)mails.size());
}

bool Pop3Client::quitSession()
{
    if (!isConnected())
    {
        loggedIn = false;
        currentUser.clear();
        return true;
    }

    if (sendCommand("QUIT"))
    {
        string resp;
        if (readResponseLine(resp))
        {
            cout << "Server: " << resp << endl;
        }
    }

    close();
    loggedIn = false;
    currentUser.clear();
    return true;
}
