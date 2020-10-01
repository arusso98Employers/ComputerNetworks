// Server side implementation of UDP client-server model


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sysexits.h>
#include <argp.h>
#include "htonll.h"
#include <time.h>
#define PORT     8080
#define MAXLINE 1024

struct server_arguments {
	int port;
	double drop_chance;
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

in_port_t get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in*)sa)->sin_port);
    }

    return (((struct sockaddr_in6*)sa)->sin6_port);
}

error_t server_parser(int key, char *arg, struct argp_state *state) {
	struct server_arguments *args = state->input;
	int num;
	error_t ret = 0;
	switch (key) {
	case 'p':
		args->port = atoi(arg);
		if (args->port == 0) { // port is invalid
			argp_error(state, "Invalid option for a port, must be a number");
		} else if (args->port <= 1024) {
			argp_error(state, "Port must be greater than 1024");
		}
		break;
	case 'd':
		num = atoi(arg); // The 0 case cannot be easily detected, so invalid input will just use 0
		if (num < 0 || num > 100) {
			argp_error(state, "Percentage must be between 0 and 100");
		}
		args->drop_chance = (double)num / 100.0;
		break;
	default:
		ret = ARGP_ERR_UNKNOWN;
		break;
	}
	return ret;
}

void *server_parseopt(struct server_arguments *args, int argc, char *argv[]) {
	memset(args, 0, sizeof(*args));

	struct argp_option options[] = {
		{ "port", 'p', "port", 0, "The port to be used for the server" , 0 },
		{ "drop", 'd', "drop", 0, "The percent chance a given packet will be dropped. Zero by default", 0 },
		{0}
	};
	struct argp argp_settings = { options, server_parser, 0, 0, 0, 0, 0 };
	if (argp_parse(&argp_settings, argc, argv, 0, NULL, args) != 0) {
		fputs("Got an error condition when parsing\n", stderr);
		exit(EX_USAGE);
	}
	if (!args->port) {
		fputs("A port number must be specified\n", stderr);
		exit(EX_USAGE);
	}

	return args;
}

// Driver code
int main(int argc, char *argv[]) {
    	int sockfd;
    	struct trequest buffer;
	struct server_arguments args;
	server_parseopt(&args, argc, argv);
 
    	struct sockaddr_in servaddr, cliaddr;
    	// Creating socket file descriptor
    	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        	perror("socket creation failed");
        	exit(EXIT_FAILURE);
    	}


    	memset(&servaddr, 0, sizeof(servaddr));
    	memset(&cliaddr, 0, sizeof(cliaddr));

	// Filling server information
    	servaddr.sin_family    = AF_INET; // IPv4
    	servaddr.sin_addr.s_addr = INADDR_ANY;
    	servaddr.sin_port = htons(args.port);

    	// Bind the socket with the server address
    	if ( bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ){
        	perror("bind failed");
        	exit(EXIT_FAILURE);
    }

    	socklen_t len;
    	len = sizeof(cliaddr);  //len is value/resuslt
	int max = 0;
	int a = 0;
	struct timespec timespec0;
	while ( a == 0){
		recvfrom(sockfd, &buffer, MAXLINE, MSG_WAITALL,
				( struct sockaddr *) &cliaddr, &len);
		
		uint16_t seq_number = ntohs(buffer.seq_num);

		if (seq_number > max) {
			max = seq_number;
		}

		buffer.nsec = htonll(buffer.nsec);
		buffer.sec = htonll(buffer.sec);
		printf("%s:%d %d %d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), seq_number, max);

		struct tresponse t_res;
		t_res.seq_num = buffer.seq_num;
		t_res.version = buffer.version;
		t_res.c_nsec = htonll(buffer.nsec);
		t_res.c_sec = htonll(buffer.sec);
		clock_gettime(CLOCK_REALTIME, &timespec0);
        	t_res.s_nsec = htonll((uint64_t) timespec0.tv_nsec);
        	t_res.s_sec = htonll((uint64_t) timespec0.tv_sec);

		sendto(sockfd, &t_res, sizeof(t_res), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
	}
    return 0;
}
