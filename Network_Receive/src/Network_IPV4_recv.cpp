/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:02:57
 * @LastEditTime: 2022-01-06 15:08:12
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */
#include "Network_IPV4_send.h"
#include "Network_IPV4_recv.h"
#include "UDP_recv_send.h"
#include "ICMP_recv_send.h"

#define MAX_DATA_SIZE 65535
#define MAX_QUE 50

u_int16_t ip_id = 0;
u_int16_t size_of_data = 0;

u_int8_t data_buffer[MAX_DATA_SIZE];

u_int8_t ip_recv_pool[MAX_QUE][MAX_DATA_SIZE];//define a ip_recv_pool as data buffer 
int ip_recv_data_size[MAX_QUE];//record the datasize of every ip data
u_int8_t data_type[MAX_QUE];
u_int8_t ip_src_address[MAX_QUE][4];
int ip_recv_que_head,ip_recv_que_tail;//use a queue to manage the data buffer above

int ip_recv_mutex = 1;
int ip_recv_empty = MAX_QUE;
int ip_recv_full = 0;

int previous = 0, current = 0;

extern u_int8_t local_ip[4];

/*
Thread1:if allow fragment, store to buffer until not allow,
Thread2:then store to file.
*/

void init_recv_data_buffer()//init queue
{
	ip_recv_que_head = 0;
	ip_recv_que_tail = 0;
}


int is_accept_ip_packet(struct ip_header *ip_hdr)
{
	int i;
	int flag = 0;
	for (i = 0; i < 4; i++)
	{
		if (ip_hdr->destination_ip[i] != local_ip[i])break;
	}

	if (i == 4)
	{
		flag = 1;
		printf("It's sended to my IP.\n");
	}

	for (i = 0; i < 4; i++)
	{
		if (ip_hdr->destination_ip[i] != 0xff)break;
	}
	if (i == 4)
	{
		flag = 1;
		printf("It's broadcast IP.\n");
	}

	if (!flag)
		return FAILURE;

	u_int16_t check_sum = calculate_check_sum((u_int8_t*)ip_hdr, 60);
	if (check_sum == 0xffff || check_sum == 0x0000)
	{
		// printf("No error in ip_header.\n");
		;
	}
	else
	{
		printf("Error in ip_header\n");
		return FAILURE;
	}

	return SUCCESS;
}

void load_data_to_buffer(u_int8_t *buffer, u_int8_t *ip_data, int len)
{
	int i;
	for (i = 0; i < len; i++)
	{
		*(buffer + i) = *(ip_data + i);
	}
}

// int load_data_to_file(u_int8_t *buffer, int len, FILE *fp)
// {
// 	int res = fwrite(buffer, sizeof(u_int8_t), len, fp);
// 	if (res != len)
// 	{
// 		printf("Write file error!\n");
// 		return 0;
// 	}
// 	fflush(fp);
// 	return SUCCESS;
// }

