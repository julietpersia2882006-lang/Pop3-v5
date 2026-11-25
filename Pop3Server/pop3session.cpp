#include "pop3session.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

Pop3Session::Pop3Session(const TcpSocket& slave, Pop3ServerConfig* conf)
    : Session(slave, conf)
{
    this->pop3Conf = conf;
    this->state = POP3_AUTH;
    this->mailboxLocked = false;
}

Pop3Session::~Pop3Session()
{
    if (mailboxLocked && pop3Conf != NULL)
    {
        pop3Conf->unlockMailbox(loggedUser);
        mailboxLocked = false;
    }
}

bool Pop3Session::sendAll(const string& data)
{
    const char* ptr = data.c_str();
    size_t remaining = data.size();
    try
    {
        while (remaining > 0)
        {
            int sent = slave.send(ptr, (int)remaining);
            if (sent <= 0)
                return false;
            ptr += sent;
            remaining -= (size_t)sent;
        }
        return true;
    }
    catch (SocketException& e)
    {
        cerr << "[POP3 Session] Loi gui du lieu: " << e.what() << endl;
        return false;
    }
}

bool Pop3Session::sendResponse(const string& line)
{
    return sendAll(line + "\r\n");
}

bool Pop3Session::sendGreeting()
{
    return sendResponse("+OK POP3 Server ready");
}

void Pop3Session::reset()
{
    for (size_t i = 0; i < mails.size(); i++)
    {
        mails[i].deleted = false;
    }
}

size_t Pop3Session::calcOctetSize(const string& filePath)
{
    ifstream ifs(filePath.c_str(), ios::binary);
    if (!ifs.is_open())
        return 0;

    size_t totalOctets = 0;
    string line;
    while (getline(ifs, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        totalOctets += line.size() + 2; // chuan CRLF cua POP3
    }
    ifs.close();
    return totalOctets;
}

bool Pop3Session::loadMailbox(const string& mailboxDir)
{
    mails.clear();
    vector<string> fileNames;

#ifdef _WIN32
    string pattern = mailboxDir;
    if (!pattern.empty() && pattern[pattern.size() - 1] != '\\' && pattern[pattern.size() - 1] != '/')
        pattern += "\\";
    pattern += "*";

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        // Thu muc khong ton tai hoac khong co file
        return true;
    }

    do
    {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            string name = findData.cFileName;
            if (!name.empty() && name[0] != '.')
            {
                fileNames.push_back(name);
            }
        }
    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);
#else
    DIR* dir = opendir(mailboxDir.c_str());
    if (dir == NULL)
        return true;

    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL)
    {
        string name = ent->d_name;
        if (name.empty() || name[0] == '.')
            continue;
        string full = mailboxDir + "/" + name;
        struct stat st;
        if (stat(full.c_str(), &st) == 0 && S_ISREG(st.st_mode))
        {
            fileNames.push_back(name);
        }
    }
    closedir(dir);
#endif

    sort(fileNames.begin(), fileNames.end());

    string dirPrefix = mailboxDir;
    if (!dirPrefix.empty() && dirPrefix[dirPrefix.size() - 1] != '\\' && dirPrefix[dirPrefix.size() - 1] != '/')
        dirPrefix += "/";

    for (size_t i = 0; i < fileNames.size(); i++)
    {
        MailEntry entry;
        entry.id = (int)(i + 1);
        entry.fileName = fileNames[i];
        entry.filePath = dirPrefix + fileNames[i];
        entry.size = calcOctetSize(entry.filePath);
        entry.deleted = false;
        mails.push_back(entry);
    }

    return true;
}

void Pop3Session::getStats(int& count, size_t& totalBytes) const
{
    count = 0;
    totalBytes = 0;
    for (size_t i = 0; i < mails.size(); i++)
    {
        if (!mails[i].deleted)
        {
            count++;
            totalBytes += mails[i].size;
        }
    }
}

void Pop3Session::doUnknown(string cmd_argv[], int cmd_argc)
{
    (void)cmd_argv;
    (void)cmd_argc;
    sendResponse("-ERR Unknown command");
}

void Pop3Session::doUser(string cmd_argv[], int cmd_argc)
{
    if (state != POP3_AUTH)
    {
        sendResponse("-ERR Command not valid in current state");
        return;
    }
    if (cmd_argc < 2)
    {
        sendResponse("-ERR Missing username");
        return;
    }

    string username = cmd_argv[1];
    if (pop3Conf == NULL || !pop3Conf->isValidUser(username))
    {
        pendingUser.clear();
        sendResponse("-ERR Invalid username");
        return;
    }

    pendingUser = username;
    sendResponse("+OK User accepted, password required");
}

