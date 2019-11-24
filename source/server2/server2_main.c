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
#include <sys/wait.h>
#include "../errlib.h"
#include "../sockwrap.h"
#include "../myLib.h"

char* prog_name;
#define MAXLINE 30
#define BKLOG 20
#define MAX_DIM 1023
#define MAX_CHILD 10000

 int n_child=0;

 pid_t pid,
 			 child_pid=-1,
			 parent_pid=-1,
       pids[MAX_CHILD];

 int  s1=-1,
		  s2=-1;

static void sig_chld(int signo) {
	int status;
	int pid;
  pid=wait(NULL);

  for(int i=0;i<MAX_CHILD;i++){
    if(pids[i]==pid){
        pids[i]=0;
        break;}
  }

  n_child--;
	return;
}

static void sig_pipe(int signo){};

static void sig_int(int signo){

	pid_t curr=getpid();

	if(curr==parent_pid){
			if(s1>=0)
				close(s1);
			signal(SIGCHLD,SIG_DFL);
			for(int i=0;i<MAX_CHILD;i++){
        if(pids[i]!=0){
            printf("PID KILLED %d\n",pids[i]);
            kill(pids[i],SIGINT);}}

			exit(0);
}
	else if(curr==child_pid){
			if(s2>=0)
				close(s2);
				exit(0);}


  exit(0);

}

static void sig_term(int signo){

	pid_t curr=getpid();

	if(curr==parent_pid){
			if(s1>=0)
				close(s1);
			signal(SIGCHLD,SIG_DFL);

      for(int j=0;j<MAX_CHILD;j++){
        if(pids[j]!=0){
            printf("PID KILLED %d\n",pids[j]);
            kill(pids[j],SIGTERM);}}

			exit(0);
}
	else if(curr==child_pid){
			if(s2>=0)
				close(s2);
				exit(0);}


  exit(0);

}

int main(int argc,char **argv){

struct sockaddr_in saddr,caddr;
uint16_t port_n;


 /*****************************************************************************/
 /* Free data struct                                                          */
 /*****************************************************************************/
 if(argc-1!=1){
   printf("(./ServerTCP)-Error: wrong number of arguments\n");
   exit(EXIT_FAILURE);
 }

 if(strlen(argv[1])>5){
   printf("(./ServerTCP)-Error: exceeded max port number\n");
   exit(EXIT_FAILURE);}

 /*****************************************************************************/
 /* Free data struct                                                          */
 /*****************************************************************************/
 port_n=atoi(argv[1]);
 if(port_n<1024){
   printf("(./ServerTCP)-Error: port number accessible only by SUPER-USER\n");
   exit(EXIT_FAILURE);}

 /*****************************************************************************/
 /* Struct inizialization                                                     */
 /*****************************************************************************/
 memset(&saddr,0,sizeof(saddr));

 inet_aton("0.0.0.0",(struct in_addr*)&saddr.sin_addr.s_addr); //SERVER ADDRESS
 saddr.sin_family=AF_INET;      //ADDRESS FAMILY
 saddr.sin_port=htons(port_n);  //SERVER PORT

 /*****************************************************************************/
 /* Socket creation                                                           */
 /*****************************************************************************/
 s1=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
 s2=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);

 if(s1==-1||s2==-1){
    perror("(./ServerTCP)-socket creation failed\n");
    exit(EXIT_FAILURE);}

 /*****************************************************************************/
 /* Socket binding                                                            */
 /*****************************************************************************/
 if ((bind(s1,(struct sockaddr *)&saddr, sizeof(saddr)))<0) {
      printf("(./ServerTCP)-socket bind failed\n");
      close(s1);
      exit(EXIT_FAILURE);}

 /*****************************************************************************/
 /* Listening                                                                 */
 /*****************************************************************************/
 if ((listen(s1,BKLOG))<0) {
       printf("(./ServerTCP)-Listen failed\n");
       close(s1);
       exit(EXIT_FAILURE); }

/*/////////////////////////////  AREA DATI  //////////////////////////////////*/
 socklen_t addrlen=sizeof(caddr);

 char *s_buffer=NULL,    //buffer to receive data from client
      *f_path=NULL,      //buffer to store the path to the file
      err[7],				     //store error message
 			*f_buffer=NULL;    //buffer that contains file data

 FILE *f;

 uint32_t f_size,   //dimensione del file
 					ts;

 int	n,
			n_read,
			n_left,
			nf,
			error,
			rd,
      j;

 struct stat st;
