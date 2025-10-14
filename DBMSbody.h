#ifndef DBMSBODY_H
#define DBMSBODY_H

#include <string>
#include <sstream>
using namespace std;
bool DBMS_Queries(int& clientSocket, const string& command, ostringstream& toClient);

#endif
