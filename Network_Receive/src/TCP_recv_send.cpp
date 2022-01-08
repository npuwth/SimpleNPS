/*
 * @Author: npuwth
 * @Date: 2022-01-04 16:55:00
 * @LastEditTime: 2022-01-08 16:11:08
 * @LastEditors: npuwth
 * @Copyright 2022
 * @Description: Network Experiment
 */

#include "Header_Include.h"
#include "TCP_recv_send.h"
// #include "UDP_recv_send.h"
#include "Network_IPV4_send.h"
#include <time.h>

#define MAX_SIZE 1500
#define MAX_QUE 1000
#define MAX_REQUEST_QUE 5
#define CLIENT_WIN_SIZE 5*1400
#define SERVER_WIN_SIZE 5*1400
#define MAX_BUFFER_SIZE 102400 //send data/ recv data buffer size 100KB

#define SYN 0x0002
#define ACK 0x0010
#define FIN 0x0001

extern u_int8_t local_ip[4];
extern u_int16_t local_port;

u_int8_t tcp_recv_pool[MAX_QUE][MAX_SIZE];
int tcp_total_size[MAX_QUE];
u_int8_t tcp_src_address[MAX_QUE][4];

int tcp_recv_mutex = 1;
int tcp_recv_empty = MAX_QUE;
int tcp_recv_full = 0;
int tcp_recv_que_head, tcp_recv_que_tail;

u_int8_t tcp_send_pool[MAX_QUE][MAX_SIZE];
int tcp_send_mutex = 1;
int tcp_send_empty = MAX_QUE;
int tcp_send_full = 0;
int tcp_send_que_head, tcp_send_que_tail;

u_int8_t tcp_request_que[MAX_REQUEST_QUE][24];
u_int8_t tcp_req_address[MAX_REQUEST_QUE][4];

int req_que_mutex = 1;
int req_que_empty = MAX_REQUEST_QUE;
int req_que_full = 0; 
int req_que_head, req_que_tail;

u_int8_t tcp_buffer[MAX_SIZE];
u_int8_t tcp_pseheader[12];

u_int8_t send_data_buffer[MAX_BUFFER_SIZE];
u_int8_t recv_data_buffer[MAX_BUFFER_SIZE];
int recv_data_bit[MAX_BUFFER_SIZE/1400 + 1]; //this is for count which part have been received in current window "tcp's accumulate ack"

u_int32_t seq;
u_int32_t ack;


struct Global_Param* client_gp = (struct Global_Param*)malloc(sizeof(struct Global_Param));
struct Global_Param* server_gp = (struct Global_Param*)malloc(sizeof(struct Global_Param));

void init_tcp_send_buffer()
{
    tcp_send_que_head = 0;
    tcp_send_que_tail = 0;
}

void init_tcp_recv_buffer()
{
    tcp_recv_que_head = 0;
    tcp_recv_que_tail = 0;
}

void init_tcp_req_que()
{
    req_que_head = 0;
    req_que_tail = 0;
}


void load_tcp_header(My_SOCKET* sockp, u_int16_t seq, u_int16_t acknum, u_int16_t code, u_int16_t totalLen)
{
    struct TCP_Header* tcphdr = (struct TCP_Header*)tcp_buffer;
    tcphdr->srcPort = htons(sockp->local_port);
    tcphdr->dstPort = htons(sockp->target_port);
    tcphdr->sequence = htonl(seq);
    tcphdr->ackNumber = htonl(acknum);
    tcphdr->headerLen = htons(((sizeof(struct TCP_Header) / 4) << 12) | code);
    tcphdr->winSize = htons(CLIENT_WIN_SIZE);
    tcphdr->checksum = 0;
    tcphdr->urgentOffset = htons(0);
    tcphdr->mssSize = htons(1400);

    struct PseTCP_Header* psehdr = (struct PseTCP_Header*)tcp_pseheader;
    for(int i = 0; i < 4; i++)
    {
        psehdr->srcAddress[i] = sockp->local_address[i];
        psehdr->dstAddress[i] = sockp->target_address[i];
    }
    psehdr->reserved = 0;
    psehdr->protocolType = IPPROTO_TCP;
    psehdr->totalLen = totalLen;

    u_int16_t checksum = calculate_check((u_int16_t*)tcp_pseheader, (u_int16_t*)tcp_buffer, totalLen);
    tcphdr->checksum = checksum;
}


