/*
 * @Author: npuwth
 * @Date: 2022-01-04 20:38:29
 * @LastEditTime: 2022-01-06 14:41:01
 * @LastEditors: npuwth
 * @Copyright 2022
 * @Description: Network Experiment
 */

#include "Header_Include.h"

struct ICMP_Header
{
    u_int8_t type;
    u_int8_t code;
    u_int16_t checksum;
    u_int16_t id;
    u_int16_t seq;
    u_int64_t timestamp;
};

int send_ICMP_echo_request(u_int8_t* target_ip);

int send_ICMP_echo_reply(u_int8_t* target_ip, u_int16_t id);

void transport_icmp_recv(u_int8_t* buffer, u_int8_t* source_ip);
