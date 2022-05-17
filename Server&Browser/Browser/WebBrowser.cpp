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

#pragma warning(disable:4996)

#define SUCCESS 1
#define FAILURE 0


#define HTTP_DEF_PORT    80
#define HTTP_BUF_SIZE    1025
#define HTTP_HOST_LEN    256
#define HTTP_URL_LEN     512
#define ERROR_LEN        128

const char* http_req_hdr_tmpl = "%s %s HTTP/1.0\r\n"
"Accept: image/gif, image/jpeg, */*\r\nAccept-Language: zh-cn\r\n"
"Accept-Encoding: gzip, deflate\r\nHost: %s:%d\r\n"
"User-Agent: wth's Browser <0.1>\r\nConnection: Keep-Alive\r\n\r\n"; // http header template

char data_show[HTTP_BUF_SIZE];
char request_buf[HTTP_URL_LEN];
char data_buf[HTTP_BUF_SIZE];
char URL[HTTP_URL_LEN];
char error_buf[ERROR_LEN];

void http_parse_request_url(const char* buf, char* host, unsigned short* port, char* file_path)
{
    int length = 0;
    char port_buf[8];
    char* buf_end = (char*)(buf + strlen(buf));
    char* begin, * host_end, * colon, * file;

    //look for where host begin
    begin = const_cast<char*>(strstr(buf, "//"));
    begin = (begin ? begin + 2 : const_cast<char*>(buf));

    colon = strchr(begin, ':');
    host_end = strchr(begin, '/');

    if (host_end == NULL)
    {
        host_end = buf_end;
    }
    else
    {   // get file name
        file = strrchr(host_end, '/');
        if (file && (file + 1) != buf_end)
            strcpy(file_path, file);
    }
    if (colon) // get port
    {
        colon++;

        length = host_end - colon;
        memcpy(port_buf, colon, length);
        port_buf[length] = 0;
        *port = atoi(port_buf);

        host_end = colon - 1;
    }

    // get host info
    length = host_end - begin;
    memcpy(host, begin, length);
    host[length] = 0;
}

int send_request(int method) // click button will call this function
{
    char host[HTTP_HOST_LEN] = "127.0.0.1";
    unsigned short port = HTTP_DEF_PORT;
    char file_path[HTTP_HOST_LEN] = "/";

    WSADATA wsa_data;
    SOCKET  http_sock = 0;
    struct sockaddr_in serv_addr;
    struct hostent* host_ent;

    int result = 0, send_len = 0;

    unsigned long addr;

    http_parse_request_url(URL, host, &port, file_path);

    WSAStartup(MAKEWORD(2, 0), &wsa_data);
    addr = inet_addr(host);
    if (addr == INADDR_NONE)
    {
        host_ent = gethostbyname(host);
        if (!host_ent)
        {
            memcpy(error_buf, "[Web] invalid host\n", 20);
            return -1;
        }

        memcpy(&addr, host_ent->h_addr_list[0], host_ent->h_length);
    }

    //server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = addr;

    http_sock = socket(AF_INET, SOCK_STREAM, 0); // init socket

    result = connect(http_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); // connect server

    if (result == SOCKET_ERROR) // connect failed
    {
        closesocket(http_sock);
        memcpy(error_buf, "[Web] fail to connect\n", 23);
        return -1;
    }

    // send http request
    if (method == 0)
    {
        send_len = sprintf(request_buf, http_req_hdr_tmpl, "GET", file_path, host, port);
    }
    else if(method == 1)
    {
        send_len = sprintf(request_buf, http_req_hdr_tmpl, "HEAD", file_path, host, port);
    }
    else
    {
        send_len = sprintf(request_buf, http_req_hdr_tmpl, "DELETE", file_path, host, port);
    }

    printf("The request is: \n%s\n", request_buf);

    result = send(http_sock, request_buf, send_len, 0);
    if (result == SOCKET_ERROR) // send failed
    {
        memcpy(error_buf, "[Web] fail to send\n", 20);
        return -1;
    }

    if (method == 2) return 1; // DELETE should return here

    FILE* fp = fopen("data.html", "w");
    int total_data_len = 0;

    printf("start to receive data\n");

    // result = recv(http_sock, data_buf, HTTP_BUF_SIZE - 1, 0);

    // char* datastart = const_cast<char*>(strstr(data_buf, "\r\n\r\n"));

    // if (result > 0 && datastart != NULL)
    // {
    //     data_buf[result] = '\0';
    //     printf("%s\n", data_buf);

    //     fwrite(datastart+4, 1, strlen(datastart+4), fp);
    // }
    // else
    // {
    //     memcpy(error_buf, "[Web] fail to receive data\n", 28);
    //     return -1;
    // }

    // while(result > 0) // receive data
    // {
    //     result = recv(http_sock, data_buf, HTTP_BUF_SIZE - 1, 0);
    //     if (result > 0)
    //     {
    //         data_buf[result] = '\0';
    //         printf("%s\n", data_buf);

    //         fwrite(data_buf, 1, result, fp);
    //     }
    // }

    do // receive data
    {
        result = recv(http_sock, data_buf + total_data_len, HTTP_BUF_SIZE - 1, 0);
        total_data_len += result;
    }while(result > 0);

    char* data_start = const_cast<char*>(strstr(data_buf, "\r\n\r\n"));
    fwrite(data_start + 4, 1, total_data_len - (data_start - data_buf + 4), fp);

    char* type_start = const_cast<char*>(strstr(data_buf, "Content-Type: "));



    closesocket(http_sock);
    fclose(fp);

    WSACleanup();

    return 1;
}

WebBrowser::WebBrowser(QWidget *parent)
    : QMainWindow(parent)
{
    
    
    ui.setupUi(this);

    connect(ui.pushButton, SIGNAL(clicked(bool)), this, SLOT(on_button1_clicked()));
    connect(ui.pushButton_2, SIGNAL(clicked(bool)), this, SLOT(on_button2_clicked()));
    connect(ui.pushButton_3, SIGNAL(clicked(bool)), this, SLOT(on_button3_clicked()));

}

void WebBrowser::on_button1_clicked() // get method
{
    QString str = ui.lineEdit->text();
    memcpy(URL, str.toStdString().c_str(), str.length());
    URL[str.length()] = 0;
    // printf("The url is:\n%s\n", URL);
    // ui.textBrowser->setText(URL);
    
    if (send_request(0) == 1)
    {
        QFile file("data.html");
        
        if (!file.open(QFile::ReadOnly | QFile::Text)) return;
                
        QTextStream filein(&file);

        ui.textBrowser->setHtml(filein.readAll());
    }
    else
    {
        ui.textBrowser->setText(error_buf);
    }
}

void WebBrowser::on_button2_clicked() // head method
{
    QString str = ui.lineEdit->text();
    memcpy(URL, str.toStdString().c_str(), str.length());
    URL[str.length()] = 0;
    // printf("The url is:\n%s\n", URL);
    if (send_request(1) == 1)
    {
        ui.textBrowser->setText("HEAD SUCCESS");
    }
    else
    {
        ui.textBrowser->setText(error_buf);
    }
}

void WebBrowser::on_button3_clicked() // delete method
{
    QString str = ui.lineEdit->text();
    memcpy(URL, str.toStdString().c_str(), str.length());
    URL[str.length()] = 0;
    // printf("The url is:\n%s\n", URL);
    if (send_request(2) == 1)
    {
        ui.textBrowser->setText("DELETE SUCCESS");
    }
    else
    {
        ui.textBrowser->setText(error_buf);
    }
}