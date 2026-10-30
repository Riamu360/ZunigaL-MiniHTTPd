#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define QLEN 5
#define BUFSZ 1024

int passiveTCP(const char *service, int qlen);

int main(int argc, char *argv[]){
    struct sockaddr_in cltAddr;
    char *fname, *ptrF, *ptrS, *ptrE, buf[BUFSZ];
    socklen_t szAddr = sizeof(cltAddr);
    int fd, sd, sn, nsd;
    int nread;

    sd = passiveTCP("http", QLEN);

    while(1){
         nsd = accept(sd, (struct sockaddr *)&cltAddr, &szAddr);
	 nread = read (nsd, buf, BUFSZ);

	 ptrS = strstr (buf, "/");
	 ptrE = strstr (ptrS, " ");
	 ptrE[0] = '\0';
	 fname = ptrS + 1;

	 if (strlen(fname) == 0) {
             fname = "index.html";
         }

	 fd = open(fname, O_RDONLY);
	 while((nread = read(fd, buf, BUFSZ))>0){
	      write(nsd, buf, nread);
	 }
	 close(fd);
	 close(nsd);
    }
}
