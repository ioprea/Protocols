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

#define BUFLEN 1024
#define HTTP_PORT 80

int flags[3]={0}; // 0 - log file , 1 - ip address , 2 - port
int everythingFlag; 
FILE* logFile;
int depth=5; //depth-ul descarcarii, default 1, se modifica in functie de parametrul -r din server
char ipAddress[BUFLEN];
int port;
char file[BUFLEN];
struct sockaddr_in serv_addr;
struct sockaddr_in serverLink;
char domain[BUFLEN],pathG[BUFLEN];
char url[BUFLEN];

//Afiseaza erorile in logFile sau in stderr in functie de flag
void printError() {
    if (flags[0]){fprintf(logFile, "%s", strerror(errno));} 
    else {printf("Error");}
}

//Scrie in fisier-ul de log un anumit string daca optiunea -o este activa.
void printLog(char buffer[]) {
    if (flags[0]==1){fprintf(logFile, "%s", buffer);}
    else{printf("Error");}
}

//Parsam si obtinem directory si filename din path
void setDirFile(char path[],char directory[],char filename[]){
    char temp[BUFLEN];
    memset(temp, 0, BUFLEN);

    //Obtinem filename
    strcpy(filename, strrchr(path, '/') + 1);

    //Obtinem directory
    
    memset(directory, 0, BUFLEN);
    int position = (int) (strrchr (path, '/') - path);
    strncpy(temp, path, position + 1);
    strcat(directory, temp);
}

//Creeaza un director folosind comanda linux mkdir
void makeDir(char directory[BUFLEN]){
    char aux[BUFLEN];
    strcpy(aux, "mkdir -p ");
    strcat(aux, directory);
    system(aux);
}

//Functie de citire implementata la laborator
ssize_t Readline(int sockd, void *vptr, size_t maxlen) {
    ssize_t n, rc;
    char c, *buffer;

    buffer = (char*) vptr;

    for ( n = 1; n < (ssize_t) maxlen; n++ ) {
        if ( (rc = read(sockd, &c, 1)) == 1 ) {
            *buffer++ = c;
            if ( c == '\n' )
                break;
        }
        else if ( rc == 0 ) {
            if ( n == 1 )
                return 0;
            else
                break;
        }
        else {
            if ( errno == EINTR )
                continue;
            return -1;
        }
    }

    *buffer = 0;
    return n;
}

//Citim ce am primit pe socket
int readSocket(int sockfd){
    ssize_t size;
    char recvbuffer[BUFLEN];

    //Citim prima linie
    Readline(sockfd, recvbuffer, BUFLEN);
    //printf("ReadLINE:%s",recvbuffer);

    //Verificam realizarea cu succes a conexiunii
    if (strstr(recvbuffer, "200 OK")==0) {

        //Scriem in logFile
        if (flags[0]){printLog(recvbuffer);} 
        else{printf("%s", recvbuffer);}

        return 0;
    }

    size = Readline(sockfd, recvbuffer, BUFLEN);
    while (size>2){
        if (size <= 0){
            return 0;
        }
        size = Readline(sockfd, recvbuffer, BUFLEN);
    }

    return 1;
}

