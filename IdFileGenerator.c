#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include<sys/stat.h>
#include<stdlib.h>
#include<fcntl.h>

int main(){
	int bkId=0;
	int fd=open("uniqueIds",O_RDWR|O_CREAT,0744);
	write(fd,&bkId,sizeof(bkId));
	int fd2=creat("allbookings",0744);
	int fd3=creat("trains",0744);
	return 0;
}
