#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<stdio.h>
#include<time.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<stdlib.h>
#include<string.h>
#include<sys/select.h>
#include<signal.h>

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

void userOperations(int sockfd,int choice){
	int flag=0;
	while(1){
		printf("\n1)Book ticket");
		printf("\n2)View previous bookings");
		printf("\n3)Update booking");
		printf("\n4)Cancel booking");
		printf("\n5)Search booking");
		printf("\n6)Exit");
		int op=-1;
		printf("\nPlease choose operation:");
		scanf("%d",&op);
		write(sockfd,&op,sizeof(op));
		switch(op){
			case 1:
			{
				struct bookingDetails booking;
				struct trainDetails trains[10];
				printf("\nFetching all available trains..");
				read(sockfd,&trains,sizeof(trains));
				printf("\nBelow are the available trains and their details\n");
				printf("\nTrain\tFrom\tTo\t#seats");
				for(int i=0;i<10;i++){
					if(trains[i].trainNo==0){
						break;
					}
					printf("\n%d\t%s\t%s\t%d",trains[i].trainNo,trains[i].from,trains[i].to,trains[i].seats);
				}
				printf("\nEnter train number:");
				scanf("%d",&booking.trainNo);
				printf("\nEnter class:");
				scanf("%s",booking.class);
				printf("\nEnter number of seats to be booked:");
				scanf("%d",&booking.numSeats);
				booking.valid=1;
								//booking.bookingId=1234;
				for(int i=0;i<10;i++){
					if(trains[i].trainNo==booking.trainNo){
						strcpy(booking.from,trains[i].from);
						strcpy(booking.to,trains[i].to);
					}
				}
								//signal(SIGPIPE,SIG_IGN);
				int p=write(sockfd,&booking,sizeof(booking));
				int valid=0;
				read(sockfd,&valid,sizeof(valid));
				if(valid==0){
					printf("\nBooking failed.Session timed out\n");
				}
				else{
					int bkId=-1;
					read(sockfd,&bkId,sizeof(bkId));
					printf("Your booking id:%d\n",bkId);
				}
								//printf("\n%d\n",p);
								//printf("\nBooking failed.Session timed out\n");
								
				break;
			}
						
			case 2:
			{
				struct bookingDetails previousBookings[20];
				struct trainDetails trains[10];
				printf("\nFetching your past bookings..");
				read(sockfd,&previousBookings,sizeof(previousBookings));
				read(sockfd,&trains,sizeof(trains));
				printf("\nBelow are past bookings details\n");
				printf("\nBkId\tTrain\tFrom\tTo\tclass\t#seats\tTrain status");
				for(int i=0;i<20;i++){
					if(previousBookings[i].bookingId==0){
						break;
					}
					printf("\n%d",previousBookings[i].bookingId);
					printf("\t%d",previousBookings[i].trainNo);
					printf("\t%s",previousBookings[i].from);
					printf("\t%s",previousBookings[i].to);
					printf("\t%s",previousBookings[i].class);
					printf("\t%d",previousBookings[i].numSeats);
					for(int j=0;j<10;j++){
						if(previousBookings[i].trainNo==trains[j].trainNo){
							if(trains[j].cancelled==1){
								printf("\tCancelled");
							}
							else{
								printf("\tScheduled");
							}
							break;
						}
					}
				}
				printf("\n");
				break;
			}
							
			case 3:
			{
				int bookingId=0;
				int found=0;
				printf("\nEnter your booking Id:");
				scanf("%d",&bookingId);
				write(sockfd,&bookingId,sizeof(bookingId));
				read(sockfd,&found,sizeof(found));
				if(found==0){
					printf("\nBooking Id not found");
				}
				else{
					struct bookingDetails bkdetails;
					read(sockfd,&bkdetails,sizeof(bkdetails));
					printf("\nBelow are the details of the booking\n");
					printf("\nBkId\tTrain\tFrom\tTo\tclass\t#seats");
					printf("\n%d",bkdetails.bookingId);
					printf("\t%d",bkdetails.trainNo);
					printf("\t%s",bkdetails.from);
					printf("\t%s",bkdetails.to);
					printf("\t%s",bkdetails.class);
					printf("\t%d",bkdetails.numSeats);
					printf("\nYou can update below details of booking");
					printf("\nEnter class:");
					scanf("%s",bkdetails.class);
					write(sockfd,&bkdetails,sizeof(bkdetails));
					printf("\nBooking has been updated");
				}
								
				break;
			}
			
			case 4:
			{
				int id=0;
				int fnd=0;
				printf("\nEnter your booking id:");
				scanf("%d",&id);
				write(sockfd,&id,sizeof(id));
				read(sockfd,&fnd,sizeof(fnd));
				if(fnd==0){
					printf("\nBooking Id not found");
				}
				else{
					printf("\nBooking cancelled");
				}
								
				break;
			}

			case 5:
			{
				int bkid=0;
				printf("\nEnter booking id:");
				scanf("%d",&bkid);
				write(sockfd,&bkid,sizeof(bkid));
				int found=0;
				read(sockfd,&found,sizeof(found));
				if(found==0){
					printf("\nBooking not found\n");
				}
				else{
					struct bookingDetails details;
					struct trainDetails train;
					read(sockfd,&details,sizeof(details));
					read(sockfd,&train,sizeof(train));
					printf("\nBkId\tTrain\tFrom\tTo\tclass\t#seats\tTrain status");
					printf("\n%d",details.bookingId);
					printf("\t%d",details.trainNo);
					printf("\t%s",details.from);
					printf("\t%s",details.to);
					printf("\t%s",details.class);
					printf("\t%d",details.numSeats);
					if(train.cancelled==1){
						printf("\tCancelled");
					}
					else{
						printf("\tScheduled");
					}
					
				}
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

int main(){
	int sockfd=socket(AF_UNIX,SOCK_STREAM,0);
	struct sockaddr_in server;
	server.sin_family=AF_UNIX;
	server.sin_addr.s_addr=INADDR_ANY;
	server.sin_port=htons(5553);
	connect(sockfd,(struct sockaddr *)&server,sizeof(server));
	printf("Connected to server\n");
	printf("1)Customer\n");
	printf("2)Agent\n");
	printf("3)Admin\n");
	int loginOption=-1;
	printf("Choose login option:");
	scanf("%d",&loginOption);
	write(sockfd,&loginOption,sizeof(loginOption));
	switch(loginOption){
		case 1:
			printf("\n1)Register Customer");
			printf("\n2)Login Customer");
			int choice=-1;
			printf("\nEnter choice:");
			scanf("%d",&choice);
			write(sockfd,&choice,sizeof(choice));
			struct custDetails cust;
			printf("Enter username:");
			scanf("%s",cust.username);
			printf("\nEnter password:");
			scanf("%s",cust.password);
			write(sockfd,&cust,sizeof(cust));
			if(choice==2){
				int loginSuccess=-1;
				read(sockfd,&loginSuccess,sizeof(loginSuccess));
				if(loginSuccess==0){
					printf("\nLogin Failed.Incorrect login credentials");
				}
				else{
					userOperations(sockfd,1);
				}
				
			}
			
			break;	
			
		
		case 2:
		{
			printf("\n1)Register Agent");
			printf("\n2)Login Agent");
			int choice=-1;
			printf("\nEnter choice:");
			scanf("%d",&choice);
			struct custDetails agent;
			printf("\nEnter agent username:");
			scanf("%s",agent.username);
			printf("\nEnter agent password:");
			scanf("%s",agent.password);
			write(sockfd,&choice,sizeof(choice));
			write(sockfd,&agent,sizeof(agent));
			if(choice==2){
				int loginSuccess=-1;
				read(sockfd,&loginSuccess,sizeof(loginSuccess));
				if(loginSuccess){
					userOperations(sockfd,2);
				}
				else{
					printf("\nLogin Failed.Incorrect credentials\n");
				}
			}
			break;
		}
		
		case 3:;
			struct custDetails admin;
			printf("Enter admin username:");
			scanf("%s",admin.username);
			printf("\nEnter admin password:");
			scanf("%s",admin.password);
			write(sockfd,&admin,sizeof(struct custDetails));
			int loginSuccess=0;
			read(sockfd,&loginSuccess,sizeof(loginSuccess));
			if(loginSuccess){
				int flag=0;
				while(1){
					printf("\n1)Add Train");
					printf("\n2)View all trains");
					printf("\n3)Update train details");
					printf("\n4)Cancel train");
					printf("\n5)Search booking");
					printf("\n6)View all bookings for a train");
					printf("\n7)Exit");
					int ch=-1;
					printf("\nEnter operation:");
					scanf("%d",&ch);
					write(sockfd,&ch,sizeof(ch));
					switch(ch){
						case 1:
						{
							struct trainDetails train;
							printf("\nEnter train number:");
							scanf("%d",&train.trainNo);
							printf("\nEnter source:");
							scanf("%s",train.from);
							printf("\nEnter destination:");
							scanf("%s",train.to);
							printf("\nEnter number of seats");
							scanf("%d",&train.seats);
							train.cancelled=0;
							write(sockfd,&train,sizeof(train));
							printf("\nTrain added successfully\n");
							break;
						}
						
						case 2:
						{
							struct trainDetails trains[10];
							printf("\nFetching all train details..");
							read(sockfd,&trains,sizeof(trains));
							printf("\nBelow are all train details\n");
							printf("\nTrain\tFrom\tTo\t#seats\tCancelled");
							for(int i=0;i<10;i++){
								if(trains[i].trainNo==0){
									break;
								}
								printf("\n%d",trains[i].trainNo);
								printf("\t%s",trains[i].from);
								printf("\t%s",trains[i].to);
								printf("\t%d",trains[i].seats);
								if(trains[i].cancelled==1){
									printf("\tYes");
								}
								else{
									printf("\tNo");
								}
							}
							printf("\n");
							break;
						}
			
						case 3:
						{
							int trainNo=-1;
							printf("\nEnter train number:");
							scanf("%d",&trainNo);
							write(sockfd,&trainNo,sizeof(trainNo));
							int found=0;
							read(sockfd,&found,sizeof(found));
							if(found==0){
								printf("\nTrain not found\n");
							}
							else{
								struct trainDetails train;
								read(sockfd,&train,sizeof(train));
								printf("\nBelow are the details of the train\n");
								printf("\nTrain\tFrom\tTo\t#seats\tCancelled");
								printf("\n%d",train.trainNo);
								printf("\t%s",train.from);
								printf("\t%s",train.to);
								printf("\t%d",train.seats);
								printf("\t%d",train.cancelled);
								printf("\nYou can update below details of train");
								printf("\nEnter source:");
								scanf("%s",train.from);
								printf("\nEnter destination:");
								scanf("%s",train.to);
								printf("\nEnter number of seats:");
								scanf("%d",&train.seats);
								write(sockfd,&train,sizeof(train));
							}
							break;
							
						}

						case 4:
						{
							int trainNo=0;
							printf("\nEnter train number:");
							scanf("%d",&trainNo);
							write(sockfd,&trainNo,sizeof(trainNo));
							int found=0;
							read(sockfd,&found,sizeof(found));
							if(found==0){
								printf("\nTrain not found\n");
							}
							else{
								printf("\nTrain has been cancelled\n");
							}
							break;
						}

						case 5:
						{
							int bkid=0;
							printf("\nEnter booking id:");
							scanf("%d",&bkid);
							write(sockfd,&bkid,sizeof(bkid));
							struct bookingDetails details;
							int found=0;
							read(sockfd,&found,sizeof(found));
							if(found==0){
								printf("\nBooking not found\n");
							}
							else{
								read(sockfd,&details,sizeof(details));
								printf("\nBelow are the details of the booking\n");
								printf("\nBkId\tTrain\tFrom\tTo\tclass\t#seats\tBooking status");
								printf("\n%d",details.bookingId);
								printf("\t%d",details.trainNo);
								printf("\t%s",details.from);
								printf("\t%s",details.to);
								printf("\t%s",details.class);
								printf("\t%d",details.numSeats);
								if(details.valid==1){
									printf("\tBooked");
								}
								else{
									printf("\tCancelled");
								}
								
							}
							break;
							
						}

						case 6:
						{
							int trainNo=0;
							printf("\nEnter train number:");
							scanf("%d",&trainNo);
							write(sockfd,&trainNo,sizeof(trainNo));
							struct bookingDetails allbookings[40];
							printf("\nFetching all bookings for this train..");
							read(sockfd,&allbookings,sizeof(allbookings));
							for(int i=0;i<40;i++){
								if(i==0){
									printf("\nTrain\tFrom\tTo");
									printf("\n%d",allbookings[i].trainNo);
									printf("\t%s",allbookings[i].from);
									printf("\t%s",allbookings[i].to);
									printf("\n");
									printf("\nBkId\t#seats\tBooking status");
								}
								if(allbookings[i].bookingId==0){
									break;
								}
								printf("\n%d",allbookings[i].bookingId);
								printf("\t%d",allbookings[i].numSeats);
								if(allbookings[i].valid==1){
									printf("\tBooked");
								}
								else{
									printf("\tCancelled");
								}
							}
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
				printf("\nLogin Failed.Incorrect credentials\n");
			}
			
			
	}
	return 0;
}
