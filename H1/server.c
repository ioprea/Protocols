#include <string.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

//Oprea Ionut 324CB
//TEMA 1


/* determines the parity byte */
int parity_byte(char* message, int msglen) {
	/* result -> the parity bit */
	int res = 0, i, j;

	for( i = 0; i < msglen; i++)
		for(j = 0; j<8; j++)
			res += (message[i] >> j) & 1;

	res %= 2;
	return res;
}

//send message normal mode
void send(char* message){
	msg t;
	
	sprintf(t.payload,"%s",message);
	t.len=strlen(t.payload) + 1;
	send_message(&t);
}

//send message parity mode
void send_parity(char* message){
	msg t;
	
	t.payload[0]=parity_byte(message,strlen(message));
	sprintf(t.payload+1,"%s",message);
	t.len=strlen(t.payload) + 1;
	printf("Sent:[%s]",t.payload+1);
	send_message(&t);

}

//receive message normal mode
void receive(){
	msg r;
	int res;
	res=recv_message(&r);
	printf("%s",r.payload);
	if (res < 0) {
			perror("[RECEIVER] Send ACK error. Exiting.\n");
			return;
		}
	
}

//send ack parity mode
void send_Par_ack(){
	msg t;
	sprintf(t.payload,"ACK");
	t.len=4;
	send_message(&t);
	
}

//send nack parity mode
void send_Par_nack(){
	msg t;
	sprintf(t.payload,"NACK");
	t.len=5;
	send_message(&t);
	

}

//send ack normal mode
void send_ack(){
	msg t;
	sprintf(t.payload,"ACK");
	t.len=strlen(t.payload)+1;
	send_message(&t);
	
}

//send nack normal mode
void send_nack(){
	msg t;
	sprintf(t.payload,"NACK");
	t.len=strlen(t.payload)+1;
	send_message(&t);

}

//receive ack normal mode
void receive_ack(){
	msg r;
	int res;
	res=recv_message(&r);
	if (res < 0) {
			perror("[RECEIVER] Send ACK error. Exiting.\n");
			return;
		}

	printf("%s\n",r.payload);
	
}

//function to guess the number of the client , ack mode
int guess_ack(int low, int high){

	msg t,r;
	int res;
	int mid=(low+high)/2;
	char buffer[4];
	sprintf(buffer,"%d",mid);

	strcpy(t.payload, buffer);
	t.len=strlen(t.payload)+1;

	send_message(&t);
	receive_ack();

	res = recv_message(&r);
	send_ack();

	printf("Received: ---> %s",r.payload);
	

	

		if (res < 0) {
			perror("[RECEIVER] Send ACK error. Exiting.\n");
			return -1;
		}

	if(strcmp(r.payload,"succes\n")==0){
		
		return 1;
	}

	else if (strcmp(r.payload,"bigger\n")==0) guess_ack(mid+1,high);
	else if (strcmp(r.payload,"smaller\n")==0) guess_ack(low,mid-1);

	return 1;
	
}



//function to guess the number of the client , normal mode
int guess(int low, int high){

	msg t,r;
	int res;
	int mid=(low+high)/2;
	char buffer[4];
	sprintf(buffer,"%d",mid);


	strcpy(t.payload, buffer);
	t.len=strlen(t.payload)+1;

	send_message(&t);
	res = recv_message(&r);

	printf("Received: ---> %s",r.payload);

		if (res < 0) {
			perror("[RECEIVER] Send ACK error. Exiting.\n");
			return -1;
		}

	if(strcmp(r.payload,"succes\n")==0){
		
		return 1;
	}

	else if (strcmp(r.payload,"bigger\n")==0) guess(mid+1,high);
	else if (strcmp(r.payload,"smaller\n")==0) guess(low,mid-1);

	return 1;
	
}

//function that runs in simple mode
void simple_mode(){

		receive();
		send("Hello");
		receive();
		send("YEY");
		receive();
		send("OK");

		receive();
		receive();

		guess(1,1000);
		receive();
		send("exit");
}

//function that runs in ack mode
void ack(){

	receive();
	send_ack();

	send("Hello");
	receive_ack();

	receive();
	send_ack();

	receive();
	send_ack();

	receive();
	send_ack();

	send("YEY");
	receive_ack();

	send("OK");
	receive_ack();

	receive();
	send_ack();

	guess_ack(1,1000);

	receive();
	send_ack();

	send("exit");

	

}

