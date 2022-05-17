#include "WebBrowser.h"
#include <winsock2.h>
#include <QtWidgets/QApplication>
#include "WebBrowser.h"
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <QFile>
#include <QTextStream>

#include<stdlib.h>
#include<time.h>

#include<math.h>

#define HAVE_REMOTE
#define WPCAP
#include<pcap.h>
#include<WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char *argv[])
{
    // WSADATA wsa_data;
    // WSAStartup(MAKEWORD(2, 0), &wsa_data); // init WinSock
    QApplication a(argc, argv);
    WebBrowser w;
    w.show();
    // WSACleanup();
    return a.exec();
}
