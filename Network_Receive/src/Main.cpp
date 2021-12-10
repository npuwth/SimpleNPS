/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:02:57
 * @LastEditTime: 2021-11-27 19:06:29
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include "Header_Include.h"
#include "Ethernet_recv.h"
#include "Ethernet_send.h"
#include "Network_IPV4_recv.h"
#include "ARP_Cache_Table.h"

int main()
{
	init_arp_table();
	output_arp_table();

	HANDLE th[3];
	th[0] = CreateThread(NULL,0,init_ethernet_receiver,NULL,0,NULL);//init ethernet receiver
	th[1] = CreateThread(NULL,0,init_ethernet_sender,NULL,0,NULL);  //init ethernet sender
	th[2] = CreateThread(NULL,0,init_ip_receiver,NULL,0,NULL);      //init ip receiver

	WaitForMultipleObjects(3, th, TRUE, INFINITE);//wait for all child threads to terminate
	for (int i = 0; i < 3; i++)
	{
		CloseHandle(th[i]);
	}
	return 0;
}