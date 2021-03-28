//Heorhii Karpenko, 312372

#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

void set_current_time_in(long* timer);

extern long tsorig[3];

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

struct icmp get_ready_icmp_header(int PID, int seq_number){
    struct icmp header;
    header.icmp_type = ICMP_ECHO;
    header.icmp_code = 0;
    header.icmp_hun.ih_idseq.icd_id = PID;
    header.icmp_hun.ih_idseq.icd_seq = seq_number;
    header.icmp_cksum = 0;
    header.icmp_cksum = compute_icmp_checksum (
    (u_int16_t*)&header, sizeof(header));

    return header;
}

struct sockaddr_in get_ready_recipient_struct(char* adres_ip, int* return_code){
    struct sockaddr_in recipient;
    bzero (&recipient, sizeof(recipient));
    recipient.sin_family = AF_INET;
    int isValid = inet_pton(AF_INET, adres_ip, &recipient.sin_addr);
    
    if (isValid == 0)  {
        fprintf(stderr, "Invalid IP adres\n"); 
        *return_code = -1;
    }
    else if (isValid < 0)  {
        fprintf(stderr, "Invalid IP adres: %s\n", strerror(errno)); 
        *return_code = -1;
    }
    
    return recipient;
}

int sendPack(int PID, int seq_number, int sockfd, char* adres_ip, int ttl){
    
    struct icmp header = get_ready_icmp_header(PID, seq_number);

    int seq_num_in_current_ttl = (seq_number-1)%3;
    set_current_time_in(&tsorig[seq_num_in_current_ttl]);

    int return_code = 0;
    struct sockaddr_in recipient = get_ready_recipient_struct(adres_ip, &return_code);
    if (return_code<0)
        return return_code;

    setsockopt (sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));
    return_code = sendto (
        sockfd,
        &header,
        sizeof(header),
        0,
        (struct sockaddr*)&recipient,
        sizeof(recipient)
    );

    if (return_code < 0)  {
        fprintf(stderr, "sendto error: %s\n", strerror(errno)); 
        return -1;
    }

    return 0;
}
