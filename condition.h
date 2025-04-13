#include <iostream>
using namespace std;

struct Condition {
    vector<int> targetColumns;
    int index;
    bool isCartesian = false;
    bool trueOrFalse;
    string condition;
    string oper;
    Condition* next;
};

void PrintFinalFile() {
    ifstream file("finalFile.tmp");
    string line;
    cout << endl << endl;
    while(getline(file,line)) {
        cout << line << endl;
    }
    cout << endl << endl;
    file.close();
}
void DeleteTmpInDirectory(const string& path) {
    DIR* dir;
    struct dirent* entry;
    int deleted = 0;

    dir = opendir(path.c_str());
    while ((entry = readdir(dir)) != nullptr) {
        string filename = entry->d_name;
        if (filename.size() >= 4 && filename.substr(filename.size() - 4) == ".tmp") {
            string fullPath = path + "/" + filename;
            if (remove(fullPath.c_str()) == 0) {
                deleted++;
            }
        }   

    }
    closedir(dir);
}
void RemoveConditionByIndex(Condition*& head, int index) {
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
    return firstElement;
}
int GetColumnIndex(json& schema, string& nameTable, string& nameColumn) {
    auto& columns = schema["structure"][nameTable];
    for (int i = 0; i < columns.size(); i++) {
        if (columns[i] == nameColumn) {
            return (i + 1);
        }
    }
    return -1;
}

