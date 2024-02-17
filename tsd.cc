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
#include <algorithm>
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
  bool connected = false;
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

    //returns true if u1 followes u2
    bool follows(Client* u1, Client* u2) {
      for (Client* user : u1->client_following) {
        if (u2 == user) {
          return true;
        }
      }
      return false;
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
