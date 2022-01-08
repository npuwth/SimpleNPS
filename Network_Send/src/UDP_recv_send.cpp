/*
 * @Author: npuwth
 * @Date: 2021-12-29 21:50:20
 * @LastEditTime: 2022-01-07 14:42:42
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include "Header_Include.h"
#include "UDP_recv_send.h"
#include "Network_IPV4_send.h"

#define MAX_UDP_SIZE 65535
#define UDP_HEADER_SIZE sizeof(UDP_Header)
#define MAX_QUE 50

extern u_int8_t local_ip[4];
extern u_int16_t local_port;

u_int8_t udp_buffer[MAX_UDP_SIZE];
u_int8_t udp_pseheader[12];

u_int8_t udp_recv_pool[MAX_QUE][MAX_UDP_SIZE];
int udp_data_size[MAX_QUE];
u_int8_t udp_src_address[MAX_QUE][4];
u_int16_t udp_src_port[MAX_QUE];
int udp_recv_mutex = 1;
int udp_recv_empty = MAX_QUE;
int udp_recv_full = 0;
int udp_recv_que_head, udp_recv_que_tail;

u_int8_t udp_send_pool[MAX_QUE][MAX_UDP_SIZE];
int udp_send_mutex = 1;
int udp_send_empty = MAX_QUE;
int udp_send_full = 0;
int udp_send_que_head, udp_send_que_tail;

void init_udp_send_buffer()
{
    udp_send_que_head = 0;
    udp_send_que_tail = 0;
}

void init_udp_recv_buffer()
{
    udp_recv_que_head = 0;
    udp_recv_que_tail = 0;
}

u_int16_t calculate_check(u_int16_t* psehdr, u_int16_t* buf, int buflen)
{
    int sum = 0;int count = 0;
    int len = sizeof(struct PseUDP_Header);
	u_int16_t *p = (u_int16_t*)psehdr;
	while (len > 1)
	{
		sum += *p;
		len -= 2;
		p++;
	}

    len = buflen; 
    int tmp = len;
    p = (u_int16_t*)buf;
    while (len > 1)
	{
		sum += *p;
		len -= 2;
		p++;
	}

    if (len)
	{
		sum += *((u_int8_t*)buf + tmp - 1);
	}

	//fold 32 bits to 16 bits
	while (sum >> 16)
	{
		sum = (sum & 0xffff) + (sum >> 16);
	}

	return ~sum;
}

void load_pse_header(u_int8_t* src_address, u_int8_t* dst_address, int buflen)
{
    struct PseUDP_Header* psehdr = (struct PseUDP_Header*)udp_pseheader;
    for(int i = 0; i < 4; i++)
    {
        psehdr->srcAddress[i] = src_address[i];
        psehdr->dstAddress[i] = dst_address[i];
    }
    psehdr->reserved = 0;
    psehdr->protocolType = IPPROTO_UDP;
    psehdr->totalLen = buflen + UDP_HEADER_SIZE;
}

void load_udp_header(My_SOCKET* sockp, int buflen)
{
    struct UDP_Header* udphdr = (struct UDP_Header *)udp_buffer;
    udphdr->srcPort = htons((u_int16_t)(sockp->local_port)); //need to transfer host to network
    udphdr->dstPort = htons((u_int16_t)(sockp->target_port));
    udphdr->udpLen = htons((u_int16_t)(buflen + UDP_HEADER_SIZE));
    udphdr->checkSum = 0;
}

void load_udp_data(u_int8_t* udpbuf, u_int8_t* data_buffer, int buflen)
{
    if(buflen > MAX_UDP_SIZE - UDP_HEADER_SIZE)
    {
        printf("Error: Data is too large!\n");
        return;
    }
    memcpy(udpbuf, data_buffer, buflen);
}

My_SOCKET* mysocket(int af, int type, int protocol)
{
    // init_udp_send_buffer();
    // init_udp_recv_buffer();
    My_SOCKET* sockp = (My_SOCKET*)malloc(sizeof(My_SOCKET));
    for(int i = 0; i < 4; i++)
    {
        sockp->local_address[i] = local_ip[i];
    }
    sockp->local_port = local_port;
    sockp->sock_type = type;
    return sockp;
}

int bind(My_SOCKET* sockp, socket_addr* server_addr, int addrlen)//服务器端使用，替换local address&ip
{
    for(int i = 0; i < 4; i++)
    {
        sockp->local_address[i] = server_addr->sin_ip[i];
    }
    sockp->local_port = server_addr->sin_port;
    return SUCCESS;
}

// buflen here is actually data length
int sendto(My_SOCKET* sockp, u_int8_t* buf, int buflen, int flags, socket_addr* dstaddr, int addrlen)
{
    sockp->target_port = dstaddr->sin_port; //use given dstaddr to fill sockp
    for (int i = 0; i < 4; i++) sockp->target_address[i] = dstaddr->sin_ip[i];

    load_pse_header(sockp->local_address, sockp->target_address, buflen);
    load_udp_header(sockp, buflen);
    load_udp_data(udp_buffer + sizeof(UDP_Header), buf, buflen);

    struct PseUDP_Header* psehdr = (struct PseUDP_Header *)udp_pseheader; //calculate checksum
    u_int16_t* udphdr = (u_int16_t *)udp_buffer;
    u_int16_t checkSum = calculate_check((u_int16_t*)psehdr, udphdr, buflen + UDP_HEADER_SIZE);
    struct UDP_Header* udpheader = (struct UDP_Header*)udp_buffer;
    udpheader->checkSum = checkSum;
    network_ipv4_send(udp_buffer, buflen + UDP_HEADER_SIZE, dstaddr->sin_ip, IPPROTO_UDP); //use ip to send datagram
    return buflen;
}

int recvfrom(My_SOCKET* sockp, u_int8_t* buf, int buflen, int flags, socket_addr* srcaddr, int addrlen)
{
    int datalen;
    P(&udp_recv_full);
    P(&udp_recv_mutex);
    datalen = udp_data_size[udp_recv_que_head];
    // if(datalen < buflen)
    memcpy(buf, udp_recv_pool[udp_recv_que_head], datalen);
    srcaddr->sin_port = udp_src_port[udp_recv_que_head];
    for(int i = 0; i < 4; i++) srcaddr->sin_ip[i] = udp_src_address[udp_recv_que_head][i];
    udp_recv_que_head = (udp_recv_que_head + 1) % MAX_QUE;
    V(&udp_recv_mutex);
    V(&udp_recv_empty);
    
    return datalen;
}

int closesocket(My_SOCKET* sockp) //free current sockp
{
    free(sockp);
    return SUCCESS;
}

void transport_udp_recv(u_int8_t* buffer, u_int8_t* source_ip) //call back function by ip layer, buffer contains whole udp datagram
{
    struct UDP_Header* udphdr = (struct UDP_Header*)buffer;
    // printf("totalLen:%04x\n", udphdr->udpLen);
    // for(int i = 0; i < udphdr->udpLen; i++) printf("%02x ", buffer[i]);
    struct PseUDP_Header pse_header;
    pse_header.reserved = 0;
    pse_header.protocolType = IPPROTO_UDP;
    pse_header.totalLen = ntohs(udphdr->udpLen);
    u_int16_t totalLen = ntohs(udphdr->udpLen);
    for(int i = 0; i < 4; i++)
    {
        pse_header.srcAddress[i] = source_ip[i];
        pse_header.dstAddress[i] = local_ip[i];
    }
    u_int16_t check_sum = calculate_check((u_int16_t*)(&pse_header), (u_int16_t* )buffer, totalLen);
    if(check_sum == 0xffff || check_sum == 0x0000)
    {
        // printf("");
        ;
    }
    else
    {
        printf("Error: UDP Check Sum Error!\n");
        printf("check_sum:%04x\n", check_sum);
        return;
    }
    if(ntohs(udphdr->dstPort) == local_port)
    {
        ;
    }
    else
    {
        printf("Error: UDP Port Error!\n");
        return;
    }
    P(&udp_recv_empty);
    P(&udp_recv_mutex);
    memcpy(udp_recv_pool[udp_recv_que_tail], buffer + sizeof(UDP_Header), totalLen - sizeof(UDP_Header));
    udp_data_size[udp_recv_que_tail] = totalLen - sizeof(UDP_Header);
    for(int i = 0; i < 4; i++)
    {
        udp_src_address[udp_recv_que_tail][i] = source_ip[i];
    }
    udp_src_port[udp_recv_que_tail] = ntohs(udphdr->srcPort); 
    udp_recv_que_tail = (udp_recv_que_tail + 1) % MAX_QUE;
    V(&udp_recv_mutex);
    V(&udp_recv_full);
    printf("--------------UDP Protocol-------------------\n");
	printf("Source IP: ");
	for (int i = 0; i < 4; i++)
	{
		if (i)printf(".");
		printf("%d", source_ip[i]);
	}
	printf("\nDestination IP: ");
	for (int i = 0; i < 4; i++)
	{
		if (i)printf(".");
		printf("%d", local_ip[i]);
	}
	printf("\n");
    printf("Source Port: %04x\n", ntohs(udphdr->srcPort));
    printf("Destination Port: %04x\n", ntohs(udphdr->dstPort));
    printf("Total Length: %04x\n", ntohs(udphdr->udpLen));
    printf("---------End of UDP Protocol-----------------\n");
    // system("pause");
}

// DWORD WINAPI thread_udp_send(LPVOID pM)

// DWORD WINAPI thread_udp_receive(LPVOID pM)