
#ifndef SERVER_ADDITIONAL_H
#define SERVER_ADDITIONAL_H
#include <atomic>
#include <iostream>
#include <map>       
#include <mutex>     
#include <chrono> 
using namespace std;

extern atomic<bool> running;
extern int serverSocket;
struct ClientSession {
    bool authorized = false;
    string username;
    string role;
};
struct LoginAttempts {
    int attempts = 0;
    chrono::steady_clock::time_point blockedUntil;
};

extern map<string, LoginAttempts> attemptsMap;
extern mutex loginMutex;
void Log(const string& message);
void SignalCheck(int signal);
bool SendMessage(int& clientSocket, const string &message);
bool ReceiveMessage(int& clientSocket, string &outMessage);
void DeleteTmp(const string& username);
string toLower(const string& input);
void RegisterFailedAttempt(const string& key);
void ResetAttempts(const string& key);
bool IsBlocked(const string& key, string& message);
string GetMinuteEnding(int& minutes);
#endif