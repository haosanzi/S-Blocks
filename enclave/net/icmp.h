#ifndef ICMP_H
#define ICMP_H
#include "../include/config.h"
/* most icmp request types: ICMP_UNREACH, ICMP_SOURCEQUENCH, ICMP_TIMXCEED */
struct click_icmp {
    uint8_t icmp_type;      /* 0     ICMP type (see below)       */
    uint8_t icmp_code;      /* 1     ICMP code (see below)       */
    uint16_t    icmp_cksum;     /* 2-3   checksum            */
    uint32_t    padding;        /* 4-7   should be zero          */
    /* followed by original IP header and initial portion of data */
};
/* ICMP type definitions and (indented) code definitions */
#define ICMP_ECHOREPLY      0       /* echo reply            */
#define ICMP_UNREACH        3       /* dest unreachable, codes:  */
#define   ICMP_UNREACH_NET      0   /*   bad net             */
#define   ICMP_UNREACH_HOST     1   /*   bad host            */
#define   ICMP_UNREACH_PROTOCOL     2   /*   bad protocol        */
#define   ICMP_UNREACH_PORT     3   /*   bad port            */
#define   ICMP_UNREACH_NEEDFRAG     4   /*   IP_DF caused drop       */
#define   ICMP_UNREACH_SRCFAIL      5   /*   src route failed        */
#define   ICMP_UNREACH_NET_UNKNOWN  6   /*   unknown net         */
#define   ICMP_UNREACH_HOST_UNKNOWN 7   /*   unknown host        */
#define   ICMP_UNREACH_ISOLATED     8   /*   src host isolated       */
#define   ICMP_UNREACH_NET_PROHIB   9   /*   net prohibited access   */
#define   ICMP_UNREACH_HOST_PROHIB  10  /*   host prohibited access  */
#define   ICMP_UNREACH_TOSNET       11  /*   bad tos for net         */
#define   ICMP_UNREACH_TOSHOST      12  /*   bad tos for host        */
#define   ICMP_UNREACH_FILTER_PROHIB    13  /*   admin prohib        */
#define   ICMP_UNREACH_HOST_PRECEDENCE  14  /*   host prec violation     */
#define   ICMP_UNREACH_PRECEDENCE_CUTOFF 15 /*   prec cutoff         */
#define ICMP_SOURCEQUENCH   4       /* packet lost, slow down    */
#define ICMP_REDIRECT       5       /* shorter route, codes:     */
#define   ICMP_REDIRECT_NET     0   /*   for network         */
#define   ICMP_REDIRECT_HOST        1   /*   for host            */
#define   ICMP_REDIRECT_TOSNET      2   /*   for tos and net         */
#define   ICMP_REDIRECT_TOSHOST     3   /*   for tos and host        */
#define ICMP_ECHO       8       /* echo service          */
#define ICMP_ROUTERADVERT   9       /* router advertisement      */
#define ICMP_ROUTERSOLICIT  10      /* router solicitation       */
#define ICMP_TIMXCEED       11      /* time exceeded, code:      */
#define   ICMP_TIMXCEED_TRANSIT     0   /*   ttl==0 in transit       */
#define   ICMP_TIMXCEED_REASSEMBLY  1   /*   ttl==0 in reassembly    */
#define ICMP_PARAMPROB      12      /* ip header bad         */
#define   ICMP_PARAMPROB_ERRATPTR   0   /*   error at param ptr      */
#define   ICMP_PARAMPROB_OPTABSENT  1   /*   req. opt. absent        */
#define   ICMP_PARAMPROB_LENGTH     2   /*   bad length          */
#define ICMP_TSTAMP     13      /* timestamp request         */
#define ICMP_TSTAMPREPLY    14      /* timestamp reply       */
#define ICMP_IREQ       15      /* information request       */
#define ICMP_IREQREPLY      16      /* information reply         */
#define ICMP_MASKREQ        17      /* address mask request      */
#define ICMP_MASKREQREPLY   18      /* address mask reply        */
#endif