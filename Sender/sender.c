#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //for socket reading and writing
#include <arpa/inet.h>  //for inet_ntoa
#include <netdb.h>  //for gethostname and hostent

#define MAX_SIZE 32

char* get_machine_ip() {
    char buffer[256];
	struct hostent *hostentry;
	int hostname;
    char *IP;
	
	hostname = gethostname(buffer, sizeof(buffer));		// To retrieve hostname
	if (hostname == -1) {
		puts("Error getting hostname");
		exit(0);
	}
	hostentry = gethostbyname(buffer);		// To retrieve host information
	if (hostentry == NULL) {
		puts("Error getting hostname");
		exit(0);
	}
    IP = inet_ntoa(*((struct in_addr*) hostentry->h_addr_list[0]));
	return IP;
}

int main() {
    int socketfd, newsocketfd, n, portno = 8080;
    char buffer[MAX_SIZE], file_name[MAX_SIZE];
    FILE *fp;
    struct sockaddr_in server_addr, client_addr;
    struct hostent *server;
    socklen_t client_len = sizeof(client_addr);

    char *ip = get_machine_ip();   //get machine public ip
    server = gethostbyname(ip); 
    socketfd = socket(AF_INET, SOCK_STREAM, 0);   //IPv4 protocols & 2-way reliable connection based
    if (socketfd < 0) {
        perror("Error while opening socket");
        return 0;
    }
    puts("Socket opened");

    bzero((char *) &server_addr, sizeof(server_addr));  //erasing sizeof(server_addr) bits starting at &server_addr
    server_addr.sin_family = AF_INET; //server address protocol
    bcopy((char *)server->h_addr, (char *)&server_addr.sin_addr.s_addr, server->h_length);    //Server IP = Host IP
    server_addr.sin_port = htons(portno);     //Port no in "Host Byte Order"

    if (bind(socketfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {  //Binding socket to server
        perror("Error while binding socket");
        return 0;
    }
    puts("Socket binded");
    printf("\nServer Address: %s:%d\n", ip, portno);
    listen(socketfd, 5);  //Listening for connections in socket -> Max 5
    while(1) {
        puts("\nWaiting for connection...");
        newsocketfd = accept(socketfd, (struct sockaddr *) &client_addr, &client_len);     //Accepting connection
        if (newsocketfd < 0) {
            perror("Error on accepting connection");
        }
        else {
            //newsocketfd -> pair socket -> Sender + Receiver Pair
            printf("Client connected with ip %s\n", inet_ntoa(client_addr.sin_addr));
            bzero(buffer, MAX_SIZE);    //Clearing Buffer cuz its C duh

            while(1) {
                puts("Waiting for request...");
                bzero(buffer, MAX_SIZE); //clearing char array (string)
                n = read(newsocketfd, buffer, MAX_SIZE);  //reading file name -> via receiver/client request
                if (n < 0) {
                    perror("Error while reading from socket");
                }
                else {
                    if (!strcmp(buffer, "exit")) {
                        printf("Client %s disconnected\n", inet_ntoa(client_addr.sin_addr));
                        close(newsocketfd); //Disconnect from pair socket
                        break;
                    }
                    strcpy(file_name, buffer);  //saving file_name for future use
                    printf("File request received from client %s\nFile name: %s\n", inet_ntoa(client_addr.sin_addr), buffer);
                    //Send file
                    if(fp = fopen(buffer, "rb")) {  //File exists
                    //TO use buffer
                        char exists[MAX_SIZE] = "exists";
                        write(newsocketfd, exists, MAX_SIZE);
                        while (1) {
                            size_t num_read = fread(buffer, 1, MAX_SIZE, fp);
                            if (num_read == 0) {    //No elements returns -> EOF
                                break;
                            }
                            n = write(newsocketfd, buffer, num_read);
                            if (n < 0) // Error
                                perror("Error while writing to socket");
                            else if (n == 0)    //Socket Closed
                                break;
                        }
                        char end[MAX_SIZE] = "EOFEOFEOFEOFEOFEOFEOFEOF";  //Repeating string of EOF to denote end of file
                        write(newsocketfd, end, MAX_SIZE);  //Sending End-of-File to receiver
                        printf("%s sent to client %s\n", file_name, inet_ntoa(client_addr.sin_addr));
                        if (fp != NULL)  {
                            fclose(fp);
                        }
                        puts("Client disconnected");
                        close(newsocketfd); //closing socket as file transfer complete
                        break;
                    }
                    else {
                        char exists[MAX_SIZE] = "!exists";
                        write(newsocketfd, exists, MAX_SIZE);
                        puts("File does not exist");
                    }
                }
            }
        }
    }
    return 0; 
}