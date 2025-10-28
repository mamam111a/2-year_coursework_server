#include "headerFiles/authorization.h"
#include "headerFiles/DBMSbody.h"
#include "headerFiles/crud.h"
#include "headerFiles/workingCSV.h"
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

#include "headerFiles/condition_additional.h"
using namespace std;

bool running = true;

void SignalCheck(int signal) {
    if (signal == SIGINT) {
        running = false;
    }
}
struct ClientSession {
    bool authorized = false;
    string username;
    string role;
};
void DeleteTmp() {
    DeleteTmpInDirectory(".");         
    DeleteTmpInDirectory("books"); 
    DeleteTmpInDirectory("shops");
}

bool SendMessage(int clientSocket, const string &message) {
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
            cerr << "Ошибка отправки данных клиенту!" << endl;
            return false;
        }
        totalSent += sent;
    }

    return true;
}

void ConnectionProcessing(int clientSocket, string clientIP, int clientPort) {
    char buffer[1024];
    int receivedBytes;
    ClientSession session;
    ostringstream toClient;

    signal(SIGPIPE, SIG_IGN);

    while (true) { 
        while (!session.authorized) {
            memset(buffer, 0, sizeof(buffer));
            receivedBytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (receivedBytes <= 0) {
                cout << "CLIENT " << clientIP << ":" << clientPort << " отключился." << endl;
                close(clientSocket);
                return;
            }

            buffer[receivedBytes] = '\0';
            string str(buffer);
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
                    toClient << "\n!Вы подключились! Ваша роль: " << role;
                    SendMessage(clientSocket, toClient.str());

                    if(role == "user") {
                        ifstream inFile("shops/shops_list_CSV.txt");
                        if (inFile.is_open()) {
                            int countShopsCSV;
                            inFile >> countShopsCSV;
                            stringstream sss;
                            sss << "*";

                            for (int i = 1; i <= countShopsCSV; i++) {
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
                    cout << "CLIENT " << clientIP << ":" << clientPort
                         << " авторизовался как " << login << " с ролью " << role << endl;
                } else {
                    toClient.str("");
                    toClient.clear();
                    toClient << "!Некорректные данные! Попробуйте снова.";
                    SendMessage(clientSocket, toClient.str());
                    cout << "CLIENT " << clientIP << ":" << clientPort << " не прошёл авторизацию." << endl;
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
                    SendMessage(clientSocket, toClient.str());
                    cout << "CLIENT " << clientIP << ":" << clientPort << " попытался зарегистрировать существующий логин: " << login << endl;
                    continue;
                } else {
                    string role;
                    if(!code.empty()) {
                        string checkCode = HashingFunc(code);
                        ifstream hashFile("adminCode.bin", ios::binary | ios::app);
                        int hashLength;
                        hashFile.read(reinterpret_cast<char*>(&hashLength), sizeof(hashLength));
                        string adminCode(hashLength, '\0');
                        hashFile.read(&adminCode[0], hashLength);
                        if(adminCode != checkCode) {
                            toClient.str("");
                            toClient.clear();
                            toClient << "!Некорректный код администратора!";
                            SendMessage(clientSocket, toClient.str());
                            cout << "CLIENT " << clientIP << ":" << clientPort << " ввел некорректный код администратора" << endl;
                            continue; 
                        }
                        role = "admin";
                    } else {
                        role = "user";
                    }
                    session.role = role;
                    SaveLoginPasswordToFile(login, password, role);
                    session.username = login;
                    toClient.str("");
                    toClient.clear();
                    toClient << "!Успешная регистрация!";
                    SendMessage(clientSocket, toClient.str());
                    cout << "CLIENT " << clientIP << ":" << clientPort << " зарегистрирован как " << login << endl;
                    continue;
                }
            }
        }

        while (session.authorized) {
            memset(buffer, 0, sizeof(buffer));
            receivedBytes = recv(clientSocket, buffer, sizeof(buffer)-1, 0);
            if (receivedBytes <= 0) {
                cout << "CLIENT " << clientIP << ":" << clientPort << " отключился." << endl;
                close(clientSocket);
                return;
            }
            buffer[receivedBytes] = '\0';
            string command(buffer);
            if (command == "0|logout") {
                cout << "CLIENT " << clientIP << ":" << clientPort << " вышел." << endl;
                session.authorized = false;
                break; 
            }
            cout << "CLIENT " << clientIP << ":" << clientPort << " -> " << command << endl;
            toClient.str("");
            toClient.clear();
            DBMS_Queries(clientSocket, command, toClient, session.role, session.username);
            string response = toClient.str();
            uint32_t len = response.size();
            uint32_t len_net = htonl(len);
            vector<char> sendBuffer(sizeof(len_net) + len);
            memcpy(sendBuffer.data(), &len_net, sizeof(len_net));
            memcpy(sendBuffer.data() + sizeof(len_net), response.data(), len);
            int totalSent = 0;
            int sendSize = sendBuffer.size();
            while (totalSent < sendSize) {
                int sent = send(clientSocket, sendBuffer.data() + totalSent, sendSize - totalSent, 0);
                if (sent <= 0) {
                    cout << "Ошибка отправки данных клиенту " << clientIP << ":" << clientPort << endl;
                    break;
                }
                totalSent += sent;
            }
            this_thread::sleep_for(chrono::milliseconds(3000));
            DeleteTmp();
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


    int serverSocket;
    int clientSocket;
    struct sockaddr_in serverSettings;
    struct sockaddr_in clientSettings;
    socklen_t clientSetLen = sizeof(clientSettings);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        cout << "SERVER: Ошибка создания сокета!!!" << endl;
        return 0;
    }

    serverSettings.sin_family = AF_INET;
    serverSettings.sin_port = htons(7432);
    serverSettings.sin_addr.s_addr = inet_addr("127.0.0.1");

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("SERVER: Ошибка установки SO_REUSEADDR");
        close(serverSocket);
        return 0;
    }

    if (bind(serverSocket, (struct sockaddr*)&serverSettings, sizeof(serverSettings)) < 0) {
        perror("SERVER: Ошибка привязки сокета");
        close(serverSocket);
        return 0;
    }

    if (listen(serverSocket, 5) < 0) {
        cout << "SERVER: Ошибка прослушивания на сокете!!!" << endl;
        close(serverSocket);
        return 0;
    }

    cout << "\nSERVER: Ожидание подключения (port 7432)" << endl;

    signal(SIGINT, SignalCheck);

    while (running) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientSettings, &clientSetLen);
        if (clientSocket < 0) {
            if (!running) break;
            cout << "SERVER: Ошибка подключения клиента!" << endl;
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientSettings.sin_addr, clientIP, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientSettings.sin_port);

        cout << "SERVER: Подключился клиент " << clientIP << ":" << clientPort << endl;
        thread t(ConnectionProcessing, clientSocket, string(clientIP), clientPort);
        t.detach();
    }

    close(serverSocket);
    cout << "SERVER: Завершение работы." << endl;
    return 0;
}
