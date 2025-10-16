
#ifndef AUTHORIZATION_H
#define AUTHORIZATION_H

#include <iostream>
using namespace std;

string HashingFunc(const string& password);
void SaveLoginPasswordToFile(const string& login, const string& password, const string& role);
bool ReadFromFile(const string& targetHash);
bool CheckAuthorization(const string& login, const string& password, string& outRole);
bool CheckLoginExists(const string& login);
#endif