
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <sstream>
#include <csignal>
#include <thread>
#include <mutex>
#include <format>
#include <atomic>
#include "headerFiles/authorization.h"
#include "headerFiles/DBMSbody.h"
#include "headerFiles/crud.h"
#include "headerFiles/workingCSV.h"
#include "headerFiles/server_additional.h"
#include "headerFiles/filelocks.h"
#include "headerFiles/condition_additional.h"

using namespace std;

void ConnectionProcessing(int clientSocket, string clientIP, int clientPort) {
    char buffer[1024];
    int receivedBytes;
    ClientSession session;
    ostringstream toClient;

    signal(SIGPIPE, SIG_IGN);

    while (true) { 
        while (!session.authorized) {
            string str;
            if (!ReceiveMessage(clientSocket, str)) {
                Log("CLIENT " + clientIP + ":" + to_string(clientPort) + " отключился.") ;
                close(clientSocket);
                return;
            }
            char command = str[0];

            int pos0, pos1, pos2;
            string login, password, code;

            if (command == '1') { 
                pos0 = str.find('|');
                pos1 = str.find('|', pos0 + 1);
                login = str.substr(pos0 + 1, pos1 - pos0 - 1);
                password = str.substr(pos1 + 1);

                string role;
                if (CheckAuthorization(login, password, role)) {
                    session.authorized = true;
                    session.username = login;
                    session.role = role;

                    toClient.str("");
                    toClient.clear();
                    Log("SERVER to CLIENT " + clientIP + ":" + to_string(clientPort) + " ==>> !Вы подключились! Ваша роль: " + role);
                    toClient << "\n!Вы подключились! Ваша роль: " << role;
                    SendMessage(clientSocket, toClient.str());

                    if(role == "user") {
                        lock_guard<recursive_mutex> lock(GetFileMutex("shops/shops_list_CSV.txt"));
                        ifstream inFile("shops/shops_list_CSV.txt");
                        if (inFile.is_open()) {
                            int countShopsCSV;
                            inFile >> countShopsCSV;
                            stringstream sss;
                            sss << "*";

                            for (int i = 1; i <= countShopsCSV; i++) {
                                lock_guard<recursive_mutex> lock(GetFileMutex("shops/shops_" + to_string(i) + ".csv"));
                                ifstream shopFile("shops/shops_" + to_string(i) + ".csv");
                                string row;
                                while (getline(shopFile, row)) {
                                    if (!row.empty()) sss << row << "\n";
                                }
                                shopFile.close();
                            }
                            SendMessage(clientSocket, sss.str());
                        }
                    }
                    Log( "CLIENT " + clientIP + ":" + to_string(clientPort) + " авторизовался | login: " + login + " | role: " + role);
                } else {
                    toClient.str("");
                    toClient.clear();
                    toClient << "!Некорректные данные! Попробуйте снова.";
                    Log("SERVER to CLIENT " + clientIP + ":" + to_string(clientPort) + " ==>> !Некорректные данные! Попробуйте снова. ");
                    SendMessage(clientSocket, toClient.str());
                    Log("CLIENT " + clientIP + ":" + to_string(clientPort) + " не прошел авторизацию.") ;
                    continue; 
                }
            }
            else if (command == '2') { 
                pos0 = str.find('|');
                pos1 = str.find('|', pos0 + 1);
                pos2 = str.find('|', pos1 + 1);
                login = str.substr(pos0 + 1, pos1 - pos0 - 1);
                password = str.substr(pos1 + 1, pos2 - pos1 - 1);
                code = str.substr(pos2 + 1);

                if (CheckLoginExists(login)) {
                    toClient.str("");
                    toClient.clear();
                    toClient << "!Пользователь с таким логином уже существует! Попробуйте другой логин.";
                    Log("SERVER to CLIENT "+ clientIP + ":" + to_string(clientPort) +" ==>> !Пользователь с таким логином уже существует! Попробуйте другой логин. ");
                    SendMessage(clientSocket, toClient.str());
                    Log("CLIENT " + clientIP + ":" + to_string(clientPort) + " попытался зарегистрировать существующий логин: " + login) ;
                    continue;
                } else {
                    string role;
                    if(!code.empty()) {
                        string checkCode = HashingFunc(code);
                        lock_guard<recursive_mutex> lock(GetFileMutex("adminCode.bin"));
                        ifstream hashFile("adminCode.bin", ios::binary | ios::app);
                        int hashLength;
                        hashFile.read(reinterpret_cast<char*>(&hashLength), sizeof(hashLength));
                        string adminCode(hashLength, '\0');
                        hashFile.read(&adminCode[0], hashLength);
                        if(adminCode != checkCode) {
                            toClient.str("");
                            toClient.clear();
                            toClient << "!Некорректный код администратора!";
                            Log("SERVER to CLIENT "+ clientIP + ":" + to_string(clientPort) +" ==>> !Некорректный код администратора!");
                            SendMessage(clientSocket, toClient.str());
                            Log("CLIENT " + clientIP + ":" + to_string(clientPort) + " ввел некорректный код администратора: " + code) ;
                            continue; 
                        }
                        hashFile.close();
                        role = "admin";
                    } else {
                        role = "user";
                    }
                    session.role = role;
                    SaveLoginPasswordToFile(login, password, role);
                    session.username = login;
                    toClient.str("");
                    toClient.clear();
                    Log("SERVER to CLIENT " + clientIP + ":" + to_string(clientPort) +" ==>> !Успешная регистрация!");
                    toClient << "!Успешная регистрация!";
                    SendMessage(clientSocket, toClient.str());
                    Log( "CLIENT " + clientIP + ":" + to_string(clientPort) + " зарегистрировался | login: " + login + " | role: " + role);
                    continue;
                }
            }
        }

        while (session.authorized) {
            string command;
            if(!ReceiveMessage(clientSocket, command)) {
                Log("CLIENT " + clientIP + ":" + to_string(clientPort) + ":" +session.username + ":" + session.role + " отключился.") ;
                close(clientSocket); 
                return;
            }
            if (command == "0|logout") {
                Log("CLIENT " + clientIP + ":" + to_string(clientPort) + ":" + session.username + ":" + session.role + " вышел из аккаунта.") ;
                session.authorized = false;
                break; 
            }
            if (command == "OK") {
                DeleteTmp(session.username);
                continue;
            }
            Log("CLIENT " + clientIP + ":" + to_string(clientPort) + ":" + session.username + ":" + session.role + " отправил запрос ==>> " + command) ;
            toClient.str("");
            toClient.clear();
            DBMS_Queries(clientSocket, command, toClient, session.role, session.username);
            SendMessage(clientSocket, toClient.str());
        }
    }
}


