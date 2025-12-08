#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

char* encript_buffer();

// Error function used for reporting issues
void error(const char *msg) {
  perror(msg);
  exit(1);
} 

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}

int main(int argc, char *argv[]){
  int connectionSocket, charsRead;
  char buffer[256];
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);

  // Check usage & args
  if (argc < 2) { 
    fprintf(stderr,"USAGE: %s port\n", argv[0]); 
    exit(1);
  } 
  
  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    error("ERROR opening socket");
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket, 
          (struct sockaddr *)&serverAddress, 
          sizeof(serverAddress)) < 0){
    error("ERROR on binding");
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5); 
  
  // Accept a connection, blocking if one is not available until one connects
  while(1){
    // Accept the connection request which creates a connection socket
    connectionSocket = accept(listenSocket, 
                (struct sockaddr *)&clientAddress, 
                &sizeOfClientInfo); 
    if (connectionSocket < 0){
      error("ERROR on accept");
    }

    printf("SERVER: Connected to client running at host %d port %d\n", 
                          ntohs(clientAddress.sin_addr.s_addr),
                          ntohs(clientAddress.sin_port));

    // Get the message from the client and display it
    memset(buffer, '\0', 256);
    // Read the client's message from the socket
    charsRead = recv(connectionSocket, buffer, 255, 0); 
    if (charsRead < 0){
      error("ERROR reading from socket");
    }
    printf("SERVER: I received this from the client: \"%s, %d\"\n", buffer, charsRead);

    //encript the message
    char* encripted_message = encript_buffer(buffer, charsRead);
    int message_len = strlen(encripted_message);
    // Send a Success message back to the client
    charsRead = send(connectionSocket, 
                    encripted_message, charsRead, 0); 
    if (charsRead < 0){
      error("ERROR writing to socket");
    }
    // Close the connection socket for this client
    close(connectionSocket); 
  }
  // Close the listening socket
  close(listenSocket); 
  return 0;
}

char* encript_buffer(char* buffer, int buffer_len){

   fflush(stdout);

  char possible_characters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
  int* message_indexes = calloc(buffer_len, sizeof(int));
  int i = 0;
  int j = 0;
  while (buffer[i] != '\n' && i < buffer_len){
    for(j = 0; j < 27; j++){
      if (buffer[i] == possible_characters[j]){
        message_indexes[i] = j;
      }
    }
    i++;
  }
  
  int message_len = i;

  fflush(stdout);

  int* keygen_index = calloc(buffer_len, sizeof(int));
  int key_start = message_len + 1;
  i++;
  for(int k = 0; k <= message_len * 2; k++){
    for (j = 0; j < 27; j++){
      if (buffer[i] == possible_characters[j]){
        keygen_index[k] = j;
      }
    }
  }

  char* encripted_message = calloc(buffer_len, sizeof(char));
  for(int h = 0; h < message_len; h++){
    int encript_index = keygen_index[h] + message_indexes[h];
    if (encript_index >= 27){
      encript_index = encript_index - 27;
    }
    encripted_message[h] = possible_characters[encript_index];
  }
  
  free(message_indexes);
  free(keygen_index);
  encripted_message[message_len] = '\n';

  return encripted_message;
}