/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:02:57
 * @LastEditTime: 2022-01-02 22:20:20
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include "Network_IPV4_send.h"
#include "Resource.h"
#include "ARP_Cache_Table.h"
#include "Network_ARP_send.h"
#include "Network_ARP_recv.h"
#include "Ethernet_recv.h"
#include "Ethernet_send.h"

#define MAX_DATA_SIZE 1500
#define MAX_QUE 100

//u_int8_t buffer[MAX_SIZE];
u_int16_t ip_packet_id = 0;//as flag in ip_header->id
u_int32_t ip_size_of_packet = 0;

u_int8_t ip_buffer[MAX_DATA_SIZE];
u_int8_t ip_send_pool[MAX_QUE][MAX_DATA_SIZE];
int ip_send_data_size[MAX_QUE];
u_int8_t ip_dmac[MAX_QUE][6];
int ip_send_que_head,ip_send_que_tail;

int ip_send_mutex = 1;
int ip_send_empty = MAX_QUE;
int ip_send_full = 0;

extern u_int8_t broadcast_mac[6];
extern u_int8_t local_ip[4];
// extern u_int8_t target_ip[4];
extern u_int8_t netmask[4];
extern u_int8_t gateway_ip[4];
extern pcap_t *handle;
extern u_int8_t local_mac[6];

void init_ip_send_buffer()
{
	ip_send_que_head = 0;
	ip_send_que_tail = 0;
}

u_int16_t calculate_check_sum(ip_header *ip_hdr, int len)
{
	int sum = 0, tmp = len;
	u_int16_t *p = (u_int16_t*)ip_hdr;
	while (len > 1)
	{
		sum += *p;
		len -= 2;
		p++;
	}

	//len=1 last one byte
	if (len)
	{
		sum += *((u_int8_t*)ip_hdr + tmp - 1);
	}

	//fold 32 bits to 16 bits
	while (sum >> 16)
	{
		sum = (sum & 0xffff) + (sum >> 16);
	}

	return ~sum;
}

void load_ip_header(u_int8_t* target_ip)
{
	struct ip_header *ip_hdr = (struct ip_header*)ip_buffer;
	ip_size_of_packet = 0;
	//initial the ip header
	ip_hdr->version_hdrlen = 0x4f;//0100 1111 means ip version4 and header length: 60 bytes
	ip_hdr->type_of_service = 0xfe;/*111 1 1110: first 3 bits: priority level,
								   then 1 bit: delay, 1 bit: throughput, 1 bit: reliability
								   1 bit: routing cost, 1 bit: unused
								   */
	ip_hdr->total_length = 0;// wait for data length, 0 for now
	ip_hdr->id = ip_packet_id;//identification
	ip_hdr->fragment_offset = 0x0000;/*0 0 0 0 00...00: first 3 bits is flag: 1 bit: 0 the last fragment,
									 1 more fragmet. 1 bit: 0 allow fragment, 1 don't fragment. 1 bit: unused
									 the last 12 bits is offset
									 */
	ip_hdr->time_to_live = 64;//default 1000ms
	ip_hdr->upper_protocol_type = IPPROTO_TCP;//default upper protocol is tcp
	ip_hdr->check_sum = 0;//initial zero

	if(is_same_lan(local_ip, target_ip))
	{
		for (int i = 0; i < 4; i++) ip_hdr->destination_ip[i] = target_ip[i];
	}
	else
	{
		for (int i = 0; i < 4; i++) ip_hdr->destination_ip[i] = gateway_ip[i];
	}

	for (int i = 0; i < 4; i++) ip_hdr->source_ip[i] = local_ip[i];

	//check_sum is associate with offset. so in the data we need to calculate check_sum
	ip_size_of_packet += sizeof(ip_header);
}

void load_ip_data(u_int8_t* ip_buffer, u_int8_t* buf, int buflen)
{
	// int i = 0;
	// char ch;
	// while (i < len && (ch = fgetc(fp)) != EOF)
	// {
	// 	*(ip_buffer + i) = ch;
	// 	i++;
	// }
	// int read_length = fread(ip_buffer,1,len,fp);
	// if(read_length == 0) printf("There is no data\n");
	memcpy(ip_buffer, buf, buflen);
	ip_size_of_packet += buflen;
}

int is_same_lan(u_int8_t *local_ip, u_int8_t *destination_ip)//judge whether under the same lan 
{
	int i;
	for (i = 0; i < 4; i++)
	{
		if ((local_ip[i] & netmask[i]) != (destination_ip[i] & netmask[i]))
			return 0;
	}
	return 1;
}

