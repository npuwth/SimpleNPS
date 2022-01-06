/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:02:57
 * @LastEditTime: 2022-01-05 22:09:06
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

#define MAX_DATA_SIZE 65535

extern u_int8_t local_ip[4];
extern u_int16_t local_port;

int main()
{
	init_arp_table();
	output_arp_table();

	HANDLE th[4];
	th[0] = CreateThread(NULL,0,init_ethernet_receiver,NULL,0,NULL);//init ethernet receiver
	th[1] = CreateThread(NULL,0,init_ethernet_sender,NULL,0,NULL);  //init ethernet sender
	th[2] = CreateThread(NULL,0,init_ip_receiver,NULL,0,NULL);      //init ip receiver
	th[3] = CreateThread(NULL,0,init_ip_sender,NULL,0,NULL);

	// My_SOCKET* receive_socket = mysocket(AF_INET, SOCK_DGRAM, 0);
	// socket_addr client_addr;

	// u_int8_t receive_data[MAX_DATA_SIZE];

	// int receive_length = recvfrom(receive_socket, (u_int8_t*)receive_data, 65535, 0, &client_addr, sizeof(client_addr));

	// u_int8_t data2[] = "I have received the picture!";

	// sendto(receive_socket, (u_int8_t*)data2, sizeof(data2), 0, &client_addr, sizeof(client_addr));

	// closesocket(receive_socket);

	// // printf("The data is:\n");

	// // printf("%s", receive_data);
	// printf("udp receive %d bytes data\n", receive_length);

	// FILE* fp = fopen("UDP_Receive.png","wb");

	// fwrite(receive_data, 1, receive_length, fp);

	// fclose(fp);

	WaitForMultipleObjects(4, th, TRUE, INFINITE);//wait for all child threads to terminate
	for (int i = 0; i < 4; i++)
	{
		CloseHandle(th[i]);
	}
	return 0;
}