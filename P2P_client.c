#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>                                                                            
#include <netinet/in.h>
#include <arpa/inet.h>                                                                          
#include <netdb.h>
#include <sys/wait.h>

#define	MSG		"Any Message \n"
#define REGISTER 'R'
char thispeername[10];
#pragma pack(1)
struct PDU{
	char type;
	char contData[100];
	struct data{
		char PeerName[10];
		char ContentName[10];
		char Address[80];
		int port;
	}dataa;
};
#pragma pack()


//Registed content structure for registered content array
struct RegisteredContentInfo{
	int portid;
	char Address[80];
	char contentName[10];
};

struct RegisteredContentInfo registeredcontent[20];
int contentList = 0;
int	port = 3000;

void registerContent(char* contentName, char* peername);

//s for udp and sd for tcp
int s,sd,listen_tcp;
int bytesread;
struct PDU  sendContent;
struct sockaddr_in indexserver, client;	/* an Internet endpoint address		*/
int	 addrLen= sizeof(struct sockaddr_in);


void addToContentList(const char *name,int portnumber,const char * address){
	
	registeredcontent[contentList].portid = portnumber;
	strncpy(registeredcontent[contentList].contentName,name,sizeof(name));
	strncpy(registeredcontent[contentList].Address,address,sizeof(address));
	contentList++;
	
	
}

void TCPlisten(int s){

	struct PDU packetin;
	//Queue up to 5 connections
	listen(s,5);
	switch (fork()){
		case 0: /* child */
			while(1){
				char nameofcontent[10];
				nameofcontent[10] = '\0';
				int client_len = sizeof(client);
				listen_tcp = accept(s,(struct sockaddr *)&client, &client_len);
				bytesread = read(listen_tcp,&packetin,sizeof(packetin));
				if(packetin.type == 'D'){
					printf("D packet\n");
					memcpy(nameofcontent,packetin.dataa.ContentName,sizeof(packetin.dataa.ContentName));
					printf("Transferred packet!\n");
					FILE *file;
                    file = fopen(nameofcontent, "rb");
					if(!file){
                         fprintf(stderr, "File: %s does not exist locally\n", nameofcontent);
                    }else{
						sendContent.type = 'C';
						while(fgets(sendContent.contData,sizeof(sendContent.contData)-1,file)>0){
							write(listen_tcp,&sendContent,sizeof(sendContent));	
						}
						
					}
					fclose(file);
					close(listen_tcp);
					break;
					
				}
				else{
					printf("Unknown\n");
					break;
				}
				
				
				
			}
		default: /* parent */
			
			break;
		case -1:
			fprintf(stderr, "fork: error\n");
			break;
 }
	
	
	
}

