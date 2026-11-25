#ifndef POP3_CONFIG_H_
#define POP3_CONFIG_H_

#include <string>
#include <mutex>
#include <unordered_set>
#include "serverconfig.h"

class Pop3Account : public Account
{
private:
    string mailboxDir;

public:
    Pop3Account();
    Pop3Account(const string& username, const string& password, const string& mailboxDir);
    virtual ~Pop3Account();

    const string& getMailboxDir() const { return mailboxDir; }
    void setMailboxDir(const string& dir) { mailboxDir = dir; }
};

class Pop3ServerConfig : public ServerConfig
{
private:
    string baseDir;
    mutex lockMtx;
    unordered_set<string> lockedUsers;

    static string trim(const string& s);

public:
    Pop3ServerConfig();
    virtual ~Pop3ServerConfig();

    virtual bool loadAccountsFromFile(const string& filename);

    bool lockMailbox(const string& username);
    void unlockMailbox(const string& username);

    size_t getAccountCount() const { return accMap.size(); }
    const string& getBaseDir() const { return baseDir; }
};

#endif // POP3_CONFIG_H_
