/*
 * @Author: npuwth
 * @Date: 2022-01-04 20:59:02
 * @LastEditTime: 2022-01-06 15:12:34
 * @LastEditors: npuwth
 * @Copyright 2022
 * @Description: Network Experiment
 */

#include "Header_Include.h"
#include "ICMP_recv_send.h"
#include "Network_IPV4_send.h"


#define ICMP_TYPE_ECHO 8
#define ICMP_TYPE_ECHO_REPLY 0
#define MAX_ICMP_SIZE 65535

u_int8_t ICMP_send_buffer[MAX_ICMP_SIZE];
// u_int8_t ICMP_recv_buffer[MAX_ICMP_SIZE];

int send_ICMP_echo_request(u_int8_t* target_ip)
{
    struct ICMP_Header* icmphdr = (struct ICMP_Header*)ICMP_send_buffer;
    icmphdr->type = ICMP_TYPE_ECHO;
    icmphdr->code = 0;
    icmphdr->checksum = 0;
    icmphdr->id = htons((u_int16_t)GetCurrentProcessId());
    icmphdr->seq = htons(0);
    icmphdr->timestamp = htonll(GetTickCount());
    icmphdr->checksum = calculate_check_sum(ICMP_send_buffer, sizeof(struct ICMP_Header));
    printf("Send icmp echo request!\n");
    network_ipv4_send(ICMP_send_buffer, sizeof(struct ICMP_Header), target_ip, IPPROTO_ICMP); //use ip to send datagram
    return SUCCESS;
}

int send_ICMP_echo_reply(u_int8_t* target_ip, u_int16_t id)
{
    struct ICMP_Header* icmphdr = (struct ICMP_Header*)ICMP_send_buffer;
    icmphdr->type = ICMP_TYPE_ECHO_REPLY;
    icmphdr->code = 0;
    icmphdr->checksum = 0;
    icmphdr->id = id;
    icmphdr->seq = htons(0);
    icmphdr->timestamp = htonll(GetTickCount());
    icmphdr->checksum = calculate_check_sum(ICMP_send_buffer, sizeof(struct ICMP_Header));
    printf("Send icmp echo reply!\n");
    network_ipv4_send(ICMP_send_buffer, sizeof(struct ICMP_Header), target_ip, IPPROTO_ICMP); //use ip to send datagram
    return SUCCESS;
}

void transport_icmp_recv(u_int8_t* buffer, u_int8_t* source_ip)
{
    struct ICMP_Header* icmphdr = (struct ICMP_Header*)buffer;
    u_int16_t check_sum = calculate_check_sum(buffer, sizeof(struct ICMP_Header));
    if(check_sum == 0xffff || check_sum == 0x0000)
    {
        ;
    }
    else
    {
        printf("Error: ICMP checksum Error!\n");
        return;
    }
    
    if(icmphdr->type == ICMP_TYPE_ECHO)
    {
        send_ICMP_echo_reply(source_ip, icmphdr->id);
    }
    else if(icmphdr->type == ICMP_TYPE_ECHO_REPLY)//get icmp echo reply
    {
        if(ntohs(icmphdr->id) == (u_int16_t)GetCurrentProcessId())
        {
            ;
        }
        else
        {
            printf("Error: ICMP someone else's message!\n");
            return;
        }

    }
    printf("-------------------ICMP Protocol-------------------\n");
    printf("ICMP type: %02x\n", icmphdr->type);
    printf("ICMP code: %02x\n", icmphdr->code);
    printf("ICMP id: %04x\n", ntohs(icmphdr->id));
    printf("ICMP seq: %04x\n", ntohs(icmphdr->seq));
    printf("ICMP time Used: %lld ms\n", (GetTickCount() - ntohll(icmphdr->timestamp)));
    printf("--------------End of ICMP Protocol-----------------\n");
    // system("pause");
}