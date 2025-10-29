#include <iostream>
#include <fstream>
#include <vector>
#include "json.hpp"
#include "headerFiles/workingCSV.h"
#include "headerFiles/condition.h"
#include "headerFiles/crud.h"
#include "headerFiles/condition_additional.h"
#include <mutex>
#include "headerFiles/filelocks.h"
using namespace std;
using json = nlohmann::json;

int ConceptTable::FindLastCSV() {
    lock_guard<recursive_mutex> lock(GetFileMutex(name + "/" + name + "_list_CSV.txt"));
    ifstream inFile(name + "/" + name + "_list_CSV.txt");
    int count;
    inFile >> count;
    return count;
}
int ConceptTable::FindLastLine() {
    lock_guard<recursive_mutex> lock(GetFileMutex(name + "/" + name + "_last_Line.txt"));
    ifstream inFile(name + "/" + name + "_last_Line.txt");
    int count;
    inFile >> count;
    return count;
}
ConceptTable::ConceptTable(const string& path, string& username) : filePath(path), jsonData(ReadSchema(path)), username(username)  {
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
bool ConceptTable::InsertLastRow(vector<string>& newLine) {
    lock_guard<recursive_mutex> lock(GetFileMutex(name + "/" + name + "_" + to_string(lastCSV) + ".csv"));
    string currFilepath = name + "/" + name + "_" + to_string(lastCSV) + ".csv";

    if (LastLine >= tupleLimit) {
        lastCSV++;
        lock_guard<recursive_mutex> lock1(GetFileMutex(name + "/" + name + "_list_CSV.txt"));
        ofstream listCSV(name + "/" + name + "_list_CSV.txt");
        if (!listCSV.is_open()) return 0; 
        listCSV << to_string(lastCSV);
        listCSV.close();

        currFilepath = name + "/" + name + "_" + to_string(lastCSV) + ".csv";
        LastLine = 0; 
    }

    int newIndex = 1;
    if (LastLine != 0) {
        lock_guard<recursive_mutex> lock2(GetFileMutex(currFilepath));
        ifstream inFile(currFilepath);
        if (!inFile.is_open()) return 0; 
        string temp, lastRow;
        while (getline(inFile, temp)) {
            lastRow = temp;
        }
        inFile.close();
        newIndex = GetRowID(lastRow) + 1;
    }
    lock_guard<recursive_mutex> lock3(GetFileMutex(currFilepath));
    ofstream outFile(currFilepath, ios::app);
    if (!outFile.is_open()) return 0; 
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
    lock_guard<recursive_mutex> lock4(GetFileMutex(name + "/" + name + "_last_Line.txt"));
    ofstream pkFile(name + "/" + name + "_last_Line.txt");
    if (!pkFile.is_open()) return 0; 
    pkFile << to_string(LastLine);
    pkFile.close();

    return 1; // успех
}

bool ConceptTable::Correction(string& criteria, string& nameColumn, string& newValue, string &username) {

    if(!FindByCriteria(criteria,username)) return false;
    string criteriaFilePath = "finalFile_" + username + ".tmp";
    string tempFilteredPath = name + "/correctedRows_" + username + ".tmp";

    json jsonData = ReadSchema(name + "/" + name + ".json");
    int columnIndex = GetColumnIndex(jsonData, name, nameColumn);
    if (columnIndex == -1) return false;

    ofstream corrected(tempFilteredPath);
    for (int i = 1; i <= lastCSV; i++) {
        lock_guard<recursive_mutex> lock3(GetFileMutex(name + "/" + name + "_" + to_string(i) + ".csv"));
        string filePath = name + "/" + name + "_" + to_string(i) + ".csv";
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
            if (match) {
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

            lock_guard<recursive_mutex> lock5(GetFileMutex(newPath));
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


    return true;
}
bool ConceptTable::DeleteRowByCriteria(string& criteria, string &username) {
   
    bool deleted = false;
    if(FindByCriteria(criteria,username)) deleted = true;
    else return false;

    string criteriaFilePath = "finalFile_" + username + ".tmp";
    string tempFilteredPath = name + "/filteredRows_" + username + ".tmp";

    ofstream filtered(tempFilteredPath);
    for (int i = 1; i <= lastCSV; i++) {
        string filePath = name + "/" + name + "_" + to_string(i) + ".csv";
        if (!filesystem::exists(filePath)) continue;
        ifstream inFile(filePath);
        lock_guard<recursive_mutex> lock3(GetFileMutex(filePath));

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
            lock_guard<recursive_mutex> lock5(GetFileMutex(newPath));
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
    lock_guard<recursive_mutex> lock6(GetFileMutex(name + "/" + name + "_list_CSV.txt"));
    ofstream lastCSVfile(name + "/" + name + "_list_CSV.txt");
    lastCSVfile << lastCSV;
    lastCSVfile.close();
    lock_guard<recursive_mutex> lock7(GetFileMutex(name + "/" + name + "_last_Line.txt"));
    ofstream lastLinefile(name + "/" + name + "_last_Line.txt");
    lastLinefile << ((rowCount == 0) ? tupleLimit : rowCount);
    lastLinefile.close();
    return deleted;
}
