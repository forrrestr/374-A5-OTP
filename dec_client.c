#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()
#include <ctype.h>      // check_key_and_text_len()
#include <fcntl.h>      // For O_RDONLY


// initialize functions
int check_key_and_text_len();
char *read_args();
int recv_full_message();
int send_full_message();

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char *msg) { 
  perror(msg); 
  exit(0); 
} 

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber, 
                        char* hostname){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname(hostname); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

int main(int argc, char *argv[]) {
  int socketFD, charsWritten, charsRead;
  struct sockaddr_in serverAddress;
  char buffer[256];
  // Check usage & args
  if (argc < 3) { 
    fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); 
    exit(0); 
  }
  // assign message and key to variables then check to ensure they are valid
  char *plaintext = read_args(argv[1]);
  char *keygen = read_args(argv[2]);
  check_key_and_text_len(plaintext, keygen);

  // if they are valid get the length of the combined file
  int total_message_length = strlen(plaintext) + strlen(keygen);
  char *plaintext_and_key = malloc(sizeof(char) * (total_message_length + 1));
  
  // add handshake character
  plaintext_and_key[0] = 'd';

  // make the full message to be sent
  strcpy(plaintext_and_key + 1, plaintext);
  strcat(plaintext_and_key, keygen);
  
  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket");
  }

   // Set up the server address struct
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connecting");
  }

  //send total number of bites to the server
  int len_plaintext_and_key = strlen(plaintext_and_key);
  send(socketFD, &len_plaintext_and_key, sizeof(len_plaintext_and_key), 0);
  send_full_message(socketFD, plaintext_and_key, &len_plaintext_and_key);
  
  // Clear out the buffer again for reuse
  memset(buffer, '\0', sizeof(buffer));

  // set up response to be just the size of the message. the key is not included in the response
  int expected_response_size = strlen(plaintext);
  char *response_buffer = malloc(expected_response_size + 1);

  // get message from server and add a null terminator at the end
  recv_full_message(socketFD, response_buffer, &expected_response_size);
  response_buffer[expected_response_size] = '\0';

  // print message, free space and close down
  printf("%s", response_buffer);
  free(response_buffer);
  // Close the socket
  close(socketFD); 
  return 0;
}

int check_key_and_text_len(char plaintext[], char keygen[]){
  // minus one to avoid the null terminator
  int plaintext_len = strlen(plaintext) - 1;
  int keygen_len = strlen(keygen) -1;

  // check to ensure key lengeth is not longer than the text
  if (plaintext_len > keygen_len){
    fprintf(stderr, "Error: key file is shorter than the plaintext\n");
    exit(1);
  }

  // check keygein and plain text for only capital letters and a space
  for (int i = 0; i < plaintext_len; i++){
    if(!isupper(plaintext[i]) && plaintext[i] != ' '){
      fprintf(stderr, "Error: invalid character in plaintext\n");
      exit(1);
    }
  }
  for (int i = 0; i < keygen_len; i++){
      if(!isupper(keygen[i]) && keygen[i] != ' '){
      fprintf(stderr, "Error: invalid character in keygen\n");
      exit(1);
    }
  }
  return 0;
}

char *read_args(char *file_name){

  FILE *file;
  file = fopen(file_name, "r");

  if (file == NULL){
    return NULL;
  }
  fseek(file, 0, SEEK_END);
  int file_len = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *string = malloc(sizeof(char) * (file_len + 1));
  char c;
  int i = 0;
  while ((c = fgetc(file)) != EOF)
  {
    string[i] = c;
    i++;
  }
  string[i] = '\0';
  fclose(file);
  return string;
}

/*
send_full_message and recv_full_message are adopted from
 Beej's Guide to Network Programming: Using Internet Sockets
 https://beej.us/guide/bgnet/html/#sendall
*/

int send_full_message(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 

int recv_full_message(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've recv
    int bytesleft = *len; // how many we have left to get
    int n;

    while(total < *len) {
        n = recv(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 