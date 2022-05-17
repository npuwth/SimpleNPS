/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:02:57
 * @LastEditTime: 2022-01-06 14:57:59
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include "Ethernet_recv.h"
#include "Network_IPV4_send.h"
#include "Network_IPV4_recv.h"
#include "Network_ARP_recv.h"

#define MAX_SIZE 1500
#define MAX_QUE 100

u_int32_t crc32_table[256];
u_int32_t packet_number = 1;
extern u_int8_t local_mac[6];
extern pcap_t *handle;

u_int8_t ethernet_recv_pool[MAX_QUE][MAX_SIZE];//define a ethernet_recv_pool as packet buffer between receive and write
int ethernet_recv_packet_size[MAX_QUE];//record the packetsize of every packet for thread_handout
u_int16_t packet_type[MAX_QUE];
int ethernet_recv_que_head,ethernet_recv_que_tail;//use a queue to manage the packet buffer above

int ethernet_recv_mutex = 1;
int ethernet_recv_empty = MAX_QUE;
int ethernet_recv_full = 0;

void init_recv_packet_buffer()//init queue
{
	ethernet_recv_que_head = 0;
	ethernet_recv_que_tail = 0;
}

void generate_crc32_table()//generate crc table
{
	int i, j;
	u_int32_t crc;
	for (i = 0; i < 256; i++)
	{
		crc = i;
		for (j = 0; j < 8; j++)
		{
			if (crc & 1)
				crc = (crc >> 1) ^ 0xEDB88320;
			else
				crc >>= 1;
		}
		crc32_table[i] = crc;
	}
}

u_int32_t calculate_crc(u_int8_t *buffer, int len)//calculate crc
{
	int i;
	u_int32_t crc;
	//very strange here, if i note this printf, the result will be wrong!!!
	// for(int i = 0; i < len; i++) printf("%02x ", buffer[i]);
	// printf("\n");
	crc = 0xffffffff;
	for (i = 0; i < len; i++)
	{
		crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ buffer[i]];
	}
	crc ^= 0xffffffff;
	return crc;
}

int is_accept_ethernet_packet(u_int8_t *packet_content, int len)
{
	struct ethernet_header *ethernet_hdr = (struct ethernet_header *)packet_content;//get ethernet header
	int i,flag = 0;
	for (i = 0; i < 6; i++)
	{
		if (ethernet_hdr->destination_mac[i] != 0xff)break;//judge equal broadcast mac address
	}
	if (i == 6)
	{
		flag = 1;
		printf("It's broadcast packet.\n");
	}

	for (i = 0; i < 6; i++)
	{
		if (ethernet_hdr->destination_mac[i] != local_mac[i])break;//judge equal my mac address
	}
	if (i == 6)
	{
		flag = 1;
		printf("It's sended to my pc.\n");
	}
	if (!flag)//if none of the conditions meeted, then return 0 (and it means failure)
		return FAILURE;

	//generate_crc32_table();
	//crc match
	u_int32_t crc = calculate_crc((u_int8_t *)(packet_content + sizeof(ethernet_header)), len - 4 - sizeof(ethernet_header));
	if (crc != *((u_int32_t *)(packet_content + len - 4)))
	{
		printf("The data has changed.\n");//calculate crc
		return FAILURE;
	}
	return SUCCESS;
}

void output_mac(u_int8_t mac[6])//output mac address to screen
{
	for (int i = 0; i < 6; i++)
	{
		if (i)printf("-");
		printf("%02x", mac[i]);
	}
	printf("\n");
}

