//this is the encoder with child forks
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<signal.h>

#define SIZE 128000
#define NUM_CONNECTIONS 5

struct sigaction sigchld_action = {
.sa_handler = SIG_DFL,
.sa_flags = SA_NOCLDWAIT
 };
void error(const char *msg){perror(msg); exit(1);}
void encoder(char key_buffer[SIZE], char plain_buffer[SIZE], int length, int newsocket, int socketfd);

int main(int argc, char* argv[]){
   int listenSocketFD, establishedConnectionFD, port_number;
   socklen_t clilen;
   struct sockaddr_in serv_addr, cli_addr;
   int n;
   char plain_buffer[SIZE];
   char key_buffer[SIZE];
   char cipher_buffer[SIZE];
   int i;
   int length_key;
   int length_plain;
   int pid;
   int sent;
   char server_id = 0;

   //arg check
   if(argc < 2){
      error("USAGE: otp_enc_d port\n");
      exit(1);
   }

   port_number = atoi(argv[1]);

   //create socket
   if((listenSocketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0){
      error("Error: otp_enc_d could not create socket\n");
      exit(1);
   }
   memset(&serv_addr, '\0', sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(port_number);
   //bind
   if(bind(listenSocketFD, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
      fprintf(stderr, "Error: otp_enc_d unable to bing socket to port %d\n", port_number);
      exit(2);
   }

   //listen for connections
   if(listen(listenSocketFD,5)==-1){
      fprintf(stderr,"Error: otp_enc_d unable to listen on port%d\n", port_number);
      exit(2);
   }
   clilen = sizeof(cli_addr);

   while(1){
      char client_id;
      establishedConnectionFD = accept(listenSocketFD, (struct sockaddr*) &cli_addr, &clilen);
      if(establishedConnectionFD < 0){
         perror("Error: unable to accept connection\n");
	 exit(1);
      }
   
   pid = fork();
   if (pid < 0){
      perror("opt_end_d: error while forking\n");
   }

   if(pid == 0){//child
      //clear out the plain_buffer
      int type_err = read(establishedConnectionFD, &client_id, sizeof(char));
      if(client_id != server_id){
         perror("Error: invalid client ID\n");
      }
      if(type_err < 0){
         perror("Failed to read from socket\n");
	 exit(1);
      }
      type_err = write(establishedConnectionFD, &server_id, sizeof(char));
      if(type_err < 0){
         perror("Error returning server id\n");
	 exit(1);
      }
      memset(plain_buffer, 0, SIZE);
      int length_plain = read(establishedConnectionFD, plain_buffer, SIZE);
      if(length_plain < 0){
         perror("Error: otp_enc_d: could not read plain text form port\n");
	 exit(2);
      }

      //send acknowledgemtn to client
      int sent = write(establishedConnectionFD, "!", 1);
      if(sent < 0){
          perror("Error: otp_enc_d error sending acknowledgment to client\n");
	  exit(2);
      }

      //clear out key_buffer
      memset(key_buffer, 0, SIZE);
      //get length and contetnts of key buffer
      int length_key = read(establishedConnectionFD, key_buffer, SIZE);
      if(length_key < 0){
         perror("Error: otp_enc_d: could not read key from port\n");
	 exit(2);
      }

      //check length match
      if(length_key < length_plain){
          perror("Error: otp_enc_d: key file is too short\n");
      }

      //look for bad chars
      for(i = 0; i < length_plain ; i++){
         if((int)plain_buffer[i] > 90 || ((int) plain_buffer[i] < 65 && (int) plain_buffer[i] != 32)){
	    perror("Error: otp_enc_d: plain text has bad characters\n");
	    exit(1);
	 }
	 if((int)key_buffer[i] > 90 || ((int) key_buffer[i] < 65 && (int)key_buffer[i] !=32)){
	    perror("Error: otp_enc_d: key text has bad characters\n");
	    exit(1);
	 }
      }
      encoder(key_buffer, plain_buffer, length_plain, establishedConnectionFD, listenSocketFD);
      exit(0);
   }//child bracket
   else{//parent
      sigaction(SIGCHLD, &sigchld_action, NULL);     
      close(establishedConnectionFD);
      continue;
   }
   }//while bracket
return 0;
}

void encoder(char key_buffer[SIZE], char plain_buffer[SIZE], int length, int newsocket, int socketfd){
   //this function is used to create the encrypted message
   int i, sent;
   char cipher_buffer[SIZE];

   for(i = 0; i < length; i++){
      //replace any spaces with $ char
      if(key_buffer[i]== ' '){
        key_buffer[i] ='@';
      }
      if(plain_buffer[i]==' '){
         plain_buffer[i] = '@';
      }
      //type cast for ASCII
      int plain_int = (int) plain_buffer[i];
      int key_int = (int) key_buffer[i];

      //change for ASCII range
      plain_int = plain_int - 64;
      key_int = key_int - 64;

      //add the two integers and mod 27
      int cipher_text_int = (plain_int + key_int) % 27;

      //change to caps per req.
      cipher_text_int = cipher_text_int + 64;

      //type cast back to chars
      cipher_buffer[i] = (char) cipher_text_int + 0;
      
      //change the $ chars back to spaces
      if(cipher_buffer[i] == '@'){
        cipher_buffer[i] = ' ';
      } 
   }//encoder for loop bracket
   //write the cipher text back to client
   sent = write(newsocket, cipher_buffer, length);
   //check for send err
   if(sent < 0){
      perror("error writing to socket\n");
      exit(2);
   }
   close(newsocket);
   close(socketfd);
   //exit(0);
}
