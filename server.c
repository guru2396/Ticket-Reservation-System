#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdio.h>
#include<time.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<stdlib.h>
#include<pthread.h>
#include<fcntl.h>
#include<string.h>
#include<sys/select.h>

struct custDetails{
	char username[20];
	char password[20];
};


struct trainDetails{
	int trainNo;
	char from[20];
	char to[20];
	int seats;
	int cancelled;
};

struct bookingDetails{
	int bookingId;
	int trainNo;
	char class[3];
	char from[20];
	char to[20];
	int numSeats;
	int valid;
};

void clientOps(int fd,int choice,struct custDetails cust){
	int flag=0;
	char name[15]="bookings_";
	strcat(name,cust.username);
	while(1){
		int choice=-1;
		read(fd,&choice,sizeof(choice));
		switch(choice){
			case 1:
			{
				int trainfd=open("trains",O_RDWR);
				struct trainDetails details;
				struct trainDetails trains[10];
				int i=0;
				struct flock lock;
				lock.l_type=F_WRLCK;
				lock.l_whence=SEEK_CUR;
				lock.l_start=0;
				lock.l_len=0;
				lock.l_pid=getpid();
				fcntl(trainfd,F_SETLKW,&lock);
				while(read(trainfd,&details,sizeof(details))){
					if(details.seats>0 && details.cancelled==0){
						trains[i]=details;
						i++;
					}
				}
				if((i+1)<10){
					for(int j=i;j<10;j++){
						struct trainDetails tr;
						tr.trainNo=0;
						trains[j]=tr;
					}
				}
				write(fd,&trains,sizeof(trains));
				struct bookingDetails booking;
								//use select() to wait for booking for 3 mins.
				struct timeval time;
				time.tv_sec=180;
				time.tv_usec=0;
				fd_set set;
				FD_ZERO(&set);
				FD_SET(fd,&set);
				int wait=select(fd+1,&set,NULL,NULL,&time);
				int valid=1;
				if(wait){
					read(fd,&booking,sizeof(booking));
					write(fd,&valid,sizeof(valid));
					int bookingfd=open(name,O_RDWR|O_CREAT,0744);
					struct flock aglock;
					if(choice==2){
						aglock.l_type=F_WRLCK;
						aglock.l_whence=SEEK_CUR;
						aglock.l_start=0;
						aglock.l_len=0;
						aglock.l_pid=getpid();
						fcntl(bookingfd,F_SETLKW,&aglock);
					}
					int Idfd=open("uniqueIds",O_RDWR);
					struct flock idlock;
					idlock.l_type=F_WRLCK;
					idlock.l_whence=SEEK_CUR;
					idlock.l_start=0;
					idlock.l_len=0;
					idlock.l_pid=getpid();
					fcntl(Idfd,F_SETLKW,&idlock);
					int bkId=-1;
					read(Idfd,&bkId,sizeof(bkId));
					bkId=bkId+1;
					booking.bookingId=bkId;
					lseek(Idfd,0,SEEK_SET);
					write(Idfd,&bkId,sizeof(bkId));
					idlock.l_type=F_UNLCK;
					fcntl(Idfd,F_SETLK,&idlock);
					close(Idfd);
					lseek(bookingfd,0,SEEK_END);
					write(bookingfd,&booking,sizeof(booking));
					if(choice==2){
						aglock.l_type=F_UNLCK;
						fcntl(bookingfd,F_SETLK,&aglock);
					}
					close(bookingfd);
					lseek(trainfd,0,SEEK_SET);
					int recCount=0;
					while(read(trainfd,&details,sizeof(details))){
						if(details.trainNo==booking.trainNo){
							break;
						}
						recCount++;
					}
					details.seats=details.seats-booking.numSeats;
					lseek(trainfd,(recCount)*sizeof(struct trainDetails),SEEK_SET);
					write(trainfd,&details,sizeof(details));
					write(fd,&bkId,sizeof(bkId));
					int allfd=open("allbookings",O_RDWR);
					struct flock alllock;
					alllock.l_type=F_WRLCK;
					alllock.l_whence=SEEK_CUR;
					alllock.l_start=0;
					alllock.l_len=0;
					alllock.l_pid=getpid();
					fcntl(allfd,F_SETLKW,&alllock);
					lseek(allfd,0,SEEK_END);
					write(allfd,&booking,sizeof(booking));
					alllock.l_type=F_UNLCK;
					fcntl(allfd,F_SETLK,&alllock);
					close(allfd);
				}
				else{
					read(fd,&booking,sizeof(booking));
					valid=0;
					write(fd,&valid,sizeof(valid));
				}
								
				lock.l_type=F_UNLCK;
				fcntl(trainfd,F_SETLK,&lock);
				close(trainfd);
				break;
			}

			case 2:
			{
				struct bookingDetails currDetails;
				struct bookingDetails previousBookings[20];
				int j=0;
				//printf("%s",name);
				int bfd=open(name,O_RDWR);
				struct flock aglock;
				if(choice==2){
					aglock.l_type=F_RDLCK;
					aglock.l_whence=SEEK_CUR;
					aglock.l_start=0;
					aglock.l_len=0;
					aglock.l_pid=getpid();
					fcntl(bfd,F_SETLKW,&aglock);
				}
				while(read(bfd,&currDetails,sizeof(currDetails))){
					if(currDetails.valid==1){
										//printf("\n%d",currDetails.bookingId);
						previousBookings[j]=currDetails;
						j++;
					}
				}
				if(choice==2){
					aglock.l_type=F_UNLCK;
					fcntl(bfd,F_SETLK,&aglock);
				}
				if((j+1)<20){
					for(int i=j;i<20;i++){
						struct bookingDetails bk;
						bk.bookingId=0;
						previousBookings[i]=bk;
					}
				}
				int trainfd=open("trains",O_RDWR);
				struct trainDetails details;
				struct trainDetails trains[10];
				int i=0;
				struct flock lock;
				lock.l_type=F_WRLCK;
				lock.l_whence=SEEK_CUR;
				lock.l_start=0;
				lock.l_len=0;
				lock.l_pid=getpid();
				fcntl(trainfd,F_SETLKW,&lock);
				while(read(trainfd,&details,sizeof(details))){
					trains[i]=details;
					i++;
				}
				if((i+1)<10){
					for(int k=i;k<10;k++){
						struct trainDetails tr;
						tr.trainNo=0;
						trains[k]=tr;
					}
				}
				lock.l_type=F_UNLCK;
				fcntl(trainfd,F_SETLK,&lock);
				write(fd,&previousBookings,sizeof(previousBookings));
				write(fd,&trains,sizeof(trains));
				close(bfd);
				close(trainfd);
				break;
			}

			case 3:
			{
				int bookingId=0;
				read(fd,&bookingId,sizeof(bookingId));
				struct bookingDetails bkdetails;
				int bkfd=open(name,O_RDWR);
				struct flock aglock;
				if(choice==2){
					aglock.l_type=F_RDLCK;
					aglock.l_whence=SEEK_CUR;
					aglock.l_start=0;
					aglock.l_len=0;
					aglock.l_pid=getpid();
					fcntl(bkfd,F_SETLKW,&aglock);
				}
				int rec=0;
				int found=0;
				while(read(bkfd,&bkdetails,sizeof(bkdetails))){
					if(bkdetails.bookingId==bookingId){
						found=1;
						break;
					}
					rec++;
				}
				write(fd,&found,sizeof(found));
				if(found==1){
					write(fd,&bkdetails,sizeof(bkdetails));
					read(fd,&bkdetails,sizeof(bkdetails));
					lseek(bkfd,(rec)*sizeof(struct bookingDetails),SEEK_SET);
					write(bkfd,&bkdetails,sizeof(bkdetails));
					int allfd=open("allbookings",O_RDWR);
					struct flock lock;
					lock.l_type=F_WRLCK;
					lock.l_whence=SEEK_CUR;
					lock.l_start=0;
					lock.l_len=0;
					lock.l_pid=getpid();
					fcntl(allfd,F_SETLKW,&lock);
					struct bookingDetails details;
					rec=0;
					while(read(allfd,&details,sizeof(details))){
						if(details.bookingId==bookingId){
							break;
						}
						rec++;
					}
					lseek(allfd,(rec)*sizeof(struct bookingDetails),SEEK_SET);
					write(allfd,&bkdetails,sizeof(bkdetails));
					lock.l_type=F_UNLCK;
					fcntl(allfd,F_SETLK,&lock);
					close(allfd);
				}
				if(choice==2){
					aglock.l_type=F_UNLCK;
					fcntl(bkfd,F_SETLK,&aglock);
				}
				close(bkfd);
				break;
			}

			case 4:
			{
				int id=0;
				read(fd,&id,sizeof(id));
				int bookfd=open(name,O_RDWR);
				struct flock aglock;
				if(choice==2){
					aglock.l_type=F_RDLCK;
					aglock.l_whence=SEEK_CUR;
					aglock.l_start=0;
					aglock.l_len=0;
					aglock.l_pid=getpid();
					fcntl(bookfd,F_SETLKW,&aglock);
				}
				struct bookingDetails currbook;
				int cnt=0;
				int fnd=0;
				while(read(bookfd,&currbook,sizeof(currbook))){
					if(currbook.bookingId==id){
						fnd=1;
						break;
					}
					cnt++;
				}
				write(fd,&fnd,sizeof(fnd));
				if(fnd==1){
					currbook.valid=0;
					int tfd=open("trains",O_RDWR);
					struct flock trlock;
					trlock.l_type=F_WRLCK;
					trlock.l_whence=SEEK_CUR;
					trlock.l_start=0;
					trlock.l_len=0;
					trlock.l_pid=getpid();
					fcntl(tfd,F_SETLKW,&trlock);
					struct trainDetails currtrain;
					int recNo=0;
					while(read(tfd,&currtrain,sizeof(currtrain))){
						if(currtrain.trainNo==currbook.trainNo){
							break;
						}
						recNo++;
					}
					currtrain.seats=currtrain.seats + currbook.numSeats;
					lseek(tfd,(recNo)*sizeof(struct trainDetails),SEEK_SET);
					write(tfd,&currtrain,sizeof(currtrain));
					trlock.l_type=F_UNLCK;
					fcntl(tfd,F_SETLK,&trlock);
					lseek(bookfd,(cnt)*sizeof(struct bookingDetails),SEEK_SET);
					write(bookfd,&currbook,sizeof(currbook));
					int allfd=open("allbookings",O_RDWR);
					struct flock lock;
					lock.l_type=F_WRLCK;
					lock.l_whence=SEEK_CUR;
					lock.l_start=0;
					lock.l_len=0;
					lock.l_pid=getpid();
					fcntl(allfd,F_SETLKW,&lock);
					int rec=0;
					struct bookingDetails details;
					while(read(allfd,&details,sizeof(details))){
						if(details.bookingId==id){
							break;
						}
						rec++;
					}
					details.valid=0;
					lseek(allfd,(rec)*sizeof(struct bookingDetails),SEEK_SET);
					write(allfd,&details,sizeof(details));
					lock.l_type=F_UNLCK;
					fcntl(allfd,F_SETLK,&lock);
					close(tfd);
					close(allfd);
				}
				if(choice==2){
					aglock.l_type=F_UNLCK;
					fcntl(bookfd,F_SETLK,&aglock);
				}			
				close(bookfd);
				break;
			}

			case 5:
			{
				int bkid=0;
				read(fd,&bkid,sizeof(bkid));
				int found=0;
				int bookfd=open(name,O_RDWR);
				struct flock aglock;
				if(choice==2){
					aglock.l_type=F_RDLCK;
					aglock.l_whence=SEEK_CUR;
					aglock.l_start=0;
					aglock.l_len=0;
					aglock.l_pid=getpid();
					fcntl(bookfd,F_SETLKW,&aglock);
				}
				struct bookingDetails details;
				while(read(bookfd,&details,sizeof(details))){
					if(details.bookingId==bkid && details.valid==1){
						found=1;
						break;
					}
				}
				write(fd,&found,sizeof(found));
				if(found){
					int tfd=open("trains",O_RDWR);
					struct flock trlock;
					trlock.l_type=F_WRLCK;
					trlock.l_whence=SEEK_CUR;
					trlock.l_start=0;
					trlock.l_len=0;
					trlock.l_pid=getpid();
					fcntl(tfd,F_SETLKW,&trlock);
					struct trainDetails train;
					while(read(tfd,&train,sizeof(train))){
						if(train.trainNo==details.trainNo){
							break;
						}
					}
					write(fd,&details,sizeof(details));
					write(fd,&train,sizeof(train));
					trlock.l_type=F_UNLCK;
					fcntl(tfd,F_SETLK,&trlock);
					close(tfd);
				}
				if(choice==2){
					aglock.l_type=F_UNLCK;
					fcntl(bookfd,F_SETLK,&aglock);
				}
				close(bookfd);
				break;
		
			}		
		
			case 6:
				flag=1;
				break;
		}
		if(flag==1){
			break;
		}
	}
}

