// Client side implementation of UDP client-server model
#include <stdio.h>


#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <argp.h>
#include <sysexits.h>
#include <time.h>
#include <poll.h>
#include "htonll.h"

#define PORT     8080
#define MAXLINE 1024

struct outputs {
	double theta;
	double delta;
};

struct client_arguments {
	struct sockaddr_in servAddr;
	int n;
	time_t t;
};
struct tresponse{
        uint16_t seq_num;
        uint16_t version;
        uint64_t c_nsec;
        uint64_t c_sec;
        uint64_t s_nsec;
        uint64_t s_sec;

};
struct trequest {
	uint16_t seq_num;
	uint16_t version;
	uint64_t nsec;
	uint64_t sec;
};

error_t client_parser(int key, char *arg, struct argp_state *state) {
	struct client_arguments *args = state->input;
	error_t ret = 0;
	int num;
	switch (key) {
	case 'a':
		memset(&args->servAddr, 0, sizeof(args->servAddr)); // Zero out structures
		args->servAddr.sin_family = AF_INET; // IPv4 address family
		// Convert address
		if (!inet_pton(AF_INET, arg, &args->servAddr.sin_addr.s_addr)) {
			argp_error(state, "Invalid address");
		}
		break;
	case 'p':
		num = atoi(arg);
		if (num <= 0) {
			argp_error(state, "Invalid option for a port, must be a number greater than 0");
		}
		args->servAddr.sin_port = htons(num); // Server port
		break;
	case 'n':
		args->n = atoi(arg);
		if (args->n < 0) {
			argp_error(state, "timereq must be a number >= 0");
		}
		break;
	case 't':
		args->t = 1000 * atoi(arg);
		if (args->t < 0) {
			argp_error(state, "timeout must be a number >= 0");
		}
		if (!args->t) args->t = -1; // Mimic poll() treatment of the value -1
		break;
	default:
		ret = ARGP_ERR_UNKNOWN;
		break;
	}
	return ret;
}


void client_parseopt(struct client_arguments *args, int argc, char *argv[]) {
	struct argp_option options[] = {
		{ "addr", 'a', "addr", 0, "The IP address the server is listening at", 0},
		{ "port", 'p', "port", 0, "The port that is being used at the server", 0},
		{ "timereq", 'n', "timereq", 0, "The number of time requests to send to the server", 0},
		{ "timeout", 't', "timeout", 0, "The time in seconds to wait after sending or receiving a response before terminating", 0},
		{0}
	};

	struct argp argp_settings = { options, client_parser, 0, 0, 0, 0, 0 };

	memset(args, 0, sizeof(*args));
	args->n = -1;
	args->t = -1;

	if (argp_parse(&argp_settings, argc, argv, 0, NULL, args) != 0) {
		printf("Got error in parse\n");
		exit(1);
	}
	if (!args->servAddr.sin_addr.s_addr) {
		fputs("addr must be specified\n", stderr);
		exit(EX_USAGE);
	}
	if (!args->servAddr.sin_port) {
		fputs("port must be specified\n", stderr);
		exit(EX_USAGE);
	}
	if (args->n < 0) {
		fputs("timereq option must be specified\n", stderr);
		exit(EX_USAGE);
	}
	if (!args->t) {
		fputs("timeout option must be specified\n", stderr);
		exit(EX_USAGE);
	}
}

// Driver code
int main(int argc, char *argv[]) {

	struct client_arguments args;	
	client_parseopt(&args, argc, argv);

	struct outputs **outs = malloc(args.n * sizeof(struct outputs*));
	
	for(int i = 0; i < args.n; i++){
		outs[i] = NULL;
	}
	
	
	srand(time(NULL));

	struct timespec timespec0;
    	int sockfd;
    	struct tresponse buffer;

	struct trequest *trqsts = malloc(args.n * sizeof(struct trequest));
	for (int i = 0; i < args.n; i++) {
		trqsts[i].seq_num = i;
		trqsts[i].version = 3;
	}

 
	struct sockaddr_in servaddr = args.servAddr;

    	// Creating socket file descriptor
    	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        	perror("socket creation failed");
        	exit(EXIT_FAILURE);
    	}

	struct pollfd sock;
	sock.events = POLLIN;
	sock.fd = sockfd;


	for(int i = 0; i < args.n; i++){
		struct trequest time_req = trqsts[i];
		clock_gettime(CLOCK_REALTIME, &timespec0);
		time_req.nsec = htonll((uint64_t) timespec0.tv_nsec);
        	time_req.sec = htonll((uint64_t) timespec0.tv_sec);
		time_req.seq_num = htons((uint16_t) i);

    		sendto(sockfd, &time_req, sizeof(time_req),
        		MSG_CONFIRM, (const struct sockaddr *) &servaddr,sizeof(servaddr));
	}


	for(int i = 0; i < args.n; i++){

		int res_val = poll(&sock, POLLIN, args.t);
		if(res_val != POLLIN){
			printf("Timeout Reached\n");
			break;
		}


    		recvfrom(sockfd, &buffer, sizeof(buffer), 0, NULL, 0);

		uint16_t seq_number = ntohs(buffer.seq_num);

		struct outputs *temp = malloc(sizeof(struct outputs));
		outs[seq_number] = temp;


		buffer.s_sec = ntohll(buffer.s_sec);
		buffer.s_nsec = ntohll(buffer.s_nsec);
		buffer.c_sec = ntohll(buffer.c_sec);
		buffer.c_nsec = ntohll(buffer.c_nsec);

		clock_gettime(CLOCK_REALTIME, &timespec0);
                int64_t nsec2 = timespec0.tv_nsec;
                int64_t sec2 = timespec0.tv_sec;
                int64_t nsec0 = buffer.c_nsec;
                int64_t sec0 = buffer.c_sec;
                int64_t nsec1 = buffer.s_nsec;
                int64_t sec1 = buffer.s_sec;
                double delta = ((double) (sec2 - sec0)) + ((double) (nsec2 - nsec0)/ 1000000000.00);
                double sub10 = ((double) (sec1 - sec0)) + ((double) (nsec1 - nsec0)/ 1000000000.00);
                double sub12 = ((double) (sec1 - sec2)) + ((double) (nsec1-nsec2)/ 1000000000.00);
                double theta = (double) (sub10  + sub12) / 2.00;
		temp->theta = theta;
		temp->delta = delta;
  	}

	for(int i = 0; i < args.n; i++){
		if(outs[i] == NULL){
			printf("%d: Dropped\n", i);
		}else{
			printf("%d: %.4f %.4f\n", i, outs[i]->theta, outs[i]->delta);
		}
	}
    close(sockfd);
    return 0;
}