void load_tcp_data(u_int8_t* tcpbuf, u_int8_t* data_buffer, int buflen)
{
    memcpy(tcpbuf, data_buffer, buflen);
}


int connect(My_SOCKET* sockp, socket_addr* server_addr, int addrlen)
{
    srand((unsigned)time(NULL));
    u_int32_t init_seq = rand() % 65536;
    seq = init_seq;
    ack = 0;

    u_int8_t* recv_buffer[MAX_SIZE];
    struct TCP_Header* tcphdr;
    
    for(int i = 0; i < 4; i++) sockp->target_address[i] = server_addr->sin_ip[i];
    sockp->target_port = server_addr->sin_port;

    load_tcp_header(sockp, seq, ack, SYN, sizeof(struct TCP_Header));

    seq = seq + 1;

    network_ipv4_send(tcp_buffer, sizeof(struct TCP_Header), server_addr->sin_ip, IPPROTO_TCP);

    while(1)
    {
        P(&tcp_recv_full);
        P(&tcp_recv_mutex);
        memcpy(recv_buffer, tcp_recv_pool[tcp_recv_que_head], tcp_total_size[tcp_recv_que_head]);
        tcp_recv_que_head = (tcp_recv_que_head + 1) % MAX_QUE;
        V(&tcp_recv_mutex);
        V(&tcp_recv_empty);
        tcphdr = (struct TCP_Header*)recv_buffer;
        if(((ntohs(tcphdr->flags) & ACK) != 0) && ((ntohs(tcphdr->flags) & SYN) != 0))
        {
            printf("get server's ack and connect request! 2rd hand shake.\n");
            break;
        }
        else
        {
            printf("not server's ack.\n");
        }
    }
    ack = ntohl(tcphdr->sequence) + 1;

    if(seq != ntohl(tcphdr->ackNumber)) printf("TCP Connect Error: seq and ack not match!\n");
    else printf("TCP Connect Right: seq and ack match.\n");
    printf("seq: %d, ack: %d.\n", seq, ntohl(tcphdr->ackNumber));

    //send ack to server
    load_tcp_header(sockp, seq, ack, ACK, sizeof(struct TCP_Header));

    network_ipv4_send(tcp_buffer, sizeof(struct TCP_Header), server_addr->sin_ip, IPPROTO_TCP);

    //global param
    client_gp->server_init_seq = ntohl(tcphdr->sequence);
    client_gp->client_init_seq = init_seq;
    client_gp->send_winSize = CLIENT_WIN_SIZE;
    client_gp->send_bufSize = MAX_BUFFER_SIZE;
    client_gp->recv_bufSize = SERVER_WIN_SIZE;
    client_gp->recv_bufSize = MAX_BUFFER_SIZE;
    client_gp->server_MSS_size = ntohs(tcphdr->mssSize);
    client_gp->client_MSS_size = 1400;
    client_gp->MSS_size = (client_gp->server_MSS_size > client_gp->client_MSS_size) ? client_gp->client_MSS_size : client_gp->server_MSS_size;

    return SUCCESS;
}


DWORD WINAPI start_listen(LPVOID pM)
{
    u_int8_t* recv_buffer[MAX_SIZE];
    u_int8_t src_address[4];
    struct TCP_Header* tcphdr;
    while(1)
    {
        P(&tcp_recv_full);
        P(&tcp_recv_mutex);
        
        memcpy(recv_buffer, tcp_recv_pool[tcp_recv_que_head], tcp_total_size[tcp_recv_que_head]);
        for(int i = 0; i < 4; i++)
        {
            src_address[i] = tcp_src_address[tcp_recv_que_head][i];
        }
        tcphdr = (struct TCP_Header*)recv_buffer;
        if((ntohs(tcphdr->flags) & SYN) != 0)
        {
            tcp_recv_que_head = (tcp_recv_que_head + 1) % MAX_QUE;
            V(&tcp_recv_mutex);
            V(&tcp_recv_empty);
        }
        else
        {
            V(&tcp_recv_mutex);
            V(&tcp_recv_full);
        }
        
        if((ntohs(tcphdr->flags) & SYN) != 0)
        {
            P(&req_que_empty);
            P(&req_que_mutex);
            memcpy(tcp_request_que[req_que_tail], recv_buffer, sizeof(struct TCP_Header));
            for(int i = 0; i < 4; i++)
            {
                tcp_req_address[req_que_tail][i] = src_address[i];
            }
            req_que_tail = (req_que_tail + 1) % MAX_REQUEST_QUE;
            V(&req_que_mutex);
            V(&req_que_full);
        }
        Sleep(1111);
    }
    exit(0);
}

