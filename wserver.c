//
//	Main webserver code file (with main() fuction)
//

#include <stdio.h>
#include <pthread.h>
#include<math.h>
#include "request.h"
#include "io_helper.h"

char default_root[] = ".";

//
// ./wserver [-d <basedir>] [-p <portnum>] 
// 
int main(int argc, char *argv[]) {
    int c;
    char *root_dir = default_root;
    int port = 10000;
    
	// below default values are defined in 'request.h'
    num_threads = DEFAULT_THREADS;
    buffer_max_size = DEFAULT_BUFFER_SIZE;
    scheduling_algo = DEFAULT_SCHED_ALGO;	
    while ((c = getopt(argc, argv, "d:p:t:b:s:")) != -1)
		switch (c) {
			case 'd':
				root_dir = optarg;
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 't':
				num_threads = atoi(optarg);
				break;
			case 'b':
				buffer_max_size = atoi(optarg);
				break;
			case 's':
				scheduling_algo = atoi(optarg);
				break;
			default:
				fprintf(stderr, "usage: wserver [-d basedir] [-p port] [-t threads] [-b buffersize] [-s schedalg (0 - FIFO, 1 - SFF)]\n");
				exit(1);
			
		}

    // run out of this directory
    chdir_or_die(root_dir);

	// create the thread pool
	pthread_t thread_pool[num_threads];
	for(int i=0; i<num_threads; i++)
    	pthread_create(&thread_pool[i], NULL, thread_request_serve_static, NULL);

	buffer_size = 0;

    // now, get to work
    int listen_fd = open_listen_fd_or_die(port);
    while (1) {
		struct sockaddr_in client_addr;
		int client_len = sizeof(client_addr);
		int conn_fd = accept_or_die(listen_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
		request_handle(conn_fd);
		//close_or_die(conn_fd);
	}
	
    return 0;
}
