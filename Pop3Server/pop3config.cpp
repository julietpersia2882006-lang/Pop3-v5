#include "pop3config.h"
#include <fstream>
#include <iostream>

Pop3Account::Pop3Account() : Account()
{
}

Pop3Account::Pop3Account(const string& username, const string& password, const string& mailboxDir)
    : Account(username)
{
    setPassword(password);
    this->mailboxDir = mailboxDir;
}

Pop3Account::~Pop3Account()
{
}

Pop3ServerConfig::Pop3ServerConfig() : ServerConfig()
{
    this->timeout = 300; // 5 phut timeout moi phien
    this->baseDir = "";
}

Pop3ServerConfig::~Pop3ServerConfig()
{
}

string Pop3ServerConfig::trim(const string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool Pop3ServerConfig::loadAccountsFromFile(const string& filename)
{
    removeAllAccount();

    string candidates[3] = {
        filename,
        "../../" + filename,
        "../" + filename
    };

    ifstream ifs;
    string resolvedPath;
    for (int i = 0; i < 3; i++)
    {
        ifs.open(candidates[i].c_str());
        if (ifs.is_open())
        {
            resolvedPath = candidates[i];
            break;
        }
        ifs.clear();
    }

    if (!ifs.is_open())
    {
        cerr << "[POP3 Config] Khong tim thay file tai khoan: " << filename << endl;
        return false;
    }

    size_t slashPos = resolvedPath.find_last_of("/\\");
    if (slashPos != string::npos)
        baseDir = resolvedPath.substr(0, slashPos + 1);
    else
        baseDir = "";

    string line;
    while (getline(ifs, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        // Dinh dang: username=password hoac username=password=mailboxDir
        size_t p1 = line.find('=');
        if (p1 == string::npos)
            continue;

        string user = trim(line.substr(0, p1));
        string rest = line.substr(p1 + 1);

        string pass;
        string mailDir;
        size_t p2 = rest.find('=');
        if (p2 != string::npos)
        {
            pass = trim(rest.substr(0, p2));
            string relDir = trim(rest.substr(p2 + 1));
            if (!relDir.empty() && (relDir[0] == '/' || relDir[0] == '\\' ||
                (relDir.size() >= 2 && relDir[1] == ':')))
            {
                mailDir = relDir;
            }
            else
            {
                mailDir = baseDir + relDir;
            }
        }
        else
        {
            pass = trim(rest);
            mailDir = baseDir + "mailboxes/" + user;
        }

        if (!user.empty())
        {
            Pop3Account* acc = new Pop3Account(user, pass, mailDir);
            addAccount(acc);
        }
    }

    ifs.close();
    return true;
}

bool Pop3ServerConfig::lockMailbox(const string& username)
{
    lock_guard<mutex> guard(lockMtx);
    if (lockedUsers.find(username) != lockedUsers.end())
        return false;
    lockedUsers.insert(username);
    return true;
}

void Pop3ServerConfig::unlockMailbox(const string& username)
{
    lock_guard<mutex> guard(lockMtx);
    lockedUsers.erase(username);
}
