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

bool ConceptTable::DeleteLastRow() {
    string currFilepath = name + "/" + name + "_" + to_string(lastCSV) + ".csv";
    if (LastLine == 0) {
        return false; 
    }
    else if (LastLine == 1) {
        filesystem::remove(currFilepath);
        LastLine = tupleLimit;
        lastCSV--;
        ofstream pkFile(name + "/" + name + "_last_Line.txt");
        pkFile << LastLine;
        pkFile.close();

        ofstream listCSV(name + "/" + name + "_list_CSV.txt");
        listCSV << lastCSV;
        listCSV.close();
        return true;
    }
    string currLine;
    string prevLine;
    string tempFilepath = "DeleteLastRow.tmp";
    ifstream inFile(currFilepath);
    ofstream tempFile(tempFilepath);

    bool isFirst = true;
    while (getline(inFile, currLine)) {
        if (isFirst==false) {
            tempFile << prevLine << endl;
        }
        prevLine = currLine;
        isFirst = false;
    }
    inFile.close();
    tempFile.close();

    filesystem::remove(currFilepath);
    filesystem::rename(tempFilepath, currFilepath);

    LastLine--;
    ofstream pkFile(name + "/" + name + "_last_Line.txt");
    pkFile << LastLine;
    pkFile.close();
    return 1;
}


bool ConceptTable::DeleteRowByCriteria(string& criteria) {
    bool parameter = false;
    FindByCriteria(criteria, parameter);
    string criteriaFilePath = "finalFile.tmp";
    bool skipRow = false;
    for (int i = 1; i <= lastCSV; i++) {
        string currFilepath = name + "/" + name + "_" + to_string(i) + ".csv";
        ifstream currFile(currFilepath);
        string tempFilepath = name + "/" + name + "_" + to_string(i) + "_temp.csv";
        ofstream tempFile(tempFilepath);
        string currLine;

        int index = 1;
        ifstream conditionFile(criteriaFilePath);
        bool needDelete = false;
        if(skipRow == false) {
            while(getline(currFile, currLine)) {
                bool deleteRow = false;

                string condition;
                while(getline(conditionFile, condition)) {
                    if (currLine.find(condition) != string::npos) {
                        deleteRow = true;
                        break;
                    }
                }
                conditionFile.clear();
                conditionFile.seekg(0, ios::beg);

                if(deleteRow == true) {
                     if(index == 1) {
                        if(i == lastCSV && index == LastLine) {
                            currFile.close();
                            LastLine = tupleLimit;
                            lastCSV--;
                            needDelete = true;
                        }
                     }
                     else if(index == tupleLimit) {
                        ifstream nextCSV(name + "/" + name + "_" + to_string(i+1) + ".csv");
                        string nextLine;
                        getline(nextCSV, nextLine);
                        tempFile << to_string(tupleLimit) << nextLine.substr(nextLine.find(';') + 1) << endl;

                        skipRow = true;
                        nextCSV.close();
                        index = 1;
                        LastLine--;
                     }
                     else {
                        auto pos = currLine.find(";");
                        string currIndexStr = currLine.substr(0,pos);
                        int currIndexInt = stoi(currIndexStr) - 1;
                        tempFile << to_string(currIndexInt) << ";" << currLine.substr(currLine.find(';') + 1) << endl;
                        index--;
                        LastLine--;
                     }

                }
                else {
                    tempFile << to_string(index) << ";" << currLine.substr(currLine.find(';') + 1) << endl;
                    index++;
                }
            }
        }
        else {
            skipRow = false;
            index = 1;
            continue;
        }
        currFile.close();
        tempFile.close();
        if (needDelete == true) {
            filesystem::remove(currFilepath);
            filesystem::remove(tempFilepath);
            needDelete = false;
        } else {
            filesystem::remove(currFilepath);  
            filesystem::rename(tempFilepath, currFilepath);
        }
        ofstream lastCSVfile(name + "/" + name + "_list_CSV.txt");
        lastCSVfile << lastCSV;
        lastCSVfile.close();
        ofstream lastLinefile(name + "/" + name + "_last_Line.txt");
        lastLinefile << LastLine;
        lastLinefile.close();
    }

    DeleteTmpInDirectory(".");         
    DeleteTmpInDirectory("books"); 
    DeleteTmpInDirectory("shops");
    return true;
}