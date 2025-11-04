#include <iostream>
#include <vector>
#include <fstream>
#include "json.hpp"
#include "headerFiles/condition_additional.h"
#include "headerFiles/workingCSV.h"
#include "headerFiles/condition.h"
#include <filesystem>
#include <mutex>
#include "headerFiles/filelocks.h"
#include <unordered_set>
using json = nlohmann::json;

using namespace std;


Condition* SplitExpressionForStruct(string& filter) {
    
    Condition* firstElement = nullptr;
    Condition* lastElement = nullptr;
    int begin = 0;
    int end;
    int conditionCount = 0;
    while (begin < filter.length()) {
        end = filter.find("AND", begin);
        int findOR = filter.find("OR", begin);
        if (findOR != string::npos && (end == string::npos || findOR < end)) {
            end = findOR;
        }
        if (end == string::npos) {
            end = filter.length();
        }
        Condition* newNode = new Condition;
        newNode->condition = filter.substr(begin, end - begin);
        newNode->next = nullptr;
        conditionCount++;
        newNode->index = conditionCount; 

        if (end < filter.length()) {
            if (filter.substr(end, 3) == "AND") {
                newNode->oper = "AND";
                end += 3;
            }
            else if (filter.substr(end, 2) == "OR") {
                newNode->oper = "OR";
                end += 2;
            }
            else {
                newNode->oper = "";
            }
        }
        else {
            newNode->oper = "";
        }

        if (firstElement == nullptr) {
            firstElement = newNode;
            lastElement = firstElement;
        }
        else {
            lastElement->next = newNode;
            lastElement = newNode;
        }

        begin = end + 1;
    }
    SetCountCondForAll(firstElement, conditionCount);
    return firstElement;
}

bool CheckCondition(vector<string>& parameters, const string &tmpFileName, const int& tmpFileCount, string& username) {

    bool isFound = false;
    if(parameters.size() == 3) {
        string nameTable = parameters[0];
        string nameColumn = parameters[1];
        string target = parameters[2];
        int lastCSV;
        lock_guard<recursive_mutex> lock1(GetFileMutex(nameTable + "/" + nameTable + "_list_CSV.txt"));
        ifstream csvFile(nameTable + "/" + nameTable + "_list_CSV.txt");
        csvFile >> lastCSV;
        csvFile.close();

        json schema = ReadSchema(nameTable + "/" + nameTable + ".json");

        ofstream tmpFile(nameTable + "/" + tmpFileName + "_" + to_string(tmpFileCount) + "_" + username +".tmp", ios::app);

        for(int i = 1; i <= lastCSV; i++) {
            lock_guard<recursive_mutex> lock3(GetFileMutex(nameTable + "/" + nameTable + "_" + to_string(i) + ".csv"));
            ifstream inFile(nameTable + "/" + nameTable + "_" + to_string(i) + ".csv");
            string temp;
            while (getline(inFile, temp)) {
                int targetColumn = GetColumnIndex(schema, nameTable, nameColumn);
                if(targetColumn == -1) return false;
                stringstream ss(temp);
                string value;
                int j = 0;

                while (getline(ss, value, ';')) {
                    if (j == targetColumn) {
                        if (value == target) {
                            tmpFile << temp << endl;
                            isFound = true;
                            break;
                        }
                        else {
                            break;
                        }
                    }
                    j++;
                }
            }
            inFile.close();
        }
        tmpFile.close(); 
    }
    
    return isFound;
}


