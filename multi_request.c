#include "io_helper.h"
#include "request.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#define MAXBUF (8192)
#define SIZE (1000)

//
//	TODO: add code to create and manage the buffer
//

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

typedef struct info{
  int fd;
  char *filename;
  int filesize;
}Info;

Info BUFFER[SIZE];
int rear = -1;
int front = 0;
int size = 0;

int enqueue(int fd,char *filename,int filesize)
{
  if (size == buffer_max_size)
    return -1; //failed to add to buffer as maximum limit has reached
  rear = (rear + 1) % buffer_max_size;
  BUFFER[rear].fd = fd;
  BUFFER[rear].filename=filename;
  BUFFER[rear].filesize=filesize;
  printf("Request for %s is added to the buffer.\n", filename);
  printf("\n\n");
  size++;
  return 1; //success
}

Info dequeue()
{
  Info retval;  //return value
  retval.fd=-1;
  retval.filename=NULL;
  retval.filesize=-1;
  if (size == 0)
    return retval;
  retval.fd= BUFFER[front].fd;
  retval.filename=BUFFER[front].filename;
  retval.filesize=BUFFER[front].filesize;
  printf("Request for %s is removed from the buffer.\n", retval.filename);
  printf("\n\n");
  front = (front + 1) % buffer_max_size;
  size--;
  return retval;
}

int Fill(int fd,char *filename,int filesize)
{
  if(size==buffer_max_size)
    return -1;
  BUFFER[size].fd=fd;
  BUFFER[size].filename=filename;
  BUFFER[size].filesize=filesize;
  printf("Request for %s is added to the buffer.\n", filename);
  printf("\n\n");
  size++;
  return 1;
}

Info Remove(int loc)
{
  Info retval;
  retval.fd=BUFFER[loc].fd;
  retval.filename=BUFFER[loc].filename;
  retval.filesize=BUFFER[loc].filesize;
  for(int i=loc;i<size-1;i++)
  {
    BUFFER[i].fd=BUFFER[i+1].fd;
    strcpy(BUFFER[i].filename,BUFFER[i+1].filename);
    BUFFER[i].filesize=BUFFER[i+1].filesize;
  }
  printf("Request for %s is removed from the buffer.\n", retval.filename);
  printf("\n\n");
  size--;
  return retval;
}

//
// Sends out HTTP response in case of errors
//
void request_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
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
                "</html>\r\n",
          errnum, shortmsg, longmsg, cause);

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
void request_read_headers(int fd)
{
  char buf[MAXBUF];

  readline_or_die(fd, buf, MAXBUF);
  while (strcmp(buf, "\r\n"))
  {
    readline_or_die(fd, buf, MAXBUF);
  }
  return;
}

//
// Return 1 if static, 0 if dynamic content (executable file)
// Calculates filename (and cgiargs, for dynamic) from uri
//
int request_parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi"))
  {
    // static
    strcpy(cgiargs, "");
    sprintf(filename, ".%s", uri);
    if (uri[strlen(uri) - 1] == '/')
    {
      strcat(filename, "index.html");
    }
    return 1;
  }
  else
  {
    // dynamic
    ptr = index(uri, '?');
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    else
    {
      strcpy(cgiargs, "");
    }
    sprintf(filename, ".%s", uri);
    return 0;
  }
}

//
// Fills in the filetype given the filename
//
void request_get_filetype(char *filename, char *filetype)
{
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
void request_serve_static(int fd, char *filename, int filesize)
{
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
// Fetches the requests from the buffer and handles them (thread locic)
//
void *thread_request_serve_static(void *arg)
{
  // TODO: write code to actualy respond to HTTP requests
  while (1)
  {
    while(scheduling_algo==0) //for first come first serve
    {
      Info info;
      pthread_mutex_lock(&lock);
      info=dequeue();
      while(info.fd == -1)
      {
        pthread_cond_wait(&cond, &lock);
        info = dequeue();
      }
      pthread_mutex_unlock(&lock);

      //we have a connection, now thread is doing its work
      request_serve_static(info.fd,info.filename,info.filesize);
      close_or_die(info.fd);
    }
    while(scheduling_algo==1) // for smallest file first method
    {
      Info info;
      int i,small=MAXBUF,store; //small variable will carry smallest filesize and store variable will store on which location in Buffer that file is present
      pthread_mutex_lock(&lock);
      while(size==0)
      {
        pthread_cond_wait(&cond,&lock);
      }
      for(i=0;i<size;i++)
      {
        if(BUFFER[i].filesize<small)
        {
          small=BUFFER[i].filesize;
          store=i;
        }
      }
      info=Remove(store);
      pthread_mutex_unlock(&lock);

      //we have a connection, now thread is doing its work
      request_serve_static(info.fd,info.filename,info.filesize);
      close_or_die(info.fd);
    }
  }
}

//
// Initial handling of the request
//
void request_handle(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXBUF], method[MAXBUF], uri[MAXBUF], version[MAXBUF];
  char filename[MAXBUF], cgiargs[MAXBUF];

  // get the request type, file path and HTTP version
  readline_or_die(fd, buf, MAXBUF);
  sscanf(buf, "%s %s %s", method, uri, version);
  printf("method:%s uri:%s version:%s\n", method, uri, version);

  // verify if the request type is GET is not
  if (strcasecmp(method, "GET"))
  {
    request_error(fd, method, "501", "Not Implemented", "server does not implement this method");
    return;
  }
  request_read_headers(fd);

  // check requested content type (static/dynamic)
  is_static = request_parse_uri(uri, filename, cgiargs);

  // get some data regarding the requested file, also check if requested file is present on server
  if (stat(filename, &sbuf) < 0)
  {
    request_error(fd, filename, "404", "Not found", "server could not find this file");
    return;
  }

  // verify if requested content is static
  if (is_static)
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode))
    {
      request_error(fd, filename, "403", "Forbidden", "server could not read this file");
      return;
    }

    //to stop traversing up the filesystem
    if(strstr(filename,".."))
    {
      request_error(fd, filename, "403","Cannot Access", "Traversing up in filesystem is not allowed");
      return;
    }

    // TODO: write code to add HTTP requests in the buffer based on the scheduling policy
    if(scheduling_algo==0)
    {
      pthread_mutex_lock(&lock);
      //adding requests to buffer
      int val=enqueue(fd,filename,sbuf.st_size);
      while(val==-1)
      {
        val=enqueue(fd,filename,sbuf.st_size); //keep checking if the request can be added or not till the point request finally gets added
      }
      //signaling the waiting thread
      pthread_cond_signal(&cond);
      pthread_mutex_unlock(&lock);
    }
    else if(scheduling_algo==1)
    {
      pthread_mutex_lock(&lock);
      int val=Fill(fd,filename,sbuf.st_size);
      while(val==-1)
      {
        val=Fill(fd,filename,sbuf.st_size);
      }
      pthread_cond_signal(&cond);
      pthread_mutex_unlock(&lock);
    }
  }
  else
  {
    request_error(fd, filename, "501", "Not Implemented", "server does not serve dynamic content request");
  }
}