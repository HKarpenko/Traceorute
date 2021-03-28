//Heorhii Karpenko, 312372

#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

int sendPack(int PID, int seq_number, int sockfd, char* adres_ip, int ttl);
int recieve(int sockfd, int PID, int ttl, char* reciever);

struct timeval tvorig, tvrecv;	/* originate & receive timeval structs */
long tsorig[3]; /* originate sended timeval (ms) */
long tsrecv; /* originate recieved timeval (ms) */

void set_current_time_in(long* timer){
    gettimeofday(&tvorig, (struct timezone *)NULL);
    *timer = tvorig.tv_usec / 1000;
}

int send_n_times(int n, int PID, int sockfd, char* adres_ip, int ttl){
    for(int i=1;i<=n;i++){
        int return_code = sendPack(PID, (ttl-1)*3+i, sockfd, adres_ip, ttl);
        if (return_code<0)
            return return_code;
    }
    return 0;
}

int send_recieve_in_iteration(int sockfd, char* adres_ip){
    int return_code = 0;
    int PID = getpid();
    char reciever[20] = "";

    for(int ttl=1;ttl<=30;ttl++){

        return_code = send_n_times(3, PID, sockfd, adres_ip, ttl);
        if (return_code<0)
            return return_code;

        return_code = recieve(sockfd, PID, ttl, reciever);
        if (return_code<0)
            return return_code;

        int final_adres_reached = !strcmp(reciever,adres_ip);
        if( final_adres_reached )
            break;
    }
    return 0;
}


int main(int argc, char *argv[])
{
    if(argc < 2)
        return -1;
    char* adres_ip = argv[1];

    int return_code = 0;

	int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) {
		fprintf(stderr, "socket error: %s\n", strerror(errno)); 
		return -1;
	}

    return_code = send_recieve_in_iteration(sockfd, adres_ip);
    if( return_code<0 ){
        return return_code;
    }

	return EXIT_SUCCESS;
}