#ifndef DBMSBODY_H
#define DBMSBODY_H

#include <string>
#include <sstream>
using namespace std;
bool DBMS_Queries(int& clientSocket, string& command, ostringstream& toClient, string& role, string& username);

#endif
