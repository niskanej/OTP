//this is the utility file that creates one time pad keys for the clients
#include<stdlib.h>
#include<strings.h>
#include<stdio.h>
#include<time.h>

int main(int argc, char* argv[]){
   //check for correct arg length
   if(argc < 2){
      perror("Use error: to few arguments for keygen\n");
      exit(1);
   }
   //initialize rand seed
   srand(time(NULL));

   int length;
   int rand_char;
   int i;
   sscanf(argv[1], "%d", &length);
   if (length < 0){
      perror("Invalid key length\n");
      exit(1);
   }
   for(i = 0; i < length; i++){
      //get random char in the ascii range
      rand_char = rand()%(90 + 1 - 64)+64;
      if((char)rand_char == '@'){
         //ascii space 
	 rand_char = 32;
      }
      printf("%c", (char)rand_char);
   }
   printf("\n");
}