//callback function when receiving packets
void ethernet_protocol_packet_callback(u_char *argument, const struct pcap_pkthdr *packet_header, const u_char *packet_content)
{
	int len = packet_header->len;
	if (is_accept_ethernet_packet((u_int8_t *)packet_content, len)==FAILURE)//judge whether the packet is "right"
	{
		return;
	}

	struct ethernet_header *ethernet_hdr = (struct ethernet_header *)packet_content;
	u_int16_t ethernet_type = ntohs(ethernet_hdr->ethernet_type);//reverse byte order
	
	printf("--------------------------Ethernet Protocol------------------------\n");
	printf("Capture %d packet\n", packet_number++);
	printf("Capture time: %d %d\n", packet_header->ts.tv_sec, packet_header->ts.tv_usec);
	printf("Packet length: %d\n", packet_header->len);

	printf("Ethernet type:  %04x\n", ethernet_type);
	printf("MAC source address: ");
	output_mac(ethernet_hdr->source_mac);
	printf("MAC destination address: ");
	output_mac(ethernet_hdr->destination_mac);
	printf("-------------------End of Ethernet Protocol----------------\n");

	
	int k = 0;//ethernet_recv_packet_size
	for (u_int8_t* p = (u_int8_t*)(packet_content + sizeof(ethernet_header)); p != (u_int8_t*)(packet_content + packet_header->len - 4); p++)
	{
		// ethernet_recv_pool[ethernet_recv_que_tail][k] = *p;//put data into ethernet_recv_pool
		k++;
	}
	
	P(&ethernet_recv_empty);
	P(&ethernet_recv_mutex);
	printf("ethernet receive one packet.\n");
	memcpy(ethernet_recv_pool[ethernet_recv_que_tail], (u_int8_t*)(packet_content + sizeof(ethernet_header)), k);
	ethernet_recv_packet_size[ethernet_recv_que_tail] = k;//record packet size
	packet_type[ethernet_recv_que_tail] = ethernet_type;
	//printf("packet size: %d\n", k);
	ethernet_recv_que_tail = (ethernet_recv_que_tail + 1) % MAX_QUE;
	V(&ethernet_recv_mutex);
	V(&ethernet_recv_full);
}

DWORD WINAPI thread_receive(LPVOID pM)
{
	pcap_loop(handle, NULL, ethernet_protocol_packet_callback, NULL);
	pcap_close(handle);
	exit(0);
}

DWORD WINAPI thread_handout(LPVOID pM)
{
	u_int16_t current_type;
	u_int8_t upper_buffer[MAX_SIZE];
	while (1)
	{
		P(&ethernet_recv_full);
		P(&ethernet_recv_mutex);
		printf("ethernet handout one packet.\n");
		memcpy(upper_buffer, ethernet_recv_pool[ethernet_recv_que_head], ethernet_recv_packet_size[ethernet_recv_que_head]);
		// u_int8_t* upper_buffer = ethernet_recv_pool[ethernet_recv_que_head];//copy to a buffer instead of directly use it !!!
		current_type = packet_type[ethernet_recv_que_head];
		ethernet_recv_que_head = (ethernet_recv_que_head + 1) % MAX_QUE;
		V(&ethernet_recv_mutex);
		V(&ethernet_recv_empty);

		switch (current_type)//deliver to different upper protocols //this part shouldn't be in the PV area
	    {
	        case 0x0800:
	        	printf("Upper layer protocol: IPV4\n");
	        	network_ipv4_recv(upper_buffer);//defined in Network_IPV4_recv.h
	        	break;
	        case 0x0806:
	        	printf("Upper layer protocol: ARP\n");
	        	network_arp_recv(upper_buffer);//defined in Network_ARP_recv.h
	        	break;
	        case 0x8035:
	        	printf("Upper layer protocol: RARP\n");
	        	//network_rarp_recv();
	        	break;
	        case 0x814c:
	        	printf("Upper layer protocol: SNMP\n");
	        	//network_snmp_recv();
	        	break;
	        case 0x8137:
	        	printf("Upper layer protocol: IPX(Internet Packet Exchange)\n");
	        	//network_ipx_recv();
	        	break;
	        case 0x86DD:
	        	printf("Upper layer protocol: IPV6\n");
	        	//network_ipv6_recv();
	        	break;
	        case 0x880B:
	        	printf("Upper layer protocol: PPP\n");
	        	//network_ppp_recv();
	        	break;
	        default:break;
	    }
	}
	exit(0);
}

void open_device()//use winpcap to open network device
{
	generate_crc32_table();
	char *device;
	char error_buffer[PCAP_ERRBUF_SIZE];

	device = pcap_lookupdev(error_buffer);

	handle = pcap_open_live(device, 65536, 1, 1000, error_buffer);//1
}

DWORD WINAPI init_ethernet_receiver(LPVOID pM)
{
	open_device();
	init_recv_packet_buffer();
	HANDLE th[2];
	th[0] = CreateThread(NULL,0,thread_receive,NULL,0,NULL);
	th[1] = CreateThread(NULL,0,thread_handout,NULL,0,NULL);
	WaitForMultipleObjects(2, th, TRUE, INFINITE);
	for (int i = 0; i < 2; i++)
	{
		CloseHandle(th[i]);
	}
	pcap_close(handle);
	exit(0);
}