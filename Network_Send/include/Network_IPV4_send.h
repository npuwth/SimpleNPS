/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:05:07
 * @LastEditTime: 2022-01-03 13:41:52
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include "Header_Include.h"

struct ip_header
{
	u_int8_t version_hdrlen;// default IP version: ipv4, header_length: 60bytes
	u_int8_t type_of_service;//
	u_int16_t total_length;//
	u_int16_t id;			//identification
	u_int16_t fragment_offset;//packet maybe need to be fraged. 
	u_int8_t time_to_live;
	u_int8_t upper_protocol_type;
	u_int16_t check_sum;

	u_int8_t source_ip[4];   
	u_int8_t destination_ip[4];

	u_int8_t optional[40];//40 bytes is optional

};

u_int16_t calculate_check_sum(ip_header *ip_hdr, int len);

void load_ip_header(u_int8_t *ip_buffer);
void load_ip_data(u_int8_t* ip_buffer, u_int8_t* buf, int buflen);

int is_same_lan(u_int8_t *local_ip, u_int8_t *destination_ip);
/*
send ip packet
call ethernet function to make a complete packet
*/
int network_ipv4_send(u_int8_t* buf, int buflen, u_int8_t* target_ip, u_int8_t upper_protocol_type);

void init_ip_send_buffer();

DWORD WINAPI thread_ip_send(LPVOID pM);
DWORD WINAPI init_ip_sender(LPVOID pM);