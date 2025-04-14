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

void SaveLoginPasswordToFile(const string& login, const string& password) {
    string hashedValue = HashingFunc(login, password);
    ofstream outFile("authorization/authorization.bin", ios::binary | ios::app);
    int length = hashedValue.length();
    outFile.write(reinterpret_cast<char*>(&length), sizeof(length)); 
    outFile.write(hashedValue.c_str(), length);
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

bool CheckAuthorization(const string& login, const string& password) {
    string targetHash = HashingFunc(login, password);
    return(ReadFromFile(targetHash));
}