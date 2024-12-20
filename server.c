
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/unistd.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>


#define BUFLEN		101			/* buffer length */


struct registerFormat{
	char PeerName[10];
	char ContentName[10];
	char Address[80];
	int port;
	int callcount;
};

struct tpacket {
  char type;
  char data[100];
};

#pragma pack(1)
struct PDU{
	char type;
	char contData[100];
	struct data{
		char PeerName[10];
		char ContentName[10];
		char host[80];
		int port;
	}dataa;
};
#pragma pack()


// Index System Data Structures
typedef struct  {
  char contentName[10]; 
  char peerName[10];
  int host;
  int port;
} contentListing;

//Global variables
int registeredSize=0;
struct registerFormat registeredcontent[10];
//Initialize variables for load managment
int minCallCount = 100;
int lastUsedIndex = 0;  


//Checks if content exists in current register list
int checkContent(struct PDU *pdu){
	int i;
	 for(i = 0; i < registeredSize; i++){
        if(strncmp(registeredcontent[i].ContentName, pdu->dataa.ContentName, 10) == 0){
			printf("content Found \n");
            return i;
        }
	 }
		return -1;
}

void registerContent(struct PDU *pdu){
	
	
	memcpy(registeredcontent[registeredSize].ContentName,pdu -> dataa.ContentName,sizeof(pdu -> dataa.ContentName));
	memcpy(registeredcontent[registeredSize].PeerName, pdu->dataa.PeerName, sizeof(pdu -> dataa.ContentName));
	memcpy(registeredcontent[registeredSize].Address, pdu->dataa.host,80);
	registeredcontent[registeredSize].port = pdu->dataa.port;
	registeredcontent[registeredSize].callcount = 0;
	printf("Registered content: %s\nPeername : %s\nIp : %s\nPort : %d\n",registeredcontent[registeredSize].ContentName,registeredcontent[registeredSize].PeerName,registeredcontent[registeredSize].Address,registeredcontent[registeredSize].port);
	registeredSize++;
	
}

//Search content that gives content name depending on 
void searchContent(struct PDU *pdu){
	int i;
	pdu->type = 'S';

    int selectedContentIndex = lastUsedIndex;
	
	for(i=0;i<registeredSize;i++){
		int currentIndex = (lastUsedIndex + i) % registeredSize;  
		printf("Comparing with %s and %s",registeredcontent[currentIndex].PeerName,pdu->dataa.PeerName);
		printf("Condition 1: %d\n", strcmp(registeredcontent[currentIndex].ContentName, pdu->dataa.ContentName) == 0);
		printf("Condition 2: %d\n", registeredcontent[currentIndex].callcount <= minCallCount);
		printf("Condition 3: %d\n", strcmp(registeredcontent[currentIndex].PeerName, pdu->dataa.PeerName) != 0);

		if(strcmp(registeredcontent[currentIndex].ContentName,pdu->dataa.ContentName)==0 && registeredcontent[currentIndex].callcount <= minCallCount && strcmp(registeredcontent[currentIndex].PeerName,pdu->dataa.PeerName)!=0){
			minCallCount = registeredcontent[currentIndex].callcount;
            selectedContentIndex = currentIndex;
			lastUsedIndex = selectedContentIndex;
			registeredcontent[selectedContentIndex].callcount++;


			memcpy(pdu->dataa.PeerName, registeredcontent[selectedContentIndex].PeerName, sizeof(registeredcontent[selectedContentIndex].PeerName));
			memcpy(pdu->dataa.host, registeredcontent[selectedContentIndex].Address, sizeof(registeredcontent[selectedContentIndex].Address));
			pdu->dataa.port = registeredcontent[selectedContentIndex].port;
			printf("S packet :\nPeer name %s \nPort number %d\n", registeredcontent[selectedContentIndex].PeerName, registeredcontent[selectedContentIndex].port);
				break;
		}
		
	}
	
	if (minCallCount == 100) {
        minCallCount = registeredcontent[lastUsedIndex].callcount;
    }

    
}