void Pop3Session::doPass(string cmd_argv[], int cmd_argc)
{
    if (state != POP3_AUTH)
    {
        sendResponse("-ERR Command not valid in current state");
        return;
    }
    if (pendingUser.empty())
    {
        sendResponse("-ERR Send USER command first");
        return;
    }
    if (cmd_argc < 2)
    {
        sendResponse("-ERR Missing password");
        return;
    }

    string password = cmd_argv[1];
    if (pop3Conf == NULL || !pop3Conf->authenticate(pendingUser, password))
    {
        pendingUser.clear();
        sendResponse("-ERR Authentication failed");
        return;
    }

    if (!pop3Conf->lockMailbox(pendingUser))
    {
        sendResponse("-ERR Mailbox already locked by another session");
        return;
    }

    loggedUser = pendingUser;
    mailboxLocked = true;

    Pop3Account* acc = dynamic_cast<Pop3Account*>(pop3Conf->getAccount(loggedUser));
    if (acc != NULL)
    {
        loadMailbox(acc->getMailboxDir());
    }

    state = POP3_TRANS;

    int count = 0;
    size_t totalBytes = 0;
    getStats(count, totalBytes);

    ostringstream oss;
    oss << "+OK Logged in, " << count << " messages (" << totalBytes << " octets)";
    sendResponse(oss.str());
}

void Pop3Session::doStat(string cmd_argv[], int cmd_argc)
{
    (void)cmd_argv;
    (void)cmd_argc;
    if (state != POP3_TRANS)
    {
        sendResponse("-ERR Not authenticated");
        return;
    }

    int count = 0;
    size_t totalBytes = 0;
    getStats(count, totalBytes);

    ostringstream oss;
    oss << "+OK " << count << " " << totalBytes;
    sendResponse(oss.str());
}

void Pop3Session::doList(string cmd_argv[], int cmd_argc)
{
    if (state != POP3_TRANS)
    {
        sendResponse("-ERR Not authenticated");
        return;
    }

    if (cmd_argc >= 2)
    {
        int id = atoi(cmd_argv[1].c_str());
        if (id < 1 || id > (int)mails.size() || mails[id - 1].deleted)
        {
            sendResponse("-ERR No such message");
            return;
        }
        ostringstream oss;
        oss << "+OK " << mails[id - 1].id << " " << mails[id - 1].size;
        sendResponse(oss.str());
        return;
    }

    int count = 0;
    size_t totalBytes = 0;
    getStats(count, totalBytes);

    ostringstream oss;
    oss << "+OK " << count << " messages (" << totalBytes << " octets)\r\n";
    for (size_t i = 0; i < mails.size(); i++)
    {
        if (!mails[i].deleted)
        {
            oss << mails[i].id << " " << mails[i].size << "\r\n";
        }
    }
    oss << ".\r\n";
    sendAll(oss.str());
}

