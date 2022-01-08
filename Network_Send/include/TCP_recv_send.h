/*
 * @Author: npuwth
 * @Date: 2022-01-04 16:54:45
 * @LastEditTime: 2022-01-08 12:51:05
 * @LastEditors: npuwth
 * @Copyright 2022
 * @Description: Network Experiment
 */

#include "Header_Include.h"
#include "UDP_recv_send.h"

struct Global_Param
{
    u_int32_t server_init_seq;
    u_int32_t client_init_seq;

    u_int16_t send_winSize;
    u_int32_t send_bufSize;

    u_int16_t recv_winSize;
    u_int32_t recv_bufSize;

    u_int16_t server_MSS_size;
    u_int16_t client_MSS_size;
    u_int16_t MSS_size;
};

struct PseTCP_Header //TCP fake header
{
    u_int8_t srcAddress[4];
    u_int8_t dstAddress[4];
    u_int8_t reserved;
    u_int8_t protocolType;
    u_int16_t totalLen;
};

struct TCP_Header //TCP header
{
    u_int16_t srcPort;
    u_int16_t dstPort;
    u_int32_t sequence;
    u_int32_t ackNumber;
    union
    {
        u_int16_t headerLen;
        u_int16_t reserved;
        u_int16_t flags;
    };
    u_int16_t winSize;
    u_int16_t checksum;
    u_int16_t urgentOffset;
    u_int16_t mssSize;
    u_int16_t padding = 0;
};

void init_tcp_send_buffer();
void init_tcp_recv_buffer();
void init_tcp_req_que();

int connect(My_SOCKET* sockp, socket_addr* server_addr, int addrlen);

int listen(My_SOCKET* sockp, int que_len);

My_SOCKET* accept(My_SOCKET* old_sockp, socket_addr* client_addr, int addrlen);

void transport_tcp_recv(u_int8_t* buffer, u_int8_t* source_ip, int totalLen);

int closesocket(My_SOCKET* sockp, struct Global_Param* gp);

int send(My_SOCKET* sockp, u_int8_t* buf, int buflen, int flags);

int recv(My_SOCKET* sockp, u_int8_t* buf, int buflen, int flags);