int makeFile(char directory[],char filename[]){
    char aux[BUFLEN];
    int f;
   
    memset(aux, 0, BUFLEN);
    strcpy(aux, directory);

    if(strlen(filename) == 0){strcat(aux, "index.html");}
    else{strcat(aux, filename);}
  
    if ((f = open(aux, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        printError();
        return 0;
    }

    return f;
}

void sendCommand(int sockfd,char path[]){
    char toSend[BUFLEN];
    char* aux;
    aux=strchr(path, '/');
    sprintf(toSend, "GET %s HTTP/1.0\n\n",aux);
    printf("Downloading %s\n",aux);

    if (send(sockfd, toSend, strlen(toSend), 0) < 0) {
        printError();
    }
}

int parse_line(char line[], char directory[], char link[]) {
    char *fstPosition; 
    char *sndPosition;
    int length;
    char temp[BUFLEN];

    //Obtinem pozitia lui href
    fstPosition = strstr(line, "href=");
    if (fstPosition==0) return 0;

    //Obtinem pozitia catre finalul linkului
    sndPosition = strstr(fstPosition + 6, "\"");

    if (sndPosition==0) {
        sndPosition = strstr(fstPosition + 6, "\'");
        if (!sndPosition) return 0;
    }

    //Calculam lungimea linkului
    length = (int) (sndPosition - fstPosition) - 6;
    memset(temp, 0, BUFLEN);
    strncpy(temp, fstPosition + 6, length);

    //Verificam formatul linkului
    if (everythingFlag==0 && !strstr(temp, "html")) return 0;
    else if (strstr(temp, "http://")) return 0;
    else if (strstr(temp, "https://")) return 0;
    else if (strstr(temp, "ftp")) return 0;
    else if (strstr(temp, ".css")) return 0;
    else if (temp[strlen(temp) - 1] == '/') return 0;
    

    //Construim calea catre fisier
    if(temp[0] == '/'){sprintf(link, "%s", domain);}
    else{sprintf(link, "%s", directory);}
    strcat(link, temp);

    return 1;
}

//descarc html
int downloadHTML(int sockfd, char directory[], int f,char links[][BUFLEN], int* i){
    ssize_t size;
    char recvbuffer[BUFLEN];

    size = Readline(sockfd, recvbuffer, BUFLEN);
    while (size > 0) {
        if (write(f, recvbuffer, size) < 0) {
            printError();
            return 0;
        }

        memset(links[*i], 0, BUFLEN);
        if(parse_line(recvbuffer, directory, links[*i])){(*i)++;}
        
        memset(recvbuffer, 0, BUFLEN);
        size = Readline(sockfd, recvbuffer, BUFLEN);
    }

    return 0;
}

//descarc normal, in functie de flag
int downloadEverything(int sockfd, int f) {
    ssize_t size;
    char recvbuffer[BUFLEN];

    size = read(sockfd, recvbuffer, BUFLEN);
    while (size > 0) {
        int check;
        check=write(f, recvbuffer, size);
        if (check < 0) {
            printError();
            return 0;
        }
        memset(recvbuffer, 0, BUFLEN);
        size = read(sockfd, recvbuffer, BUFLEN);
    }
    exit(0);
    return 0;
}

//descarcarea fisierelor
void download(char path[],int current_depth){
    int sockfd;
    int i=0;
    char links[512][BUFLEN];

    //verificam sa nu depaseasca depth-ul maxim
    if(current_depth>depth) {return;}
    //verificam existenta fisierului
    if (access(path, F_OK) != -1) return;

    char filename[BUFLEN],directory[BUFLEN];

    //aflam numele fisierului si directorul
    setDirFile(path,directory,filename);
    //cream directorul
    makeDir(directory);

    int f;
    f=makeFile(directory, filename);
    //cream fisierul
    if (f==0) return;
    //ne conectam si retinem socketul
    sockfd = connectHTTP();
    if (sockfd==0) return;
    //trimitem comanda
    sendCommand(sockfd,path);

    //citim raspunsul de la socket
    if (!readSocket(sockfd)){
        close(sockfd);
        close(f);
        return;
    }

    //descarcam fisierul in functie de tip si flag
    if (everythingFlag && (strstr(filename, "htm")==0 || !strlen(filename))){
        if (downloadEverything(sockfd, f)==0){
            close(sockfd);
            close(f);
        }
    }else{
        if ((downloadHTML(sockfd, directory, f, links, &i))==0){
            close(sockfd);
            close(f);
        }
    }

    //apelam recursiv
    int n = i;
    for (i = 0; i < n; i++) {
        if (everythingFlag && !strstr(links[i], "htm")){download(links[i], current_depth);}
        else{download(links[i], current_depth + 1);}

        if(i==n) exit(0);
    }

    return;


}

//parsam argumentele si setam flagurile aferente
void set_flags(int argc, char *argv[]){
    int i;
    for(i=0;i<argc;i++){
        if(strcmp(argv[i],"-p")==0){port=atoi(argv[i+1]);}

        if(strcmp(argv[i],"-a")==0){strcpy(ipAddress,argv[i+1]);}

        if(strcmp(argv[i],"-o")==0){
            strcpy(file,argv[i+1]);
            logFile=fopen(file,"w");
        }

    }
    return;

}

//setam datele necesare conectari la site-ul primit ca parametru de la server
void setServerLink(){
    char *ip;
    struct hostent *he = gethostbyname(domain);
    if (he!=NULL) {
        serverLink.sin_family = AF_INET;
        serverLink.sin_port = htons (HTTP_PORT);
        ip = strdup(inet_ntoa(*((struct in_addr *)he->h_addr)));
        inet_aton(ip, &serverLink.sin_addr);
    }
    else{printLog("Sever not found!");}
}

//setam domeniul
void setDomainPath(){
    int position =strlen(url)-strlen(index(url+7,'/'));
    snprintf(domain, position-6, "%s",url+7);
    strcpy(pathG, url + position);
}

//conetarea la site
int connectHTTP(){
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    //Deschid socket
    if (sockfd  < 0){
        printError();
        return 0;
    }

    int check;
    check=connect(sockfd,(struct sockaddr *) &serverLink,sizeof(struct sockaddr));
    //Ma conectez
    if (check != 0){
        printError(); //afisam eroare in cazul unei conectari nereusite
        return 0;
    }

    return sockfd;
}

int connectServer(){

    char buffer[BUFLEN];
    int n,i;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_aton(ipAddress, &serv_addr.sin_addr);


    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");    

    fd_set read_fs, aux_fs;
    FD_SET(sockfd, &read_fs);
    FD_SET(0, &read_fs);



    while(1){
       aux_fs = read_fs;
        select(sockfd + 1, &aux_fs, NULL,NULL,NULL);
        memset(buffer, 0, BUFLEN);

        for(i =0; i <= sockfd; i++)
            if(FD_ISSET(i, &aux_fs))
               if ( i == 0){

                    //citesc de la tastatura
                    memset(buffer, 0 , BUFLEN);
                    fgets(buffer, BUFLEN-1, stdin);

                    //trimit mesaj la server
                    n = send(sockfd,buffer,strlen(buffer), 0);
                    if (n < 0) 
                         error("ERROR writing to socket");
                }else{
                    char* tempurl;
                    n = recv(i, buffer, sizeof(buffer), 0);

                    if( n > 0){
                       // if(strstr(buffer,"recursive")==0 && strstr(buffer,"everythingFlag")==0)
                        //printf("Received message from Server : %s\n\n",buffer);

                        if(strstr(buffer,"5")!=0){depth=5;}
                        if(strstr(buffer,"everythingFlag")!=0){everythingFlag=1;}
                        if(strstr(buffer,"http")!=0){
                            

                            buffer[strlen(buffer) - 1] = '\0';
                            tempurl=strtok(buffer," ");


                            strcpy(url,buffer);
                            setDomainPath(url,domain,pathG);

                            setServerLink();

                            char temp[BUFLEN];
                            memset(temp, 0, BUFLEN);
                            strcpy(temp, domain);
                            strcat(temp, pathG);

                            download(temp,1);
                            return;
                           

                        }



                        if(strstr(buffer,"exit") != 0) {
                           printf("Serverul se va inchide!\n");
                           exit(0);return 0;
                        }
                    }
                }
            

    }



    return sockfd;
}

int main(int argc, char *argv[]){

    set_flags(argc,argv);
    int sockfd=connectServer();


    
}