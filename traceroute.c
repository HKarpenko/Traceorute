#include <netinet/ip.h>
#include <arpa/inet.h>
#include "ip_icmp.h"
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

struct timeval	tvorig, tvrecv;	/* originate & receive timeval structs */
long		tsorig, tsrecv;	/* originate & receive timestamps */


int recieve(int sockfd, int PID, int ttl){
    struct sockaddr_in 	sender;	
    socklen_t 			sender_len = sizeof(sender);
    u_int8_t 			buffer[IP_MAXPACKET];
    int seconds = 1;

    fd_set descriptors;
    FD_ZERO (&descriptors);
    FD_SET (sockfd, &descriptors);

    struct timeval tv; 
    tv.tv_sec = seconds; 
    tv.tv_usec = 0;

    int ready = select(sockfd+1, &descriptors, NULL, NULL, &tv);

    if (ready < 0) {
        fprintf(stderr, "select error: %s\n", strerror(errno)); 
        return EXIT_FAILURE;
    }

    int expected_pack_count = 0;
    long avgTime = 0;

    while(ready > 0){
        for(int i=0;i<ready;i++){
            ssize_t packet_len = recvfrom (sockfd, buffer, IP_MAXPACKET, 0, (struct sockaddr*)&sender, &sender_len);

            if (packet_len < 0) {
                fprintf(stderr, "select error: %s\n", strerror(errno)); 
                return EXIT_FAILURE;
            }

            struct ip* ip_header = (struct ip*) buffer;
            ssize_t	ip_header_len = 4 * ip_header->ip_hl;

            u_int8_t* icmp_packet = buffer + ip_header_len;
            struct icmp* icmp_header = (struct icmp*) icmp_packet;

            if( icmp_header->icmp_type == 11){

                ip_header = &(icmp_header->icmp_ip);
                ip_header_len = 4 * ip_header->ip_hl;

                icmp_packet = (u_int8_t*)ip_header + ip_header_len;
                icmp_header = (struct icmp*)icmp_packet;
            }

            int rcv_ttl = (icmp_header->icmp_seq - 1)/3 + 1;

            if(icmp_header->icmp_id != PID || rcv_ttl != ttl )
                continue;
            
            if(expected_pack_count == 0){
                char sender_ip_str[20]; 
                inet_ntop(AF_INET, &(sender.sin_addr), sender_ip_str, sizeof(sender_ip_str));
                printf ("%d. %s ", ttl, sender_ip_str);
            }
            expected_pack_count++;

            gettimeofday(&tvorig, (struct timezone *)NULL);
            tsorig = (tvorig.tv_sec % (24*60*60)) * 1000 + tvorig.tv_usec / 1000;
            tsrecv = ntohl(icmp_header->icmp_otime);
            avgTime += tsorig - tsrecv;
        }
        ready = select (sockfd+1, &descriptors, NULL, NULL, &tv);
    }
    
    if(expected_pack_count==0)
        printf ("*\n");
    else if(expected_pack_count < 3)
        printf ("???\n");
    else
        printf("%ld ms\n",avgTime/3);
    
   return 0;
}

u_int16_t compute_icmp_checksum (const void *buff, int length)
{
	u_int32_t sum;
	const u_int16_t* ptr = buff;
	assert (length % 2 == 0);
	for (sum = 0; length > 0; length -= 2)
		sum += *ptr++;
	sum = (sum >> 16) + (sum & 0xffff);
	return (u_int16_t)(~(sum + (sum >> 16)));
}

int sendPack(int PID, int seq_number, int sockfd, char* adres_ip, int ttl){
    struct icmp header;
    header.icmp_type = ICMP_ECHO;
    header.icmp_code = 0;
    header.icmp_hun.ih_idseq.icd_id = PID;
    header.icmp_hun.ih_idseq.icd_seq = seq_number;

    gettimeofday(&tvorig, (struct timezone *)NULL);
    tsorig = (tvorig.tv_sec % (24*60*60)) * 1000 + tvorig.tv_usec / 1000;
	header.icmp_otime =  htonl(tsorig);

    header.icmp_cksum = 0;
    header.icmp_cksum = compute_icmp_checksum (
    (u_int16_t*)&header, sizeof(header));

    struct sockaddr_in recipient;
    bzero (&recipient, sizeof(recipient));
    recipient.sin_family = AF_INET;
    inet_pton(AF_INET, adres_ip, &recipient.sin_addr);

    setsockopt (sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));

    int bytes = sendto (
        sockfd,
        &header,
        sizeof(header),
        0,
        (struct sockaddr*)&recipient,
        sizeof(recipient)
    );
    if (bytes < 0)  {
        fprintf(stderr, "sendto error: %s\n", strerror(errno)); 
        return EXIT_FAILURE;
    }

    return 0;
}


int main(int argc, char *argv[])
{
    int PID = getpid();
    if(argc < 2)
        return -1;
    char* adres_ip = argv[1];

	int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) {
		fprintf(stderr, "socket error: %s\n", strerror(errno)); 
		return EXIT_FAILURE;
	}
    for(int ttl=1;ttl<=30;ttl++){
        for(int i=1;i<=3;i++)
            sendPack(PID, (ttl-1)*3+i, sockfd, adres_ip, ttl);

        recieve(sockfd, PID, ttl);
    }

	return EXIT_SUCCESS;
}