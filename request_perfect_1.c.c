#include<stdio.h>
#include<pthread.h>
#include<math.h>
#include<string.h>
#include "io_helper.h"
#include "request.h"

#define MAXBUF (8192)
#define SIZE (1000)

//
//	TODO: add code to create and manage the buffer
//
typedef enum BOOL {FALSE, TRUE}BOOL;
//Acquire lock and condition variable
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

typedef struct Node
{
    char *file_name;
    int fd;
    int file_size;
}Node;

Node CONTAINER_BUFFER_QUEUE[SIZE];

int front = 0;
int rear = 0;
int counter =0;




BOOL Enqueue(int fd, char *file_name, int file_size)
{
    if(counter >= size_of_buffer)
    return FALSE;

    CONTAINER_BUFFER_QUEUE[rear].fd = fd;
    CONTAINER_BUFFER_QUEUE[rear].file_name = file_name;
    CONTAINER_BUFFER_QUEUE[rear].file_size = file_size;
    //printf("\n**** Enqueuing file with id : %d ****\n",fd);
    printf("Request for %s is added to the buffer.\n", file_name);
    counter++;
    rear = (rear+1)%SIZE;
    return TRUE;
    
}

Node Dequeue()
{
    Node retval;
    if(counter <= 0)
    {
        retval.fd = -1;
        retval.file_size = -1;
        retval.file_name = "NONE\0";
        return retval;
    }
    else
    {
        retval.fd = CONTAINER_BUFFER_QUEUE[front].fd;
        retval.file_name = CONTAINER_BUFFER_QUEUE[front].file_name;
        retval.file_size = CONTAINER_BUFFER_QUEUE[front].file_size;
        counter--;
        front = (front+1)%SIZE;
        //printf("\n**** Dequeuing file with id : %d ****\n",retval.fd);
        printf("Request for %s is removed from the buffer.\n\n", retval.file_name);
        return retval;
    }
}

Node CONTAINER_BUFFER_HEAP[SIZE];


void Heapify(int itr)
{
    int left,right,smallest;

    left = 2*itr + 1;
    right = 2*itr + 2;

    smallest = itr;
    if(left < counter && CONTAINER_BUFFER_HEAP[left].file_size < CONTAINER_BUFFER_HEAP[itr].file_size)
    {
        smallest = left;
    }

    if(right< counter && CONTAINER_BUFFER_HEAP[right].file_size < CONTAINER_BUFFER_HEAP[smallest].file_size)
    {
        smallest = right;
    }

    if(itr!=smallest)
    {
        Node k;

        k.fd = CONTAINER_BUFFER_HEAP[smallest].fd;
        CONTAINER_BUFFER_HEAP[smallest].fd = CONTAINER_BUFFER_HEAP[itr].fd;
        CONTAINER_BUFFER_HEAP[itr].fd = k.fd;

        k.file_name = CONTAINER_BUFFER_HEAP[smallest].file_name;
        CONTAINER_BUFFER_HEAP[smallest].file_name = CONTAINER_BUFFER_HEAP[itr].file_name;
        CONTAINER_BUFFER_HEAP[itr].file_name = k.file_name;

        k.file_size = CONTAINER_BUFFER_HEAP[smallest].file_size;
        CONTAINER_BUFFER_HEAP[smallest].file_size = CONTAINER_BUFFER_HEAP[itr].file_size;
        CONTAINER_BUFFER_HEAP[itr].file_size = k.file_size;

        Heapify(smallest);
    }
}

BOOL Insert(int fd, char *file_name, int file_size)
{
    if(counter >= size_of_buffer)
    return FALSE;

    if(counter == 0)
    {
        CONTAINER_BUFFER_HEAP[counter].fd = fd;
        CONTAINER_BUFFER_HEAP[counter].file_name = file_name;
        CONTAINER_BUFFER_HEAP[counter].file_size = file_size;
        counter++;
        return TRUE;
    }
    else
    {
        CONTAINER_BUFFER_HEAP[counter].fd = fd;
        CONTAINER_BUFFER_HEAP[counter].file_name = file_name;
        CONTAINER_BUFFER_HEAP[counter].file_size = file_size;

        int parent,child,i;
        i = counter;
        child = i;
        parent=ceil((float)i/2)-1;
        Node temp;
        while(i>=0 && CONTAINER_BUFFER_HEAP[child].file_size < CONTAINER_BUFFER_HEAP[parent].file_size)
        {
            temp = CONTAINER_BUFFER_HEAP[child];
            CONTAINER_BUFFER_HEAP[child] = CONTAINER_BUFFER_HEAP[parent];
            CONTAINER_BUFFER_HEAP[parent] = temp;

            i = parent;
            child = i;
            parent = ceil((float)child/2)-1;
        }

        counter++;
        printf("Request for %s is added to the buffer.\n", file_name);
        return TRUE;
    }

    return TRUE;
}

