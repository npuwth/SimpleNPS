/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:05:07
 * @LastEditTime: 2022-01-08 16:24:53
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include "Header_Include.h"
#include "Ethernet_recv.h"
#include "Ethernet_send.h"
#include "Network_IPV4_send.h"
#include "Network_IPV4_recv.h"
#include "ARP_Cache_Table.h"
#include "UDP_recv_send.h"
#include "ICMP_recv_send.h"
#include "TCP_recv_send.h"

#define MAX_DATA_SIZE 65535

extern u_int8_t target_ip[4];
extern u_int16_t target_port;
extern struct Global_Param* client_gp;

// u_int8_t data[MAX_DATA_SIZE] = "helloworld";
u_int8_t data[MAX_DATA_SIZE];


int main()
{
	init_arp_table();
	output_arp_table();
	init_tcp_send_buffer();
	init_tcp_recv_buffer();
	init_tcp_req_que();

	HANDLE th[4];
	th[0] = CreateThread(NULL,0,init_ethernet_receiver,NULL,0,NULL);//init ethernet receiver
	th[1] = CreateThread(NULL,0,init_ethernet_sender,NULL,0,NULL);  //init ethernet sender
	th[2] = CreateThread(NULL,0,init_ip_receiver,NULL,0,NULL);
	th[3] = CreateThread(NULL,0,init_ip_sender,NULL,0,NULL);      //init ip sender

	My_SOCKET* send_socket = mysocket(AF_INET, SOCK_STREAM, 0);
	socket_addr server_addr;
	for(int i = 0; i < 4; i++) server_addr.sin_ip[i] = target_ip[i];
	server_addr.sin_port = target_port;

	

	FILE* fp = fopen("data.png","rb");

	int read_length = fread(data, 1, 22775, fp);

	fclose(fp);

	printf("read %d bytes data from file\n", read_length);

	connect(send_socket, &server_addr, sizeof(server_addr));

	int send_length = send(send_socket, data, 22775, 0);
	// int send_length = send(send_socket, data, 11, 0);

	printf("send %d bytes data from client.\n", send_length);

	closesocket(send_socket, client_gp);

	WaitForMultipleObjects(4, th, TRUE, INFINITE);//wait for all child threads to terminate
	for (int i = 0; i < 4; i++)
	{
		CloseHandle(th[i]);
	}
	system("pause");
	return 0;
}