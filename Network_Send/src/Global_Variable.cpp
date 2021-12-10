/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:02:57
 * @LastEditTime: 2021-12-10 16:01:00
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include "Header_Include.h"

u_int8_t local_mac[6] = { 0x50, 0xE0, 0x85, 0xA7, 0x3D, 0x7F };
u_int8_t local_ip[4] = { 10, 27, 36, 217 };
u_int8_t gateway_ip[4] = { 10, 27, 0, 1 };
u_int8_t netmask[4] = { 255, 255, 0, 0 };
u_int8_t dns_server_ip[4] = { 202, 117, 80, 6 };
u_int8_t dhcp_server_ip[4] = { 10, 27, 0, 1 };
u_int8_t broadcast_mac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

pcap_t *handle;
u_int8_t target_ip[4] = { 10, 27, 36, 216 };

void P(int* s)
{
    while ((*s) <= 0) Sleep(1111);
    (*s)--;
}

void V(int* s)
{
    (*s)++;
}