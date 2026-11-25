#ifndef POP3_SERVER_CLI_H_
#define POP3_SERVER_CLI_H_

#include "cli.h"
#include "pop3server.h"

class Pop3ServerCLI : public CmdLineInterface
{
private:
    Pop3Server* pop3Server;

public:
    Pop3ServerCLI();
    virtual ~Pop3ServerCLI();

protected:
    virtual void initCmd();
    virtual void doQuit();

private:
    void doStart(string cmd_argv[], int cmd_argc);
    void doStop(string cmd_argv[], int cmd_argc);
    void doStatus(string cmd_argv[], int cmd_argc);
    void doHelp(string cmd_argv[], int cmd_argc);
};

#endif // POP3_SERVER_CLI_H_