bool SelectFromManyTables(vector<string> parameters, const int& tmpFileCount) {
    bool isFound = false;
    vector<string> parametersA = {parameters[0], parameters[1]};
    vector<string> parametersB = {parameters[2], parameters[3]};

    string nameTableA, targetColumnA;
    string nameTableB, targetColumnB;
    if(parameters[0] == "books") {
        nameTableA = "books"; targetColumnA = parameters[1];
        nameTableB = "shops"; targetColumnB = parameters[3];
    }
    else {
        nameTableB = "books"; targetColumnB = parameters[1];
        nameTableA = "shops"; targetColumnA = parameters[3];
    }
    json jsonDataBooks = ReadSchema(nameTableA + "/" + nameTableA + ".json");
    json jsonDataShops = ReadSchema(nameTableB + "/" + nameTableB + ".json");
    ofstream finalFileCartesian("SelectFromCartesian_" + to_string(tmpFileCount) + ".tmp", ios::app);
    int lastCSVBooks;
    ifstream csvFile(nameTableA + "/" + nameTableA + "_list_CSV.txt");
    csvFile >> lastCSVBooks;
    csvFile.close();

    int lastCSVShops;
    ifstream csvFile1(nameTableB + "/" + nameTableB + "_list_CSV.txt");
    csvFile1 >> lastCSVShops;
    csvFile1.close();

    for(int i = 1; i <= lastCSVBooks; i++) {
        string rowBooks;
        ifstream fileBooks(nameTableA + "/" + nameTableA + "_" + to_string(i) + ".csv");
        while(getline(fileBooks, rowBooks)) {
            int columnBooks = GetColumnIndex(jsonDataBooks, nameTableA, targetColumnA);
            string cellBooks = GetCellByIndex(rowBooks, columnBooks);
            for(int j = 1; j <= lastCSVShops; j++) {
                ifstream fileShops(nameTableB + "/" + nameTableB + "_" + to_string(j) + ".csv");
                string rowShops;
                while(getline(fileShops, rowShops)) {
                
                    int columnShops = GetColumnIndex(jsonDataShops, nameTableB, targetColumnB);
                    string cellShops = GetCellByIndex(rowShops, columnShops);

                    if(cellBooks == cellShops) {
                        finalFileCartesian << rowBooks << ";" << rowShops << endl;
                        isFound = true;

                    }
                }
            }
        }
    };

    finalFileCartesian.close();
    return isFound;
}
bool CheckCondition(vector<string>& parameters, const string &tmpFileName, const int& tmpFileCount, bool& hasCartezian) {
    bool isFound = false;
    if(parameters.size() == 3) {
        string nameTable = parameters[0];
        string nameColumn = parameters[1];
        string target = parameters[2];
        int lastCSV;
        ifstream csvFile(nameTable + "/" + nameTable + "_list_CSV.txt");
        csvFile >> lastCSV;
        csvFile.close();

        json schema = ReadSchema(nameTable + "/" + nameTable + ".json");
        ofstream tmpFile(nameTable + "/" + tmpFileName + "_" + to_string(tmpFileCount) + ".tmp", ios::app);

        for(int i = 1; i <= lastCSV; i++) {
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
    else {
        string nameTableA = parameters[0];
        string nameColumnA = parameters[1];
        string nameTableB = parameters[2];
        string nameColumnB = parameters[3];

        if(nameTableA == nameTableB ) { 
            int lastCSV;
            ifstream csvFile(nameTableA + "/" + nameTableA + "_list_CSV.txt");
            csvFile >> lastCSV;
            csvFile.close();

            json schema = ReadSchema(nameTableA+ "/" + nameTableA + ".json");
            ofstream tmpFile(nameTableA + "/" + tmpFileName + "_" + to_string(tmpFileCount) + ".tmp", ios::app);
            
            for(int i = 1; i <= lastCSV; i++) {
                ifstream inFile(nameTableA + "/" + nameTableA + "_" + to_string(i) + ".csv");
                string temp;
                while (getline(inFile, temp)) {
                    int targetColumnA = GetColumnIndex(schema, nameTableA, nameColumnA);
                    int targetColumnB = GetColumnIndex(schema, nameTableA, nameColumnB);
                    if(targetColumnA == -1 || targetColumnB == -1) return false;
                    stringstream ss(temp);
                    string value;
                    int j = 0;
                    string targetA = "nothing";
                    string targetB = "nothing";
                    while (getline(ss, value, ';')) {
                        if ((j == targetColumnA || j == targetColumnB) && (targetA != "nothing" || targetB != "nothing")) {
                            targetB = value;
                            if(targetA == targetB) {
                                tmpFile << temp << endl;
                                isFound = true;
                                break; 
                            }
                            else {
                                break;
                            }
                        }
                        else if( (j == targetColumnA || j == targetColumnB) && (targetA == "nothing" || targetB == "nothing")) {
                            targetA = value;
                        }
                        
                        j++;
                    }
                }
                inFile.close();
            }
            tmpFile.close();
        }
        else { 
            hasCartezian = true;
            if(SelectFromManyTables(parameters, tmpFileCount)) isFound = true;
        }
    }
    return isFound;
}


Condition* ReplacingConditionsWithBool(Condition* expressions) {
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
            bool isCartezian = false;
            if(CheckCondition(parameters, "CheckCondition", i, isCartezian)) {
                if(isCartezian) expressions->isCartesian = true;
                expressions->trueOrFalse = true;
                expressions->targetColumns.push_back(indexTargetColumn);
            }
            else expressions->trueOrFalse = false;

        }
        else {
            string nameTableA = leftSide.substr(0, leftSide.find('.')); 
            int pointPosA = leftSide.find('.');
            string nameColumnA = leftSide.substr(pointPosA + 1, equalPos - pointPosA - 1); 

            string nameTableB = rightSide.substr(0, rightSide.find('.')); 
            int pointPosB = rightSide.find('.');
            string nameColumnB = rightSide.substr(pointPosB + 1);
            
            nameTableA = CleanString(nameTableA);
            nameColumnA = CleanString(nameColumnA);
            nameTableB = CleanString(nameTableB);
            nameColumnB = CleanString(nameColumnB);
            vector<string> parameters = {nameTableA, nameColumnA, nameTableB, nameColumnB};
            json jsonDataA = ReadSchema(nameTableA + "/" + nameTableA + ".json");
            json jsonDataB = ReadSchema(nameTableB + "/" + nameTableB + ".json");
            int indexTargetColumnA = GetColumnIndex(jsonDataA,nameTableA,nameColumnA);
            int indexTargetColumnB = GetColumnIndex(jsonDataB,nameTableB,nameColumnB);
            bool isCartezian = false;
            if(CheckCondition(parameters, "CheckCondition", i, isCartezian)) {
                if(isCartezian) expressions->isCartesian = true;
                expressions->trueOrFalse = true;
                expressions->targetColumns.push_back(indexTargetColumnA);
                expressions->targetColumns.push_back(indexTargetColumnB);
            }
            else expressions->trueOrFalse = false;
            

        }
        i++;
        expressions = expressions->next;
    }
    return head;
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
bool isLineInFile(const std::string& filename, const std::string& lineToCheck) {
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

bool FilteringForOneFile(Condition* condition) {
    string result;
    Condition* tempHead = condition;
    bool fullness = false;
    bool isFoundOR = ConstFindConditionOper(condition, "OR");
    while(condition != nullptr) {
        if(condition->oper == "AND" && condition->trueOrFalse == false) {
            if(isFoundOR == false) return false;
            RemoveConditionByIndex(tempHead, condition->index);
            int tempCond = condition->index + 1;
            RemoveConditionByIndex(tempHead, tempCond);
        }
        else if(condition->oper == "AND" && condition->trueOrFalse == true) {
            if(condition->next->trueOrFalse == false) {
                if(isFoundOR == false) return false;
                RemoveConditionByIndex(tempHead, condition->index);
                int tempCond = condition->index + 1;
                RemoveConditionByIndex(tempHead, tempCond);
            }
        }
        condition = condition->next;
    }
    Condition* newTempHead = tempHead;
    int countConditions = GetSizeCondition(tempHead);

    while(tempHead != nullptr) {
        if(tempHead->isCartesian) {
            if(tempHead->oper == "OR" && tempHead->trueOrFalse == 1) {
                int indexCondition = tempHead->index;
                ofstream finalCartezianFile("finalFile.tmp", ios::app);
                ifstream cartezianFile("SelectFromCartesian_" + to_string(indexCondition)+ ".tmp");
                string temp;
                while(getline(cartezianFile, temp)) {
                    finalCartezianFile << temp << endl;
                    fullness = true;
                }
                finalCartezianFile.close();
                cartezianFile.close();
                RemoveConditionByIndex(tempHead, indexCondition);
                continue;
            }
            else if(tempHead->oper == "AND") { 
                string condition = tempHead->condition;
                string nameTable;
                if(condition.substr(0,5) == "books") nameTable = "books";
                else nameTable = "shops";
                
                int targetPartCartesian;
                if(condition.substr(0,5) == "books" && tempHead->next->condition.substr(0,5) == "books") targetPartCartesian = 1;
                else if(condition.substr(0,5) == "books" && tempHead->next->condition.substr(0,5) == "shops") targetPartCartesian = 2;
                else if(condition.substr(0,5) == "shops" && tempHead->next->condition.substr(0,5) == "books") targetPartCartesian = 2;
                else if(condition.substr(0,5) == "shops" && tempHead->next->condition.substr(0,5) == "shops") targetPartCartesian = 1;
                ofstream finalCartezianFile("finalFile.tmp", ios::app);

                int indexConditionA = tempHead->index;
                int indexConditionB = tempHead->next->index;
                string nameTableB = (tempHead->next->condition).substr(0,5);
                ifstream cartezianFile("SelectFromCartesian_" + to_string(indexConditionA)+ ".tmp"); 
                ifstream ordinaryFile(nameTableB + "/" + "CheckCondition_" + to_string(indexConditionB)+ ".tmp"); 
                int targetColumnOrdinary = tempHead->targetColumns[0];
                int targetColumnCartesian;
                string tempA;
                while(getline(cartezianFile, tempA)) {
                    int countColumnCartesian;
                    string cellA;
                    if(targetPartCartesian == 1) {
                        cellA = GetCellByIndex(tempA,targetColumnOrdinary);
                    }
                    else {
                        if(condition.substr(0,5) == "books") cellA = GetCellByIndex(tempA,(targetColumnOrdinary + 10 ));
                        else if(condition.substr(0,5) == "shops") cellA = GetCellByIndex(tempA,(targetColumnOrdinary + 5 ));
                    }
                    string tempB;
                    ordinaryFile.clear();            
                    ordinaryFile.seekg(0, ios::beg);
                    while(getline(ordinaryFile,tempB)) {
                        string tempB1;
                        tempB1 = GetCellByIndex(tempB, targetColumnOrdinary);
                        if(tempB1 == cellA) {
                            finalCartezianFile << tempA<< endl;
                            fullness = true;
                        } 
                    }
                }
                cartezianFile.close();
                ordinaryFile.close();
                finalCartezianFile.close();
                RemoveConditionByIndex(tempHead, indexConditionA);
                RemoveConditionByIndex(tempHead, indexConditionB);
                countConditions = countConditions - 2;
            }
            else if (tempHead->oper == ""){
                int indexCondition = tempHead->index;
                ifstream cartezianFile("SelectFromCartesian_" + to_string(indexCondition)+ ".tmp"); 
                ofstream finalFile("finalFile.tmp", ios::app);
                string temp;
                while(getline(cartezianFile, temp)) {
                    finalFile << temp << endl;
                    fullness = true;
                }
                cartezianFile.close();
                finalFile.close();
                RemoveConditionByIndex(tempHead, indexCondition);
            }
        }
        else if(tempHead->oper == "AND" || ConstFindConditionOper(tempHead, "AND") != nullptr) {
            if(tempHead->next->isCartesian == true) {
                string condition = tempHead->condition;
                string nameTable;
                if (condition.substr(0, 5) == "books") nameTable = "books";
                else nameTable = "shops";

                int targetPartCartesian = (nameTable == "books") ? 1 : 2;
                ofstream finalCartezianFile("finalFile.tmp", ios::app);
                int indexConditionOrdinary = tempHead->index;
                int indexConditionCartesian = tempHead->next->index;
                ifstream ordinaryFile(nameTable + "/CheckCondition_" + to_string(indexConditionOrdinary) + ".tmp");
                ifstream cartezianFile("SelectFromCartesian_" + to_string(indexConditionCartesian) + ".tmp");
                int targetColumnOrdinary = tempHead->targetColumns[0];
                string tempCartesian;

                while (getline(cartezianFile, tempCartesian)) {
                    string cellFromCartesian;
                    if (targetPartCartesian == 1) {
                        cellFromCartesian = GetCellByIndex(tempCartesian, targetColumnOrdinary);
                    } else {
                        cellFromCartesian = GetCellByIndex(tempCartesian, targetColumnOrdinary + 10); 
                    }
                    string tempOrdinary;

                    ordinaryFile.clear();
                    ordinaryFile.seekg(0, ios::beg);
                    while (getline(ordinaryFile, tempOrdinary)) {
                        string cellFromOrdinary = GetCellByIndex(tempOrdinary, targetColumnOrdinary);
                        if (cellFromOrdinary == cellFromCartesian) {
                            finalCartezianFile << tempCartesian << endl;
                            fullness = true;
                            
                        }
                    }
                    ordinaryFile.clear();
                    ordinaryFile.seekg(0);
                }
                cartezianFile.close();
                ordinaryFile.close();
                finalCartezianFile.close();
            }
            Condition* head = tempHead;
            tempHead = FindConditionOper(tempHead, "AND");
            string condition = tempHead->condition;
            string nameTable;
            if(condition.substr(0,5) == "books") nameTable = "books";
            else nameTable = "shops";

            int count;
            int indexConditionA = tempHead->index;
            int indexConditionB = tempHead->next->index;
            int targetColumnB;
            int targetColumnB1, targetColumnB2;
            if(tempHead->targetColumns.size() == 1) {
                targetColumnB = tempHead->next->targetColumns[0];
                count = 1;

            }
            else {
                targetColumnB1 = tempHead->next->targetColumns[0];
                targetColumnB2 = tempHead->next->targetColumns[1];
                count = 2;

            }
            ofstream finalFile("finalFile.tmp", ios::app);
            ifstream conditionFileA(nameTable + "/"+ "CheckCondition_" + to_string(indexConditionA) + ".tmp");
            ifstream conditionFileB(nameTable + "/"+ "CheckCondition_" + to_string(indexConditionB) + ".tmp");
            if(count == 1) {
                string tempA;
                while(getline(conditionFileA,tempA)) {
                    string tempB;
                    conditionFileB.clear();              
                    conditionFileB.seekg(0, ios::beg);
                    getline(conditionFileB,tempB);
                    string cellA =GetCellByIndex(tempA, targetColumnB);
                    string cellB =GetCellByIndex(tempB, targetColumnB);
                    if(cellA == cellB) {
                        finalFile << tempA << endl;
                        fullness = true;
                    }
                }
            }
            else {
                string tempA;
                while(getline(conditionFileA,tempA)) { 
                    string tempB;
                    getline(conditionFileB,tempB);
                    stringstream ss(tempA);
                    string cell;
                    int countColumns = 1;
                    int countPairs = 0;
                    while(getline(ss,cell, ';')) {
                        if(countPairs == 2) {
                            finalFile << tempA << endl;
                            fullness = true;
                            break; 
                        }
                        countColumns++;
                        if(countColumns == targetColumnB1 || countColumns == targetColumnB2) {
                            countPairs++;
                        }
                    }
                }
            }
            finalFile.close();
            conditionFileA.close();
            conditionFileB.close();
            tempHead = head;
            RemoveConditionByIndex(tempHead,indexConditionA);
            RemoveConditionByIndex(tempHead,indexConditionB);
            countConditions = countConditions - 2;
            
        }
        else {
            string condition = tempHead->condition; 
            string nameTable;
            if(condition.substr(0,5) == "books") nameTable = "books";
            else nameTable = "shops";
            int indexCondition = tempHead->index;

            
            ifstream conditionFileA(nameTable + "/" + "CheckCondition_" + to_string(indexCondition) + ".tmp");
            if(tempHead->trueOrFalse == 1) {
                string temp;
                while(getline(conditionFileA,temp)) {
                    if(isLineInFile("finalFile.tmp", temp) == false) {
                        ofstream finalFile("finalFile.tmp", ios::app);
                        finalFile << temp << endl;
                        fullness = true;
                        finalFile.close();
                    }
                }
            }
            conditionFileA.close();

            RemoveConditionByIndex(tempHead,indexCondition);
            countConditions = countConditions - 1;

        }
        if(countConditions == 0) return fullness;
        if(countConditions == 2) continue;
        if(countConditions != 1) tempHead = tempHead->next;
        
    }
    return fullness; 
}

bool FindByCriteria(string expression) {
    bool fullness;
    Condition* condition = SplitExpressionForStruct(expression);
    Condition* newCondition = ReplacingConditionsWithBool(condition);
    if(FilteringForOneFile(newCondition)) {
        fullness = true;
    }
    else {
        fullness = false;
    }
    PrintFinalFile();
    DeleteTmpInDirectory(".");         
    DeleteTmpInDirectory("books"); 
    DeleteTmpInDirectory("shops"); 
    return fullness;
}