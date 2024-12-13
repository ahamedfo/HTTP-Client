#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.compare(0, prefix.size(), prefix) == 0;
}

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";
  
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";
  
  int client = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  std::cout << "Client connected\n";
  // std::cout << "argv is: " << argv[0] << " argc is: " << argc;
  // the int response in recv tells me how many bytes were returned, -1 means error, 0 means connections closed
  std::string input_buffer(1024, '\0');
  recv(client, (void *)&input_buffer[0],input_buffer.max_size(), 0);
  std::string input = std::string(input_buffer);
  std::string output_message = ""; 
  bool is_home_page = startsWith(std::string(input_buffer), "GET / HTTP/1.1");
  bool is_echo_string = startsWith(std::string(input_buffer), "GET /echo");
  if(is_home_page) {
    output_message = "HTTP/1.1 200 OK\r\n\r\n";
  } else if(is_echo_string){
    int echo_cmd_len = 10;
    std::string full_str_no_cmd = input_buffer.substr(echo_cmd_len);
    int end_of_echo = full_str_no_cmd.find(' ');
    std::string echo = full_str_no_cmd.substr(0, end_of_echo);

    output_message = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(echo.length()) + "\r\n\r\n" + echo;
    std::cout << echo;

  } else {
    output_message = "HTTP/1.1 404 Not Found\r\n\r\n";
  }

  send(client, output_message.c_str(), output_message.length(), 0);
  close(server_fd);

  return 0;
}
