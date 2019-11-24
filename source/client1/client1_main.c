#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include "../errlib.h"
#include "../sockwrap.h"
#include "../myLib.h"

int s=-1;

static void sig_int_term(int signo){

 if(s>=0)
     close(s);

  exit(0);
}



 char* prog_name;
 #define MAXLINE 35
 #define MAX_DIM 1023

 int main(int argc,char **argv){

 ///////////////////////////// AREA DATI ///////////////////////////////////////
  uint16_t port_n;                  /*     port number           */
  struct sockaddr_in saddr;         /*     server addr           */


int    f_len,                       /*     file name lenght      */
       b_len,                       /*     GET MESSAGE lenght    */
       n_a=argc,                    /*     number of arguments   */
       i=3,                         /*     first file 3rd arg    */
       wait=5,                      /*     timeout for select    */
       n,                           /*     select() result       */
       n_read=0,                    /*     bytes read            */
       n_left=0;                   /*     bytes not read        */

  uint32_t fdim,                    /*     file dimension        */
           ftime;                   /*     file timestamp        */

  char *cl_file=NULL,               /*     store file name       */
       *s_buffer=NULL,              /*     store GET MSG         */
       *file_buffer=NULL,           /*     store file contents   */
       *header=NULL;                /*     store response header */


  FILE *local_f;                    /*    file to wrtite         */

  struct stat st;                   /*    save file info         */

  struct addrinfo *info;
  struct sockaddr_in *resAddr;

 ///////////////////////////////////////////////////////////////////////////////
 signal(SIGINT,sig_int_term);
 signal(SIGTERM,sig_int_term);
 /*****************************************************************************/
 /* check passed arguments                                                    */
 /*****************************************************************************/
 if(argc<4){
   printf("(./ClientTCP)-Error: wrong number of arguments\n");
   exit(EXIT_FAILURE);}

 if(strlen(argv[2])>5){
   printf("(./ClientTCP)-Error: exceeded max port number\n");
   exit(EXIT_FAILURE);}

 if(strlen(argv[1])>15){
   printf("(./ClientTCP)-Error: wrong server address\n");
   exit(EXIT_FAILURE);}

  port_n=atoi(argv[2]);
  if(port_n<1024){
    printf("(./ClientTCP)-Error: port number accessible only by SUPER-USER\n");
    exit(EXIT_FAILURE);}

 /*****************************************************************************/
 /* Struct inizialization                                                     */
 /*****************************************************************************/

 if(getaddrinfo(argv[1],argv[2],NULL,&info)!=0){
    printf("(./ClientTCP)-Error, cant't resolve address!\n");
    exit(EXIT_FAILURE);}

 resAddr=(struct sockaddr_in *) info->ai_addr;

 memset(&saddr,0,sizeof(saddr));

 saddr.sin_family= AF_INET;
 saddr.sin_port= resAddr->sin_port;
 saddr.sin_addr.s_addr= resAddr->sin_addr.s_addr;

 /*****************************************************************************/
 /* socket creation                                                           */
 /*****************************************************************************/
 s=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);

 if(s == -1){
    printf("(./ClientTCP)-socket creation failed\n");
    exit(EXIT_FAILURE);}

 /*****************************************************************************/
 /* connection to server                                                      */
 /*****************************************************************************/

 if(connect(s, (struct sockaddr *)&saddr, sizeof(saddr))<0){
    exit(EXIT_FAILURE);}


 /*****************************************************************************/
 /* taking i file from command line                                           */
 /*****************************************************************************/

 while( i < n_a){

 //printf("(./ClientTCP)\t\tPROCESSING %d FILE\t\t\t",i-2);
 //fflush(stdout);

 f_len=strlen(argv[i]);
 cl_file=takeNameFromPath(argv[i]);

 if((s_buffer=(char*)malloc(sizeof(char)*(f_len+6+1)))==NULL){
   printf("Memory Leak, not enough memory\n");
   close(s);
   exit(EXIT_FAILURE);
 }


 strcpy(s_buffer,"GET ");
 strcat(s_buffer,argv[i]);
 strcat(s_buffer,"\r\n\0");   /* addded /0 only for local rappesentation */

 b_len=strlen(s_buffer);

 /*****************************************************************************/
 /* sending message GET file.txt to server                                    */
 /*****************************************************************************/

 if((sendn(s,s_buffer,b_len,0))<b_len){
      printf("Error sending request to server");
      freeDataStruct(&s_buffer,NULL,NULL,&info);
      exit(EXIT_FAILURE);}

 /*****************************************************************************/
 /* Reading header from server response                                       */
 /*****************************************************************************/
 header=(char*)malloc(sizeof(char)*(7));
 memset(header,0,7);


 if(readline_unbuffered_t(s,header,5,15)<5){
   printf("Error reading +OK/-ERR MSG\n");
   freeDataStruct(&s_buffer,&header,NULL,&info);
   close(s);
   exit(EXIT_FAILURE);
 }else if(header[0]=='-'){
   readn_t(s,header,1,15);
   printf("File not found\n");
   freeDataStruct(&s_buffer,&header,NULL,&info);
   exit(EXIT_FAILURE);}

 header[6]='\0';
 /*****************************************************************************/
 /* Reading file dimension from server response                               */
 /*****************************************************************************/

 if((readn_t(s,&fdim,(int)sizeof(int),15))<(int)sizeof(int)){
   printf("Error saving file dimension\n");
   close(s);
   exit(EXIT_FAILURE);
 }
 fdim=ntohl(fdim);

 /*****************************************************************************/
 /* Read file and save in local repository                                    */
 /*****************************************************************************/
 if(fdim>MAX_DIM){ /* alloco 1023 */
   if((file_buffer=(char*)malloc(sizeof(char)*(MAX_DIM+1)))==NULL){
       printf("Memory Leak, not enough memory to allocate file_buffer\n");
       freeDataStruct(&s_buffer,&header,NULL,&info);
       close(s);
       exit(EXIT_FAILURE);}}
 else{ /* alloco dimensione giusta */
   if((file_buffer=(char*)malloc(sizeof(char)*(fdim+1)))==NULL){
       printf("Memory Leak, not enough memory to allocate file_buffer\n");
       freeDataStruct(&s_buffer,&header,NULL,&info);
       close(s);
       exit(EXIT_FAILURE);}}


 if((local_f=fopen(cl_file,"wb"))==NULL){
     printf("Error: fopen() to create file failed\n");
     freeDataStruct(&s_buffer,&header,&file_buffer,&info);
     close(s);
     exit(EXIT_FAILURE);
   }

n_left=fdim;    /* all bytes need to be read */

while(n_left>0){


 if(n_left<MAX_DIM){

        n_read=readn_t(s,file_buffer,n_left,15);

        if(n_read<n_left){
          printf("Error saving file\n");
          close(s);
          freeDataStruct(&s_buffer,&file_buffer,&header,&info);
          exit(EXIT_FAILURE);
        }

        file_buffer[n_read]='\0';

        if((fwrite(file_buffer,sizeof(char),n_left,local_f))<n_left){
            printf("Error-fwrite failed\n");
            close(s);
            freeDataStruct(&s_buffer,&file_buffer,&header,&info);
            exit(EXIT_FAILURE);
          }

        break;}
 else{

     n_read=readn_t(s,file_buffer,MAX_DIM,15);

    if(n_read<MAX_DIM){
     printf("Error saving file\n");
     close(s);
     freeDataStruct(&s_buffer,&file_buffer,&header,&info);
     exit(EXIT_FAILURE);
   }
      file_buffer[n_read]='\0';


      if((fwrite(file_buffer,sizeof(char),MAX_DIM,local_f))<MAX_DIM){
          printf("Error- fwrite() failed\n");
          close(s);
          freeDataStruct(&s_buffer,&file_buffer,&header,&info);
          exit(EXIT_FAILURE);
        }
     }

 n_left-=n_read;
 memset(file_buffer,0,strlen(file_buffer));} /* chiude while */

 if((fclose(local_f))==-1){
    printf("Error-fclose() failed\n");
    freeDataStruct(&s_buffer,&file_buffer,&header,&info);
    exit(EXIT_FAILURE);}


 printf("Saved %s by %d!\n",cl_file,getpid());


 /*****************************************************************************/
 /* Read file timestamp                                                       */
 /*****************************************************************************/


 if((readn_t(s,&ftime,(int)sizeof(int),15))<(int)sizeof(int)){
   printf("Error saving timesmap\n");
   close(s);
   freeDataStruct(&s_buffer,&file_buffer,&header,&info);
   exit(EXIT_FAILURE);
 }

 ftime=ntohl(ftime);

//////////////////////////////// DEBUG /////////////////////////////////////////
 /*printf("(DEBUG)\t\t\tmsg header\t%.3s\n",header);
 printf("(DEBUG)\t\t\tfile dimension\t%d\n",fdim);
 printf("(DEBUG)\t\t\tfile timestamp\t%d\n\n",ftime);*/
//////////////////////////////// DEBUG /////////////////////////////////////////

 freeDataStruct(&s_buffer,&file_buffer,&header,&info);


 i++; /* iterate on next file */


 }    /* close while */
 close(s);
 return 0;
 }    /* close main */
