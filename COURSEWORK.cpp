#include <iostream>
#include <string>
#include <functional>
#include <sstream>
#include <fstream>
#include <vector>
#include "json.hpp"
#include "authorization.h"
#include "workingCSV.h"
#include "crud.h"
using namespace std;


int main() {
    int temp = 0;
    cout << endl << "Авторизация - 1" << endl << "Регистрация 2" << endl;
    cin >> temp;
    string login;
    string password;
    cout << endl << "Введите логин и пароль через пробел ==>> ";
    cin >> login >> password;
    if(temp == 2) {
        SaveLoginPasswordToFile(login, password);
        return 0;
    }
    else {
        if (CheckAuthorization(login, password)) cout << endl << "Успешный вход!" << endl;
        else {
            cout << endl << "Ошибка!" << endl;
            return 0;
        }
    }
    CreateTableFromJson("./books/books.json");
    CreateTableFromJson("./shops/shops.json");

    ConceptTable table("./books/books.json");
    
    


}