#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define BUFLEN 1024
#define LEN 10000

//structura in care retinem clientii
typedef struct {
	char* ipAddr;
	int portNo;
}Clients;


Clients clients[MAX_CLIENTS];
int count=0; //counter pentru clienti
int count2=0;
int flags[3]={0}; // 0 - recursive , 1 - everything , 2 - logfile
FILE* logFile;
int port;
char file[BUFLEN];
int recursive_depth=1;
char message[LEN][BUFLEN];


//Afisarea erorilor in cazul in care optiunea -o este activa in fisier-ul de log
//si in stderr in caz contrar
void printError() {
    if (flags[2]) {fprintf(logFile, "%s", strerror(errno));} 
    else {printf("Error");}
}

//Scrie in fisier-ul de log un anumit string daca optiunea -o este activa.
void printLog(char buffer[]) {
    if (flags[2]==1){fprintf(logFile, "%s", buffer);}
    else{printf("Error");}
}

//Parsam argumentele primite si setam flag-urile respective
void set_flags(int argc, char *argv[]){
	int i;
	for(i=0;i<argc;i++){

		//setam flags[0] - corespunzator optiunii -r(recursive)
		if(strcmp(argv[i],"-r")==0){flags[0]=1;recursive_depth=5;}

		//setam flags[1] - corespunzator optiunii -e(everything)
		if(strcmp(argv[i],"-e")==0){flags[1]=1;}

		//setam flags[2] - corespunzator optiunii -o(log file) 
		//si retinem numele fisierului
		if(strcmp(argv[i],"-o")==0){
			flags[2]=1;
			strcpy(file,argv[i+1]);
		}

		//retinem portul primit ca parametru in variabila port
		if(strcmp(argv[i],"-p")==0){port=atoi(argv[i+1]);}
	}
	return;

}

//Afisam lista de clienti
void statusPrint(){
	int i;
	for(i=0;i<count;i++)		
		printf("Connected client %d on port:%d with ip:%s\n",i+1,clients[i].portNo,clients[i].ipAddr);

}

//functie ajutatoare
void statusPrint2(){
	int i;
	memset(message[count2++], 0, BUFLEN);
	strcat(message[count2++],"status\n");
	for(i=0;i<count;i++){
		char tmp[7],tmp2[7];	
		strcat(message[count2++],"Connected client ");
		sprintf(tmp2, "%d", (i+1));
		strcat(message[count2++],tmp2);
		strcat(message[count2++]," on port:");
		sprintf(tmp, "%d", clients[i].portNo);
		strcat(message[count2++],tmp);
		strcat(message[count2++]," and ip:");
		strcat(message[count2++],clients[i].ipAddr);
		strcat(message[count2++],"\n");		
		
	}
}

//Afisam lista de clienti in fisier in functie de flag
void statusPrintFile(){
	logFile=fopen(file,"w");
	int j;
	for(j=0;j<count2;j++)
	printLog(message[j]);
	
}

//multiplexare clienti
int connectClient(){
	
	int sockfd, newsockfd, clilen, dest;
    char buffer[BUFLEN];
    struct sockaddr_in serv_addr, cli_addr;
    int n, k, i, j,ct;
    int ok=0,ok2=0;

    fd_set read_fds;	//multimea de citire folosita in select()
    fd_set tmp_fds;	//multime folosita temporar 
    int fdmax;		//valoare maxima file descriptor din multimea read_fds


     //golim multimea de descriptori de citire (read_fds) si multimea tmp_fds 
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
     
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
       error("ERROR opening socket");
     

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
    serv_addr.sin_port = htons(port);
     
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
            error("ERROR on binding");
     
    listen(sockfd, MAX_CLIENTS);

     //adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
    FD_SET(sockfd, &read_fds);
    FD_SET(0, &read_fds);
    fdmax = sockfd;

     // main loop
	while (1) {
		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
			error("ERROR in select");
	
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {

				
				if (i == sockfd) {
					// a venit ceva pe socketul inactiv(cel cu listen) = o noua conexiune
					// actiunea serverului: accept()
					clilen = sizeof(cli_addr);
					if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) == -1) {
						error("ERROR in accept");
					} 
					else {
						//adaug noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fdmax) { 
							fdmax = newsockfd;
						}
					}

					clients[count].ipAddr=(char*)malloc(sizeof(char)*strlen(inet_ntoa(cli_addr.sin_addr)));
					clients[count].ipAddr=inet_ntoa(cli_addr.sin_addr);
					clients[count++].portNo=ntohs(cli_addr.sin_port);


					memset(buffer, 0, BUFLEN);
            		sprintf(buffer, "S-a conectat %d\n", newsockfd);

            		//trimit parametrii pe care ii primeste serverul
            		//catre client (-r,-e) daca este cazul

            		if(ok==0 && flags[0]==1){
						char temp[BUFLEN];
						memset(temp, 0 , BUFLEN);
						strcpy(temp,"5\n");

						//trimit tuturor clientilor
						for (ct = 4; ct <= fdmax;ct ++) {
							send(ct,temp,strlen(temp), 0);
							ok=1;	
						}

					}

					if(ok2==0 && flags[1]==1){
						char temp[BUFLEN];
						memset(temp, 0 , BUFLEN);
						strcpy(temp,"everythingFlag\n");
						for (ct=4; j<=fdmax;j++) {
							send(ct,temp,strlen(temp), 0);
							ok2=1;	
						}

					}


				}
					
				else {

						memset(buffer, 0 , BUFLEN);
	                    fgets(buffer, BUFLEN-1, stdin);

	                    //parsam string-ul de trimis
	                    if(strstr(buffer,"download http")!=0){
	                    	if(count>=5){
		                    char* tempurl;
		                    tempurl=strtok(buffer," ");
		                    tempurl=strtok(NULL," ");
		                    strcpy(buffer,tempurl);
		                    for (ct=4;ct<=fdmax;ct++) {
							send(ct,buffer,strlen(buffer), 0);
							}

							return;
			         		}
		                	
	                	}


						//afisam clientii conectati
						if(strcmp(buffer,"status\n") == 0) {
								
								if(flags[2]==0){statusPrint();}
								else{statusPrint2();statusPrintFile();}
						}

						//conditie de iesire
						if(strstr(buffer,"exit") != 0) {
								for (ct=4;ct<=fdmax;ct++) {
								send(ct,buffer,strlen(buffer), 0);
								}	
								printf("Serverul se va inchide!\n");
								return 0;
							}
					

					
				}
				 
	
			}
		}
    }

    //inchid fisier si socket
    close(sockfd);
    if(flags[2]==1){fclose(logFile);}
    
}



int main(int argc, char *argv[]){

	set_flags(argc,argv);
	int sockfd=connectClient();
	
	return 1;

}