Node Extract()
{
    Node retval;

    if(counter <= 0)
    {
        retval.fd = -1;
        retval.file_size = -1;
        retval.file_name = "NONE\0";
        return retval;
    }
    else
    {
        retval.fd = CONTAINER_BUFFER_HEAP[0].fd;
        retval.file_name = CONTAINER_BUFFER_HEAP[0].file_name;
        retval.file_size = CONTAINER_BUFFER_HEAP[0].file_size;

        CONTAINER_BUFFER_HEAP[0].fd =  CONTAINER_BUFFER_HEAP[counter-1].fd;
        CONTAINER_BUFFER_HEAP[0].file_name =  CONTAINER_BUFFER_HEAP[counter-1].file_name;
        CONTAINER_BUFFER_HEAP[0].file_size =  CONTAINER_BUFFER_HEAP[counter-1].file_size;

        counter--;

        Heapify(0);
        printf("Request for %s is removed from the buffer.\n\n", retval.file_name);
        return retval;
    }
}
//

//
// Sends out HTTP response in case of errors
//
void request_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXBUF], body[MAXBUF];
    
    // Create the body of error message first (have to know its length for header)
    sprintf(body, ""
	    "<!doctype html>\r\n"
	    "<head>\r\n"
	    "  <title>OSTEP WebServer Error</title>\r\n"
	    "</head>\r\n"
	    "<body>\r\n"
	    "  <h2>%s: %s</h2>\r\n" 
	    "  <p>%s: %s</p>\r\n"
	    "</body>\r\n"
	    "</html>\r\n", errnum, shortmsg, longmsg, cause);
    
    // Write out the header information for this response
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    write_or_die(fd, buf, strlen(buf));
    
    sprintf(buf, "Content-Type: text/html\r\n");
    write_or_die(fd, buf, strlen(buf));
    
    sprintf(buf, "Content-Length: %lu\r\n\r\n", strlen(body));
    write_or_die(fd, buf, strlen(buf));
    
    // Write out the body last
    write_or_die(fd, body, strlen(body));
    
    // close the socket connection
    close_or_die(fd);
}

//
// Reads and discards everything up to an empty text line
//
void request_read_headers(int fd) {
    char buf[MAXBUF];
    
    readline_or_die(fd, buf, MAXBUF);
    while (strcmp(buf, "\r\n")) {
		readline_or_die(fd, buf, MAXBUF);
    }
    return;
}

//
// Return 1 if static, 0 if dynamic content (executable file)
// Calculates filename (and cgiargs, for dynamic) from uri
//
int request_parse_uri(char *uri, char *filename, char *cgiargs) {
    char *ptr;
    
    if (!strstr(uri, "cgi")) { 
	// static
	strcpy(cgiargs, "");
	sprintf(filename, ".%s", uri);
	if (uri[strlen(uri)-1] == '/') {
	    strcat(filename, "index.html");
	}
	return 1;
    } else { 
	// dynamic
	ptr = index(uri, '?');
	if (ptr) {
	    strcpy(cgiargs, ptr+1);
	    *ptr = '\0';
	} else {
	    strcpy(cgiargs, "");
	}
	sprintf(filename, ".%s", uri);
	return 0;
    }
}

//
// Fills in the filetype given the filename
//
void request_get_filetype(char *filename, char *filetype) {
    if (strstr(filename, ".html")) 
		strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif")) 
		strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg")) 
		strcpy(filetype, "image/jpeg");
    else 
		strcpy(filetype, "text/plain");
}

