#include <iostream>
#include <fstream>
#include <string>  
#include <vector> 
#include <mutex>   
#include <chrono>     
#include <ctime>      
#include <cstring>      
#include <arpa/inet.h>  
#include <sys/socket.h> 
#include <unistd.h>     
#include <signal.h>
#include "headerFiles/server_additional.h"
#include "headerFiles/condition_additional.h"
#include "headerFiles/filelocks.h"
using namespace std;
const string logFileName = "server_log.txt";

atomic<bool> running = true; 
int serverSocket = -1;

void Log(const string& message) {
    lock_guard<recursive_mutex> lock(GetFileMutex(logFileName));
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now_time));
    ofstream logFile(logFileName, ios::app);
    if (logFile.is_open()) {
        logFile << "[" << buf << "] " << message << endl;
    }
}
void SignalCheck(int signal) {
    if (signal == SIGINT || signal == SIGHUP) {
        running = false;
        if (serverSocket != -1) {
            close(serverSocket);
            serverSocket = -1;
            Log("SERVER: Получен сигнал завершения (" + to_string(signal) + "), сокет закрыт");
        }
    }
}
void DeleteTmp(const string& username) {
    DeleteTmpInDirectory(".", username);         
    DeleteTmpInDirectory("books", username); 
    DeleteTmpInDirectory("shops", username);
}

bool SendMessage(int& clientSocket, const string &message) {
    uint32_t len = message.size();
    uint32_t len_net = htonl(len);
    vector<char> sendBuffer(sizeof(len_net) + len);
    memcpy(sendBuffer.data(), &len_net, sizeof(len_net));
    memcpy(sendBuffer.data() + sizeof(len_net), message.data(), len);

    int totalSent = 0;
    int sendSize = sendBuffer.size();

    while (totalSent < sendSize) {
        int sent = send(clientSocket, sendBuffer.data() + totalSent, sendSize - totalSent, 0);
        if (sent <= 0) {
            Log("SERVER: Ошибка отправки данных клиенту!");
            return false;
        }
        totalSent += sent;
    }

    return true;
}
bool ReceiveMessage(int& clientSocket, string &outMessage) {
    uint32_t len_net;
    int received = 0;
    char *lenPtr = reinterpret_cast<char*>(&len_net);
    while (received < 4) {
        int ret = recv(clientSocket, lenPtr + received, 4 - received, 0);
        if (ret <= 0) return false; 
        received += ret;
    }
    uint32_t len = ntohl(len_net); 
    vector<char> buffer(len);
    received = 0;
    while (received < len) {
        int ret = recv(clientSocket, buffer.data() + received, len - received, 0);
        if (ret <= 0) return false; 
        received += ret;
    }
    outMessage.assign(buffer.data(), buffer.size());
    return true;
}

string toLower(const string& input) {
    locale loc("ru_RU.UTF-8");
    wstring_convert<codecvt_utf8<wchar_t>> conv;
    wstring wstr = conv.from_bytes(input);
    for (wchar_t& c : wstr) {
        c = tolower(c, loc); 
        if (c == L'ё' || c == L'Ё') {
            c = L'е';
        }
    }
    return conv.to_bytes(wstr);
}