
#ifndef WORKINGCSV_H
#define WORKINGCSV_H

#include <iostream>
using namespace std;
using json = nlohmann::json;

json ReadSchema(const string& filepath);
void CreateLastLine(const string& filename);
void CreateListCSV(const string& filename);
bool FileExist(const string& filepath);
void CreateTableFromJson(const string& filepath);

#endif