/*
 * @Author: npuwth
 * @Date: 2021-12-29 21:50:20
 * @LastEditTime: 2021-12-29 23:44:08
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include "Header_Include.h"
#include "UDP_recv_send.h"

extern u_int8_t local_ip[4];
extern int local_port;

u_int8_t buffer[65535];

void load_udp_header(SOCKET* sockp, int buflen)
{
    struct UDP_Header* udphdr = (struct UDP_Header *)buffer;
    udphdr->srcPort = htons((u_int16_t)(sockp->local_port));
    udphdr->dstPort = htons((u_int16_t)(sockp->target_port));
    udphdr->udpLen = htons((u_int16_t)(buflen + 8));
    udphdr->checkSum = 0;
}

void load_udp_data(u_int8_t* data_buffer, int len)
{
    if(len > 65535 - 8)
    {
        prinf("Error: Data is too large!\n");
        return;
    }
    
}

SOCKET* socket(int af, int type, int protocol)
{
    sockp = (SOCKET*)malloc(sizeof(SOCKET));
    for(int i = 0; i < 4; i++)
    {
        sockp->local_address[i] = local_ip[i];
        
    }
    sockp->local_port = local_port;
    return sockp;
}

int bind(SOCKET* sockp, socket_addr* server_addr, int addrlen)
{
    for(int i = 0; i < 4; i++)
    {
        sockp->target_address[i] = server_addr->sin_ip[i];
    }
    sockp->target_port = server_addr->sin_port;
    return SUCCESS;
}

int sendto(SOCKET* sockp, u_int8_t* buf, int buflen, int flags, socket_addr* dstaddr, int addrlen)
{
    
}

int recvfrom(SOCKET* sockp, u_int8_t* buf, int buflen, int flags, socket_addr* srcaddr, int addrlen);

int closesocket(SOCKET* sockp);