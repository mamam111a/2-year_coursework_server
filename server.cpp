#include <iostream>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>//содержит функции для работы с сетевыми адресами, такие как htons.
#include <sys/socket.h> //определяет символы, начинающиеся с " AF_ " для различных видов сетей и для работы сокетов
#include <netinet/in.h> //содержащую определения для работы с интернет-адресами и портами.
#include <fstream>
#include <sstream>
#include <csignal>
#include <thread>
#include "DBMSbody.h"
using namespace std;


bool running = true;

void SignalCheck(int signal) { //сигнал прерывания
    if (signal == SIGINT) {
        running = false; 
    }
}

void ConnectionProcessing(int clientSocket, int serverSocket, ostringstream& toClient) {
    char buffer[1024];
    int receivedBytes;

    while (true) {
        memset(buffer, 0, 1024);  // очистка буфера, заполнение нулями
        receivedBytes = recv(clientSocket, buffer, 1024 - 1, 0); //функция получения данных (0 это флаг), 
        if (receivedBytes < 0) {                                       // -1 так как оставляем байт на знак окончания
            cout << "SERVER: Ошибка чтения сокета клиента!!!" << endl;
            break;
        }

        if (receivedBytes == 0) {
            cout << "SERVER: КЛИЕНТ ПОКИНУЛ НАС!!!" << endl;
            break;
        }

        string command(buffer); 

        // очищаем поток перед новым выводом
        toClient.str("");   
        toClient.clear(); //сброс флагов

        DBMS_Queries(command);

        string response = toClient.str();
        cout << "SERVER: Отправка данных: " << response << endl; // Для отладки
        int bytesSent = send(clientSocket, response.c_str(), response.size(), 0);
        if (bytesSent < 0) {
            cout << "SERVER: Ошибка отправки данных клиенту" << endl;
            break;
        }
    }

    close(clientSocket);
}
int main() {
    int serverSocket; //дескрипторы сокета
    int clientSocket;
    struct sockaddr_in serverSettings; //структура, хранит сетевую информацию для соединения по протоколу ipv4
    struct sockaddr_in clientSettings;

    socklen_t clientSetLen = sizeof(clientSettings); //чтобы знать, сколько памяти выделено, чтобы записать

    //создание сокета
    serverSocket = socket(AF_INET, SOCK_STREAM, 0); //AF_INET это семейство сокетов ipv4
    if (serverSocket < 0) { //SOCK_STREAM это стримовый сокет, использующий TCP  //0 - выбирает операционка, IPPROTO_TCP устанавливает TCP протокол
        cout << endl <<"SERVER: Ошибка создания сокета!!!" << endl;
        return 0;
    }

    //настройка адреса сервера
    serverSettings.sin_family = AF_INET;
    serverSettings.sin_port = htons(7432); //преобразует номер порта из формата хоста в сетевой порядок байт
    serverSettings.sin_addr.s_addr = inet_addr("127.0.0.1");  

    int opt = 1; //включение опции сокета SO_REUSEADDR
    //setsockopt настраивает сокет. sql-socket для настройки параметров (левел)
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { //для повторного дсоутпа к занятому порту
        perror("SERVER: Ошибка установки SO_REUSEADDR");
        close(serverSocket);
        return 0;
    }
    //привязка сокета к адресу
    if (bind(serverSocket, (struct sockaddr*)& serverSettings, sizeof(serverSettings)) < 0) { //struct sockaddr* это приведение типа in
        perror("SERVER: Ошибка привязки сокета");
        close(serverSocket);
        return 0;
    }

    // прослушивание
    if (listen(serverSocket, 1) < 0) {
        cout << endl <<"SERVER: Ошибка прослушивания на сокете!!!" << endl;
        close(serverSocket);
        return 0;
    }

    ifstream File("Salute.txt");
    string token;
    while(getline(File,token)) {
        cout << token << endl;
    }
    ostringstream toClient;
    //ReadConfiguration("schema.json", toClient);

    cout << endl << endl << "SERVER: Ожидание подключения (port 7432)" << endl;
    
    signal(SIGINT, SignalCheck);

    while (running) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientSettings, &clientSetLen);
        if (clientSocket < 0) {
            if (!running) break;
            cout << "Ошибка подключения клиента!" << endl;
            continue;
        }
        
        // новый поток для каждого клиента
        thread stream(ConnectionProcessing, clientSocket, serverSocket, ref(toClient));//ref передает ссылку
        stream.detach(); // работать асинхронно
    }
    

    close(serverSocket);
    cout << "SERVER: Завершение работы." << endl;
    return 0;
}