#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>

#define BUF_SIZE 4096

static void die(const char *s) { perror(s); exit(1); }

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    if (argc != 5) {
        fprintf(stderr, "usage: %s <server-port> <web_root> <mdb-lookup-host> <mdb-lookup-port>\n", argv[0]);
        exit(1);
    }

    unsigned short server_port = atoi(argv[1]);
    unsigned short mdb_lookup_port=atoi(argv[4]);
    char *web_root=argv[2];
    char *mdb_lookup_host=argv[3];

    struct hostent *he;
    char *hostIP;

  //create a socket for TCP connection as client
     int sock; // socket descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket failed");

   // get host ip from host name
      if ((he = gethostbyname(mdb_lookup_host)) == NULL) {
          die("gethostbyname failed");
      }
      hostIP = inet_ntoa(*(struct in_addr *)he->h_addr);

 //construct a server address for mdb-lookup-host
    struct sockaddr_in hostaddr;
    memset(&hostaddr, 0, sizeof(hostaddr)); // must zero out the structure
    hostaddr.sin_family      = AF_INET;
    hostaddr.sin_addr.s_addr = inet_addr(hostIP);
    hostaddr.sin_port        = htons(mdb_lookup_port); // must be in network byte order
  
    // Establish a TCP connection to the server
    if (connect(sock, (struct sockaddr *) &hostaddr, sizeof(hostaddr)) < 0)
        die("connect failed");
   
   


    // Create a listening socket (also called server socket) 

    int servsock;
    if ((servsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket failed");

    // Construct local address structure

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // any network interface
    servaddr.sin_port = htons(server_port);

    // Bind to the local address

    if (bind(servsock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        die("bind failed");

    // Start listening for incoming connections

    if (listen(servsock, 5 /* queue size for connection requests */ ) < 0)
        die("listen failed");

     int clntsock;
    socklen_t clntlen;
    struct sockaddr_in clntaddr;
    

    while (1) {

        // Accept an incoming connection
        clntlen = sizeof(clntaddr); // initialize the in-out parameter

    if ((clntsock = accept(servsock,(struct sockaddr *) &clntaddr, &clntlen)) < 0){
	fprintf(stderr,"accept failed\n");continue;
     }
       char *client_ip=inet_ntoa(clntaddr.sin_addr);
        FILE *fd;
    if((fd=fdopen(clntsock,"r"))==NULL){
        fprintf(stderr,"fdopen falied\n");close(clntsock); continue;
        }

    char *buf=(char *)malloc(500);
    if(!buf){
	    fprintf(stderr,"malloc return NULL\n");fclose(fd);continue;
        }
     char *status400="400 Bad Request";
       //read the first line
       //if browser crash during typing sending request,close connection
    if (fgets(buf, 500, fd) == NULL) {
    if (ferror(fd)) {
        free(buf);
        fprintf(stderr, "IO error\n");
        fclose(fd);
        continue;
    } else {
        fprintf(stderr, "%s \" \" %s\n", client_ip, status400);
        fclose(fd);
        free(buf);
        continue;
    }
}
      //parse the request line 
        char *token_separators = "\t \r\n"; // tab, space, new line
	char *method = strtok(buf, token_separators);
	char *requestURI = strtok(NULL, token_separators);
	char *httpVersion = strtok(NULL, token_separators);
        char *version="HTTP/1.0 ";
        char *status501="501 Not Implemented";
        char *status404="404 Not Found";
        char response[BUF_SIZE];

        //check if any part is null,yes, free(buf) and close connection and continue
       if(method==NULL||requestURI==NULL||httpVersion==NULL){
           snprintf(response,sizeof(response),"%s%s\r\n""\r\n""<html><body>\r\n<h1>%s</h1>\r\n</body></html>\r\n",version,status400,status400);
           fprintf(stderr,"%s \"%s %s %s\" %s\n",client_ip,method,requestURI,httpVersion,status400);
       if(send(clntsock,response,strlen(response),0)!=strlen(response)){
           free(buf); fprintf(stderr,"send failed\n");fclose(fd);continue;
       }
       free(buf);
       fclose(fd);
        continue;
       } 

      //check method, free buf
   if(strcmp("GET",method)!=0){
       snprintf(response,sizeof(response),"%s%s\r\n""\r\n""<html><body>\r\n<h1>%s</h1>\r\n</body></html>\r\n",version,status501,status501);
        fprintf(stderr,"%s \"%s %s %s\" %s\n",client_ip,method,requestURI,httpVersion,status501);
      if(send(clntsock,response,strlen(response),0)!=strlen(response)){
          free(buf); fprintf(stderr,"send failed\n");fclose(fd);continue;
      }
      free(buf);
      fclose(fd);
      continue; }

   //check version, free buf
   if(strcmp("HTTP/1.0",httpVersion)!=0&&strcmp("HTTP/1.1",httpVersion)!=0){
    snprintf(response,sizeof(response),"%s%s\r\n""\r\n""<html><body>\r\n<h1>%s</h1>\r\n</body></html>\r\n",version,status501,status501);
    fprintf(stderr,"%s \"%s %s %s\" %s\n",client_ip,method,requestURI,httpVersion,status501);
  if(send(clntsock,response,strlen(response),0)!=strlen(response)){
    free(buf);fprintf(stderr,"send failed\n");fclose(fd);continue;
  }
    free(buf);
    fclose(fd);
    continue; 
   }

   //check request URI,free buf
   int len=strlen(requestURI);
   if(requestURI[0]!='/'||strstr(requestURI,"/../")!=NULL||(len>=3&&strcmp(requestURI+len-3,"/..")==0)){
       snprintf(response,sizeof(response),"%s%s\r\n""\r\n""<html><body>\r\n<h1>%s</h1>\r\n</body></html>\r\n",version,status400,status400);
       fprintf(stderr,"%s \"%s %s %s\" %s\n",client_ip,method,requestURI,httpVersion,status400);
       if(send(clntsock,response,strlen(response),0)!=strlen(response)){
          free(buf); fprintf(stderr,"send failed\n");fclose(fd);continue;
       }
       free(buf);
       fclose(fd);
       continue; 
       }
    // Now, skip the header lines and check client if crashes before new line after status line
    char headers[BUF_SIZE]; char *t;
    while((t=fgets(headers,sizeof(headers),fd))!=NULL){
        if(strcmp(headers,"\r\n")==0||strcmp(headers,"\n")==0){
                    break;}}
         if(t==NULL) {
             if (ferror(fd)){
                 free(buf);
                 fprintf(stderr,"IO error\n");fclose(fd);continue;}
                 //client crashes without new line after status line
             else {
                 fprintf(stderr, "%s \"%s %s %s\" %s\n",client_ip,method,requestURI,httpVersion,status400);
                 free(buf);
                 fclose(fd);
              continue;
             }
         }
     
    
//if the request specifically for "/mdb-lookup" or "mdb-lookup?key="
//situation 1:"/mdb-lookup"
    if(strcmp(requestURI,"/mdb-lookup")==0){
        const char *form =
             "<html><body>\n"
             "<h1>mdb-lookup</h1>\n"
             "<p>\n"
             "<form method=GET action=/mdb-lookup>\n"
             "lookup: <input type=text name=key>\n"
             "<input type=submit>\n"
             "</form>\n"
             "<p>\n"
             "</body></html>\n";
             
        int len_form=strlen(form);
        //status line sending
    snprintf(response,sizeof(response),"%s 200 OK\r\n""\r\n",version);
  if(send(clntsock,response,strlen(response),0)!=strlen(response)){
             free(buf); fprintf(stderr,"send failed\n");fclose(fd);continue;}
  fprintf(stderr,"%s \"%s %s %s\" 200 OK\n",client_ip,method,requestURI,httpVersion);
        
      //send the form
       if(send(clntsock,form,len_form,0)!=len_form){
           free(buf); fprintf(stderr,"send failed\n");fclose(fd);continue;
       }
         free(buf);
	 fclose(fd);
	 continue;
    }

    //situation 2:"mdb-lookup?key="

    char *prefix="/mdb-lookup?key=";
    size_t prefixLen=strlen(prefix);

    //check the URI starts with prefix
   if(len<prefixLen?0:strncmp(prefix,requestURI,prefixLen)==0){

       //check if the lookup-server connected
       FILE *host_fd;
       char *status500="500 Internal Server Error";
      if((host_fd=fdopen(sock,"r"))==NULL){    
                  fprintf(stderr,"mdb-lookup-server connection failed:Brokenpipe\n");
                  snprintf(response,sizeof(response),"%s%s\r\n""\r\n""<html><body>\r\n<h1>%s</h1>\r\n</body></html>\r\n",version,status500,status500);
          if(send(clntsock,response,strlen(response),0)!=strlen(response)){
             free(buf); fprintf(stderr,"send failed\n");fclose(fd);continue;
	  }
          fprintf(stderr,"%s \"%s %s %s\" %s\n",client_ip,method,requestURI,httpVersion,status500);
         free(buf);fclose(fd); continue; 
      }

       const char *form =
           "<html><body>\n"
	    "<h1>mdb-lookup</h1>\n"
	    "<p>\n"
	    "<form method=GET action=/mdb-lookup>\n"
	    "lookup: <input type=text name=key>\n"
	    "<input type=submit>\n"
	    "</form>\n"
	    "<p>\n"
            "<p><table border>\n";
       int len_form=strlen(form);

       //status line sending
   snprintf(response,sizeof(response),"%s 200 OK\r\n""\r\n",version);
 if(send(clntsock,response,strlen(response),0)!=strlen(response)){
            free(buf); fprintf(stderr,"send failed\n");fclose(fd);continue;
 }
               //send the form
if(send(clntsock,form,len_form,0)!=len_form){
          free(buf); fprintf(stderr,"send failed\n");fclose(fd);continue;
 }
   
       //checked the key
         char key[1000];
          char result[BUF_SIZE];  
           char *suffix=requestURI+prefixLen;
           fprintf(stderr,"looking up [%s]:",suffix);
           snprintf(key,sizeof(key),"%s\n",suffix);
           if(send(sock,key,strlen(key),0)!=strlen(key)){
             free(buf); fprintf(stderr,"send failed\n");fclose(fd);continue;}

           //send the content from looup-server to browser
         char *format="<tr><td bgcolor=red>";
     
               while(fgets(result,sizeof(result),host_fd)!=NULL){
       
               if( strcmp(result,"\n")==0){break;}
               //send format with each result
               if(send(clntsock,format,strlen(format),0)!=strlen(format)){
                   free(buf);fprintf(stderr,"send failed\n");fclose(fd);continue;}
                if(send(clntsock,result,strlen(result),0)!=strlen(result)){
                    free(buf); fprintf(stderr,"send failed\n");fclose(fd);continue;}
                char *line="\n";
                 if(send(clntsock,line,strlen(line),0)!=strlen(line)){
                    free(buf);fprintf(stderr,"send failed\n");fclose(fd);continue;}}           
               //send ending format
               char *end_format="</table>\n</body></html>\n";
               if(send(clntsock,end_format,strlen(end_format),0)!=strlen(end_format)){
                    free(buf); fprintf(stderr,"send failed\n");fclose(fd);continue;}


       fprintf(stderr,"%s \"%s %s %s\" 200 OK\n",client_ip,method,requestURI,httpVersion);
free(buf);fclose(fd);continue; }


 //check if the right request file 
char fullPath[BUF_SIZE];
snprintf(fullPath,BUF_SIZE,"%s%s",web_root,requestURI);
struct stat st;
int result; result=stat(fullPath,&st);
if(result!=0){

       snprintf(response,sizeof(response),"%s%s\r\n""\r\n""<html><body>\r\n<h1>%s</h1>\r\n</body></html>\r\n",version,status404,status404);
       if(send(clntsock,response,strlen(response),0)!=strlen(response)){
          free(buf); fprintf(stderr,"send failed\n");fclose(fd);continue;}
       fprintf(stderr,"%s \"%s %s %s\" %s\n",client_ip,method,requestURI,httpVersion,status404);
     /*  if (errno == ENOENT) {
            fprintf(stderr,"The directory does not exist.\n");
        } else if (errno == EACCES) {
            fprintf(stderr,"Permission denied.\n");
        } else {
            fprintf(stderr,"Error: %s\n", strerror(errno));
        }*/
       free(buf);
       fclose(fd);
       continue; }

//check if it is directory
 int n;
 char read[BUF_SIZE];
if(S_ISDIR(st.st_mode)){if(strcmp(requestURI+len-1,"/")==0){
snprintf(fullPath, BUF_SIZE,"%s%sindex.html",web_root,requestURI);
 FILE *fp=fopen(fullPath,"rb");

//check if it is good directory
if(fp==NULL){ snprintf(response,sizeof(response),"%s%s\r\n""\r\n""<html><body>\r\n<h1>%s</h1>\r\n</body></html>\r\n",version,status404,status404);
         if(send(clntsock,response,strlen(response),0)!=strlen(response)){
            free(buf); fprintf(stderr,"send failed\n");fclose(fp);fclose(fd);continue;}
         fprintf(stderr,"%s \"%s %s %s\" %s\n",client_ip,method,requestURI,httpVersion,status404);
         free(buf);
         fclose(fd);
        continue; }

//if it is good path and readable
    else{ snprintf(response,sizeof(response),"%s 200 OK\r\n""\r\n",version);
if(send(clntsock,response,strlen(response),0)!=strlen(response)){
           free(buf); fprintf(stderr,"send failed\n");fclose(fp);fclose(fd);continue;}
fprintf(stderr,"%s \"%s %s %s\" 200 OK\n",client_ip,method,requestURI,httpVersion);
 
 while((n=fread(read,1,sizeof(read),fp))>0){
     if(send(clntsock,read,n,0)!=n){
         free(buf);
         fprintf(stderr,"send failed\n");fclose(fp);fclose(fd);continue;}}
     if(ferror(fp)){
             free(buf);
            
             fprintf(stderr,"fread failed\n");fclose(fp);fclose(fd);continue;
           }
     //close fp here and will free buf and close fd at the end of big while loop
          fclose(fp); }}

//directory not end with "/"
 else{ char *status301="301 Moved Permanently";
       char local_hostname[1000];
       char location[1500];

     fprintf(stderr,"%s \"%s %s %s\" %s\n",client_ip,method,requestURI,httpVersion,status301);
 
       if(gethostname(local_hostname,sizeof(local_hostname))!=0){
          free(buf); fprintf(stderr,"get local host name falied\n");fclose(fd);continue;}
       snprintf(location,sizeof(location),"http://%s:%s%s/",local_hostname,argv[1],requestURI);
       
     snprintf(response,sizeof(response),
             "%s%s\r\n"
             "Location: %s\r\n"
             "\r\n"
             "<html><body>\r\n"
             "<h1>%s</h1>\r\n<p>"
             "The document has moved <a href=\"%s\">here</a>.</p>\r\n"
             "</body></html>\r\n",version,status301,location,status301,location);
         if(send(clntsock,response,strlen(response),0)!=strlen(response)){
           free(buf); fprintf(stderr,"send failed\n");fclose(fd);continue;}
      free(buf);
       fclose(fd);
          continue;}}

// right path and it is file 
else{
    //check if the file can be opened successfully
    FILE *fp=fopen(fullPath,"rb");
    //not readable file path
    if(fp==NULL){snprintf(response,sizeof(response),"%s%s\r\n""\r\n""<html><body>\r\n<h1>%s</h1>\r\n</body></html>\r\n",version,status404,status404);
        if(send(clntsock,response,strlen(response),0)!=strlen(response)){
           free(buf);fprintf(stderr,"send failed\n");fclose(fp);fclose(fd);continue;}
        fprintf(stderr,"%s \"%s %s %s\" %s\n",client_ip,method,requestURI,httpVersion,status404);
        free(buf);
        fclose(fd);
        continue; }
    //readable file path
    else{
snprintf(response,sizeof(response),"%s 200 OK\r\n""\r\n",version);        
if(send(clntsock,response,strlen(response),0)!=strlen(response)){
            free(buf); fprintf(stderr,"send failed\n");fclose(fp);fclose(fd);continue;}
 fprintf(stderr,"%s \"%s %s %s\" 200 OK\n",client_ip,method,requestURI,httpVersion);
 while((n=fread(read,1,sizeof(read),fp))>0){
      if(send(clntsock,read,n,0)!=n){
          free(buf);
          fprintf(stderr,"send failed\n");fclose(fp);fclose(fd);continue;
      }
 }
      if(ferror(fp)){
           free(buf);
           fprintf(stderr,"send failed\n");fclose(fp);fclose(fd);continue;
      }
      //after reading successfully, close fp here, free(buf) and close fd will do at the end of while loop
           fclose(fp); 
    }
}
//after reading files(from directory OR file path)  successfully, clean up
          free(buf);fclose(fd); 
    }
    
}

    
       









