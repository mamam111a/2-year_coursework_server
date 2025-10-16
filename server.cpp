#include "headerFiles/authorization.h"
#include "DBMSbody.h"
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
void ConnectionProcessing(int clientSocket, string clientIP, int clientPort) {
    char buffer[1024];
    int receivedBytes;
    ClientSession session;
    ostringstream toClient;

    signal(SIGPIPE, SIG_IGN);

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

        if (command == '1') { // вход
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
                toClient << "\nВы подключились! Ваша роль: " << role << endl;

                send(clientSocket, toClient.str().c_str(), toClient.str().size(), 0);

                cout << "CLIENT " << clientIP << ":" << clientPort
                    << " авторизовался как " << login << " с ролью " << role << endl;
            
            } else {
                toClient.str("");
                toClient.clear();
                toClient << "Некорректные данные! Попробуйте снова.";
                send(clientSocket, toClient.str().c_str(), toClient.str().size(), 0);
                cout << "CLIENT " << clientIP << ":" << clientPort << " не прошёл авторизацию." << endl;
                continue;
            }
        }
        else if (command == '2') { // регистрация
            pos0 = str.find('|');
            pos1 = str.find('|', pos0 + 1);
            pos2 = str.find('|', pos1 + 1);
            login = str.substr(pos0 + 1, pos1 - pos0 - 1);
            password = str.substr(pos1 + 1, pos2 - pos1 - 1);
            code = str.substr(pos2 + 1);

            if (CheckLoginExists(login)) {
                toClient.str("");
                toClient.clear();
                toClient << "Пользователь с таким логином уже существует! Попробуйте другой логин.";
                send(clientSocket, toClient.str().c_str(), toClient.str().size(), 0);
                cout << "CLIENT " << clientIP << ":" << clientPort << " попытался зарегистрировать существующий логин: " << login << endl;
                continue; // ← позволяет клиенту попробовать снова
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
                        toClient << "Некорректный код администратора!";
                        send(clientSocket, toClient.str().c_str(), toClient.str().size(), 0);
                        cout << "CLIENT " << clientIP << ":" << clientPort << " ввел некорректный код администратора" << endl;
                        continue; 
                    }
                    role = "admin";
                }
                else {
                    role = "user";
                }
                session.role = role;
                SaveLoginPasswordToFile(login, password, role);
                session.username = login;
                toClient.str("");
                toClient.clear();
                toClient << "\nУспешная регистрация!";
                send(clientSocket, toClient.str().c_str(), toClient.str().size(), 0);
                cout << "CLIENT " << clientIP << ":" << clientPort << " зарегистрирован как " << login << endl;
                continue;
            }
        }
    }
    CreateTableFromJson("books/books.json");
    CreateTableFromJson("shops/shops.json");

    ConceptTable tableBooks("books/books.json");
    ConceptTable tableShops("shops/shops.json");

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        receivedBytes = recv(clientSocket, buffer, sizeof(buffer)-1, 0);

        if (receivedBytes <= 0) {
            cout << "CLIENT " << clientIP << ":" << clientPort << " отключился." << endl;
            break;
        }

        buffer[receivedBytes] = '\0';
        string command(buffer);

        if (!session.authorized) continue;

        cout << "CLIENT " << clientIP << ":" << clientPort << " -> " << command << endl;

        toClient.str("");
        toClient.clear();
        DBMS_Queries(clientSocket, command, toClient);
        string response = toClient.str();
        if (send(clientSocket, response.c_str(), response.size(), 0) <= 0) {
            cout << "Ошибка отправки данных клиенту " << clientIP << ":" << clientPort << endl; //ВЫДАЕТ ОШИБКУ
            break;
        }
    }

    close(clientSocket);
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
    int serverSocket;
    int clientSocket;
    struct sockaddr_in serverSettings;
    struct sockaddr_in clientSettings;
    socklen_t clientSetLen = sizeof(clientSettings);

    // Создание сокета
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

    ifstream File("Salute.txt");
    string token;
    while (getline(File, token)) {
        cout << token << endl;
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

        // Получаем IP и порт клиента
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientSettings.sin_addr, clientIP, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientSettings.sin_port);

        cout << "SERVER: Подключился клиент " << clientIP << ":" << clientPort << endl;

        // Создаём поток и передаём копию IP/порта
        thread t(ConnectionProcessing, clientSocket, string(clientIP), clientPort);
        t.detach();
    }

    close(serverSocket);
    cout << "SERVER: Завершение работы." << endl;
    return 0;
}