void downloadContent(char* contentName, char* peername){
	int n;
	int socket2;
	uint32_t ip;
	struct	hostent		*hp;
	struct	sockaddr_in server;
	char	*host;
	int downloadlen;
	
	
	//Create intial send packet 
	struct PDU sspdu;
	sspdu.type = 'S';
	printf("Sending S packet with %s and %s\n",contentName,peername);
	memcpy(sspdu.dataa.PeerName,peername,sizeof(peername));
	memcpy(sspdu.dataa.ContentName,contentName,sizeof(contentName));
	sendto(s,&sspdu,sizeof(sspdu),0,(struct sockaddr *)&indexserver, sizeof(indexserver));
	
	
	
	//Wait for response from server to get address of content server
	struct PDU responsepdu;
	if (recvfrom(s, &responsepdu, sizeof(responsepdu), 0, (struct sockaddr *)&indexserver, &addrLen) < 0) {
            printf("Error: Recvfrom failed\n"); // Print an error message if receiving data fails
        }
		if(responsepdu.type = 'S'){
			printf("Recieved S packet\n");
			
			
		}
	printf("Sent S packet, waiting for recieve from peer:%s\n",responsepdu.dataa.PeerName);
    
	
	

	
	//Create tcp connection for download, need to get correct port number
	if ((socket2 = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}
	
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(responsepdu.dataa.port);
	if (hp = gethostbyname(responsepdu.dataa.Address)) 
	bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	else if ( inet_aton(host, (struct in_addr *) &server.sin_addr) ){
	  fprintf(stderr, "Can't get server's address\n");
	  exit(1);
	}

	/* Connecting to the server */
	if (connect(socket2, (struct sockaddr *)&server, sizeof(server)) == -1){
	  fprintf(stderr, "Can't connect \n");
	  exit(1);
	}
	
	//Creating structure for download request and send to content server
	struct PDU dpdu;
	dpdu.type = 'D';
	memcpy(dpdu.dataa.PeerName,thispeername,sizeof(dpdu.dataa.PeerName));
	memcpy(dpdu.dataa.ContentName,contentName,sizeof(dpdu.dataa.ContentName));
	
	write(socket2,&dpdu,sizeof(struct PDU));
	
	//download file
	FILE *df;
	struct PDU downloadf;
	df = fopen(contentName,"w+");
	memset(&downloadf, '\0', sizeof(downloadf));
	while((downloadlen = read(socket2, (struct pdu*)&downloadf, sizeof(downloadf))) > 0){
		if(downloadf.type = 'C'){
			printf("recieved data\n");
			fprintf(df,"%s",downloadf.contData);
		}
	}
	//if no error then return 1 else return 0
	fclose(df);
	
	//If no error
	registerContent(contentName,peername);
	
	
	
	
}


//Register content and open tcp socket for download requests
void registerContent(char* contentName, char* peername){
	struct PDU pdur, spdu;
	struct sockaddr_in reg_addr,sa;
	char readpack[101];
	int n;
	int alen;
	
	//Check if file exists
	FILE* file = fopen(contentName,"rb");
	if (file == NULL){
		//error
		printf("Error");
	}
	printf("File found\n");
	
	//create tcp connection for content transfer
	/* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}
	printf("Created TCP socket\n");
	
	//TCP for content download
	reg_addr.sin_family = AF_INET;
	reg_addr.sin_port = htons(0);
	reg_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(sd,(struct sockaddr*)&reg_addr,sizeof(reg_addr));
	alen = sizeof(struct sockaddr_in);
	getsockname(sd,(struct sockaddr*)&reg_addr,&alen);

	struct in_addr addr = reg_addr.sin_addr;
    char ip_address[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.s_addr), ip_address, INET_ADDRSTRLEN);
	
	//Get ip and send information to index server
	
	pdur.type = 'R';
	memcpy(pdur.dataa.PeerName, peername, sizeof(peername));
	memcpy(pdur.dataa.ContentName,contentName,sizeof(contentName));
	memcpy(pdur.dataa.Address,ip_address,sizeof(ip_address));
	pdur.dataa.port = htons(reg_addr.sin_port);
	printf("Sent R packet!!\n");
	sendto(s,&pdur,sizeof(pdur),0,(struct sockaddr *)&indexserver, sizeof(indexserver));
	n=read(s,readpack,101);
	memcpy(&spdu,readpack,sizeof(struct PDU));
	
	if(spdu.type == 'E'){
		//Create error packet and send
		printf("Error");
	}
	else if(spdu.type == 'A'){
		//register content 
		//MAY not work, htons(reg_addr.sin_port) may be incorrect coversion, need to test
		TCPlisten(sd);
		addToContentList(contentName,htons(reg_addr.sin_port),pdur.dataa.Address);
		
		//Create TCP connection for download
		
		
		
	}
	
}

int main(int argc, char **argv){

int peernamelen = 0;
int contentnamelen = 0;
char contentreq[10];
int contentreqlen = 0;

char contentname[10];
char ip[10] = "10.1.1.5";
char	*host = "localhost";

char option1[10];
int option1len=0;
switch (argc) {
	case 1:
		break;
	case 2:
		host = argv[1];
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "usage: UDPtime [host [port]]\n");
		exit(1);
	}
	

//Create UDP connection:
	memset(&indexserver, 0, sizeof(indexserver));
	indexserver.sin_family = AF_INET;                                                                
	indexserver.sin_port = htons(port);

	s = socket(AF_INET, SOCK_DGRAM, 0);
	  if (s < 0)
		fprintf(stderr, "Can't create socket \n");

																				
	/* Connect the socket */
		if (connect(s, (struct sockaddr *)&indexserver, sizeof(indexserver)) < 0)
		fprintf(stderr, "Can't connect to %s %s \n", host, "Time");
	
printf("Enter peer name\n");
peernamelen = read(0,thispeername,10);
thispeername[peernamelen-1] = '\0';

while(1){
printf("Enter one of the given options:\n"
           "R for content registration\n"
           "D for content download Request\n"
           "T for content De-registration\n"
           "O for listing all registered content\n"
		   "L for listing your registered content\n");
	option1len = read(0,option1,10);
	option1[option1len-1] = '\0';
	int downloadComplete = 0;


	switch(option1[0]){
		case 'R' :
			

			printf("Enter content name\n");
			contentnamelen = read(0,contentname,10);


			contentname[contentnamelen-1] = '\0';
			
			registerContent(contentname,thispeername);
			//Maybe add file transfer tcp socket
			break;
			
			
		case 'D' :
			
			//send s packet and recieve packet and pass information to download function
			printf("Enter content name\n");
			contentreqlen = read(0,contentreq,10);
			contentreq[contentreqlen-1] = '\0';
			downloadContent(contentreq,thispeername);
			break;

		case 'T':
			// Send a 'T' type PDU to the server for de-registration
			{
				printf("Enter content name to de-register:\n");
				char contentName[10];
				fgets(contentName, sizeof(contentName), stdin);
				contentName[strcspn(contentName, "\n")] = '\0'; // Remove newline
				
				struct PDU dpdu;
				dpdu.type = 'T';
				memcpy(dpdu.dataa.ContentName, contentName, sizeof(contentName));
				memcpy(dpdu.dataa.PeerName, thispeername, sizeof(thispeername));
				
				sendto(s, &dpdu, sizeof(dpdu), 0, (struct sockaddr *)&indexserver, sizeof(indexserver));
				
				// Receive acknowledgment from the server
				struct PDU ackPDU;
				recvfrom(s, &ackPDU, sizeof(ackPDU), 0, (struct sockaddr *)&indexserver, &addrLen);
				
				if (ackPDU.type == 'A') {
					printf("Content '%s' de-registered successfully.\n", contentName);
				} else {
					printf("Failed to de-register content '%s'.\n", contentName);
				}
			}
			break;
	case 'O':
    {
        // Create and send an 'O' type PDU to request the content list
        struct PDU listPDU;
        listPDU.type = 'O';
        sendto(s, &listPDU, sizeof(listPDU), 0, (struct sockaddr *)&indexserver, sizeof(indexserver));

        // Receive and display the list of registered content from the server
        printf("Registered Content List:\n");
        printf("==================================\n");
        while (recvfrom(s, &listPDU, sizeof(listPDU), 0, (struct sockaddr *)&indexserver, &addrLen) > 0) {
            if (listPDU.type == 'C') {
                printf("Content Name: %s\n", listPDU.dataa.ContentName);
                printf("Peer Name: %s\n", listPDU.dataa.PeerName);
                printf("Address: %s\n", listPDU.dataa.Address);
                printf("Port: %d\n", listPDU.dataa.port);
                printf("----------------------------------\n");
            } else {
                break; // End of content list
            }
        }
    }
    break;

	case 'L':
	{
	// Create and send an 'L' type PDU to request the content list
        struct PDU listPDU;
        listPDU.type = 'L';
        sendto(s, &listPDU, sizeof(listPDU), 0, (struct sockaddr *)&indexserver, sizeof(indexserver));

        // Receive and display the list of registered content from the server
        printf("Registered Content List from %s:\n",thispeername);
        printf("==================================\n");
        while (recvfrom(s, &listPDU, sizeof(listPDU), 0, (struct sockaddr *)&indexserver, &addrLen) > 0) {
            if (listPDU.type == 'Y') {
				if (strcmp(listPDU.dataa.PeerName, thispeername) == 0){
                printf("Content Name: %s\n", listPDU.dataa.ContentName);
                printf("Peer Name: %s\n", listPDU.dataa.PeerName);
                printf("Address: %s\n", listPDU.dataa.Address);
                printf("Port: %d\n", listPDU.dataa.port);
                printf("----------------------------------\n");
				}
            } else {
                break; // End of content list
            }
        }
	}
	break;
	case 'Q':
	{
				struct PDU listPDU;
				listPDU.type = 'L';
				sendto(s, &listPDU, sizeof(listPDU), 0, (struct sockaddr *)&indexserver, sizeof(indexserver));
					while (recvfrom(s, &listPDU, sizeof(listPDU), 0, (struct sockaddr *)&indexserver, &addrLen) > 0) {
        					if (listPDU.type == 'Y' && strcmp(listPDU.dataa.PeerName, thispeername) == 0) {
								struct PDU dpdu;
								dpdu.type = 'T';
								memcpy(dpdu.dataa.ContentName, listPDU.dataa.ContentName, sizeof(listPDU.dataa.ContentName));
								memcpy(dpdu.dataa.PeerName, thispeername, sizeof(thispeername));

								sendto(s, &dpdu, sizeof(dpdu), 0, (struct sockaddr *)&indexserver, sizeof(indexserver));

								// Receive acknowledgment from the server
								struct PDU ackPDU;
								recvfrom(s, &ackPDU, sizeof(ackPDU), 0, (struct sockaddr *)&indexserver, &addrLen);

								if (ackPDU.type == 'A') {
									printf("Content '%s' de-registered successfully.\n", listPDU.dataa.ContentName);
								} else {
									printf("Failed to de-register content '%s'.\n", listPDU.dataa.ContentName);
								}
					}
				}

                

                // Close the socket and exit the program gracefully
                close(s);
                return 0;
	}	
	break;

	default:
			printf("That value is invalid\n");
	}
}

}