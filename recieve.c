//Heorhii Karpenko, 312372

#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void set_current_time_in(long* timer);
extern long tsorig[3];
extern long tsrecv; 

struct icmp* get_icmp_from_ip(u_int8_t* buffer){
    struct ip* ip_header = (struct ip*) buffer;
    ssize_t	ip_header_len = 4 * ip_header->ip_hl;

    u_int8_t* icmp_packet = buffer + ip_header_len;
    return (struct icmp*)icmp_packet;
}

void process_valid_package(int count, int ttl, char* reciever, char* ip_buffer1, char* ip_buffer2, struct sockaddr_in* sender){
    char sender_ip_str[20];

    if(count == 0){
        inet_ntop(AF_INET, &(sender->sin_addr), sender_ip_str, sizeof(sender_ip_str));
        strncpy(ip_buffer1, sender_ip_str, 20);
        printf ("%d. %s ", ttl, sender_ip_str);
        strncpy(reciever, sender_ip_str, 20);
    }
    else if(count == 1){
        inet_ntop(AF_INET, &(sender->sin_addr), sender_ip_str, sizeof(sender_ip_str));

        if( strcmp(ip_buffer1,sender_ip_str) != 0 ){
            strncpy(ip_buffer2, sender_ip_str, 20);
            printf ("%s ", sender_ip_str);
        }
    }
    else if(count == 2){
        inet_ntop(AF_INET, &(sender->sin_addr), sender_ip_str, sizeof(sender_ip_str));

        if( strcmp(ip_buffer1,sender_ip_str) != 0 && strcmp(ip_buffer2,sender_ip_str) != 0 )
            printf ("%s ", sender_ip_str);
    }
}

int recieve_n_packages(int n, int sockfd, int PID, int ttl, char* reciever, int* pack_count, long* avg_time){
    struct sockaddr_in 	sender;	
    socklen_t 			sender_len = sizeof(sender);
    u_int8_t 			buffer[IP_MAXPACKET];

    int count = *pack_count;

    char sender_ip_buffer1[20]; 
    char sender_ip_buffer2[20]; 

    for(int i=0;i<n;i++){
        ssize_t packet_len = recvfrom (sockfd, buffer, IP_MAXPACKET, 0, (struct sockaddr*)&sender, &sender_len);

        if (packet_len < 0) {
            fprintf(stderr, "select error: %s\n", strerror(errno)); 
            return -1;
        }

        struct icmp* icmp_header = get_icmp_from_ip(buffer);

        if( icmp_header->icmp_type == 11)
            icmp_header = get_icmp_from_ip((uint8_t*)&(icmp_header->icmp_ip));

        int rcv_id = icmp_header->icmp_seq - 1;
        int rcv_ttl = rcv_id/3 + 1;

        //validation PID and seq_number
        if(icmp_header->icmp_id != PID || rcv_ttl != ttl )
            continue;
        
        process_valid_package(count, ttl, reciever, sender_ip_buffer1, sender_ip_buffer2, &sender);
        
        count++;

        set_current_time_in(&tsrecv);
        if(tsrecv < tsorig[rcv_id%3])
            tsrecv+=1000;
        *avg_time += tsrecv - tsorig[rcv_id%3];
    }

    *pack_count = count;
    return 0;  
}

int recieve(int sockfd, int PID, int ttl, char* reciever){
    int return_code = 0;

    fd_set descriptors;
    FD_ZERO (&descriptors);
    FD_SET (sockfd, &descriptors);

    struct timeval tv; 
    tv.tv_sec = 1; 
    tv.tv_usec = 0;

    int ready = select(sockfd+1, &descriptors, NULL, NULL, &tv);

    if (ready < 0) {
        fprintf(stderr, "select error: %s\n", strerror(errno)); 
        return -1;
    }

    int expected_pack_count = 0;
    long avg_time = 0;

    while(ready > 0){
        return_code = recieve_n_packages(ready, sockfd, PID, ttl, reciever, &expected_pack_count, &avg_time);
        if (return_code<0)
            return return_code;
        ready = select(sockfd+1, &descriptors, NULL, NULL, &tv);
    }
    
    if(expected_pack_count==0)
        printf ("*\n");
    else if(expected_pack_count < 3)
        printf ("???\n");
    else
        printf("%ld ms\n", avg_time/3);
    
   return 0;
}