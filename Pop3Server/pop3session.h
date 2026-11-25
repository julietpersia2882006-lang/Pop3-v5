#ifndef POP3_SESSION_H_
#define POP3_SESSION_H_

#include <string>
#include <vector>
#include "session.h"
#include "pop3config.h"

struct MailEntry
{
    int id;
    string fileName;
    string filePath;
    size_t size;
    bool deleted;
};

enum Pop3State
{
    POP3_AUTH,
    POP3_TRANS,
    POP3_UPDATE
};

class Pop3Session : public Session
{
private:
    Pop3ServerConfig* pop3Conf;
    Pop3State state;
    string pendingUser;
    string loggedUser;
    bool mailboxLocked;
    vector<MailEntry> mails;

    bool sendAll(const string& data);
    bool sendResponse(const string& line);
    size_t calcOctetSize(const string& filePath);
    bool loadMailbox(const string& mailboxDir);
    void getStats(int& count, size_t& totalBytes) const;

public:
    Pop3Session(const TcpSocket& slave, Pop3ServerConfig* conf);
    virtual ~Pop3Session();

    bool sendGreeting();
    virtual void reset();
    virtual void doUnknown(string cmd_argv[], int cmd_argc);

    // Cac lenh giao thuc POP3 (RFC 1939)
    void doUser(string cmd_argv[], int cmd_argc);
    void doPass(string cmd_argv[], int cmd_argc);
    void doStat(string cmd_argv[], int cmd_argc);
    void doList(string cmd_argv[], int cmd_argc);
    void doRetr(string cmd_argv[], int cmd_argc);
    void doDele(string cmd_argv[], int cmd_argc);
    void doRset(string cmd_argv[], int cmd_argc);
    void doNoop(string cmd_argv[], int cmd_argc);
    void doTop(string cmd_argv[], int cmd_argc);
    void doUidl(string cmd_argv[], int cmd_argc);
    void doQuit(string cmd_argv[], int cmd_argc);
};

#endif // POP3_SESSION_H_
