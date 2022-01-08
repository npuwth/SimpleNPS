/*
 * @Author: npuwth
 * @Date: 2021-12-29 17:28:43
 * @LastEditTime: 2022-01-06 21:26:50
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */
#ifndef _UDP_RECV_SEND_H_
#define _UDP_RECV_SEND_H_
#include "Header_Include.h"

typedef struct SOCKET_ADDR //address type
{
    u_int16_t sin_family;
    u_int16_t sin_port;
    u_int8_t sin_ip[4];
}socket_addr;

typedef struct FIVE_PARAM //five param turple
{
    u_int8_t local_address[4];
    u_int16_t local_port;
    u_int8_t target_address[4];
    u_int16_t target_port;
    u_int16_t sock_type;
}My_SOCKET;

struct PseUDP_Header //UDP fake header
{
    u_int8_t srcAddress[4];
    u_int8_t dstAddress[4];
    u_int8_t reserved;
    u_int8_t protocolType;
    u_int16_t totalLen;
};

struct UDP_Header //UDP header
{
    u_int16_t srcPort;
    u_int16_t dstPort;
    u_int16_t udpLen;
    u_int16_t checkSum;
};

u_int16_t calculate_check(u_int16_t* psehdr, u_int16_t* buf, int buflen);

My_SOCKET* mysocket(int af, int type, int protocol);

int bind(My_SOCKET* sockp, socket_addr* server_addr, int addrlen);

int sendto(My_SOCKET* sockp, u_int8_t* buf, int buflen, int flags, socket_addr* dstaddr, int addrlen);

int recvfrom(My_SOCKET* sockp, u_int8_t* buf, int buflen, int flags, socket_addr* srcaddr, int addrlen);

int closesocket(My_SOCKET* sockp);

void transport_udp_recv(u_int8_t* buffer, u_int8_t* source_ip);

#endif