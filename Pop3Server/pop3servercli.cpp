#include "pop3servercli.h"
#include <cstdlib>
#include <iostream>

Pop3ServerCLI::Pop3ServerCLI() : CmdLineInterface("Pop3Server> ")
{
    cout << "==========================================" << endl;
    cout << "  POP3 Server v5 (Thong ke & Sao luu thu) " << endl;
    cout << "==========================================" << endl;
    pop3Server = new Pop3Server(110);
    cout << "Da nap " << pop3Server->getAccountCount() << " tai khoan tu accounts.txt" << endl;
    cout << "Nhap 'start' de khoi dong server, 'help' de xem huong dan." << endl;
    initCmd();
}

Pop3ServerCLI::~Pop3ServerCLI()
{
    if (pop3Server != NULL)
    {
        if (pop3Server->isRunning())
            pop3Server->stop();
        delete pop3Server;
        pop3Server = NULL;
    }
}

void Pop3ServerCLI::initCmd()
{
    addCmd("start",  CLI_CAST(&Pop3ServerCLI::doStart));
    addCmd("stop",   CLI_CAST(&Pop3ServerCLI::doStop));
    addCmd("status", CLI_CAST(&Pop3ServerCLI::doStatus));
    addCmd("help",   CLI_CAST(&Pop3ServerCLI::doHelp));
}

void Pop3ServerCLI::doStart(string cmd_argv[], int cmd_argc)
{
    if (pop3Server->isRunning())
    {
        cout << "POP3 Server dang chay tai cong " << pop3Server->getPort() << "." << endl;
        return;
    }

    if (cmd_argc >= 2)
    {
        int p = atoi(cmd_argv[1].c_str());
        if (p <= 0 || p > 65535)
        {
            cout << "Cong khong hop le. Cu phap: start [port]" << endl;
            return;
        }
        pop3Server->setPort((unsigned short)p);
    }

    // Tai lai danh sach tai khoan moi nhat truoc khi khoi dong
    pop3Server->configServer();

    if (pop3Server->start())
    {
        cout << "POP3 Server da khoi dong va lang nghe tai cong " << pop3Server->getPort() << "." << endl;
    }
    else
    {
        cout << "Khoi dong POP3 Server that bai (kiem tra cong " << pop3Server->getPort() << ")." << endl;
    }
}

void Pop3ServerCLI::doStop(string cmd_argv[], int cmd_argc)
{
    (void)cmd_argv;
    (void)cmd_argc;
    if (!pop3Server->isRunning())
    {
        cout << "POP3 Server hien chua chay." << endl;
        return;
    }
    pop3Server->stop();
    cout << "POP3 Server da tat." << endl;
}

void Pop3ServerCLI::doStatus(string cmd_argv[], int cmd_argc)
{
    (void)cmd_argv;
    (void)cmd_argc;
    if (pop3Server->isRunning())
    {
        cout << "Trang thai: DANG CHAY (cong " << pop3Server->getPort()
             << ", " << pop3Server->getAccountCount() << " tai khoan)." << endl;
    }
    else
    {
        cout << "Trang thai: DA TAT (cong mac dinh " << pop3Server->getPort()
             << ", " << pop3Server->getAccountCount() << " tai khoan)." << endl;
    }
}

void Pop3ServerCLI::doHelp(string cmd_argv[], int cmd_argc)
{
    (void)cmd_argv;
    (void)cmd_argc;
    cout << "Cac lenh POP3 Server:" << endl;
    cout << "  start [port]   Khoi dong server va lang nghe ket noi (mac dinh cong 110)" << endl;
    cout << "  stop           Tat server" << endl;
    cout << "  status         Xem trang thai server" << endl;
    cout << "  help           Hien thi tro giup" << endl;
    cout << "  quit           Tat server va thoat chuong trinh" << endl;
}

void Pop3ServerCLI::doQuit()
{
    if (pop3Server != NULL && pop3Server->isRunning())
    {
        pop3Server->stop();
        cout << "POP3 Server da tat." << endl;
    }
    CmdLineInterface::doQuit();
}