//function that runs parity mode
void parity(){

	msg r,t;
	int x;
	x = (r.payload[0] >> 0) & 1;


		recv_message(&r);
		while( x != parity_byte(r.payload+1,r.len-2)){
			send_Par_nack();
			recv_message(&r);
			x = (r.payload[0] >> 0) & 1;
			
		}
		send_Par_ack();
		printf("%s\n", r.payload+1);



		send_parity("Hello");
		recv_message(&r); //receive ACK
		while( r.len != 4 ){ //check if we receive correct ACK
			send_parity("Hello");
			recv_message(&r); //receive ACK
			

		} 
		printf("%s\n", r.payload);


		recv_message(&r);
		while( x != parity_byte(r.payload+1,r.len-2)){
			send_Par_nack();
			recv_message(&r);
			x = (r.payload[0] >> 0) & 1;
			
		}
		send_Par_ack();
		printf("%s\n", r.payload+1);


		

		recv_message(&r);
		while( x != parity_byte(r.payload+1,r.len-2)){
			send_Par_nack();
			recv_message(&r);
			x = (r.payload[0] >> 0) & 1;
			
		}
		send_Par_ack();
		printf("%s\n", r.payload+1);




		recv_message(&r);
		while( x != parity_byte(r.payload+1,r.len-2)){
			send_Par_nack();
			recv_message(&r);
			x = (r.payload[0] >> 0) & 1;
			
		}
		send_Par_ack();
		printf("%s\n", r.payload+1);




		send_parity("YEY");
		recv_message(&r); //receive ACK
		printf("%s\n", r.payload);
		while( r.len != 4){
			send_parity("YEY"); //check if we receive ACK
			recv_message(&r); //receive ACK		

		} 



		send_parity("OK");
		recv_message(&r); //receive ACK
		while( r.len != 4){

			send_parity("OK"); //check if we receive ACK
			recv_message(&r); //receive ACK	

		} 
		printf("%s\n", r.payload);


		recv_message(&r);
		while( x != parity_byte(r.payload+1,r.len-2)){
			send_Par_nack();
			recv_message(&r);
			x = (r.payload[0] >> 0) & 1;
			
		}
		send_Par_ack();
		printf("%s\n", r.payload+1);


		int middle;
		int first=1;
		int last=1000;


		while (first <= last) {

			middle = (first + last)/2;


			char buffer[5];
			sprintf(buffer,"%d",middle);
			
			memset(t.payload,0,1400);
			sprintf(t.payload,"%d%s",parity_byte(buffer,strlen(buffer)), buffer);		//	sprintf(t.payload+1,"%s",buffer);
			

			
			t.len=strlen(t.payload)+1;
			printf("%s\n",t.payload);
			//send message


			send_message(&t);

			

			//get ACK
			memset(r.payload,0,1400);
			recv_message(&r); //receive ACK
			printf("ReceivedACK: ---> %s",r.payload+1);


			
			//check ACK
			while( r.len != 4){
				send_message(&t); 
				memset(r.payload,0,1400);
				recv_message(&r); //receive ACK
				//check if we receive ACK
				
			} 

			


			recv_message(&r);
			
			x = (r.payload[0] >> 0) & 1;
			
			printf("Marimea: %d",r.len);

			while( x != parity_byte(r.payload+1,r.len-2)){

				send_Par_nack();
				memset(r.payload,0,1400);
				recv_message(&r);
				x = (r.payload[0] >> 0) & 1;
				
			}
			send_Par_ack();

			printf("\n<----%s",r.payload+1);
			if (strstr(r.payload+1,"ig")){

				first = middle + 1;    
			}
			else if (strstr(r.payload+1,"uc")) {

				break;
			}

			else
				last = middle - 1;

		
   	}
   



   		recv_message(&r);
		while( x != parity_byte(r.payload+1,r.len-2)){
			send_Par_nack();
			recv_message(&r);
			x = (r.payload[0] >> 0) & 1;
			
		}
		send_Par_ack();
		printf("%s\n", r.payload+1);
		

		memset(t.payload,0,1400);
		char* ex="exit";
		sprintf(t.payload,"%d%s",parity_byte(ex,strlen(ex)), ex);
		send_message(&t);
		recv_message(&r); //receive ACK
		
		
		while( r.len != 4){
			memset(t.payload,0,1400);
			sprintf(t.payload,"%d%s",parity_byte(ex,strlen(ex)), ex);
			send_message(&t);
			recv_message(&r); //receive ACK		

		} 
		

	

}


int main(int argc,char** argv)
{
	
	
	
	printf("[RECEIVER] Starting.\n");
	init(HOST, PORT);
	
	if (argc < 1) {
       printf ("\nError.\n");
       
       return 1;
    }


    //check which mode is required 
	if(argc == 1) simple_mode();
	else {
		
		
		if(strcmp(argv[1],"parity")==0){
			
			parity();
		}

		if(strcmp(argv[1],"ack")==0){
			
			ack();
		}

		
	}


	
	printf("[RECEIVER] Finished receiving..\n");
	return 0;
}