void Pop3Session::doRetr(string cmd_argv[], int cmd_argc)
{
    if (state != POP3_TRANS)
    {
        sendResponse("-ERR Not authenticated");
        return;
    }
    if (cmd_argc < 2)
    {
        sendResponse("-ERR Missing message number");
        return;
    }

    int id = atoi(cmd_argv[1].c_str());
    if (id < 1 || id > (int)mails.size() || mails[id - 1].deleted)
    {
        sendResponse("-ERR No such message");
        return;
    }

    const MailEntry& entry = mails[id - 1];
    ifstream ifs(entry.filePath.c_str(), ios::binary);
    if (!ifs.is_open())
    {
        sendResponse("-ERR Cannot open message file");
        return;
    }

    ostringstream header;
    header << "+OK " << entry.size << " octets\r\n";
    if (!sendAll(header.str()))
    {
        ifs.close();
        return;
    }

    // Gui noi dung tung dong va ap dung dot-stuffing chuan RFC 1939
    string line;
    string chunk;
    chunk.reserve(4096);

    while (getline(ifs, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if (!line.empty() && line[0] == '.')
            chunk.push_back('.');

        chunk.append(line);
        chunk.append("\r\n");

        if (chunk.size() >= 4096)
        {
            if (!sendAll(chunk))
            {
                ifs.close();
                return;
            }
            chunk.clear();
        }
    }
    ifs.close();

    chunk.append(".\r\n");
    sendAll(chunk);
}

void Pop3Session::doDele(string cmd_argv[], int cmd_argc)
{
    if (state != POP3_TRANS)
    {
        sendResponse("-ERR Not authenticated");
        return;
    }
    if (cmd_argc < 2)
    {
        sendResponse("-ERR Missing message number");
        return;
    }

    int id = atoi(cmd_argv[1].c_str());
    if (id < 1 || id > (int)mails.size() || mails[id - 1].deleted)
    {
        sendResponse("-ERR No such message");
        return;
    }

    mails[id - 1].deleted = true;
    ostringstream oss;
    oss << "+OK Message " << id << " marked for deletion";
    sendResponse(oss.str());
}

void Pop3Session::doRset(string cmd_argv[], int cmd_argc)
{
    (void)cmd_argv;
    (void)cmd_argc;
    if (state != POP3_TRANS)
    {
        sendResponse("-ERR Not authenticated");
        return;
    }

    reset();
    int count = 0;
    size_t totalBytes = 0;
    getStats(count, totalBytes);

    ostringstream oss;
    oss << "+OK Maildrop has " << count << " messages (" << totalBytes << " octets)";
    sendResponse(oss.str());
}

void Pop3Session::doNoop(string cmd_argv[], int cmd_argc)
{
    (void)cmd_argv;
    (void)cmd_argc;
    if (state != POP3_TRANS)
    {
        sendResponse("-ERR Not authenticated");
        return;
    }
    sendResponse("+OK");
}

void Pop3Session::doTop(string cmd_argv[], int cmd_argc)
{
    if (state != POP3_TRANS)
    {
        sendResponse("-ERR Not authenticated");
        return;
    }
    if (cmd_argc < 3)
    {
        sendResponse("-ERR Usage: TOP <msg> <n>");
        return;
    }

    int id = atoi(cmd_argv[1].c_str());
    int nLines = atoi(cmd_argv[2].c_str());
    if (id < 1 || id > (int)mails.size() || mails[id - 1].deleted || nLines < 0)
    {
        sendResponse("-ERR Invalid message number or line count");
        return;
    }

    ifstream ifs(mails[id - 1].filePath.c_str(), ios::binary);
    if (!ifs.is_open())
    {
        sendResponse("-ERR Cannot open message file");
        return;
    }

    ostringstream oss;
    oss << "+OK Top of message follows\r\n";

    string line;
    bool inBody = false;
    int bodyCount = 0;
    while (getline(ifs, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if (!inBody)
        {
            if (line.empty())
            {
                inBody = true;
                oss << "\r\n";
                if (nLines == 0)
                    break;
            }
            else
            {
                if (line[0] == '.')
                    oss << '.';
                oss << line << "\r\n";
            }
        }
        else
        {
            if (bodyCount >= nLines)
                break;
            if (!line.empty() && line[0] == '.')
                oss << '.';
            oss << line << "\r\n";
            bodyCount++;
        }
    }
    ifs.close();

    oss << ".\r\n";
    sendAll(oss.str());
}

void Pop3Session::doUidl(string cmd_argv[], int cmd_argc)
{
    if (state != POP3_TRANS)
    {
        sendResponse("-ERR Not authenticated");
        return;
    }

    if (cmd_argc >= 2)
    {
        int id = atoi(cmd_argv[1].c_str());
        if (id < 1 || id > (int)mails.size() || mails[id - 1].deleted)
        {
            sendResponse("-ERR No such message");
            return;
        }
        ostringstream oss;
        oss << "+OK " << mails[id - 1].id << " " << mails[id - 1].fileName;
        sendResponse(oss.str());
        return;
    }

    ostringstream oss;
    oss << "+OK Unique-ID listing follows\r\n";
    for (size_t i = 0; i < mails.size(); i++)
    {
        if (!mails[i].deleted)
        {
            oss << mails[i].id << " " << mails[i].fileName << "\r\n";
        }
    }
    oss << ".\r\n";
    sendAll(oss.str());
}

void Pop3Session::doQuit(string cmd_argv[], int cmd_argc)
{
    (void)cmd_argv;
    (void)cmd_argc;

    if (state == POP3_TRANS)
    {
        state = POP3_UPDATE;
        // Chi xoa file thuc su neu truoc do da bi danh dau DELE
        for (size_t i = 0; i < mails.size(); i++)
        {
            if (mails[i].deleted)
            {
                remove(mails[i].filePath.c_str());
            }
        }
        if (mailboxLocked && pop3Conf != NULL)
        {
            pop3Conf->unlockMailbox(loggedUser);
            mailboxLocked = false;
        }
        sendResponse("+OK POP3 server signing off");
    }
    else
    {
        sendResponse("+OK Goodbye");
    }

    quitSession = true;
}
