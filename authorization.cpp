#include <iostream>
#include <openssl/sha.h>
#include <fstream>
#include <vector>
#include <sstream>
#include "headerFiles/authorization.h"
#include <iomanip>
using namespace std;

string HashingFunc(const string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.size(), hash);
    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str(); 
}

void SaveLoginPasswordToFile(const string& login, const string& password, const string& role) {
    string hashedValue = HashingFunc(password);
    ofstream outFile("authorization/authorization.bin", ios::binary | ios::app);

    int loginLength = login.length();
    int hashLength = hashedValue.length();
    int roleLength = role.length();
    outFile.write(reinterpret_cast<char*>(&loginLength), sizeof(loginLength));
    outFile.write(login.c_str(), loginLength);
    outFile.write(reinterpret_cast<char*>(&hashLength), sizeof(hashLength));
    outFile.write(hashedValue.c_str(), hashLength);
    outFile.write(reinterpret_cast<char*>(&roleLength), sizeof(roleLength));
    outFile.write(role.c_str(), roleLength);

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
bool CheckAuthorization(const string& login, const string& password, string& outRole) {
    ifstream inFile("authorization/authorization.bin", ios::binary);
    if (!inFile.is_open()) return false;

    string inputHash = HashingFunc(password);

    while (inFile.peek() != EOF) {
        int loginLength, hashLength, roleLength;

        inFile.read(reinterpret_cast<char*>(&loginLength), sizeof(loginLength));
        if (inFile.eof()) break;

        string existingLogin(loginLength, '\0');
        inFile.read(&existingLogin[0], loginLength);

        inFile.read(reinterpret_cast<char*>(&hashLength), sizeof(hashLength));
        string storedHash(hashLength, '\0');
        inFile.read(&storedHash[0], hashLength);

        inFile.read(reinterpret_cast<char*>(&roleLength), sizeof(roleLength));
        string storedRole(roleLength, '\0');
        inFile.read(&storedRole[0], roleLength);

        if (existingLogin == login) {
            if (storedHash == inputHash) {
                outRole = storedRole; 
                inFile.close();
                return true;
            } else {
                inFile.close();
                return false; 
            }
        }
    }

    inFile.close();
    return false; 
}