int listen(My_SOCKET* sockp, int que_len)
{
    printf("start tcp listening...\n");
    HANDLE th[1];
    th[0] = CreateThread(NULL,0,start_listen,NULL,0,NULL);
    
    return SUCCESS;
}

My_SOCKET* accept(My_SOCKET* old_sockp, socket_addr* client_addr, int addrlen)
{
    srand((unsigned)time(NULL));
    u_int32_t init_seq = rand() % 65536;
    seq = init_seq;
    ack = 0;

    u_int8_t* req_buffer[24];
    u_int8_t* recv_buffer[MAX_SIZE];
    u_int8_t src_address[4];
    P(&req_que_full);
    P(&req_que_mutex);
    memcpy(req_buffer, tcp_request_que[req_que_head], sizeof(struct TCP_Header));
    for(int i = 0; i < 4; i++)
    {
        src_address[i] = tcp_req_address[req_que_head][i];
    }
    req_que_head = (req_que_head + 1) % MAX_REQUEST_QUE;
    V(&req_que_mutex);
    V(&req_que_empty);

    struct TCP_Header* tcphdr = (struct TCP_Header*)req_buffer;
    My_SOCKET* new_sockp = (My_SOCKET*)malloc(sizeof(My_SOCKET));
    new_sockp->local_port = local_port;
    new_sockp->target_port = ntohs(tcphdr->srcPort);
    old_sockp->target_port = ntohs(tcphdr->srcPort);
    for(int i = 0; i < 4; i++)
    {
        new_sockp->local_address[i] = local_ip[i];
        new_sockp->target_address[i] = src_address[i];
        old_sockp->target_address[i] = src_address[i];
    }
    new_sockp->sock_type = IPPROTO_TCP;

    client_addr->sin_port = ntohs(tcphdr->srcPort);
    for(int i = 0; i < 4; i++) client_addr->sin_ip[i] = src_address[i];

    ack = ntohl(tcphdr->sequence) + 1;

    load_tcp_header(old_sockp, seq, ack, SYN | ACK, sizeof(struct TCP_Header));

    network_ipv4_send(tcp_buffer, sizeof(struct TCP_Header), client_addr->sin_ip, IPPROTO_TCP);

    seq = seq + 1;

    server_gp->server_init_seq = init_seq;
    server_gp->client_init_seq = ntohl(tcphdr->sequence);
    server_gp->send_winSize = CLIENT_WIN_SIZE;
    server_gp->send_bufSize = MAX_BUFFER_SIZE;
    server_gp->recv_bufSize = SERVER_WIN_SIZE;
    server_gp->recv_bufSize = MAX_BUFFER_SIZE;
    server_gp->server_MSS_size = 1400;
    server_gp->client_MSS_size = ntohs(tcphdr->mssSize);
    server_gp->MSS_size = (client_gp->server_MSS_size > client_gp->client_MSS_size) ? client_gp->client_MSS_size : client_gp->server_MSS_size;
    
    while(1)
    {
        P(&tcp_recv_full);
        P(&tcp_recv_mutex);
        memcpy(recv_buffer, tcp_recv_pool[tcp_recv_que_head], tcp_total_size[tcp_recv_que_head]);
        tcp_recv_que_head = (tcp_recv_que_head + 1) % MAX_QUE;
        V(&tcp_recv_mutex);
        V(&tcp_recv_empty);
        tcphdr = (struct TCP_Header*)recv_buffer;
        if((ntohs(tcphdr->flags) & ACK) != 0)
        {
            printf("server get 3th hand shake!\n");
            
            if(seq != ntohl(tcphdr->ackNumber)) printf("TCP Accept Error: seq and ack not match!\n");
            else printf("TCP Accept Right: seq and ack match.\n");
            printf("seq: %d, ack: %d.\n", seq, ntohl(tcphdr->ackNumber));

            ack = ntohl(tcphdr->sequence);
            break;
        }
        else
        {
            printf("not client's ack.\n");
        }
    }

    return new_sockp;
}