int network_ipv4_send(u_int8_t* buf, int buflen, u_int8_t* target_ip)
{
	//get the size of file
	// int file_len;
	// fseek(fp, 0, SEEK_END);
	// file_len = ftell(fp);
	// rewind(fp);
	// printf("The file is %d bytes long.\n",file_len);
	//get how many fragments
	int number_of_fragment = (int)ceil(buflen*1.0 / MAX_IP_PACKET_SIZE);
	u_int16_t offset = 0;
	int ip_data_len;
	u_int16_t fragment_offset;
	while (number_of_fragment)
	{
		load_ip_header(target_ip);
		struct ip_header *ip_hdr = (struct ip_header *)ip_buffer;
		if (number_of_fragment == 1)//no need to fragment
		{
			fragment_offset = 0x0000;//16bits
			ip_data_len = buflen - offset;
		}
		else
		{
			fragment_offset = 0x2000;//allow the next fragment
			ip_data_len = MAX_IP_PACKET_SIZE;
		}

		fragment_offset |= ((offset / 8) & 0x0fff);
		ip_hdr->fragment_offset = htons(fragment_offset);

		//printf("%04x\n", ip_hdr->fragment_offset);
		ip_hdr->total_length = htons(ip_data_len + sizeof(ip_header));
		ip_hdr->check_sum = calculate_check_sum(ip_hdr, 60);
		//printf("%04x\n", ip_hdr->check_sum);
		load_ip_data(ip_buffer + sizeof(ip_header), buf, ip_data_len);

		//check if the target pc mac is in arp_table
		u_int8_t *destination_mac = is_existed_ip(ip_hdr->destination_ip);
		
		if (destination_mac == NULL)
		{
			int count = 0;
			printf("need to use arp protocol.\n");
			//check if the target pc and the local host is in the same lan
			// if (is_same_lan(local_ip, ip_hdr->destination_ip))//here exists some logical problem
            network_arp_send_request(ip_hdr->destination_ip, broadcast_mac);

			//wait for replying, get the destination mac
			struct pcap_pkthdr *pkt_hdr;
			u_int8_t *pkt_content;
			Sleep(2000);
			while(count < 3)
			{
				while (pcap_next_ex(handle, &pkt_hdr, (const u_char **)&pkt_content) != 0)
			    {
			    	printf("pcap next ex receive one packet!\n");
			    	destination_mac = NULL;
			    	struct ethernet_header *ethernet_hdr = (struct ethernet_header *)(pkt_content);
			    	//check if is acceptable packet
			    	if (ntohs(ethernet_hdr->ethernet_type) != ETHERNET_ARP)continue;
			    	int i;
			    	for (i = 0; i < 6; i++)
			    	{
			    		if (ethernet_hdr->destination_mac[i] != local_mac[i]) break;
			    	}
			    	if (i < 6)continue;
    
			    	switch (ntohs(ethernet_hdr->ethernet_type))
			    	{
			    	case ETHERNET_ARP:
			    		destination_mac = network_arp_recv(pkt_content + sizeof(struct ethernet_header));
			    		break;
			    	case ETHERNET_RARP:
			    		break;
			    	}
    
			    	if (destination_mac != NULL)
			    		break;
			    }
				count++;
			}
			if (destination_mac == NULL)
			{
				printf("Error: IP can not get destination mac!\n");
				return 0;
			}
		}
		
		//send the data to pool
		P(&ip_send_empty);
		P(&ip_send_mutex);
		printf("ip generate one fragment.\n");
		memcpy(ip_send_pool[ip_send_que_tail], ip_buffer, ip_size_of_packet);
		ip_send_data_size[ip_send_que_tail] = ip_size_of_packet;
        for(int i = 0; i < 6; i++)
		{
			ip_dmac[ip_send_que_tail][i] = destination_mac[i];
		}
		ip_send_que_tail = (ip_send_que_tail + 1) % MAX_QUE;
		V(&ip_send_mutex);
		V(&ip_send_full);

		offset += MAX_IP_PACKET_SIZE;
		// printf("number of left fragment is %d\n",number_of_fragment);
		number_of_fragment--;
		buf += MAX_IP_PACKET_SIZE;
	}
	//auto increase one
	ip_packet_id++;

	return SUCCESS;
}

DWORD WINAPI thread_ip_send(LPVOID pM)
{
	u_int8_t ip_send_buffer[MAX_DATA_SIZE];
	int ip_send_packet_size;
	u_int8_t ip_send_dmac[6];
	while(1)
	{
		P(&ip_send_full);
		P(&ip_send_mutex);
		printf("ip send one fragment.\n");
		ip_send_packet_size = ip_send_data_size[ip_send_que_head];
		memcpy(ip_send_buffer,ip_send_pool[ip_send_que_head],ip_send_packet_size);
		for(int i = 0; i < 6; i++)
		{
			ip_send_dmac[i] = ip_dmac[ip_send_que_head][i];
		}
		ip_send_que_head = (ip_send_que_head + 1) % MAX_QUE;
		V(&ip_send_mutex);
		V(&ip_send_empty);
		ethernet_send_packet(ip_send_buffer, ip_send_dmac, ETHERNET_IP, ip_send_packet_size);
	}
	system("pause");
	exit(0);
}

// DWORD WINAPI thread_read(LPVOID pM)
// {
// 	printf("start reading...\n");
// 	FILE *fp;
// 	fp = fopen("data.png", "rb");

// 	network_ipv4_send(ip_buffer, fp);

// 	fclose(fp);
// 	system("pause");
// 	exit(0);
// }

DWORD WINAPI init_ip_sender(LPVOID pM)
{
	printf("init_ip_sender!\n");
	init_ip_send_buffer();
	HANDLE th[1];
	th[0] = CreateThread(NULL,0,thread_ip_send,NULL,0,NULL);
	// th[1] = CreateThread(NULL,0,thread_read,NULL,0,NULL);
	WaitForMultipleObjects(1, th, TRUE, INFINITE);
	// for (int i = 0; i < 2; i++)
	// {
		CloseHandle(th[0]);
	// }
	system("pause");
	exit(0);
}