void clienthandler(int fd){
	//int fd=*((int *)sockfd);
	int loginOption=-1;
	read(fd,&loginOption,sizeof(loginOption));
	switch(loginOption){
		case 1:;
			int option=-1;
			read(fd,&option,sizeof(option));
			struct custDetails cust;
			read(fd,&cust,sizeof(struct custDetails));
			if(option==1){
				int filefd=creat(cust.username,0744);
				write(filefd,&cust,sizeof(struct custDetails));
			}
			else{
				int loginSuccess=0;
				int filefd=open(cust.username,O_RDWR);
				if(filefd==-1){
					write(fd,&loginSuccess,sizeof(loginSuccess));
				}
				else{
					struct custDetails currcust;
					read(filefd,&currcust,sizeof(currcust));
					int ret=strcmp(currcust.password,cust.password);
					if(ret==0){
						loginSuccess=1;
					}

				}
				write(fd,&loginSuccess,sizeof(loginSuccess));
				if(loginSuccess){
					clientOps(fd,1,cust);	
				}
			}
			break;
			
		
		case 2:
		{
			int choice=-1;
			read(fd,&choice,sizeof(choice));
			struct custDetails agent;
			read(fd,&agent,sizeof(agent));
			if(choice==1){
				int agfd=creat(agent.username,0744);
				write(agfd,&agent,sizeof(agent));
			}
			else{
				int loginSuccess=0;
				int agfd=open(agent.username,O_RDWR);
				if(agfd==-1){
					write(fd,&loginSuccess,sizeof(loginSuccess));
				}
				else{
					struct flock aglock;
					aglock.l_type=F_RDLCK;
					aglock.l_whence=SEEK_CUR;
					aglock.l_start=0;
					aglock.l_len=0;
					aglock.l_pid=getpid();
					fcntl(agfd,F_SETLKW,&aglock);
					struct custDetails currag;
					read(agfd,&currag,sizeof(currag));
					aglock.l_type=F_UNLCK;
					fcntl(agfd,F_SETLK,&aglock);
					int ret=strcmp(agent.password,currag.password);
					if(ret==0){
						loginSuccess=1;
					}
					write(fd,&loginSuccess,sizeof(loginSuccess));
					if(loginSuccess){
						clientOps(fd,2,agent);
					}
				}
				
				
			}
			break;
		}
		
		case 3:;
			struct custDetails admin;
			read(fd,&admin,sizeof(struct custDetails));
			char name[]="admin";
			char pass[]="admin";
			//printf("%s\n",admin.username);
			//printf("%s\n",admin.password);
			int ret1=strcmp(name,admin.username);
			int ret2=strcmp(pass,admin.password);
			int loginSuccess=0;
			if(ret1==0 && ret2==0){
				loginSuccess=1;
				write(fd,&loginSuccess,sizeof(loginSuccess));
				int flag=0;
				while(1){
					int choice=-1;
					read(fd,&choice,sizeof(choice));
					switch(choice){
						case 1:
						{
							struct trainDetails train;
							read(fd,&train,sizeof(train));
							int trainfd=open("trains",O_CREAT|O_RDWR,0744);
							struct flock lock;
							lock.l_type=F_WRLCK;
							lock.l_whence=SEEK_CUR;
							lock.l_start=0;
							lock.l_len=0;
							lock.l_pid=getpid();
							fcntl(trainfd,F_SETLKW,&lock);
							lseek(trainfd,0,SEEK_END);
							write(trainfd,&train,sizeof(train));
							lock.l_type=F_UNLCK;
							fcntl(trainfd,F_SETLK,&lock);
							close(trainfd);
							break;
						}
						
						case 2:
						{
							int trfd=open("trains",O_RDONLY);
							struct flock trlock;
							trlock.l_type=F_RDLCK;
							trlock.l_whence=SEEK_CUR;
							trlock.l_start=0;
							trlock.l_len=0;
							trlock.l_pid=getpid();
							fcntl(trfd,F_SETLKW,&trlock);
							struct trainDetails trains[10];
							struct trainDetails train;
							int i=0;
							while(read(trfd,&train,sizeof(train))){
								trains[i]=train;
								i++;
							}
							if(i+1<10){
								for(int j=i;j<10;j++){
									struct trainDetails tr;
									tr.trainNo=0;
									trains[j]=tr;
								}
							}
							trlock.l_type=F_UNLCK;
							fcntl(trfd,F_SETLK,&trlock);
							write(fd,&trains,sizeof(trains));
							close(trfd);
							break;
						}

						case 3:
						{
							int trainNo=-1;
							read(fd,&trainNo,sizeof(trainNo));
							int tfd=open("trains",O_RDWR);
							struct flock trlock;
							trlock.l_type=F_WRLCK;
							trlock.l_whence=SEEK_CUR;
							trlock.l_start=0;
							trlock.l_len=0;
							trlock.l_pid=getpid();
							fcntl(tfd,F_SETLKW,&trlock);
							struct trainDetails train;
							int rec=0;
							int found=0;
							while(read(tfd,&train,sizeof(train))){
								if(train.trainNo==trainNo && train.cancelled==0){
									found=1;
									break;
								}
								rec++;
							}
							write(fd,&found,sizeof(found));
							if(found==1){
								write(fd,&train,sizeof(train));
								read(fd,&train,sizeof(train));
								lseek(tfd,(rec)*sizeof(struct trainDetails),SEEK_SET);
								write(tfd,&train,sizeof(train));
							}
							trlock.l_type=F_UNLCK;
							fcntl(tfd,F_SETLK,&trlock);
							close(tfd);
							break;
						}

						case 4:
						{
							int trainNo=0;
							read(fd,&trainNo,sizeof(trainNo));
							int tfd=open("trains",O_RDWR);
							struct flock trlock;
							trlock.l_type=F_WRLCK;
							trlock.l_whence=SEEK_CUR;
							trlock.l_start=0;
							trlock.l_len=0;
							trlock.l_pid=getpid();
							fcntl(tfd,F_SETLKW,&trlock);
							struct trainDetails train;
							int rec=0;
							int found=0;
							while(read(tfd,&train,sizeof(train))){
								if(train.trainNo==trainNo){
									found=1;
									break;
								}
								rec++;
							}
							write(fd,&found,sizeof(found));
							if(found==1){
								train.cancelled=1;
								lseek(tfd,(rec)*sizeof(struct trainDetails),SEEK_SET);
								write(tfd,&train,sizeof(train));
							}
							trlock.l_type=F_UNLCK;
							fcntl(tfd,F_SETLK,&trlock);
							close(tfd);
							break;
						}

						case 5:
						{
							int bkid=0;
							read(fd,&bkid,sizeof(bkid));
							int allfd=open("allbookings",O_RDWR);
							struct flock lock;
							lock.l_type=F_RDLCK;
							lock.l_whence=SEEK_CUR;
							lock.l_start=0;
							lock.l_len=0;
							lock.l_pid=getpid();
							fcntl(allfd,F_SETLKW,&lock);
							struct bookingDetails details;
							int found=0;
							while(read(allfd,&details,sizeof(details))){
								if(details.bookingId==bkid){
									found=1;
									break;
								}
							}
							write(fd,&found,sizeof(found));
							if(found){
								write(fd,&details,sizeof(details));
							}
							lock.l_type=F_UNLCK;
							fcntl(allfd,F_SETLK,&lock);
							close(allfd);
							break;
						}

						case 6:
						{
							int trainNo=0;
							read(fd,&trainNo,sizeof(trainNo));
							int allfd=open("allbookings",O_RDWR);
							struct flock lock;
							lock.l_type=F_RDLCK;
							lock.l_whence=SEEK_CUR;
							lock.l_start=0;
							lock.l_len=0;
							lock.l_pid=getpid();
							fcntl(allfd,F_SETLKW,&lock);
							struct bookingDetails allbookings[40];
							struct bookingDetails details;
							int i=0;
							while(read(allfd,&details,sizeof(details))){
								if(details.trainNo==trainNo){
									allbookings[i]=details;
									i++;
								}
							}
							if(i+1<40){
								struct bookingDetails det;
								det.bookingId=0;
								allbookings[i]=det;
							}
							lock.l_type=F_UNLCK;
							fcntl(allfd,F_SETLK,&lock);
							write(fd,&allbookings,sizeof(allbookings));
							close(allfd);
							break;
						}					
		
						case 7:
							flag=1;
							break;
					}
					if(flag==1){
						break;
					}
				}
				
			}
			else{
				write(fd,&loginSuccess,sizeof(loginSuccess));
			}
			break;
	}
	close(fd);
}

int main(){
	int sockfd=socket(AF_UNIX,SOCK_STREAM,0);
	struct sockaddr_in server,client;
	server.sin_family=AF_UNIX;
	server.sin_addr.s_addr=INADDR_ANY;
	server.sin_port=htons(5553);
	socklen_t size=sizeof(struct sockaddr_in);
	bind(sockfd,(struct sockaddr *)&server,sizeof(server));
	listen(sockfd,5);
	while(1){
		int newsockfd=accept(sockfd,(struct sockaddr *)&client,&size);
		if(!fork()){
			close(sockfd);
			clienthandler(newsockfd);
			exit(0);
		}
		else{
			close(newsockfd);
		}
		//pthread_t thread;
		//pthread_create(&thread,NULL,clienthandler,(void *)&newsockfd);
		//close(newsockfd);	
	}
	return 0;
}