/*////////////////////////////////////////////////////////////////////////////*/
 signal(SIGCHLD,sig_chld);
 signal(SIGPIPE,sig_pipe);
 signal(SIGINT,sig_int);
 signal(SIGTERM,sig_term);

 strcpy(err,"-ERR\r\n");
 printf("PARENT %d\n",getpid());

 memset(pids,0,MAX_CHILD);

 while(1){
	 nf=0;
	 error=0;
 /*****************************************************************************/
 /* Waiting for new connection                                                */
 /*****************************************************************************/

 if((s2=(accept(s1,(struct sockaddr *)&caddr,&addrlen))) < 0){
     printf("(./ServerTCP)-accept() failed");
     close(s1);
     exit(EXIT_FAILURE);}


 /*****************************************************************************/
 /* fork() : 																																	*/
 /*           - father will wait for new connections   												*/
 /*         	- son send files to the client and exit.               					*/
 /*****************************************************************************/
 pid=fork();

 if(pid<0){
     printf("(./ServerTCP)-fork() failed, fatal eror \n");
     exit(EXIT_FAILURE);}
 else if(pid>0){
     close(s2);
     for(j=0;j<MAX_CHILD;j++){
       if(pids[j]==0){
          pids[j]=pid;
          break;}
     }
		 parent_pid=getpid();
      n_child++;
	 	 continue;}
 else if(pid==0){
     close(s1);
		 child_pid=getpid();
     parent_pid=getppid();


 /*****************************************************************************/
 /* While interno itera su diversi file della stessa connessione              */
 /*****************************************************************************/
 while(1){

 nf++;

 if((s_buffer=(char*)malloc(sizeof(char)*(MAXLINE+1)))==NULL){
      printf("Error-malloc() failed\n");
      break;}

 if((f_path=(char*)malloc(sizeof(char)*(MAXLINE+1)))==NULL){
      printf("Error-malloc() failed\n");
      break;};

 if((rd=readline_unbuffered_t(s2,s_buffer,MAXLINE,15))<0){
   //printf("Error receving GETMSG\n");
   close(s2);
   break;
 }else if(rd==0){
   close(s2);
   break;
 }
 s_buffer[rd]='\0';

 if(s_buffer[0]!='G'||s_buffer[1]!='E'||s_buffer[2]!='T'){
   printf("Received wrong GET_MSG");
   if((send(s2,err,6,0))<6){
       printf("(./ServerTCP)-Error: ERR MSG sending failed\n");}
   close(s2);
   break;

 }

 /*****************************************************************************/
 /* Taking file name from client message                                      */
 /*****************************************************************************/
 for(int i=4;;i++){
      if(s_buffer[i]=='\r'){
            f_path[i-4]='\0';
            break;}
     f_path[i-4]=s_buffer[i];}

 /*****************************************************************************/
 /* Opening file requested from client,if file not present close connection   */
 /*****************************************************************************/
 if((f=fopen(f_path,"rb"))==NULL){
      printf("Error: file does not exist\n");
      if((send(s2,err,6,MSG_NOSIGNAL))<6){
          printf("(./ServerTCP)-Error: ERR MSG sending failed\n");}
    break;}

 /*****************************************************************************/
 /* Using struct stat to take file dimension and timestamp                    */
 /*****************************************************************************/

  if((stat(f_path,&st))==-1){
    printf("(./serverTCP)-Error: stat() failed, file can not be sent\n");
    break;}

  f_size=(int)st.st_size; //file dimension
  ts=(int)st.st_mtime;    //file timestamp

 /*****************************************************************************/
 /* Sending +OK MSG and file dimension to client                              */
 /*****************************************************************************/
 if((send(s2,"+OK\r\n",5,MSG_NOSIGNAL)<5)){
			printf("Error sending +OK MSG");
			break;}



 int f_size_c=htonl(f_size); //conversion then send

 if((send(s2,&f_size_c,(int)sizeof(int),0)<(int)sizeof(int))){
			printf("Error sending file dimension");
			break;}

 /*****************************************************************************/
 /* Alllocazione file buffer                                                  */
 /*****************************************************************************/

 if(f_size>MAX_DIM){
 if((f_buffer=(char*)malloc(sizeof(char)*(MAX_DIM+1)))==NULL){  //alloco 1024
			printf("Error-malloc() failed\n");
			break;}}
 else{
	 if((f_buffer=(char*)malloc(sizeof(char)*(f_size+1)))==NULL){ //alloco giusta dimensione
				printf("Error-malloc() failed\n");
				break;}}

 /*****************************************************************************/
 /* Reading file content and send block to client                             */
 /*****************************************************************************/
 n_left=f_size;
 n_read=0;
 while(n_left>0){

 if((n_read=fread(f_buffer,sizeof(char),MAX_DIM,f))<0){
			printf("Error: can't read the entire file)");
			break;}

 f_buffer[n_read]='\0';

 if((send(s2,f_buffer,n_read,MSG_NOSIGNAL)<n_read)){
			error=1;
			printf("Error sending %s! by %d\n",takeNameFromPath(f_path),getpid());
			fflush(stdout);
			break;  }

 n_left=n_left-n_read;
 memset(f_buffer,0,strlen(f_buffer));}//chiude while

 if(!error){
		printf("Sent %s! by %d\n",takeNameFromPath(f_path),getpid());
		fflush(stdout);}
 else
		break;

 if((fclose(f))==-1){
			printf("(ServerTCP)-Error closing file\n");
      break;
		}

 /*****************************************************************************/
 /* Sending file timestamp to the client                                      */
 /*****************************************************************************/

 int ts_c=htonl(ts); //conversion then send

 fflush(stdout);
 if((send(s2,&ts_c,(int)sizeof(int),MSG_NOSIGNAL)<(int)sizeof(int))){
      printf("Error sending file timestamp");
      break;}

//////////////////////////////// DEBUG /////////////////////////////////////////
/*printf("(DEBUG)\t\t\tmsg header\t%s",s_buffer);
printf("(DEBUG)\t\t\tfile dimension\t%d\n",f_size);
printf("(DEBUG)\t\t\tfile timestamp\t%d\n\n",ts);*/
//////////////////////////////// DEBUG /////////////////////////////////////////
freeDataStruct(&s_buffer,&f_path,&f_buffer,NULL);

 } //chiude while interno
 close(s2);
 freeDataStruct(&s_buffer,&f_path,&f_buffer,NULL);
 exit(0);
} //chiude figlio fork

} //chiude while esterno
return 0;

} //chiude main
