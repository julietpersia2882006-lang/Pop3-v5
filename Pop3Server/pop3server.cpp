#include "pop3server.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

Pop3Server::Pop3Server(unsigned short localPort) : TCPServer(localPort)
{
    this->pop3Conf = new Pop3ServerConfig();
    this->conf = this->pop3Conf;
    initCmd();
    configServer();
}

Pop3Server::~Pop3Server()
{
    if (isRunning())
        stop();
    delete pop3Conf;
    pop3Conf = NULL;
    conf = NULL;
}

bool Pop3Server::configServer()
{
    if (pop3Conf == NULL)
        return false;
    return pop3Conf->loadAccountsFromFile("accounts.txt");
}

void Pop3Server::setPort(unsigned short newPort)
{
    if (!running)
    {
        this->port = newPort;
    }
}

size_t Pop3Server::getAccountCount() const
{
    return (pop3Conf != NULL) ? pop3Conf->getAccountCount() : 0;
}

void Pop3Server::initCmd()
{
    addCmd("USER", FUNC_CAST(&Pop3Session::doUser));
    addCmd("PASS", FUNC_CAST(&Pop3Session::doPass));
    addCmd("STAT", FUNC_CAST(&Pop3Session::doStat));
    addCmd("LIST", FUNC_CAST(&Pop3Session::doList));
    addCmd("RETR", FUNC_CAST(&Pop3Session::doRetr));
    addCmd("DELE", FUNC_CAST(&Pop3Session::doDele));
    addCmd("RSET", FUNC_CAST(&Pop3Session::doRset));
    addCmd("NOOP", FUNC_CAST(&Pop3Session::doNoop));
    addCmd("TOP",  FUNC_CAST(&Pop3Session::doTop));
    addCmd("UIDL", FUNC_CAST(&Pop3Session::doUidl));
    addCmd("QUIT", FUNC_CAST(&Pop3Session::doQuit));
}

void Pop3Server::logMessage(const string& msg)
{
    lock_guard<mutex> guard(logMtx);
    cout << "\n[POP3 Server] " << msg << endl;
}

int Pop3Server::readCmd(TcpSocket& slave, string& cmdLine)
{
    char buf[SERVER_CMD_BUF_LEN];
    try
    {
        int bytes = slave.recvLine(buf, sizeof(buf));
        if (bytes > 0)
        {
            int len = bytes;
            if (len >= 1 && buf[len - 1] == '\n') len--;
            if (len >= 1 && buf[len - 1] == '\r') len--;
            buf[len] = '\0';
            cmdLine = buf;
        }
        else
        {
            cmdLine.clear();
        }
        return bytes;
    }
    catch (SocketException&)
    {
        cmdLine.clear();
        return -1;
    }
}

uint8_t Pop3Server::parseCmd(const string& cmdLine, string cmdArgv[], int& cmdArgc)
{
    cmdArgc = 0;
    if (cmdLine.empty())
        return SERVER_CMD_UNKNOWN;

    istringstream iss(cmdLine);
    while (cmdArgc < SERVER_CMD_ARG_NUM && (iss >> cmdArgv[cmdArgc]))
    {
        cmdArgc++;
    }

    if (cmdArgc == 0)
        return SERVER_CMD_UNKNOWN;

    // Lenh POP3 khong phan biet chu hoa / chu thuong
    string cmdUpper = cmdArgv[0];
    for (size_t i = 0; i < cmdUpper.size(); i++)
    {
        cmdUpper[i] = (char)toupper((unsigned char)cmdUpper[i]);
    }
    cmdArgv[0] = cmdUpper;

    for (unsigned short i = 0; i < this->numCmd; i++)
    {
        if (cmdUpper == this->cmdNameList[i])
            return (uint8_t)i;
    }
    return SERVER_CMD_UNKNOWN;
}

void Pop3Server::startNewSession(TcpSocket slave)
{
    string clientInfo = "unknown";
    try
    {
        ostringstream oss;
        oss << slave.getRemoteAddress() << ":" << slave.getRemotePort();
        clientInfo = oss.str();
    }
    catch (SocketException&)
    {
    }

    logMessage("Client ket noi tu " + clientInfo);

    Pop3Session session(slave, pop3Conf);

    if (session.sendGreeting())
    {
        string cmdLine;
        string cmdArgv[SERVER_CMD_ARG_NUM];
        int cmdArgc = 0;

        while (running && !session.isQuit())
        {
            int bytes = readCmd(slave, cmdLine);
            if (bytes <= 0)
            {
                break;
            }
            if (cmdLine.empty())
                continue;

            uint8_t cmdId = parseCmd(cmdLine, cmdArgv, cmdArgc);
            doCmd(&session, cmdId, cmdArgv, cmdArgc);
        }
    }

    slave.close();
    logMessage("Dong ket noi voi " + clientInfo);
}
