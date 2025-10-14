#include <iostream>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
using namespace std;

int main() {
    int clientSocket;
    struct sockaddr_in serverSettings;
    char buffer[1024];
    string message;

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) { 
        cout << "Ошибка создания сокета!!!" << endl; 
        return 1; 
    }

    serverSettings.sin_family = AF_INET;
    serverSettings.sin_port = htons(7432);
    serverSettings.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientSocket, (struct sockaddr*)&serverSettings, sizeof(serverSettings)) < 0) {
        cout << "Ошибка подключения к серверу!!!" << endl; 
        close(clientSocket); 
        return 1;
    }

    // Получаем и выводим начальное сообщение сервера
    int receivedBytes = recv(clientSocket, buffer, sizeof(buffer)-1, 0);
    if (receivedBytes > 0) {
        buffer[receivedBytes] = '\0';
        cout << buffer;
    }

    while (true) {
        // Ввод команды от пользователя
        cout << "\n>> ";
        getline(cin, message);

        // Отправка команды серверу
        if (send(clientSocket, message.c_str(), message.size(), 0) <= 0) {
            cout << "Ошибка отправки данных на сервер!" << endl;
            break;
        }

        // Получение ответа сервера
        receivedBytes = recv(clientSocket, buffer, sizeof(buffer)-1, 0);
        if (receivedBytes <= 0) {
            cout << "Сервер закрыл соединение!" << endl;
            break;
        }
        buffer[receivedBytes] = '\0';
        cout << buffer;
    }

    close(clientSocket);
    return 0;
}
