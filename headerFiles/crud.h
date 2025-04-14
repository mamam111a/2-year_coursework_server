
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

public:
    ConceptTable(const string& path);
    bool InsertLastRow(vector<string> newLine);
    bool DeleteLastRow();
    bool DeleteForCriteria(string criteria);
    bool DeleteRowByCriteria(string& criteria);

};

#endif