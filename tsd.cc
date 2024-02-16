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
#include <iostream>
#include <memory>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>
#include<glog/logging.h>
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
  bool connected = true;
  int following_file_size = 0;
  std::vector<Client*> client_followers;
  std::vector<Client*> client_following;
  ServerReaderWriter<Message, Message>* stream = 0;
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
    return Status::OK;
  }

  //Inputs: request (usernmae, arguments)
  //Func: verify the user exists and then add it to the appropriate lists of following/followers
  //Outputs: replies with the success of the command
  Status Follow(ServerContext* context, const Request* request, Reply* reply) override {

    /*********
    YOUR CODE HERE
    **********/
    return Status::OK; 
  }

  //Inputs: request (username, arguements)
  //Func: verify the users exists, remove them from appropriate lists of following/followers
  //Outputs: replies with success of the command
  Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {

    /*********
    YOUR CODE HERE
    **********/

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
    //If client is alread logged in then return they can't log in here
    if (c->connected) {
      return grpc::Status::UNKNOWN;
    }
    
    c->connected = true;
    return Status::OK;
  }


  Status Timeline(ServerContext* context, 
		ServerReaderWriter<Message, Message>* stream) override {
    


    /*********
    YOUR CODE HERE
    **********/
    
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
