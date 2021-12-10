/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:02:57
 * @LastEditTime: 2021-12-03 08:00:59
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include "Header_Include.h"

// u_int16_t calculate_check_sum(ip_header *ip_hdr, int len);
//there is some bits that value is 0, so len as a parameter join the function
int network_ipv4_recv(u_int8_t *ip_buffer);
int is_accept_ip_packet(struct ip_header *ip_hdr);

void init_recv_data_buffer();
DWORD WINAPI thread_write(LPVOID pM);
DWORD WINAPI init_ip_receiver(LPVOID pM);