int main(int argc, char *argv[]) 
{
  // Packets
  struct  PDU packetR;


  // Index System
  contentListing contentList[10];
  contentListing tempContentBlock;
  char lsList[10][10];
  
  int lsPointer = 0;
  int endPointer = 0;
  
  int match = 0;
  int lsmatch = 0;
  int unique = 1;

  // Transmission Variables
  struct  PDU packetrecieve,errorpack;
  struct  tpacket packetsend;
  int     r_host, r_port;
  char tcp_port[6];
	char tcp_ip_addr[5];

  // Socket Primitives
  struct  sockaddr_in fsin;	        /* the from address of a client	*/
	char    *pts;
	int		  sock;			                /* server socket */
	time_t	now;			                /* current time */
	int		  alen;			                /* from-address length */
	struct  sockaddr_in sin;          /* an Internet endpoint address */
  int     s,sz, type;               /* socket descriptor and socket type */
	int 	  port = 3000;              /* default port */
	int 	  counter, n,m,i,bytes_to_read,bufsize;
  int     errorFlag = 0;


	switch(argc){
		case 1:
			break;
		case 2:
			port = atoi(argv[1]);
			break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		exit(1);
	}
    /* Complete the socket structure */
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;         //it's an IPv4 address
    sin.sin_addr.s_addr = INADDR_ANY; //wildcard IP address
    sin.sin_port = htons(port);       //bind to this port number
                                                                                                 
    /* Allocate a socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
		fprintf(stderr, "can't create socket\n");
                                                                                
    /* Bind the socket */
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "can't bind to %d port\n",port);
    listen(s, 5);	
	  alen = sizeof(fsin); 
	  
	while(1)
	{
		printf("Listening\n");
	if (recvfrom(s, &packetrecieve, sizeof(packetrecieve), 0,(struct sockaddr *)&fsin, &alen) < 0)
			  fprintf(stderr, "[ERROR] recvfrom error\n");
      fprintf(stderr, "Msg Recieved\n");
      switch(packetrecieve.type){
        /* Peer Server Content Registration */
		
        case 'R':
			if(checkContent(&packetrecieve)!=-1){
				printf("content already exists\n");
				
				//send error packet
				
			}
			else{
			//Register content
			registerContent(&packetrecieve);
			//Acknowledge packet
			struct PDU ackpdu;
			ackpdu.type = 'A';
			memcpy(ackpdu.contData,"Acknowledged",100);
			ackpdu.contData[12] = '\0';
	
			sendto(s,&ackpdu,sizeof(ackpdu),0,(struct sockaddr *)&fsin,sizeof(fsin));
			}
			break;
			
		case 'S':
			//search for content in array
			
			if(checkContent(&packetrecieve)!= -1){
				printf("Sent s packet to index\n");
				//Need to sent S packet with address of content server 
				searchContent(&packetrecieve);
				
				sendto(s,&packetrecieve,sizeof(packetrecieve),0,(struct sockaddr *)&fsin,sizeof(fsin));
			} 
			break;
        case 'T':
			// Logic to de-register content
			{
				int index = checkContent(&packetrecieve);
				struct PDU ackpdu;
				ackpdu.type = 'A';
				
				if (index != -1) {
					// Shift elements to remove the de-registered content from the list
					int j;
					for (j = index; j < registeredSize - 1; j++) {
						memcpy(&registeredcontent[j], &registeredcontent[j + 1], sizeof(struct registerFormat));
					}
					registeredSize--;
					printf("Content '%s' de-registered successfully.\n", packetrecieve.dataa.ContentName);
					
					sendto(s,&ackpdu,sizeof(ackpdu),0,(struct sockaddr *)&fsin,sizeof(fsin));

				} else {
					printf("Content '%s' not found for de-registration.\n", packetrecieve.dataa.ContentName);
					ackpdu.type = 'E'; 
					sendto(s,&ackpdu,sizeof(ackpdu),0,(struct sockaddr *)&fsin,sizeof(fsin));
				}
			}
			break;	
		case 'O':
		{
		// Iterate through registered content and send each entry as a response
        int i;
        for (i = 0; i < registeredSize; ++i) {
            struct PDU contentPDU;
            contentPDU.type = 'C';
            memcpy(contentPDU.dataa.PeerName, registeredcontent[i].PeerName, sizeof(registeredcontent[i].PeerName));
            memcpy(contentPDU.dataa.ContentName, registeredcontent[i].ContentName, sizeof(registeredcontent[i].ContentName));
            memcpy(contentPDU.dataa.host, registeredcontent[i].Address, sizeof(registeredcontent[i].Address));
            contentPDU.dataa.port = registeredcontent[i].port;

            sendto(s, &contentPDU, sizeof(contentPDU), 0, (struct sockaddr *)&fsin, alen);
        }

        // Send an 'X' type PDU to indicate the end of content list
        struct PDU endPDU;
        endPDU.type = 'X';
        sendto(s, &endPDU, sizeof(endPDU), 0, (struct sockaddr *)&fsin, alen);

        // Move the break statement inside the 'O' case
        break;
		}
		break;

		case 'L':
		{
		// Iterate through registered content and send each entry as a response
        int i;
        for (i = 0; i < registeredSize; ++i) {
            struct PDU contentPDU;
            contentPDU.type = 'Y';
            memcpy(contentPDU.dataa.PeerName, registeredcontent[i].PeerName, sizeof(registeredcontent[i].PeerName));
            memcpy(contentPDU.dataa.ContentName, registeredcontent[i].ContentName, sizeof(registeredcontent[i].ContentName));
            memcpy(contentPDU.dataa.host, registeredcontent[i].Address, sizeof(registeredcontent[i].Address));
            contentPDU.dataa.port = registeredcontent[i].port;

            sendto(s, &contentPDU, sizeof(contentPDU), 0, (struct sockaddr *)&fsin, alen);
        }

        // Send an 'Z' type PDU to indicate the end of content list
        struct PDU endPDU;
        endPDU.type = 'Z';
        sendto(s, &endPDU, sizeof(endPDU), 0, (struct sockaddr *)&fsin, alen);

        break;
		}
		break;
	}
}
}