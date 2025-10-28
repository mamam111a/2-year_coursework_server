
#ifndef CONCEPTTABLE_H
#define CONCEPTTABLE_H

#include "../json.hpp"
using json = nlohmann::json;

class ConceptTable {
private:
    string filePath;
    json jsonData;
    string name;
    int tupleLimit;
    int LastLine;
    int lastCSV;

    int FindLastCSV();
    int FindLastLine();
    int GetRowID(const string& str);
    
    string username;

public:
    ConceptTable(const string& path, string& username);
    bool InsertLastRow(vector<string> newLine);
    bool DeleteRowByCriteria(string& criteria, string &username);
    bool Correction(string& criteria, string& nameColumn, string newValue, string &username);

};

#endif