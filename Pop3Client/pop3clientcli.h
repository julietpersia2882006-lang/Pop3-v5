#ifndef POP3_CLIENT_CLI_H_
#define POP3_CLIENT_CLI_H_

#include "cli.h"
#include "pop3client.h"

class Pop3ClientCLI : public CmdLineInterface
{
private:
    Pop3Client client;

public:
    Pop3ClientCLI();
    virtual ~Pop3ClientCLI();

protected:
    virtual void initCmd();
    virtual void doQuit();

private:
    void doLogin(string cmd_argv[], int cmd_argc);
    void doStat(string cmd_argv[], int cmd_argc);
    void doList(string cmd_argv[], int cmd_argc);
    void doBackup(string cmd_argv[], int cmd_argc);
    void doLogout(string cmd_argv[], int cmd_argc);
    void doHelp(string cmd_argv[], int cmd_argc);
};

#endif // POP3_CLIENT_CLI_H_
