#ifndef POP3_SERVER_H_
#define POP3_SERVER_H_

#include <string>
#include <mutex>
#include "tcpserver.h"
#include "pop3config.h"
#include "pop3session.h"

class Pop3Server : public TCPServer
{
private:
    Pop3ServerConfig* pop3Conf;
    mutex logMtx;

    int readCmd(TcpSocket& slave, string& cmdLine);
    uint8_t parseCmd(const string& cmdLine, string cmdArgv[], int& cmdArgc);
    void logMessage(const string& msg);

public:
    Pop3Server(unsigned short localPort = 110);
    virtual ~Pop3Server();

    virtual bool configServer();
    void setPort(unsigned short newPort);
    unsigned short getPort() const { return port; }
    size_t getAccountCount() const;

protected:
    virtual void initCmd();
    virtual void startNewSession(TcpSocket slave);
};

#endif // POP3_SERVER_H_
