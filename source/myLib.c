#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdarg.h>

#include "sockwrap.h"
#include "myLib.h"

int recv_t(int fd, void *bufptr, size_t nbytes, int flags,int t){

 int read,n;
                
 struct timeval tval;
 fd_set cset;
	
 FD_ZERO(&cset);
 FD_SET(fd,&cset);
 tval.tv_sec=t;
 tval.tv_usec=0;

 n=select(fd+1,&cset,NULL,NULL,&tval);

if(n==-1){
      printf("Error, select() failed");
      return -1;}
 
 if(n>0){
   if(FD_ISSET(fd,&cset))
     read=recv(fd,bufptr,nbytes,flags);
     
}
 else{
   printf("Timeout expired: ");
   return -1;
 }
	
return read;


 }

int readline_unbuffered_t(int fd, void *bufptr, size_t nbytes,int t){

 int read,n;
                
 struct timeval tval;
 fd_set cset;
	
 FD_ZERO(&cset);
 FD_SET(fd,&cset);
 tval.tv_sec=t;
 tval.tv_usec=0;

 n=select(fd+1,&cset,NULL,NULL,&tval);

if(n==-1){
      printf("Error, select() failed");
      return -1;}
 
 if(n>0){
   if(FD_ISSET(fd,&cset))
      read=readline_unbuffered(fd,bufptr,nbytes);       
}
 else{
   printf("Timeout expired: ");
   return -1;
 }
	
return read;


 }


int readn_t(int fd, void *bufptr, size_t nbytes,int t){

 int read,n;
                
 struct timeval tval;
 fd_set cset;
	
 FD_ZERO(&cset);
 FD_SET(fd,&cset);
 tval.tv_sec=t;
 tval.tv_usec=0;

 n=select(fd+1,&cset,NULL,NULL,&tval);

if(n==-1){
      printf("Error, select() failed");
      return -1;}
 
 if(n>0){
   if(FD_ISSET(fd,&cset))
     read=readn(fd,bufptr,nbytes);
     
}
 else{
   printf("Timeout expired: ");
   return -1;
 }
	
return read;

 }
 
void freeDataStruct(char** buf1,char** buf2,char** buf3,struct addrinfo** info){


if(buf1!=NULL){
free(*buf1);
*buf1=NULL;}

if(buf2!=NULL){
free(*buf2);
*buf2=NULL;}

if(buf3!=NULL){
free(*buf3);
*buf3=NULL;}

if(info!=NULL){
  freeaddrinfo(*info);
  *info=NULL;}

}


char* takeNameFromPath(char* path){
   
   char* fileName;
    
   fileName=strrchr(path,'/');
   
   if(fileName==NULL)
      return path;
   else
      return fileName+1;

}



