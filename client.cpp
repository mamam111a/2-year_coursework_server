#include <iostream>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>//содержит функции для работы с сетевыми адресами, такие как htons.
#include <sys/socket.h> //определяет символы, начинающиеся с " AF_ " для различных видов сетей и для работы сокетов
#include <netinet/in.h> //содержащую определения для работы с интернет-адресами и портами.
#include <fstream>

using namespace std;

 
int main() {
    int clientSocket;
    struct sockaddr_in serverSettings;
    char buffer[1024];
    string message;

    // Создание сокета
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cout << endl << "CLIENT: Ошибка создания сокета!!!" << endl;
        return 1;
    }

    // настройка структуры адреса сервера
    serverSettings.sin_family = AF_INET;
    serverSettings.sin_port = htons(7432);
    serverSettings.sin_addr.s_addr = inet_addr("127.0.0.1"); // преобразование в числовой формат

    // подключение к серверу
    if (connect(clientSocket, (struct sockaddr*)&serverSettings, sizeof(serverSettings)) < 0) {
        cout << endl << "CLIENT: Ошибка подключения к серверу!!!" << endl;
        close(clientSocket);
        return 1;
    }

    cout << endl << "CLIENT: Вы подключены к серверу!!! УРА!!!" << endl;

    while (true) {

        cout << endl << "CLIENT: Введите команду == >> ";
        getline(cin, message);
        // Отправка команды серверу
        int bytesSent = send(clientSocket, message.c_str(), message.size(), 0);
        if (bytesSent < 0) {
            cout << endl << "CLIENT: Ошибка отправки данных на сервер!" << endl;
            break;
        }
        int receivedBytes = recv(clientSocket, buffer, 1024 - 1, 0);
        if (receivedBytes < 0) {
            cout << endl << "CLIENT: Ошибка получения данных от сервера!" << endl;
            break;
        }

        if (receivedBytes == 0) {
            cout << endl << "CLIENT: Сервер закрыл соединение!!!" << endl;
            break;
        }
        buffer[receivedBytes] = '\0';
        cout << endl << buffer << endl;

    }

    close(clientSocket);
    
    return 0;
}