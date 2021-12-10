/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:02:57
 * @LastEditTime: 2021-12-03 08:00:31
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include "Header_Include.h"

int is_accept_arp_packet(struct arp_pkt *arp_packet);
u_int8_t* network_arp_recv(u_int8_t *arp_buffer);

void output(struct arp_pkt *arp_packet);