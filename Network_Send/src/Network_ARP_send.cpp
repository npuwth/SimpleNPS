/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:02:57
 * @LastEditTime: 2021-12-10 15:26:36
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include "Network_ARP_send.h"
#include "Ethernet_recv.h"
#include "Resource.h"
#include "Ethernet_send.h"

#define MAX_SIZE 2048

extern u_int8_t local_mac[6];
extern u_int8_t local_ip[4];

u_int8_t arp_buffer[MAX_SIZE];

void load_arp_packet(u_int8_t *destination_ip)
{
	struct arp_pkt *arp_packet = (struct arp_pkt *)(arp_buffer);
	arp_packet->hardware_type = htons(ARP_HARDWARE);
	arp_packet->protocol_type = htons(ETHERNET_IP);
	arp_packet->hardware_addr_length = 6;
	arp_packet->protocol_addr_length = 4;
	// arp_packet->op_code = htons(ARP_REPLY);
	arp_packet->op_code = htons(ARP_REQUEST);
	int i;
	for (i = 0; i < 6; i++)
	{
		arp_packet->source_mac[i] = local_mac[i];
	}
	for (i = 0; i < 4; i++)
	{
		arp_packet->source_ip[i] = local_ip[i];
	}

	for (i = 0; i < 6; i++)
	{
		arp_packet->destination_mac[i] = 0x00;
	}
	for (i = 0; i < 4; i++)
	{
		arp_packet->destination_ip[i] = destination_ip[i];
	}
}

void network_arp_send(u_int8_t *destination_ip, u_int8_t *ethernet_dest_mac)
{
	printf("network arp send!\n");
	struct arp_pkt *arp_packet = (struct arp_pkt *)arp_buffer;
	load_arp_packet(destination_ip);
	int i;
	for (i = 0; i < 6; i++)
	{
		;
		// arp_packet->destination_mac[i] = ethernet_dest_mac[i];
	}
	//use ethernet fuction to send the packet
	ethernet_send_packet(arp_buffer, ethernet_dest_mac, ETHERNET_ARP, sizeof(struct arp_pkt));

}