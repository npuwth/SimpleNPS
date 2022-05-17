/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:02:57
 * @LastEditTime: 2022-01-06 14:57:10
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include "Ethernet_send.h"
#include "Resource.h"
#include "Ethernet_recv.h"

#define MAX_SIZE 1518//max packet length
#define MAX_QUE 100//max queue length

u_int32_t size_of_packet = 0;
u_int8_t ethernet_send_buffer[MAX_SIZE];

u_int8_t ethernet_send_pool[MAX_QUE][MAX_SIZE];//define a ethernet_send_pool as packet buffer between generate and send
int ethernet_send_packet_size[MAX_QUE];//record packet size for thread_send
int ethernet_send_que_head,ethernet_send_que_tail;//use a queue to manage the packet buffer above

int ethernet_send_mutex = 1;//define some ethernet_send_mutex for P&V options
int ethernet_send_empty = MAX_QUE;
int ethernet_send_full = 0;

extern pcap_t *handle;//device handle in data link layer
extern u_int8_t local_mac[6];//defined in global_variable.cpp

void init_send_packet_buffer()//init queue
{
	ethernet_send_que_head = 0;
	ethernet_send_que_tail = 0;
}

void load_ethernet_header(u_int8_t *destination_mac, u_int16_t ethernet_type)
{
	size_of_packet = 0;
	struct ethernet_header *hdr = (struct ethernet_header *)ethernet_send_buffer;
	int i;
	for (i = 0; i < 6; i++)
	{
		hdr->destination_mac[i] = destination_mac[i];
		hdr->source_mac[i] = local_mac[i];
	}

	hdr->ethernet_type = htons(ethernet_type);//reverse byte order
	size_of_packet += sizeof(struct ethernet_header);
}

void load_ethernet_data(u_int8_t *buffer, u_int8_t *upper_buffer, int len)
{
	if (len > 1500)
	{
		printf("IP buffer is too large. So we stop the procedure.");
		return;
	}

	int i;
	for (i = 0; i < len; i++)
	{
		*(buffer + i) = *(upper_buffer + i);
	}

	//add a serial 0 at the end
	while (len < 46)
	{
		*(buffer + len) = 0;
		len++;
	}
	// printf("buffer address:%p\n", buffer);
	u_int32_t crc = calculate_crc((u_int8_t *)buffer, len);

	*((u_int32_t *)(buffer + len)) = crc;
	size_of_packet += (len + 4);
	// printf("src crc:%08x\n", crc);
}

int ethernet_send_packet(u_int8_t *upper_buffer, u_int8_t *destination_mac, u_int16_t ethernet_type, int ethernet_upper_len)
{
	load_ethernet_header(destination_mac, ethernet_type);
	load_ethernet_data(ethernet_send_buffer + sizeof(struct ethernet_header), upper_buffer, ethernet_upper_len);
	
	P(&ethernet_send_empty);
	P(&ethernet_send_mutex);
	printf("ethernet generate one packet.\n");
	memcpy(ethernet_send_pool[ethernet_send_que_tail], ethernet_send_buffer, size_of_packet);
	ethernet_send_packet_size[ethernet_send_que_tail] = size_of_packet;
	ethernet_send_que_tail = (ethernet_send_que_tail + 1) % MAX_QUE;
	V(&ethernet_send_mutex);
	V(&ethernet_send_full);
	// printf("generate OK\n");
	return SUCCESS;
}

DWORD WINAPI thread_send(LPVOID pM)
{
	u_int8_t send_buffer[MAX_SIZE];
	int send_packet_size;
	open_device();
	while(1)
	{
		P(&ethernet_send_full);
		P(&ethernet_send_mutex);
		printf("ethernet send one packet.\n");
		send_packet_size = ethernet_send_packet_size[ethernet_send_que_head];
		// printf("send_packet_size:%d\n", send_packet_size);
		memcpy(send_buffer, ethernet_send_pool[ethernet_send_que_head], send_packet_size);
		// for(int i = 0; i < send_packet_size; i++) printf("%02x ", send_buffer[i]);
		// printf("aaa\n");
		ethernet_send_que_head = (ethernet_send_que_head + 1) % MAX_QUE;
		V(&ethernet_send_mutex);
		V(&ethernet_send_empty);
		if(pcap_sendpacket(handle,send_buffer,send_packet_size)!=0) 
		printf("Error: pcap send error!!!\n");//better copy than directly use ethernet_send_pool
	}
	system("pause");
	exit(0);
}

DWORD WINAPI init_ethernet_sender(LPVOID pM)
{
	printf("init_ethernet_sender!\n");
	init_send_packet_buffer();
	HANDLE th[1];
	th[0] = CreateThread(NULL,0,thread_send,NULL,0,NULL);
	WaitForMultipleObjects(1, th, TRUE, INFINITE);
	CloseHandle(th[0]);
	system("pause");
	exit(0);
}