#include<arpa/inet.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<netdb.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>

#define SIZE 128000

int main(int argc, char *argv[]){
   int sockFD, port_number, n;
   struct sockaddr_in serv_addr;
   struct hostent *server;
   char plain_buffer[SIZE];
   char key_buffer[SIZE];
   char cipher_buffer[SIZE];
   int fp;
   int i;
   int length_key;
   int length_plain;
   int retrieved;
   int sent;
   char server_id;
   char client_id = 0;
   
   if(argc < 4){
     printf("not engough args\n");
     exit(1);
   }

   port_number = atoi(argv[3]);

   //read only
   fp = open(argv[1], O_RDONLY);

   if(fp < 0){
      perror("cannot open file\n");
      exit(1);
   }
   //get contents and length
   length_plain = read(fp, plain_buffer, SIZE);
   
   //check for bad char in plain
   //should be handled on server side
   
   close(fp); 
   
   fp = open(argv[2], O_RDONLY);
   if(fp < 0 ){
      perror("cannot open file\n");
   }
   length_key = read(fp, key_buffer, SIZE);
   close(fp);

   //need to do error checking above
   //socket connection portion
   sockFD = socket(AF_INET, SOCK_STREAM, 0);
   //check error
   memset(&serv_addr, '\0', sizeof(serv_addr));
   server = gethostbyname("localhost");
   if(server == NULL){
      perror("Error: could not connect to otp_enc-d\n");
      exit(2);
   }
   serv_addr.sin_family = AF_INET;
   bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr, server->h_length);
   serv_addr.sin_port = htons(port_number);

   if(connect(sockFD, (struct sockaddr *) &serv_addr, sizeof(serv_addr))<0){
      fprintf(stderr, "Error: could not connect to otp_enc_d on port %d\n", port_number);
      exit(2);
   }
   int type_err = write(sockFD, &client_id, sizeof(char));
   if(type_err < 0 ){
      perror("error writing to the erver socket\n");
      exit(1);
   }
   type_err = read(sockFD, &server_id, sizeof(char));
   if(type_err < 0){
      perror("error reading from the server\n");
      exit(1);
   }
   if(client_id != server_id){
      perror("Error: wrong server\n");
      shutdown(sockFD,2);
      close(sockFD);
      exit(1);
   }
   //write the input file
   sent = write(sockFD, plain_buffer, length_plain - 1);
   if(sent < length_plain - 1){
     fprintf(stderr, "Error: could not send plain text to otp_enc_d on port %d\n",port_number);
      exit(2);
   }
   memset(cipher_buffer, 0,1);

   //get ack for server
   retrieved = read(sockFD, cipher_buffer,1);
   if(retrieved < 0){
      perror("Error recieving acknowledgment\n");
      exit(2);
   }
   //write the key
   sent = write(sockFD, key_buffer, length_key-1);
   if (sent < length_key - 1){
      fprintf(stderr, "Error: could not senf key to otp_enc_d on port %d\n", port_number);
      exit(2);
   }
   //clear buff
   memset(plain_buffer, 0, SIZE);
   do {
      //recieve the incoming cipher text
      retrieved = read(sockFD, plain_buffer, length_plain);  
   }
   while(retrieved > 0);

   if (retrieved < 0){
      perror("Error recieveing cipher text from otp_enc_d\n");
      exit(2);
   }

   //print out
   for(i=0; i< length_plain-1; i++){
      printf("%c",plain_buffer[i]);
   }
   printf("\n");
   //close the socket
   close(sockFD);
   return 0;
}
