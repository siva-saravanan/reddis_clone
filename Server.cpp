#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

size_t max_msg = 4096 ; // 4kb
//size_t only for storing the no of bytes 
//ssize_t we can store the -1 as well to indicate err in case occurs

int read_full(int client_fd , char *buff , size_t n){
  while (n>0)
  {
    ssize_t NoOfBytesRead= recv(client_fd , buff ,n , 0 ) ; 
    if(NoOfBytesRead <= 0 ) return -1  ; 
    n-=NoOfBytesRead ; 
    buff+=NoOfBytesRead ; 

  }
  return 0  ; 
  
}


int write_full(int client_fd , char* buff , size_t n ){
  while(n>0){
    ssize_t sent_data = send(client_fd , buff , n , 0) ; 
    if(sent_data == -1 ){
      if(errno  == EINTR){
        continue; // interupted so retry 
      }
      return -1 ; // failure
    }
    buff+=sent_data ;
    n-=sent_data ; 

  }
  return 0  ; 
}








int one_request(int client_fd){

  // read the 1st bytes header to know the length of the request 
 
  char buff[4+max_msg] ; // to store the client req 
  errno = 0  ; // errno is inbuuilt it is code no for systcall things 

  int err =read_full(client_fd , buff , 4) ; // 4 is no of bytes to read  
  if(err){
    
    return err ; 
  }
  // if no err we have the lenght mnow 
  /*
  void *memcpy(void *to, const void *from, size_t numBytes);
  Parameters
  to: A pointer to the memory location where the copied data will be stored.
  from: A pointer to the memory location from where the data is to be copied.
  numBytes: The number of bytes to be copied.
 */
  int len = 0  ; 
  std::memcpy(&len , buff , 4) ; // copie first 4 bytes 
  if(len > max_msg){
   
    return -1;
  }

  // if not now read the full request 
  err = read_full(client_fd , &buff [4],len) ;
  if (err) {
    
    return err;
  }

  std::cout << "Client says: " << std::string(&buff[4], len) << std::endl;


  //reply 
  const std::string reply =  std::string(&buff[4], len) ; 
  char wbuff[4+reply.size()] ; 
  len = reply.size() ; 
  memcpy(wbuff  ,&len , 4 )  ; 
  memcpy(wbuff + 4 ,  reply.c_str() , len) ;

  return write_full(client_fd, wbuff, 4 + len);


}


int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,(const char*) &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6380);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 6379\n";
    return 1; 
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
 

  // You can use print statements as follows for debugging, they'll be visible when running tests.
  //std::cout << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage
  // 
 
  while(true){
    // this is for accepting multiple client request and adding it into the queue 
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    std::cout << "Waiting for a client to connect...\n";
    int client_fd= accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    std::cout << "Client connected\n";
    // recv for handling multiple requests 
    if(client_fd < 0 ){
      continue; // error
    }

    while(true){
      //this is for handling one request at one time 
      int err = one_request(client_fd) ;
      if(err){
        break;
      }
    }

    close(client_fd) ; 


      
  }


  close(server_fd);

  return 0;
}
