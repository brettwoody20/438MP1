/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <algorithm>
#include <stdlib.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>
#include <glog/logging.h>
#define log(severity, msg) LOG(severity) << msg; google::FlushLogFiles(google::severity); 

#include "sns.grpc.pb.h"


using google::protobuf::Timestamp;
using google::protobuf::Duration;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using csce438::Message;
using csce438::ListReply;
using csce438::Request;
using csce438::Reply;
using csce438::SNSService;


struct Client {
  std::string username;
  bool connected = false;
  int following_file_size = 0;
  std::vector<Client*> client_followers;
  std::vector<Client*> client_following;
  ServerReaderWriter<Message, Message>* stream = nullptr;
  bool operator==(const Client& c1) const{
    return (username == c1.username);
  }
};

//Vector that stores every client (and their data)
std::vector<Client*> client_db;


class SNSServiceImpl final : public SNSService::Service {
  
  //replies with ListReply where all_users is the name of all users and followers is the list of users that follow the user
  //Note: request contains the user
  Status List(ServerContext* context, const Request* request, ListReply* list_reply) override {
    /*
    c = client_db[request->username];
    add users in client_db to list_reply;
    add (followers of c) to list_reply
    */
    
    /*********
    YOUR CODE HERE
    **********/
    //add all followers of user to reply
    Client* c = getClient(request->username());

    //if client is for some reason not found, return error message in the reply (followers vector chosen arbitrarily)
    if (c == nullptr) {
      list_reply->add_followers("FAILURE UNKNOWN");
      return Status::OK;
    }

    //add all followers of user to reply
    for (Client* follower : c->client_followers) {
      list_reply->add_followers(follower->username);
    }

    //add all clients' usernames to reply
    for (Client* client : client_db) {
      list_reply->add_all_users(client->username);
    }

    return Status::OK;
  }

  //Inputs: request (usernmae, arguments)
  //Func: verify the user exists and then add it to the appropriate lists of following/followers
  //Outputs: replies with the success of the command
  Status Follow(ServerContext* context, const Request* request, Reply* reply) override {

    /*********
    YOUR CODE HERE
    **********/
    
    //first validate that both users exist and are seperate users
    Client* u1 = getClient(request->username());
    Client* u2 = getClient(request->arguments()[0]);

    if (u1 == nullptr || u2 == nullptr || u1 == u2) {
      reply->set_msg("I");
      return Status::OK;
    }


    //if both users exist, then add them to appropriate following/followers list (u1 follows u2)
    u1->client_following.push_back(u2);
    u2->client_followers.push_back(u1);

    reply->set_msg("S");
    
    return Status::OK; 
  }

  //Inputs: request (username, arguements)
  //Func: verify the users exists, remove them from appropriate lists of following/followers
  //Outputs: replies with success of the command
  Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {

    /*********
    YOUR CODE HERE
    **********/

    Client* u1 = getClient(request->username());
    Client* u2 = getClient(request->arguments()[0]);

    //ensure both usernames exist in database as well as that they are not the same
    if (u1 == nullptr || u2 == nullptr || u1 == u2) {
      reply->set_msg("I");
      return Status::OK;
    }

    //if everything is valid, u1 unfollows u2

    //find u2 in u1's following list
    auto it1 = std::find(u1->client_following.begin(), u1->client_following.end(), u2);
    
    //if it wasn't found, return the error, otherwise remove it from the vector
    if (it1 == u1->client_following.end()) {
      reply->set_msg("U");
      return Status::OK;
    } else {
      u1->client_following.erase(it1);
    }

    //attempt to find and remove u1 from u2 followers list
    auto it2 = std::find(u2->client_followers.begin(), u2->client_followers.end(), u1);
    if ( it2 != u2->client_followers.end()) {
      u2->client_followers.erase(it2);
    }

    reply->set_msg("S");
    return Status::OK;
  }


  Status Login(ServerContext* context, const Request* request, Reply* reply) override {
    /*
    c = getClient(request.username)
    if (c.connected) {
      login failed
    } else {
      login suceeded
    }
    return Status::Ok
    */
    /*********
    YOUR CODE HERE
    **********/

    Client* c = getClient(request->username());

    //if the client was not found (DNE) then create it and add it to the database
    if (c == nullptr) {
      c = new Client;
      c->username = request->username();
      client_db.push_back(c);
      //create new files to store data about its timeline
      std::ofstream postsFile(c->username+"_posts.txt");
      std::ofstream timelineFile(c->username+"_timeline.txt");
      postsFile.close();
      postsFile.close();

    }

    //If client is alread logged in then return they can't log in here, otherwise log them in
    //The F and S are used to communicate with the client which branch occured
    if (c->connected) {
      reply->set_msg("F");
    } else {
      reply->set_msg("S");
      c->connected = true;
    }
    
    return Status::OK;
  }


