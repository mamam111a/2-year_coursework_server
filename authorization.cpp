#include <iostream>
#include <fstream>
#include <vector>
#include "headerFiles/authorization.h"

using namespace std;

string HashingFunc(const string& login, const string& password) {
    string combination = login + password;
    hash<string> hasher;
    unsigned long long hashedValue = hasher(combination);
    return to_string(hashedValue);
}
/*
void SaveLoginPasswordToFile(const string& login, const string& password) {
    string hashedValue = HashingFunc(login, password);
    ofstream outFile("authorization/authorization.bin", ios::binary | ios::app);
    int length = hashedValue.length();
    outFile.write(reinterpret_cast<char*>(&length), sizeof(length)); 
    outFile.write(hashedValue.c_str(), length);
    outFile.close();
}
    */

void SaveLoginPasswordToFile(const string& login, const string& password) {
    string hashedValue = HashingFunc(login, password);
    ofstream outFile("authorization/authorization.bin", ios::binary | ios::app);

    int loginLength = login.length();
    int hashLength = hashedValue.length();

    outFile.write(reinterpret_cast<char*>(&loginLength), sizeof(loginLength));
    outFile.write(login.c_str(), loginLength);

    outFile.write(reinterpret_cast<char*>(&hashLength), sizeof(hashLength));
    outFile.write(hashedValue.c_str(), hashLength);

    outFile.close();
}
bool ReadFromFile(const string& targetHash) {
    ifstream inFile("authorization/authorization.bin",  ios::binary);
    while (!inFile.eof()) {
        int length;
        inFile.read(reinterpret_cast<char*>(&length), sizeof(length));
        if (inFile.eof()) break;
        vector<char> buffer(length);
        inFile.read(buffer.data(), length);
        string hashedValue(buffer.begin(), buffer.end());

        if (hashedValue == targetHash) return 1;
    }
    inFile.close();
    return 0;
}
bool CheckLoginExists(const string& login) {
    ifstream inFile("authorization/authorization.bin", ios::binary);
    if (!inFile.is_open()) return false;

    while (inFile.peek() != EOF) {
        int loginLength, hashLength;
        inFile.read(reinterpret_cast<char*>(&loginLength), sizeof(loginLength));
        if (inFile.eof()) break;

        string existingLogin(loginLength, '\0');
        inFile.read(&existingLogin[0], loginLength);

        inFile.read(reinterpret_cast<char*>(&hashLength), sizeof(hashLength));
        string hash(hashLength, '\0');
        inFile.read(&hash[0], hashLength);

        if (existingLogin == login) {
            inFile.close();
            return true;
        }
    }

    inFile.close();
    return false;
}
bool CheckAuthorization(const string& login, const string& password) {
    string targetHash = HashingFunc(login, password);
    return(ReadFromFile(targetHash));
}