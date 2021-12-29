/*
 * @Author: npuwth
 * @Date: 2021-11-26 15:02:57
 * @LastEditTime: 2021-12-29 22:26:25
 * @LastEditors: npuwth
 * @Copyright 2021
 * @Description: Network Experiment
 */

#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>
#include<math.h>

#define HAVE_REMOTE
#define WPCAP
#include<pcap.h>
#include<WinSock2.h>

#pragma warning(disable:4996)

#define SUCCESS 1
#define FAILURE 0
#define u_int16_t unsigned short

//define P&V options
// inline void P(int* s)
// {
//     while ((*s) <= 0);
//     (*s)--;
// }

// inline void V(int* s)
// {
//     (*s)++;
// }

void P(int* s);
void V(int* s);