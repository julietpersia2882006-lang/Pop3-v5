#include "pop3clientcli.h"
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

Pop3ClientCLI::Pop3ClientCLI() : CmdLineInterface("Pop3Client> ")
{
    cout << "==========================================" << endl;
    cout << "  POP3 Client v5 (Thong ke & Sao luu thu) " << endl;
    cout << "==========================================" << endl;
    cout << "Nhap 'help' de xem danh sach lenh ho tro." << endl;
    initCmd();
}

Pop3ClientCLI::~Pop3ClientCLI()
{
    if (client.isConnected())
    {
        client.quitSession();
    }
}

void Pop3ClientCLI::initCmd()
{
    addCmd("login",  CLI_CAST(&Pop3ClientCLI::doLogin));
    addCmd("stat",   CLI_CAST(&Pop3ClientCLI::doStat));
    addCmd("backup", CLI_CAST(&Pop3ClientCLI::doBackup));
    addCmd("list",   CLI_CAST(&Pop3ClientCLI::doList));
    addCmd("logout", CLI_CAST(&Pop3ClientCLI::doLogout));
    addCmd("help",   CLI_CAST(&Pop3ClientCLI::doHelp));
}

void Pop3ClientCLI::doLogin(string cmd_argv[], int cmd_argc)
{
    if (cmd_argc != 4 && cmd_argc != 5)
    {
        cout << "Cu phap: login <IP server> <username> <password> [port]" << endl;
        return;
    }

    string serverIP = cmd_argv[1];
    string username = cmd_argv[2];
    string password = cmd_argv[3];
    unsigned short port = 110;

    if (cmd_argc == 5)
    {
        int p = atoi(cmd_argv[4].c_str());
        if (p <= 0 || p > 65535)
        {
            cout << "Cong khong hop le." << endl;
            return;
        }
        port = (unsigned short)p;
    }

    if (client.login(serverIP, username, password, port))
    {
        cout << "Dang nhap thanh cong voi tai khoan '" << username
             << "' tai " << serverIP << ":" << port << "." << endl;
    }
    else
    {
        cout << "Dang nhap that bai." << endl;
    }
}

void Pop3ClientCLI::doStat(string cmd_argv[], int cmd_argc)
{
    (void)cmd_argv;
    (void)cmd_argc;

    int mailCount = 0;
    long long totalBytes = 0;
    if (client.stat(mailCount, totalBytes))
    {
        cout << "=== THONG KE HOP THU ===" << endl;
        cout << "Tai khoan      : " << client.getCurrentUser() << endl;
        cout << "Tong so email  : " << mailCount << endl;
        cout << "Tong dung luong: " << totalBytes << " byte" << endl;
    }
}

void Pop3ClientCLI::doList(string cmd_argv[], int cmd_argc)
{
    (void)cmd_argv;
    (void)cmd_argc;

    vector<pair<int, long long> > mails;
    if (client.listMails(mails))
    {
        cout << "=== DANH SACH EMAIL (" << mails.size() << ") ===" << endl;
        if (mails.empty())
        {
            cout << "(Hop thu rong)" << endl;
            return;
        }
        for (size_t i = 0; i < mails.size(); i++)
        {
            cout << "  Email #" << mails[i].first << " - " << mails[i].second << " byte" << endl;
        }
    }
}

void Pop3ClientCLI::doBackup(string cmd_argv[], int cmd_argc)
{
    if (cmd_argc < 2)
    {
        cout << "Cu phap: backup <ten file luu>" << endl;
        return;
    }

    string outputFile = cmd_argv[1];
    for (int i = 2; i < cmd_argc; i++)
    {
        outputFile += " " + cmd_argv[i];
    }

    client.backup(outputFile);
}

void Pop3ClientCLI::doLogout(string cmd_argv[], int cmd_argc)
{
    (void)cmd_argv;
    (void)cmd_argc;

    if (!client.isConnected())
    {
        cout << "Hien khong co ket noi nao." << endl;
        return;
    }
    client.quitSession();
    cout << "Da dang xuat va dong ket noi." << endl;
}

void Pop3ClientCLI::doQuit()
{
    if (client.isConnected())
    {
        client.quitSession();
        cout << "Da ket thuc phien POP3 va dong ket noi." << endl;
    }
    CmdLineInterface::doQuit();
}

void Pop3ClientCLI::doHelp(string cmd_argv[], int cmd_argc)
{
    (void)cmd_argv;
    (void)cmd_argc;
    cout << "Cac lenh POP3 Client:" << endl;
    cout << "  login <IP server> <username> <password> [port]  Ket noi va dang nhap (mac dinh cong 110)" << endl;
    cout << "  stat                                            Xem tong so email va tong dung luong (byte)" << endl;
    cout << "  backup <ten file luu>                           Sao luu toan bo email vao 1 file (khong xoa mail)" << endl;
    cout << "  list                                            Xem danh sach ID va kich thuoc tung email" << endl;
    cout << "  logout                                          Ket thuc phien POP3 (khong thoat chuong trinh)" << endl;
    cout << "  quit                                            Ket thuc phien, dong ket noi va thoat" << endl;
    cout << "  help                                            Hien thi tro giup" << endl;
}
