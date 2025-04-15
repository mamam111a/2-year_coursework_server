#include <iostream>
#include <fstream>
#include <vector>
#include "json.hpp"
#include "headerFiles/workingCSV.h"
#include "headerFiles/condition.h"
#include "headerFiles/crud.h"
#include "headerFiles/condition_additional.h"

using namespace std;
using json = nlohmann::json;


int ConceptTable::FindLastCSV() {
    ifstream inFile(name + "/" + name + "_list_CSV.txt");
    int count;
    inFile >> count;
    return count;
}
int ConceptTable::FindLastLine() {
    ifstream inFile(name + "/" + name + "_last_Line.txt");
    int count;
    inFile >> count;
    return count;
}
ConceptTable::ConceptTable(const string& path) : filePath(path), jsonData(ReadSchema(path)) {
    name = jsonData["name"];
    LastLine = FindLastLine();
    lastCSV = FindLastCSV();
    tupleLimit = jsonData["tuples_limit"];
};
int ConceptTable::GetRowID(const string& str) {
    int pos = str.find(';'); 
    string temp = str.substr(0, pos);
    return stoi(temp);    
}
bool ConceptTable::InsertLastRow(vector<string> newLine) {
    string currFilepath = name + "/" + name + "_" + to_string(lastCSV) + ".csv";
    if (LastLine >= tupleLimit) {
        lastCSV++;
        ofstream listCSV(name + "/" + name + "_list_CSV.txt");
        listCSV << to_string(lastCSV);
        listCSV.close();

        currFilepath = name + "/" + name + "_" + to_string(lastCSV) + ".csv";
        LastLine = 0; 
    }
    int newIndex = 1;
    if (LastLine != 0) {
        ifstream inFile(currFilepath);
        string temp, lastRow;
        while (getline(inFile, temp)) {
            lastRow = temp;
        }
        inFile.close();
        newIndex = GetRowID(lastRow) + 1;
    }

    ofstream outFile(currFilepath, ios::app);
    outFile << to_string(newIndex) + ";";
    for (int i = 0; i < newLine.size(); i++) {
        outFile << newLine[i];
        if (i < newLine.size() - 1) {
            outFile << ";";
        }
    }
    outFile << endl;
    outFile.close();

    LastLine = newIndex;

    ofstream pkFile(name + "/" + name + "_last_Line.txt");
    pkFile << to_string(LastLine);
    pkFile.close();
    return 1;
}

bool ConceptTable::Correction(string& criteria, string& nameColumn, string newValue) {
    bool parameter = false;
    FindByCriteria(criteria, parameter);
    string criteriaFilePath = "finalFile.tmp";
    string tempFilteredPath = name + "/correctedRows.tmp";

    json jsonData = ReadSchema(name + "/" + name + ".json");
    int columnIndex = GetColumnIndex(jsonData, name, nameColumn);
    if (columnIndex == -1) return false;

    ofstream corrected(tempFilteredPath);
    for (int i = 1; i <= lastCSV; i++) {
        string filePath = name + "/" + name + "_" + to_string(i) + ".csv";
        if (!filesystem::exists(filePath)) continue;
        ifstream inFile(filePath);
        ifstream conditionFile(criteriaFilePath);
        string line, condition;
        while (getline(inFile, line)) {
            bool match = false;
            conditionFile.clear();
            conditionFile.seekg(0, ios::beg);
            while (getline(conditionFile, condition)) {
                if (!condition.empty() && line.find(condition) != string::npos) {
                    match = true;
                    break;
                }
            }
            string processedLine = line.substr(line.find(';') + 1);
            vector<string> fields;
            stringstream ss(processedLine);
            string field;
            while (getline(ss, field, ';')) {
                fields.push_back(field);
            }
            if (match && columnIndex < fields.size()) {
                fields[columnIndex-1] = newValue;
            }
            string newLine = to_string(0); 
            for (auto& f : fields) {
                newLine += ";" + f;
            }
            corrected << newLine << endl;
        }
        inFile.close();
        conditionFile.close();
        filesystem::remove(filePath);
    }
    corrected.close();

    ifstream tempFile(tempFilteredPath);
    int index = 1;
    int rowCount = 0;
    string line;
    ofstream newFile;
    bool fileOpen = false;
    while (getline(tempFile, line)) {
        if (rowCount == 0) {
            string newPath = name + "/" + name + "_" + to_string(index) + ".csv";
            newFile.open(newPath);
            fileOpen = true;
        }
        newFile << to_string(rowCount + 1) << ";" << line.substr(line.find(';') + 1) << endl;
        rowCount++;
        if (rowCount == tupleLimit) {
            newFile.close();
            fileOpen = false;
            rowCount = 0;
            index++;
        }
    }
    if (fileOpen) newFile.close();
    tempFile.close();
    filesystem::remove(tempFilteredPath);

    DeleteTmpInDirectory(".");
    DeleteTmpInDirectory("books");
    DeleteTmpInDirectory("shops");

    return true;
}
bool ConceptTable::DeleteRowByCriteria(string& criteria) {
    bool parameter = false;
    FindByCriteria(criteria,parameter);
    string criteriaFilePath = "finalFile.tmp";
    string tempFilteredPath = name + "/filteredRows.tmp";

    ofstream filtered(tempFilteredPath);
    for (int i = 1; i <= lastCSV; i++) {
        string filePath = name + "/" + name + "_" + to_string(i) + ".csv";
        if (!filesystem::exists(filePath)) continue;
        ifstream inFile(filePath);
        ifstream conditionFile(criteriaFilePath);
        string line;
        string condition;

        while (getline(inFile, line)) {
            bool found = false;
            conditionFile.clear();
            conditionFile.seekg(0, ios::beg);
            while (getline(conditionFile, condition)) {
                if (!condition.empty() && line.find(condition) != string::npos) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                filtered << line << endl;
            }
        }
        inFile.close();
        conditionFile.close();
        filesystem::remove(filePath);
    }
    filtered.close();
    ifstream tempFile(tempFilteredPath);
    int index = 1;
    int rowCount = 0;
    string line;
    ofstream newFile;
    bool fileOpen = false;

    while (getline(tempFile, line)) {
        if (rowCount == 0) {
            string newPath = name + "/" + name + "_" + to_string(index) + ".csv";
            newFile.open(newPath);
            fileOpen = true;
        }
        newFile << to_string(rowCount + 1) << ";" << line.substr(line.find(';') + 1) << endl;
        rowCount++;
        if (rowCount == tupleLimit) {
            newFile.close();
            fileOpen = false;
            rowCount = 0;
            index++;
        }
    }
    if (fileOpen) newFile.close();
    tempFile.close();
    filesystem::remove(tempFilteredPath);
    if (rowCount == 0) lastCSV = index - 1;
    else lastCSV = index;
    
    ofstream lastCSVfile(name + "/" + name + "_list_CSV.txt");
    lastCSVfile << lastCSV;
    lastCSVfile.close();
    ofstream lastLinefile(name + "/" + name + "_last_Line.txt");
    lastLinefile << ((rowCount == 0) ? tupleLimit : rowCount);
    lastLinefile.close();

    DeleteTmpInDirectory(".");
    DeleteTmpInDirectory("books");
    DeleteTmpInDirectory("shops");
    return true;
}
