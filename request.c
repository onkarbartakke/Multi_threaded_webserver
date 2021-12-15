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
//Boolean variable

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; //lock
pthread_cond_t cond_fill = PTHREAD_COND_INITIALIZER; //Condition variable
pthread_cond_t cond_empty = PTHREAD_COND_INITIALIZER; //Condition variable

//int request_cnt = 0; // Total Request Count processed

typedef struct Node    // Buffer Node, buffer container is made of this nodes
{
    BOOL state;   // State is Used to determine whether the Buffer Node is active or not, if active we will not overwrite it, if it is not active we will overwrite it for new incoming data
    char *file_name;
    int fd;
    int file_size;
}Node;


Node CONTAINER_BUFFER[SIZE]; // This is the main buffer container to store the data of request
int iterator = 0; 

typedef struct Key_Data_Node 
{
    int index;
    int fd;
}KEYS;
// We handle our buffer container with set of keys, what all operations(swap , shift) we perform on the keys not on the buffer container.
// Why we do this ?
// It is observed that to implement FIFO and SFF many swaping, shifting operations are required, doing these operations on the drict data node 
// makes it heavy and costly. So we save our data in the CONTAINER_BUFFER and save the index of the container as key, and what all required operations
// are there those are performed on keys, finally we access the data Node using key take all requried data from the container and after that 
// finally we turn off its state so that it can be over written when iterator is on that place.


KEYS BUFFER_QUEUE[SIZE];  // Queue for FIFO

KEYS BUFFER_HEAP[SIZE];   //Min-Heap for SFF

int front = 0;  // Front ptr for Queue
int rear = 0;  // Rear ptr for Queue
int counter = 0; // To count total number of requests in the buffer

BOOL Initialised = FALSE;  

void Initialise() //Initialise function called only once at start
{
    int i;
    for(i=0;i<SIZE;i++)
    {
        CONTAINER_BUFFER[i].state = FALSE; // Setting the state as off for all the places in the buffer as it will be empty
    }
}

BOOL Enqueue(int fd, char *file_name, int file_size) 
{
    if(counter >= buffer_max_size)
    return FALSE;

    while(CONTAINER_BUFFER[iterator].state == TRUE) // Finding the first empty place, most of the time it will be O(1) operation 
    {                                               // As we are treating it as a circular structure front places will become empty as
        iterator = (iterator + 1) % SIZE;           // we move forward (After request is processed)
    }

    CONTAINER_BUFFER[iterator].state = TRUE; // Set state as TRUE (ON)
    CONTAINER_BUFFER[iterator].file_name = file_name; //Store the data
    CONTAINER_BUFFER[iterator].file_size = file_size;
    CONTAINER_BUFFER[iterator].fd = fd;

    BUFFER_QUEUE[rear].fd = fd;
    BUFFER_QUEUE[rear].index = iterator; // store the index of the buffer in the queue this will act as a key

    printf("Request for %s is added to the buffer.\n", file_name);
    counter++;
    rear = (rear+1)%SIZE;
    return TRUE;
}

int Dequeue()
{
    int key;

    if(counter <= 0)
    {
       key = -1;
       return key;
    }
    else
    {
        key = BUFFER_QUEUE[front].index; // Take the key from front for FIFO
        counter--;
        front = (front+1)%SIZE;
        //printf("\n**** Dequeuing file with id : %d ****\n",retval.fd);
        
        return key;
        
    }

    return key;
}

void Heapify(int itr) // Heapify Function, this builds min heap in avg O(logN) time where N is num of nodes(requests)
{   
    // itr is the parent in this case
    int left,right,smallest;

    left = 2*itr + 1; // left Child
    right = 2*itr + 2; // Right Child

    smallest = itr;
    if(left < counter && CONTAINER_BUFFER[BUFFER_HEAP[left].index].file_size < CONTAINER_BUFFER[BUFFER_HEAP[itr].index].file_size) 
    {   //Goal is to find smallest node among parent,left child and right child in terms of size
        smallest = left;
    }

    if(right< counter && CONTAINER_BUFFER[BUFFER_HEAP[right].index].file_size < CONTAINER_BUFFER[BUFFER_HEAP[smallest].index].file_size)
    {
        smallest = right;
    }

    if(itr!=smallest) // If parent is not the smallest node, swap the parent key with the smallest child key in terms of file_size
    {
       KEYS k;

       k = BUFFER_HEAP[itr];
       BUFFER_HEAP[itr] = BUFFER_HEAP[smallest];
       BUFFER_HEAP[smallest] = k;

        Heapify(smallest); // Heapify below that node which was swapped
    }
}

BOOL Insert(int fd, char *file_name, int file_size) // Inserting in Min-Heap
{
    if(counter >= buffer_max_size)
    return FALSE;

    while(CONTAINER_BUFFER[iterator].state)
    {
        iterator = (iterator+1)%SIZE;
    }

    CONTAINER_BUFFER[iterator].state = TRUE; // This are same operations as performed above on queue
    CONTAINER_BUFFER[iterator].fd = fd;
    CONTAINER_BUFFER[iterator].file_name = file_name;
    CONTAINER_BUFFER[iterator].file_size = file_size;

    BUFFER_HEAP[counter].index = iterator;
    BUFFER_HEAP[counter].fd = fd; // store the index of the buffer in the Min-Heap this will act as a key

   //As we are implementing Heap on an array(starting from index 0), counter is the num of request so we store the new request
   // at the end of the heap and then adjust it to its place by performing following operation 

    if(counter>0)
    {
        int parent,child,i;
        i = counter; //Last newly inserted data
        child = i;
        parent = ceil((float)i/2)-1;

        KEYS temp;

        while(i>=0 && CONTAINER_BUFFER[BUFFER_HEAP[child].index].file_size < CONTAINER_BUFFER[BUFFER_HEAP[parent].index].file_size )
        {
            temp = BUFFER_HEAP[child];
            BUFFER_HEAP[child] = BUFFER_HEAP[parent]; // Swaping of parent and child if child file_size is less than parent file_size
            BUFFER_HEAP[parent] = temp;

            i = parent;
            child = i;
            parent = ceil((float)child/2)-1; // Goinf back (up the heap) making old parent as our new child and then finding new parent 
                                            // of new child, its like traveling up the heap so worst case time is O(log N)

            if(parent<0)
            break;
        }
    }

    counter++;
    printf("Request for %s is added to the buffer.\n", file_name);
    return TRUE;
}