// check some basic info: checksum, port
void transport_tcp_recv(u_int8_t* buffer, u_int8_t* source_ip, int totalLen)
{
    struct TCP_Header* tcphdr = (struct TCP_Header*)buffer;

    struct PseTCP_Header pse_header;
    pse_header.reserved = 0;
    pse_header.protocolType = IPPROTO_TCP;
    pse_header.totalLen = totalLen;
    for(int i = 0; i < 4; i++)
    {
        pse_header.srcAddress[i] = source_ip[i];
        pse_header.dstAddress[i] = local_ip[i];
    } 
    u_int16_t check_sum = calculate_check((u_int16_t*)(&pse_header), (u_int16_t*)buffer, totalLen);
    if(check_sum == 0xffff || check_sum == 0x0000)
    {
        ;
    }
    else
    {
        printf("Error: TCP Check Sum Error!\n");
        printf("packet_size: %d\n", totalLen);
        printf("check_sum: %04x\n", check_sum);
        return;
    }
    if(ntohs(tcphdr->dstPort) == local_port)
    {
        ;
    }
    else
    {
        printf("Error: TCP Port Error!\n");
        return;
    }
    P(&tcp_recv_empty);
    P(&tcp_recv_mutex);
    memcpy(tcp_recv_pool[tcp_recv_que_tail], buffer, totalLen);
    tcp_total_size[tcp_recv_que_tail] = totalLen;
    for(int i = 0; i < 4; i++)
    {
        tcp_src_address[tcp_recv_que_tail][i] = source_ip[i];
    }

    tcp_recv_que_tail = (tcp_recv_que_tail + 1) % MAX_QUE;
    V(&tcp_recv_mutex);
    V(&tcp_recv_full);
    printf("---------------------TCP Protocol---------------------\n");
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
    printf("Source Port: %04x\n", ntohs(tcphdr->srcPort));
    printf("Destination Port: %04x\n", ntohs(tcphdr->dstPort));
    printf("Sequence: %d\n", ntohl(tcphdr->sequence));
    printf("AckNumber: %d\n", ntohl(tcphdr->ackNumber));
    printf("HeaderLen&Flags: %04x\n", ntohs(tcphdr->flags));
    printf("WinSize: %04x\n", ntohs(tcphdr->winSize));
    printf("CheckSum: %04x\n", ntohs(tcphdr->checksum));
    printf("UrgentOffset: %04x\n", ntohs(tcphdr->urgentOffset));
    printf("MSSSize: %04x\n", ntohs(tcphdr->mssSize));
    printf("---------------End Of TCP Protocol--------------------\n");
}

int closesocket(My_SOCKET* sockp, struct Global_Param* gp)
{
    free(sockp);
    free(gp);
    return SUCCESS;
}

