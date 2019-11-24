int recv_t(int fd, void *bufptr, size_t nbytes, int flags,int t);
int readline_unbuffered_t(int fd, void *bufptr, size_t nbytes,int t);
int readn_t(int fd, void *bufptr, size_t nbytes,int t);
void freeDataStruct(char** buf1,char** buf2,char** buf3,struct addrinfo** info);
char* takeNameFromPath(char* path);