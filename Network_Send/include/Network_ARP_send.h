/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:02:57
 * @LastEditTime: 2021-12-29 11:55:05
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include "Header_Include.h"

struct arp_pkt
{
	u_int16_t hardware_type;
	u_int16_t protocol_type;
	u_int8_t hardware_addr_length;
	u_int8_t protocol_addr_length;
	u_int16_t op_code;
	u_int8_t source_mac[6];
	u_int8_t source_ip[4];
	u_int8_t destination_mac[6]; //request the mac addr
	u_int8_t destination_ip[4];
};


void load_arp_packet(u_int8_t *destination_ip, u_int16_t arp_code);
/*
if the needer mac addr is not in arp_table, so request
*/
void network_arp_send_reply(u_int8_t *destination_ip, u_int8_t *ethernet_dest_mac);

void network_arp_send_request(u_int8_t *destination_ip, u_int8_t *ethernet_dest_mac);
