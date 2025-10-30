#include <iostream>
#include <fstream>
#include <dirent.h>
#include "json.hpp"
#include "headerFiles/condition.h"
#include "headerFiles/crud.h"
#include "headerFiles/condition_additional.h"
#include "headerFiles/filelocks.h"
using namespace std;
using json = nlohmann::json;

recursive_mutex& GetDirMutex(const string& dirPath) {
    static map<string, recursive_mutex> dirMutexes;
    return dirMutexes[dirPath]; 
}

void DeleteTmpInDirectory(const string& path, const string& username) {
    lock_guard<recursive_mutex> lock(GetDirMutex(path));
    DIR* dir;
    struct dirent* entry;
    int deleted = 0;
    dir = opendir(path.c_str());
    if (!dir) return; 
    const string suffix = "_" + username + ".tmp";
    while ((entry = readdir(dir)) != nullptr) {
        string filename = entry->d_name;
        if (filename.size() >= suffix.size() &&
            filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0) {
            string fullPath = path + "/" + filename;
            if (remove(fullPath.c_str()) == 0) {
                deleted++;
            }
        }
    }
    closedir(dir);
}
void RemoveConditionByIndex(Condition*& head, int& index) {
    
    Condition* curr = head;
    Condition* prev = nullptr;
    if (curr == nullptr) return;
    if (curr->index == index) {
        head = curr->next;  
        delete curr; 
        return;
    }
    while (curr != nullptr && curr->index != index) {
        prev = curr;
        curr = curr->next;
    }
    if (curr == nullptr) {
        return;
    }
    prev->next = curr->next;
    delete curr;
}
string GetCellByIndex(const string& row, int index) {
    stringstream ss(row);
    string cell;
    int count = 0;
    while (getline(ss, cell, ';')) {
        if (count == index) {
            return cell;  
        }
        count++;
    }
    return "";  
}

string CleanString(const string& str) {
    int first = str.find_first_not_of(" \t\n\r");
    int last = str.find_last_not_of(" \t\n\r");
    if (first == string::npos || last == string::npos) {
        return "";
    }
    return str.substr(first, last - first + 1);
}

int GetColumnIndex(json& schema, string& nameTable, string& nameColumn) {
    auto& columns = schema["structure"][nameTable];
    for (int i = 0; i < columns.size(); i++) {
        if (columns[i] == nameColumn) {
            if(nameTable == "books") i++;
            return i;
        }
    }
    return -1;
}
Condition* FindConditionOper(Condition* head, const string& oper) {
    Condition* current = head;
    
    while (current != nullptr) {
        if (current->oper == oper) {
            return current;  
        }
        current = current->next;  
    }
    
    return nullptr; 
}

Condition* ConstFindConditionOper(const Condition* head, const string& oper) {
    const Condition* current = head; 
    while (current != nullptr) {
        if (current->oper == oper) {
            return const_cast<Condition*>(current);  
        }
        current = current->next; 
    }
    return nullptr; 
}

int GetSizeCondition(Condition* head) {
    int size = 0;
    Condition* current = head;
    while (current != nullptr) {
        size++;
        current = current->next;
    }
    return size;
}
bool isLineInFile(const string& filename, const string& lineToCheck) {
    ifstream file(filename); 
    string currentLine;
    while (getline(file, currentLine)) {
        if (currentLine == lineToCheck) {
            file.close(); 
            return true;
        }
    }
    file.close(); 
    return false; 
}