int send(My_SOCKET* sockp, u_int8_t* buf, int buflen, int flags)
{
    memcpy(send_data_buffer, buf, buflen);
    // three pointer in the buffer
    int win_start = 0;
    int win_end = CLIENT_WIN_SIZE > buflen ? buflen : CLIENT_WIN_SIZE;
    int win_cfront = 0; //MSS 1400

    u_int8_t recv_buffer[MAX_SIZE];

    while(win_start < buflen)
    {
        int data_size = win_end - win_start; //send a window
        int fragment_num = (int)ceil(data_size*1.0 / 1400);
        int offset = 0; //offset within a send window 
        while(fragment_num)
        {
            int data_len;
            if(fragment_num == 1)
            {
                data_len = data_size - offset;
            }
            else
            {
                data_len = 1400; //MSS
            }

            load_tcp_header(sockp, seq + win_cfront, ack, 0x0000, sizeof(struct TCP_Header) + data_len);
            load_tcp_data(tcp_buffer + sizeof(struct TCP_Header), buf + win_cfront, data_len);

            network_ipv4_send(tcp_buffer, sizeof(struct TCP_Header) + data_len, sockp->target_address, IPPROTO_TCP);

            fragment_num--;
            offset = offset + 1400;
            win_cfront = win_start + offset;
        }

        Sleep(3333); //re send time
        
        P(&tcp_recv_full);
        P(&tcp_recv_mutex);
        memcpy(recv_buffer, tcp_recv_pool[tcp_recv_que_head], tcp_total_size[tcp_recv_que_head]);
        tcp_recv_que_head = (tcp_recv_que_head + 1) % MAX_QUE;
        V(&tcp_recv_mutex);
        V(&tcp_recv_empty);

        struct TCP_Header* tcphdr = (struct TCP_Header*)recv_buffer;
        
        if((ntohs(tcphdr->flags) & ACK) != 0)
        {
            printf("send and receive ack!\n");
        }
        else
        {
            printf("get a not ack.\n");
            continue;
        }

        win_start = ntohl(tcphdr->ackNumber) - seq;
        win_cfront = win_start;
        win_end = (win_start + CLIENT_WIN_SIZE > buflen) ? buflen : (win_start + CLIENT_WIN_SIZE);
    }

    seq = seq + buflen;
    return buflen;
}

int recv(My_SOCKET* sockp, u_int8_t* buf, int buflen, int flags)
{
    memset(recv_data_buffer, 0, MAX_BUFFER_SIZE);
    int win_start = 0;
    int win_end = CLIENT_WIN_SIZE > buflen ? buflen : CLIENT_WIN_SIZE;
    int win_cfront = 0;

    u_int8_t recv_buffer[MAX_SIZE];
    int current_size;

    while(win_start < buflen)
    {
        int tag = 1;
        while(tag)
        {
            P(&tcp_recv_full);
            P(&tcp_recv_mutex);
            if((tcp_recv_que_head + 1) % MAX_QUE == tcp_recv_que_tail) tag = 0;
            memcpy(recv_buffer, tcp_recv_pool[tcp_recv_que_head], tcp_total_size[tcp_recv_que_head]);
            current_size = tcp_total_size[tcp_recv_que_head];
            tcp_recv_que_head = (tcp_recv_que_head + 1) % MAX_QUE;
            V(&tcp_recv_mutex);
            V(&tcp_recv_empty);

            struct TCP_Header* tcphdr = (struct TCP_Header*)recv_buffer;
            win_cfront = ntohl(tcphdr->sequence) - (server_gp->client_init_seq + 1);
            if(win_cfront >= win_start && win_cfront < win_end)
            {
                memcpy(recv_data_buffer + win_cfront, recv_buffer + sizeof(struct TCP_Header), current_size - sizeof(struct TCP_Header));
            
                recv_data_bit[win_cfront/1400] = 1;
            }
            else
            {
                printf("TCP Error: data not experted!\n");
                printf("seq: %d\n", ntohl(tcphdr->sequence));
                printf("win_cfront: %d, win_start: %d, win_end: %d\n", win_cfront, win_start, win_end);
            }
        } 

        int i = 0;
        for(i = 0; i < (MAX_BUFFER_SIZE/1400) + 1; i++)
        {
            if(recv_data_bit[i] == 0)
            {
                break;
            }
        }

        ack = i * 1400 + server_gp->client_init_seq + 1;

        printf("The next byte wanted is %d\n", ack);

        load_tcp_header(sockp, seq, ack, ACK, sizeof(struct TCP_Header));

        network_ipv4_send(tcp_buffer, sizeof(struct TCP_Header), sockp->target_address, IPPROTO_TCP);

        win_start = i * 1400;
        win_end = (win_start + CLIENT_WIN_SIZE) > buflen ? buflen : (win_start + CLIENT_WIN_SIZE);

        Sleep(2222);
    }

    memcpy(buf, recv_data_buffer, buflen);
    return buflen;
}