  Status Timeline(ServerContext* context, 
		ServerReaderWriter<Message, Message>* streem) override {
    


    /*********
    YOUR CODE HERE
    Message m;
    while(stream.read(&m)){
        string u = m.username
        Client c = getClient(u)
        ffo = format_file_output(timestamp, request.username, m)
        if(!first_timeline_stream()){
            append ffo to file u.txt
        }else{
            lat20 = read 20 latest massages from file u_following.txt;
            stream->write(lat20);
        }
        for f in c.followers:
            f->stream->write(ffo)
            append ffo to the file f_following.txt        
    }

    **********/

    Message m;
    //std::cout<< "connection established, waiting for request" << std::endl;
    while(streem->Read(&m)) {
      std::string username = m.username();
      Client* c = getClient(username);
      //std::cout << "recieved message from " << username << ": " << m.msg() << std::endl;
      std::string ffo = formatFileOutput(m);
      if (c->stream == nullptr) {
        c->stream = streem;
        std::vector<Message> posts = getPosts(username+"_timeline", 20);
        for (Message message : posts) {
          //std::cout << "sending message to client (20)" << std::endl;
          streem->Write(message);
        }
      } else {
        appendPost(ffo, username+"_posts");
        for (Client* follower : c->client_followers) {
          if (follower->stream != nullptr) {
            //std::cout << "writing post from " << username << " to " << follower->username << std::endl;
            follower->stream->Write(m);
          }
          appendPost(ffo, follower->username+"_timeline");
        }
      }
    }
    
    return Status::OK;
  }

  private :

    Client* getClient(std::string username) {
      Client* ret = nullptr;
      for (Client* c : client_db) {
        if (c->username == username) {
          ret = c;
        }
      }
      return ret;
    }

    int appendPost(std::string ffo, std::string filename) {
      //std::cout << "appending to " << filename << ".txt" << std::endl;
      // Open the file for reading
      std::ifstream inputFile(filename+".txt");
      if (!inputFile.is_open()) {
          return 1;
      }

      // Read the contents of the file into a string
      std::stringstream buffer;
      buffer << inputFile.rdbuf();
      std::string fileContents = buffer.str();

      // Close the input file
      inputFile.close();

      // Open the file for writing (truncate it)
      std::ofstream outputFile(filename+".txt");
      if (!outputFile.is_open()) {
          return 1;
      }

      // Write the new data at the beginning of the file
      //std::cout << ffo << std::endl;
      outputFile << ffo << fileContents;


      // Close the output file
      outputFile.close();

      return 0;
    }

    std::string formatFileOutput(const Message& m) {
      int64_t seconds = m.timestamp().seconds();
      std::string ret = "T " + std::to_string(seconds) + 
                  "\nU http://twitter.com/" + m.username() + 
                  "\nW " + m.msg() + "\n";
      return ret;
    }

    std::vector<Message> getPosts(std::string filename, int numPosts) {
      //std::cout << "getting posts from " << filename << ".txt" << std::endl;
      std::vector<Message> ret;

      std::ifstream inputFile(filename+".txt");
      if (!inputFile.is_open()) {
          return ret;
      }

      std::string time, username, message;
      std::string line;
      int i = 0;
      while (std::getline(inputFile, line)) {
          if (line.empty()) {
              // Add new message to ret
              Message m;
              //std::cout<< "Read Message from " << filename <<".txt: " << username << " : " << message << std::endl;
              m.set_username(username);
              m.set_msg(message);
              //set timestamp
              long long temp1 = strtoll(time.c_str(), NULL, 0);
              int64_t temp2 = temp1;
              google::protobuf::Timestamp* timestamp = new google::protobuf::Timestamp();
              timestamp->set_seconds(temp2);
              timestamp->set_nanos(0);
              m.set_allocated_timestamp(timestamp);

              ret.push_back(m);

              // Reset variables for the next entry
              time.clear();
              username.clear();
              message.clear();
              ++i;
              if (i >= numPosts) {
                break;
              }
          } else if (line[0] == 'T') {
              time = line.substr(2); // Extract timestamp (remove "T ")
          } else if (line[0] == 'U') {
              username = line.substr(21); // Extract username (remove "U ")
          } else if (line[0] == 'W') {
              message = line.substr(2); // Extract message (remove "W ")
          }
      }
      inputFile.close();
      
      return ret;
    }


};

void RunServer(std::string port_no) {
  //construct server address
  std::string server_address = "0.0.0.0:"+port_no;
  //create instance of SNSSServiceImpl to implement grpc methods
  SNSServiceImpl service;

  //serverBuilder is a grpc class provided to help construct servers
  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  log(INFO, "Server listening on "+server_address);

  //wait for rpc requests
  server->Wait();
}

int main(int argc, char** argv) {

  std::string port = "3010";
  
  int opt = 0;
  while ((opt = getopt(argc, argv, "p:")) != -1){
    switch(opt) {
      case 'p':
          port = optarg;break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }
  
  std::string log_file_name = std::string("server-") + port;
  google::InitGoogleLogging(log_file_name.c_str());
  log(INFO, "Logging Initialized. Server starting...");
  RunServer(port);

  return 0;
}
