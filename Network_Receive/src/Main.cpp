/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:02:57
 * @LastEditTime: 2022-01-08 16:25:37
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include "Header_Include.h"
#include "Ethernet_recv.h"
#include "Ethernet_send.h"
#include "Network_IPV4_recv.h"
#include "Network_IPV4_send.h"
#include "ARP_Cache_Table.h"
#include "UDP_recv_send.h"
#include "TCP_recv_send.h"

#define MAX_DATA_SIZE 65535

extern u_int8_t local_ip[4];
extern u_int16_t local_port;
extern struct Global_Param* server_gp;

u_int8_t receive_data[MAX_DATA_SIZE];

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
	th[2] = CreateThread(NULL,0,init_ip_receiver,NULL,0,NULL);      //init ip receiver
	th[3] = CreateThread(NULL,0,init_ip_sender,NULL,0,NULL);

	My_SOCKET* old_socket = mysocket(AF_INET, SOCK_STREAM, 0);
	socket_addr client_addr;

	

	listen(old_socket, 5);

	My_SOCKET* new_socket = accept(old_socket, &client_addr, sizeof(client_addr));

	int receive_length = recv(new_socket, (u_int8_t*)receive_data, 22775, 0);
	// int receive_length = recv(new_socket, (u_int8_t*)receive_data, 11, 0);

	printf("tcp server receive %d bytes data.\n", receive_length);
	// printf("The data is:\n");
	// printf("%s", receive_data);
	// for(int i = 0; i < 11; i++) printf("%c", receive_data[i]);


	FILE* fp = fopen("TCP_Receive.png","wb");

	fwrite(receive_data, 1, receive_length, fp);

	fclose(fp);

	closesocket(new_socket, server_gp);

	closesocket(old_socket);

	WaitForMultipleObjects(4, th, TRUE, INFINITE);//wait for all child threads to terminate
	for (int i = 0; i < 4; i++)
	{
		CloseHandle(th[i]);
	}
	return 0;
}