int network_ipv4_recv(u_int8_t *ip_buffer)
{
	struct ip_header *ip_hdr = (struct ip_header *)ip_buffer;
	int len = ntohs(ip_hdr->total_length) - sizeof(ip_header);//ip_header 60 Bytes

	//check the valid
	if (is_accept_ip_packet(ip_hdr) == FAILURE)
	{
		return FAILURE;
	}

	u_int16_t fragment;
	fragment = ntohs(ip_hdr->fragment_offset);

	int dural = 0;
	if (previous == 0)
	{
		previous = (int)time(NULL);
	}
	else
	{
		//get current time
		current = (int)time(NULL);
		dural = current - previous;
		printf("current time: %d previous time: %d\n", current, previous);
		//current time became previous
		previous = current;
	}

	//interval can not larger than 30s
	if (dural >= 30)
	{
		printf("Time Elapsed.\n");
		return FAILURE;
	}

	if ((fragment & 0x2000) && (ip_id == ip_hdr->id))//true means more fragment
	{
		load_data_to_buffer(data_buffer + size_of_data, ip_buffer + sizeof(ip_header), len);
		size_of_data += len;
		return SUCCESS;
	}
	else if (ip_id == ip_hdr->id)//means the last fragment or not fragmented
	{
		load_data_to_buffer(data_buffer + size_of_data, ip_buffer + sizeof(ip_header), len);
		size_of_data += len;

		P(&ip_recv_empty);
		P(&ip_recv_mutex);
		load_data_to_buffer(ip_recv_pool[ip_recv_que_tail], data_buffer, size_of_data);
		ip_recv_data_size[ip_recv_que_tail] = size_of_data;
		for(int i = 0; i < 4; i++)
		{
			ip_src_address[ip_recv_que_tail][i] = ip_hdr->source_ip[i];
		}
		data_type[ip_recv_que_tail] = ip_hdr->upper_protocol_type;
		ip_recv_que_tail = (ip_recv_que_tail + 1) % MAX_QUE;
		V(&ip_recv_mutex);
		V(&ip_recv_full);
		
		size_of_data = 0;//restore the value
		ip_id++;
	}
	else
	{
		printf("Lost packets.\n");//pass the last fragment, make move
		size_of_data = 0;
		ip_id++;
		return FAILURE;
	}
	
	//show a whole IP packet after building up all fragments 
	printf("-----------------IP Protocol-------------------\n");
	printf("IP version: %d\n", (ip_hdr->version_hdrlen & 0xf0));
	printf("Type of service: %02x\n", ip_hdr->type_of_service);
	printf("IP packet length: %d\n", len + sizeof(ip_header));
	printf("IP identification: %d\n", ip_hdr->id);
	printf("IP fragment & offset: %04x\n", ntohs(ip_hdr->fragment_offset));
	printf("IP time to live: %d\n", ip_hdr->time_to_live);
	printf("Upper protocol type: %02x\n", ip_hdr->upper_protocol_type);
	printf("Check sum: %04x\n", ip_hdr->check_sum);
	printf("Source IP: ");
	for (int i = 0; i < 4; i++)
	{
		if (i)printf(".");
		printf("%d", ip_hdr->source_ip[i]);
	}
	printf("\nDestination IP: ");
	for (int i = 0; i < 4; i++)
	{
		if (i)printf(".");
		printf("%d", ip_hdr->destination_ip[i]);
	}
	printf("\n");
	printf("-----------------End of IP Protocol---------------\n");
	return SUCCESS;
}

DWORD WINAPI thread_ip_handout(LPVOID pM)
{
	u_int8_t current_type;
	u_int8_t buffer[MAX_DATA_SIZE];
	u_int8_t source_ip[4];
	while(1)
	{
		P(&ip_recv_full);
		P(&ip_recv_mutex);
		memcpy(buffer, ip_recv_pool[ip_recv_que_head], ip_recv_data_size[ip_recv_que_head]);
		current_type = data_type[ip_recv_que_head];
		for(int i = 0; i < 4; i++)
		{
			source_ip[i] = ip_src_address[ip_recv_que_head][i];
		}
		ip_recv_que_head = (ip_recv_que_head + 1) % MAX_QUE;
		V(&ip_recv_mutex);
		V(&ip_recv_empty);
		
		switch (current_type)//this part shouldn't be in PV area
		{
		case IPPROTO_TCP:
			//transport_tcp_recv(buffer);
			break;
		case IPPROTO_UDP:
			transport_udp_recv(buffer, source_ip);
			break;
		case IPPROTO_ICMP:
			transport_icmp_recv(buffer, source_ip);
			break;
		}
	}
	exit(0);
}

DWORD WINAPI init_ip_receiver(LPVOID pM)
{
	init_recv_data_buffer();
	HANDLE th[1];
	th[0] = CreateThread(NULL,0,thread_ip_handout,NULL,0,NULL);
	WaitForMultipleObjects(1,th,TRUE,INFINITE);
	CloseHandle(th[0]);
	exit(0);
}