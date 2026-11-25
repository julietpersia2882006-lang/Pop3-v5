#ifndef POP3_CLIENT_H_
#define POP3_CLIENT_H_

#include <string>
#include <utility>
#include <vector>
#include "tcpclient.h"

class Pop3Client : public TcpClient
{
private:
    bool loggedIn;
    string currentUser;
    string currentHost;
    unsigned short currentPort;

    bool sendCommand(const string& cmd);
    bool readResponseLine(string& line);
    bool readMultiLine(vector<string>& lines);
    static bool isOkResponse(const string& resp);

public:
    Pop3Client();
    virtual ~Pop3Client();

    bool isLoggedIn() const { return connected && loggedIn; }
    const string& getCurrentUser() const { return currentUser; }
    const string& getCurrentHost() const { return currentHost; }
    unsigned short getCurrentPort() const { return currentPort; }

    bool login(const string& serverIP, const string& username, const string& password, unsigned short port = 110);
    bool stat(int& mailCount, long long& totalBytes);
    bool listMails(vector<pair<int, long long> >& mails);
    bool retrMail(int mailId, string& content);
    bool backup(const string& outputFile);
    bool quitSession();
};

#endif // POP3_CLIENT_H_
