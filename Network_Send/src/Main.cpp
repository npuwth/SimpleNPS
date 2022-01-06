/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:05:07
 * @LastEditTime: 2022-01-05 22:14:03
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

#define MAX_DATA_SIZE 65535

extern u_int8_t target_ip[4];
extern u_int16_t target_port;

int main()
{
	init_arp_table();
	output_arp_table();

	HANDLE th[4];
	th[0] = CreateThread(NULL,0,init_ethernet_receiver,NULL,0,NULL);//init ethernet receiver
	th[1] = CreateThread(NULL,0,init_ethernet_sender,NULL,0,NULL);  //init ethernet sender
	th[2] = CreateThread(NULL,0,init_ip_receiver,NULL,0,NULL);
	th[3] = CreateThread(NULL,0,init_ip_sender,NULL,0,NULL);      //init ip sender

	// My_SOCKET* send_socket = mysocket(AF_INET, SOCK_DGRAM, 0);
	// socket_addr server_addr;
	// for(int i = 0; i < 4; i++) server_addr.sin_ip[i] = target_ip[i];
	// server_addr.sin_port = target_port;

	// u_int8_t data[MAX_DATA_SIZE];

	// u_int8_t receive_data2[100];

	// FILE* fp = fopen("data.png","rb");

	// int read_length = fread(data, 1, 27649, fp);

	// fclose(fp);

	// printf("read %d bytes data from file\n", read_length);

	// sendto(send_socket, (u_int8_t*)data, read_length, 0, &server_addr, sizeof(server_addr));

	// int receive_length2 = recvfrom(send_socket, receive_data2, 100, 0, &server_addr, sizeof(server_addr));

	// closesocket(send_socket);

	// printf("From server:%s", receive_data2);

	send_ICMP_echo_request(target_ip);

	WaitForMultipleObjects(4, th, TRUE, INFINITE);//wait for all child threads to terminate
	for (int i = 0; i < 4; i++)
	{
		CloseHandle(th[i]);
	}
	system("pause");
	return 0;
}