int Extract()
{
    int key;

    if(counter <= 0)
    {
        key = -1;
        return key;
    }
    else
    {
        //We extract the top element(which is at index 0) of the min-heap as it will be the most minimum in file_size 
        key = BUFFER_HEAP[0].index;
        BUFFER_HEAP[0] = BUFFER_HEAP[counter-1]; // we bring the last element to the 0th index
        counter--;
        
        Heapify(0); // Heapify down the heap so next smaller element is at top and the new 0th element goes at its correct place
    }

    return key;
}
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
    while(TRUE) // We intentionally use while loops with never ending condition so our threads dont die by exiting the block, they just sleep when called wait
    {
        while(scheduling_algo == 0)
        {
            Node data;
            int key;
            pthread_mutex_lock(&lock); // Start of Critical Section as we access global variables
            key = Dequeue();

            while(key == -1) // if return is -1 sleep/ condition_wait because the buffer is empty
            {
                pthread_cond_wait(&cond_fill,&lock);
                key = Dequeue();
            }

            data.fd = CONTAINER_BUFFER[key].fd; // we extract all the required data from the main buffer with the help of key
            data.file_size = CONTAINER_BUFFER[key].file_size;
            data.file_name = CONTAINER_BUFFER[key].file_name;

            CONTAINER_BUFFER[key].state = FALSE; // After taking all required data we set state as false so that new place is made (overwriting)
            pthread_cond_signal(&cond_empty); // Now the place is available for new input requests so signal to the waiting main thread if it was  waiting

            printf("Request for %s is removed from the buffer.\n\n", data.file_name);
            //request_cnt++;
           // printf("REQUEST COUNT NOW : %d\n",request_cnt);
            pthread_mutex_unlock(&lock); //Unlock end of the Critical section 

            request_serve_static(data.fd,data.file_name,data.file_size); //Serve the Request
            close_or_die(data.fd);
        }

        while(scheduling_algo == 1)
        {
            Node data;
            int key;
            pthread_mutex_lock(&lock); // Start of Critical Section as we access global variables
            key = Extract(); 

            while(key == -1) // if return is -1 sleep/ condition_wait because the buffer is empty
            {
                pthread_cond_wait(&cond_fill, &lock);
                key = Extract();
            }

            data.fd = CONTAINER_BUFFER[key].fd; // we extract all the required data from the main buffer with the help of key
            data.file_size = CONTAINER_BUFFER[key].file_size;
            data.file_name = CONTAINER_BUFFER[key].file_name;

            CONTAINER_BUFFER[key].state = FALSE; // After taking all required data we set state as false so that new place is made (overwriting)
            pthread_cond_signal(&cond_empty); // Now the place is available for new input requests so signal to the waiting main thread if its waiting

            printf("Request for %s is removed from the buffer.\n\n", data.file_name);
            //request_cnt++;
            //printf("REQUEST COUNT NOW : %d\n",request_cnt);
            pthread_mutex_unlock(&lock); //Unlock end of the Critical section 

            request_serve_static(data.fd,data.file_name,data.file_size); //Serve the Request
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
        if(strstr(filename,"..")) // Security measure to Avoid traversing up the file system
        {
            request_error(fd, filename, "403","Cannot Access", "Traversing up in filesystem is not allowed");
            return;
        }
		
		// TODO: write code to add HTTP requests in the buffer based on the scheduling policy
        if(!Initialised) // If our BUFFER is not initailsed , initialise it once, as only main thread will call it at start, while other threads will be sleeping no need for lock here, also the function and bool var is accessed by main thread only
        {
            Initialise();
            Initialised = TRUE;
        }
        if(scheduling_algo == 0)
        {
            //printf("\nInserting---\n");
            pthread_mutex_lock(&lock);
            BOOL inserted = Enqueue(fd,filename,sbuf.st_size);
            while(!inserted) // if not inserted that means buffer is full so either keep trying and waste cpu cycles or sleep till signal comes
            {
                pthread_cond_wait(&cond_empty,&lock);
                inserted = Enqueue(fd,filename,sbuf.st_size); //Keep Checking
            }

            //signaling the waiting threads to wake as new data has added to the buffer to be processed
            pthread_cond_signal(&cond_fill);
            pthread_mutex_unlock(&lock);
        }
        else if(scheduling_algo == 1)
        {
            pthread_mutex_lock(&lock);
            BOOL inserted = Insert(fd,filename,sbuf.st_size);
            while(!inserted) // if not inserted that means buffer is full so either keep trying and waste cpu cycles or sleep till signal comes
            {
                pthread_cond_wait(&cond_empty,&lock);
                inserted = Insert(fd,filename,sbuf.st_size); //Keep Checking
            }

            //signaling the waiting threads to wake as new data has added to the buffer to be processed
            pthread_cond_signal(&cond_fill);
            pthread_mutex_unlock(&lock);
        }
    } else {
		request_error(fd, filename, "501", "Not Implemented", "server does not serve dynamic content request");
    }
}