//
// Handles requests for static content
//
void request_serve_static(int fd, char *filename, int filesize) {
    int srcfd;
    char *srcp, filetype[MAXBUF], buf[MAXBUF];
    
    request_get_filetype(filename, filetype);
    srcfd = open_or_die(filename, O_RDONLY, 0);
    
    // Rather than call read() to read the file into memory, 
    // which would require that we allocate a buffer, we memory-map the file
    srcp = mmap_or_die(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close_or_die(srcfd);
    
    // put together response
    sprintf(buf, ""
	    "HTTP/1.0 200 OK\r\n"
	    "Server: OSTEP WebServer\r\n"
	    "Content-Length: %d\r\n"
	    "Content-Type: %s\r\n\r\n", 
	    filesize, filetype);
       
    write_or_die(fd, buf, strlen(buf));
    
    //  Writes out to the client socket the memory-mapped file 
    write_or_die(fd, srcp, filesize);
    munmap_or_die(srcp, filesize);
}

//
// Fetches the requests from the buffer and handles them (thread logic)
//
void* thread_request_serve_static(void* arg)
{
	// TODO: write code to actualy respond to HTTP requests
  while(TRUE)
  {
    
    while(scheduling_algo == 0)
    {
      Node data;
      pthread_mutex_lock(&lock);
      data = Dequeue();
      while(data.fd == -1)
      {
        pthread_cond_wait(&cond,&lock);
        data = Dequeue();
      }
      //we have a connection, now thread is doing its work
      //pthread_cond_signal(&cond);
      pthread_mutex_unlock(&lock); 
      // if(data.fd!=-1)
      // {
      //     request_serve_static(data.fd,data.file_name,data.file_size);
      //     close_or_die(data.fd);
      // }
      request_serve_static(data.fd,data.file_name,data.file_size);
      close_or_die(data.fd);
    }

    while(scheduling_algo == 1)
    {
            printf("\n inside sedule algo 1 of thread function\n");
            Node data;
            pthread_mutex_lock(&lock);
            data = Extract();

            while(data.fd == -1)
            {
                printf("\n waiting---\n");
                pthread_cond_wait(&cond,&lock);
                printf("\nAwake---\n");
                data = Extract();
                printf("\ndata.fd = %d\n",data.fd);
            }
            //we have a connection, now thread is doing its work
            //pthread_cond_signal(&cond);
            pthread_mutex_unlock(&lock); 
            printf("\n Requesting---\n");
            request_serve_static(data.fd,data.file_name,data.file_size);
            close_or_die(data.fd);
    }

        
  }
    
}

//
// Initial handling of the request
//
void request_handle(int fd) {
    int is_static;
    struct stat sbuf;
    char buf[MAXBUF], method[MAXBUF], uri[MAXBUF], version[MAXBUF];
    char filename[MAXBUF], cgiargs[MAXBUF];
    
	// get the request type, file path and HTTP version
    readline_or_die(fd, buf, MAXBUF);
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("method:%s uri:%s version:%s\n", method, uri, version);

	// verify if the request type is GET or not
    if (strcasecmp(method, "GET")) {
		request_error(fd, method, "501", "Not Implemented", "server does not implement this method");
		return;
    }
    request_read_headers(fd);
    
	// check requested content type (static/dynamic)
    is_static = request_parse_uri(uri, filename, cgiargs);
    
	// get some data regarding the requested file, also check if requested file is present on server
    if (stat(filename, &sbuf) < 0) {
		request_error(fd, filename, "404", "Not found", "server could not find this file");
		return;
    }
    
	// verify if requested content is static
    if (is_static) {
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
			request_error(fd, filename, "403", "Forbidden", "server could not read this file");
			return;
		}
    if(strstr(filename,".."))
    {
        request_error(fd, filename, "403","Cannot Access", "Traversing up in filesystem is not allowed");
        return;
    }
		// TODO: write code to add HTTP requests in the buffer based on the scheduling policy

    if(scheduling_algo == 0)
    {
      printf("\nInserting---\n");
      pthread_mutex_lock(&lock);
      BOOL GOT = Enqueue(fd,filename,sbuf.st_size);
      while(!GOT)
      {
        //pthread_cond_wait(&cond,&lock);
        GOT = Enqueue(fd,filename,sbuf.st_size); //Keep Checking
      }

      //signaling the waiting thread
      pthread_cond_signal(&cond);
      pthread_mutex_unlock(&lock);
    }
     else if(scheduling_algo == 1)
    {
            printf("\nInserting---\n");
            pthread_mutex_lock(&lock);
            printf("\nInserting---\n");
            BOOL GOT = Insert(fd,filename,sbuf.st_size);
            while(!GOT)
            {
                GOT = Insert(fd,filename,sbuf.st_size);
            }

            //signaling the waiting thread
            //sleep(1);
            printf("\nCalling awake cond call\n");
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&lock);
    }
		
		// TODO: write code to add HTTP requests in the buffer based on the scheduling policy

    } else {
		request_error(fd, filename, "501", "Not Implemented", "server does not serve dynamic content request");
    }
}
