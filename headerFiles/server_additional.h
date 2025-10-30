
#ifndef SERVER_ADDITIONAL_H
#define SERVER_ADDITIONAL_H

#include <iostream>
using namespace std;

extern bool running;
struct ClientSession {
    bool authorized = false;
    string username;
    string role;
};
void Log(const string& message);
void SignalCheck(int signal);
bool SendMessage(int& clientSocket, const string &message);
bool ReceiveMessage(int& clientSocket, string &outMessage);
void DeleteTmp(const string& username);
string toLower(const string& input);
#endif