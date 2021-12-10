/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:02:57
 * @LastEditTime: 2021-12-10 15:11:07
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include "ARP_Cache_Table.h"
#include "Resource.h"

extern u_int8_t local_mac[6];//defined in global variable
extern u_int8_t local_ip[4];
extern u_int8_t gateway_ip[4];
extern u_int8_t netmask[4];
extern u_int8_t dns_server_ip[4];//currently unused
extern u_int8_t dhcp_server_ip[4];//currently unused

arp_table_header arp_table;//organized as single linked list

struct arp_node* make_arp_node(u_int8_t *ip, u_int8_t *mac, int state)//make an arp node with ip and mac and state
{
	int i;
	struct arp_node *node = (struct arp_node *)malloc(sizeof(struct arp_node));
	for (i = 0; i < 4; i++)
	{
		node->ip[i] = ip[i];
	}

	for (i = 0; i < 6; i++)
	{
		node->mac[i] = mac[i];
	}
	node->state = state;
	node->next = NULL;
	return node;
}

void init_arp_table()
{
	struct arp_node *node;
	node = make_arp_node(local_ip, local_mac, STATIC_STATE);//initialize local node

	arp_table.queue = node;
	arp_table.head = node;
	arp_table.tail = node;
}

void insert_arp_node(struct arp_node *node)
{
	if (!is_existed_ip(node->ip))//if not existed then insert, if existed then should use update_arp_node
	{
		arp_table.tail->next = node;
		arp_table.tail = node;
	}
}

int delete_arp_node(struct arp_node *node)
{
	struct arp_node *pre = arp_table.head;
	struct arp_node *p = pre->next;
	int flag = 1;
	while (p != NULL)
	{
		int i;
		flag = 1;
		for (i = 0; i < 4; i++)
		{
			if (node->ip[i] != p->ip[i])
			{
				flag = 0;
				break;
			}
		}

		for (i = 0; i < 6; i++)
		{
			if (node->mac[i] != p->mac[i])
			{
				flag = 0;
				break;
			}
		}

		if (flag)//find the node waiting to be deleted
		{
			pre->next = p->next;
			free(p);
			break;
		}

		pre = p;
		p = p->next;//traverse the list
	}
	if (flag)
	{
		printf("delete arp node succeed!!!\n");
		return 1;
	}
	else
	{
		printf("Failed delete\n");
		return 0;
	}
}

u_int8_t* is_existed_ip(u_int8_t *destination_ip)//use ip to find mac
{
	struct arp_node *p = arp_table.head;
	int flag = 1;
	while (p != NULL)
	{
		int i;
		flag = 1;
		for (i = 0; i < 4; i++)
		{
			if (p->ip[i] != destination_ip[i])
			{
				flag = 0;
				break;
			}
		}

		if (flag)
		{
			return p->mac;
		}
		p = p->next;
	}
	return NULL;
}

int update_arp_node(struct arp_node *node)
{
	u_int8_t *mac = is_existed_ip(node->ip);
	if (mac)
	{
		int i;
		for (i = 0; i < 6; i++)
		{
			mac[i] = node->mac[i];
		}
		printf("Update succeed.\n");
		return 1;
	}
	else
	{
		printf("Update failed.\n");
		return 0;
	}
}

void output_arp_table()//output current arp_table
{
	printf("current arp table:\n");
	struct arp_node *p = arp_table.head;
	while (p != NULL)
	{
		int i;
		for (i = 0; i < 4; i++)
		{
			if (i)printf(".");
			printf("%d", p->ip[i]);
		}
		printf("\t");
		for (i = 0; i < 6; i++)
		{
			if (i)printf("-");
			printf("%02x", p->mac[i]);
		}
		printf("\n");

		p = p->next;
	}
	printf("---------------------------------\n");
}