Condition* ReplacingConditionsWithBool(Condition* expressions, string &username) {

    Condition* head = expressions;
    int i = 1;
    while(expressions != nullptr) {
        string condition = expressions->condition;
        int equalPos = condition.find('=');

        string leftSide = condition.substr(0, equalPos);
        string rightSide = condition.substr(equalPos + 1);
        
        leftSide = CleanString(leftSide);
        rightSide = CleanString(rightSide);

        if (rightSide[0] == '\'') {
            rightSide.erase(remove(rightSide.begin(), rightSide.end(), '\''), rightSide.end());
            int pointPos = condition.find('.');
            string nameTable = condition.substr(0, condition.find('.'));
            string nameColumn = condition.substr(pointPos + 1, equalPos - pointPos - 1);
            string target = rightSide;

            nameTable = CleanString(nameTable);
            nameColumn = CleanString(nameColumn);
            target = CleanString(target);

            vector<string> parameters = {nameTable, nameColumn, target};
            json jsonData = ReadSchema(nameTable + "/" + nameTable + ".json");
            int indexTargetColumn = GetColumnIndex(jsonData,nameTable,nameColumn);
            if(CheckCondition(parameters, "CheckCondition", i, username)) {
                expressions->trueOrFalse = true;
                expressions->targetColumns.push_back(indexTargetColumn);
            }
            else {
                expressions->trueOrFalse = false;
                expressions->targetColumns.push_back(indexTargetColumn);
            }

        }

        i++;
        expressions = expressions->next;
    }
    return head;
}


