#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <sstream>
#include <thread>
#include <fstream>
  
  


bool startsWith(const std::string& str, const std::string& prefix) {
    return str.compare(0, prefix.size(), prefix) == 0;
}

std::vector<std::string> split_message_by_line(const std::string& message){
  std::vector<std::string> tokens;
  std::stringstream ss(message);
  std::string token;

  while (std::getline(ss, token)){
    tokens.push_back(token);
  }


  return tokens;
} 

void new_client(int client, int argc, char** argv){
    char input_buffer[1024] = {0};
    int bytes_read = recv(client, input_buffer, sizeof(input_buffer) - 1, 0);
    std::string(input_buffer, bytes_read);
    std::cout << input_buffer;
    std::string output_message = ""; 
    std::vector<std::string> parts = split_message_by_line(input_buffer);
    bool is_home_page = startsWith(std::string(input_buffer), "GET / HTTP/1.1");
    bool is_echo_string = startsWith(std::string(input_buffer), "GET /echo/");
    bool is_user_agent = startsWith(std::string(input_buffer), "GET /user-agent");
    bool is_file_req = startsWith(std::string(input_buffer), "GET /files/");
    bool is_file_upload = startsWith(std::string(input_buffer), "POST /files/");

    //std::cout << input_buffer;


    if(is_home_page) {
      output_message = "HTTP/1.1 200 OK\r\n\r\n";
    } else if(is_echo_string){
      int echo_cmd_len = 10;
      std::string full_str_no_cmd = std::string(input_buffer).substr(echo_cmd_len);
      int end_of_echo = full_str_no_cmd.find(' ');
      std::string echo = full_str_no_cmd.substr(0, end_of_echo);
      if (parts.size() > 2 && startsWith(parts[2], "Accept-Encoding")){
        int encoding_idx = parts[2].find(':') + 2;
        std::string encoding_type = parts[2].substr(encoding_idx);
        printf("gotcha!");
        if (encoding_type.find("gzip") != std::string::npos) {
          output_message = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Encoding: gzip\r\n\r\n";
        } else {
          output_message = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" ;
        }
      } else {
        output_message = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(echo.length()) + "\r\n\r\n" + echo;
      }
    } else if (is_user_agent){
      std::string user_agent_content = parts[2].substr(parts[2].find(':') + 2);
      output_message = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(user_agent_content.length() - 1) + "\r\n\r\n" + user_agent_content;
    } else if (is_file_req || is_file_upload) {
      std::string file_cmd = is_file_req ? "GET /files/" : "POST /files/";
      std::string file_access = parts[0].substr(file_cmd.length());
      file_access = file_access.substr(0,file_access.find(' '));
      std::string return_url = argv[2] + file_access;


      //std::cout << "This is Part 4 : " << parts[4] << ' ';
      //std::cout << "This is the whole buffer : " << input_buffer;

      
      if (is_file_req){

        std::string host_server = parts[1].substr(parts[1].find(':') + 2);
        std::ifstream file(return_url, std::ios::binary | std::ios::ate); // Open file in binary mode and move to the end

        if(file.is_open()) {
          std::streamsize size = file.tellg(); // Get the position of the file pointer (size of the file)
          file.seekg(0, std::ios::beg);
          std::string content(size, '\0');
          if (file.read(&content[0], size)) {
            output_message = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length:" + std::to_string(size) + "\r\n\r\n" + content;
          }
          file.close();
        } else {
          output_message = "HTTP/1.1 404 Not Found\r\n\r\n";
        }
      } else if (is_file_upload) {
        std::ofstream outfile(return_url);
        outfile << std::string(parts[5]);
        output_message = "HTTP/1.1 201 Created\r\n\r\n";
      }

    } else {
      output_message = "HTTP/1.1 404 Not Found\r\n\r\n";
    }
    send(client, output_message.c_str(), output_message.length(), 0);
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
  while(true) {
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    
    std::cout << "Waiting for a client to connect...\n";
    
    int client = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    std::cout << "Client connected\n";
    if (argc > 2) {
      if (argv[1] == "--directory") {
        
      }
    }
    std::thread th(new_client, client, argc, argv);
    th.detach();
    //add a copy of client from memroy into another datastructure so it's okayy if it is written over
  }
  close(server_fd);
  return 0;
}