int main() {
    /*
    string code = "1906";
    string hashedValue = HashingFunc(code);
    ofstream codeFile("adminCode.bin", ios::binary | ios::app);
    int hashLength = hashedValue.length();
    codeFile.write(reinterpret_cast<char*>(&hashLength), sizeof(hashLength));
    codeFile.write(hashedValue.c_str(), hashLength);
    codeFile.close();
    */
    CreateTableFromJson("books/books.json");
    CreateTableFromJson("shops/shops.json");

    int clientSocket;
    struct sockaddr_in serverSettings;
    struct sockaddr_in clientSettings;
    socklen_t clientSetLen = sizeof(clientSettings);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        Log("SERVER: Ошибка создания сокета!");
        return 0;
    }

    serverSettings.sin_family = AF_INET;
    serverSettings.sin_port = htons(7432);
    serverSettings.sin_addr.s_addr = inet_addr("127.0.0.1");

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        Log("SERVER: Ошибка установки SO_REUSEADDR");
        close(serverSocket);
        return 0;
    }

    if (bind(serverSocket, (struct sockaddr*)&serverSettings, sizeof(serverSettings)) < 0) {
        Log("SERVER: Ошибка привязки сокета");
        close(serverSocket);
        return 0;
    }

    if (listen(serverSocket, 5) < 0) {
        Log("SERVER: Ошибка прослушивания на сокете!");
        close(serverSocket);
        return 0;
    }

    Log("SERVER: СЕРВЕР ЗАПУЩЕН!");
    cout << endl << "SERVER: СЕРВЕР ЗАПУЩЕН!" << endl;
    signal(SIGINT, SignalCheck);
    signal(SIGHUP, SignalCheck);

    while (running) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientSettings, &clientSetLen);
        if (clientSocket < 0) {
            if (!running) break;
            Log("SERVER: Ошибка подключения клиента!");
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientSettings.sin_addr, clientIP, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientSettings.sin_port);

        Log("SERVER: Подключился клиент " + string(clientIP) + ":" + to_string(clientPort));
        thread t(ConnectionProcessing, clientSocket, string(clientIP), clientPort);
        t.detach();
    }

    close(serverSocket);
    return 0;
}