bool FilteringForOneFile(Condition* condition, string& username) { 
    Condition* tempHead = condition;
    Condition* currentCondition = tempHead;
    Condition* prevCondition;
    string nameTable;

    bool fullness = false;
    bool openNewBlockAnd = false;
    int countBlocksAnd = 1;
    bool isFoundOR = ConstFindConditionOper(condition, "OR");
    bool isFoundAND = ConstFindConditionOper(condition, "AND");
    bool withoutBlocks = false;
    vector<int> blocksToSkip;
    while(true) {
        if((isFoundAND == false && isFoundOR == false) || (isFoundAND == false && isFoundOR == true) ) {

            if(currentCondition->trueOrFalse == false) {
                if(currentCondition->index == currentCondition->maxCount) break;
                currentCondition = currentCondition->next;
                continue;
            }
            

            int indexCondition = currentCondition->index;
            if(currentCondition->condition.substr(0,5) == "books") nameTable = "books";
            else nameTable = "shops";
            ofstream finalFile("finalFile_" + username + ".tmp", ios::app);
            ifstream conditionFile(nameTable + "/"+ "CheckCondition_" + to_string(indexCondition) + "_" + username +".tmp");
            string line;
            while(getline(conditionFile,line)) {
                finalFile << line << endl;
                finalFile.flush();
                fullness = true;
            }
            finalFile.close();
            conditionFile.close();
        }

        else if(isFoundAND == true && isFoundOR == false) {

            if(currentCondition->trueOrFalse == false) return false;

            withoutBlocks = true;
            if(currentCondition->index == 1) {
                currentCondition = currentCondition->next;
                continue;
            }
            if(currentCondition->condition.substr(0,5) == "books") nameTable = "books";
            else nameTable = "shops";

            int targetColumn = currentCondition->targetColumns[0];
            int indexCondition = currentCondition->index;
            ifstream conditionFileA;
            ofstream BlockAND;

            if (indexCondition == 2) conditionFileA.open(nameTable + "/"+ "CheckCondition_" + to_string(indexCondition - 1) + "_" + username + ".tmp");
            else conditionFileA.open(nameTable + "/" + "BlockAND_" + to_string(indexCondition - 2) + "_" + username + ".tmp");
            
            string targetCell;
            ifstream conditionFileB(nameTable + "/"+ "CheckCondition_" + to_string(indexCondition) + "_" + username +".tmp");
            string rowSecondFile;
            getline(conditionFileB, rowSecondFile);

            targetCell = GetCellByIndex(rowSecondFile, targetColumn);
            conditionFileB.close();
            string rowFirstFile;

            ofstream BlockANDnew(nameTable + "/" + "BlockAND_" + to_string(indexCondition - 1) + "_" + username +".tmp", ios::app);
            while(getline(conditionFileA, rowFirstFile)) {
                string currentTarget = GetCellByIndex(rowFirstFile,targetColumn);
                if(currentTarget == targetCell) {
                    BlockANDnew << rowFirstFile << endl;
                    BlockANDnew.flush();
                    fullness = true;
                }
            }
            BlockANDnew.close();
            conditionFileA.close();
        }

        else if(isFoundAND == true && isFoundOR == true) {
            
            if (currentCondition->trueOrFalse == false || currentCondition->targetColumns[0] == -1) {
                while(currentCondition->index != currentCondition->maxCount && currentCondition->oper != "OR") {
                    prevCondition = currentCondition;
                    currentCondition = currentCondition->next;  
                }
                if (currentCondition->index == currentCondition->maxCount) break;
            }

            if(currentCondition->index == 1) {
                prevCondition = currentCondition;
                if(currentCondition->index == currentCondition->maxCount) break;
                currentCondition = currentCondition->next;
                continue;
            }
            else if(prevCondition->oper== "OR") {
                openNewBlockAnd = true;
                prevCondition = currentCondition;
                if(currentCondition->index == currentCondition->maxCount) break;
                currentCondition = currentCondition->next;
                continue;
            }
            
            if(currentCondition->condition.substr(0,5) == "books") nameTable = "books";
            else nameTable = "shops";


            int targetColumn = currentCondition->targetColumns[0];

            int indexCondition = currentCondition->index;
            ifstream conditionFileA;
            if (indexCondition == 2 || openNewBlockAnd == true) {

                if(openNewBlockAnd) countBlocksAnd++;
                openNewBlockAnd = false;
                conditionFileA.open(nameTable + "/"+ "CheckCondition_" + to_string(indexCondition - 1) + "_" + username + ".tmp");
                
            }
            
            else {
                conditionFileA.open(nameTable + "/" + "BlockAND_" + to_string(indexCondition - 1) + "_" + to_string(countBlocksAnd) + "_" + username + ".tmp");
            }
            
            string targetCell;
            ifstream conditionFileB(nameTable + "/"+ "CheckCondition_" + to_string(indexCondition) + "_" + username +".tmp");
            string rowSecondFile;
            getline(conditionFileB, rowSecondFile);

            targetCell = GetCellByIndex(rowSecondFile, targetColumn);
            conditionFileB.close();
            string rowFirstFile;

            ofstream BlockAND(nameTable + "/" + "BlockAND_" + to_string(indexCondition) + "_" + to_string(countBlocksAnd) + "_" + username +".tmp", ios::app);
            bool rowAdded = false;
            while(getline(conditionFileA, rowFirstFile)) {
                string currentTarget = GetCellByIndex(rowFirstFile,targetColumn);
                
                if(currentTarget == targetCell) {
                    BlockAND << rowFirstFile << endl;
                    BlockAND.flush();
                    fullness = true;
                    rowAdded = true;
                }
            }
            if(rowAdded == false) blocksToSkip.push_back(countBlocksAnd);
            BlockAND.close();
            conditionFileA.close();
            prevCondition = currentCondition;
        }

        if(currentCondition->index == currentCondition->maxCount) break;
        else {
            currentCondition = currentCondition->next;
        }

    }


    if(currentCondition->index == currentCondition->maxCount) {
        if(isFoundAND == true) {
            ofstream finalFile("finalFile_" + username + ".tmp", ios::app);
            for(int i = 1; i <= countBlocksAnd; i++ ) {
                if(find(blocksToSkip.begin(), blocksToSkip.end(), i) != blocksToSkip.end()) {
                    continue;
                }
                ifstream BlockANDnew;
                if(withoutBlocks == false) {
                    int targetFirstNumber = getLastJForI("books", i, username);
                    BlockANDnew.open(nameTable + "/" + "BlockAND_" + to_string(targetFirstNumber) + "_" + to_string(i) + "_" + username +".tmp");
                }
                else {
                    BlockANDnew.open(nameTable + "/" + "BlockAND_" + to_string(currentCondition->index - 1) + "_" + username + ".tmp");
                }
                
                if (!BlockANDnew.is_open()) continue;
                string row;
                while(getline(BlockANDnew, row)) {
                    finalFile << row << endl;
                    finalFile.flush();
                    fullness = true;
                }
                BlockANDnew.close();
                
            }
            finalFile.close();
        }
        
        return fullness;
    }
}

bool FindByCriteria(string& expression, string &username) { 
    bool fullness;
    Condition* condition = SplitExpressionForStruct(expression);
    Condition* newCondition = ReplacingConditionsWithBool(condition,username);
    if(FilteringForOneFile(newCondition,username)) {
        fullness = true;
    }
    else {
        fullness = false;
    }
    return fullness;
}