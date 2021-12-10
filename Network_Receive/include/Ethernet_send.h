/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:02:57
 * @LastEditTime: 2021-11-27 20:33:29
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include "Header_Include.h"

void load_ethernet_header(u_int8_t *destination_mac, u_int16_t ethernet_type);

void load_ethernet_data(u_int8_t *buffer, u_int8_t *upper_buffer, int len);

int ethernet_send_packet(u_int8_t *upper_buffer, u_int8_t *destination_mac, u_int16_t ethernet_type, int ethernet_upper_len);

void init_send_packet_buffer();

DWORD WINAPI thread_send(LPVOID pM);

DWORD WINAPI init_ethernet_sender(LPVOID pM);