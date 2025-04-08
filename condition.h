#include <iostream>
using namespace std;

struct Condition {
    int index;
    bool trueOrFalse;
    string condition;
    string oper;
    Condition* next;
};
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
/*
int CountConditions(Condition* head) {
    int count = 0;
    Condition* current = head;
    while (current != nullptr) {
        count++;
        current = current->next;
    }
    return count;
}*/
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
bool SelectFromOneTable(vector<string> &parameters, const string &tempName, const int& tempNumber) {
    bool isFound = false;
    string nameTable = parameters[0];
    string nameColumn = parameters[1];

    int lastCSV;
    ifstream csvFile(nameTable + "/" + nameTable + "_list_CSV.txt");
    csvFile >> lastCSV;
    csvFile.close();

    json schema = ReadSchema(nameTable + "/" + nameTable + ".json");
    ofstream tmpFile(nameTable + "/" + tempName + "_" + to_string(tempNumber) + ".tmp", ios::app);

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
                    tmpFile << value << endl;
                    isFound = true;
                    break;
                }
                j++;
            }
        }
        inFile.close();
    }
    tmpFile.close(); //НУЖНО УДАЛИТЬ
    return isFound;
}
bool SelectFromManyTables (vector<string> &parameters) {
    bool isFound = false;
    if(parameters.size() == 2) {
        SelectFromOneTable(parameters, "SelectFrom", 1);
        //вывод + УДАЛЕНИЕ ТЕМПА
    }
    else {
        vector<string> parametersA = {parameters[0], parameters[1]};
        vector<string> parametersB = {parameters[2], parameters[3]};
        SelectFromOneTable(parametersA, "SelectFromCartezian", 1);
        SelectFromOneTable(parametersB, "SelectFromCartezian", 2);
        ifstream inFileA(parameters[0] + "/" + "SelectFromCartezian_1.tmp");
        ifstream inFileB(parameters[2] + "/" + "SelectFromCartezian_2.tmp");
        ofstream inFileCartezian("SelectFromCartezian.tmp",ios::app);
        string tempA;
        string tempB;
        
        if(parameters[0] == "books") {
            while (getline(inFileA, tempA)) {
                inFileB.seekg(0); 
                while (getline(inFileB, tempB)) {
                    inFileCartezian << tempA << ";" << tempB << endl;
                }
            }
        }
        else {
            while (getline(inFileB, tempB)) {
                inFileB.seekg(0); 
                while (getline(inFileA, tempA)) {
                    inFileCartezian << tempB << ";" << tempA<< endl;
                }
            }
        }
        //вывод + УДАЛЕНИЕ ТЕМПА
        inFileA.close();
        inFileB.close();
        inFileCartezian.close();

    }
    return isFound;
}
bool CheckCondition(vector<string>& parameters, const string &tmpFileName, const int& tmpFileCount) {
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
        tmpFile.close(); //НУЖНО УДАЛИТЬ
    }
    else {
        string nameTableA = parameters[0];
        string nameColumnA = parameters[1];
        string nameTableB = parameters[2];
        string nameColumnB = parameters[3];

        if( nameTableA == nameTableB ) {
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
                        if ((j == targetColumnA || j == targetColumnB) && (targetA == "nothing" || targetB == "nothing")) {
                            targetA == value;
                        }
                        else if((j == targetColumnA || j == targetColumnB) && (targetA != "nothing" || targetB != "nothing")) {
                            targetB = value;
                            if(targetA == targetB) {
                                tmpFile << temp << endl;
                                isFound = true;
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
            tmpFile.close();//НУЖНО УДАЛИТЬ
        }
        else {
           SelectFromManyTables(parameters);
           ifstream inFile("SelectFromCartezian.tmp");
           ofstream outFile("SelectFromCartezianGETIT" + to_string(tmpFileCount) + ".tmp", ios::app);
           string temp;
           stringstream ss(temp);
           string value;

           while(getline(inFile,temp)) {
            int pos = temp.find(';');
            string partA = temp.substr(0, pos);
            string partB = temp.substr(pos + 1);
                if(partA == partB) {
                    outFile << temp << endl;
                }
           }
           outFile.close();//ВЫВЕСТИ И УДАЛИТЬ
           inFile.close();

           //filesystem::remove("SelectFromCartezianGETIT.tmp");
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
            //int countConditions = CountConditions(head);

            if(CheckCondition(parameters, "CheckConditionOne", i)) expressions->trueOrFalse = true;
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
            //int countConditions = CountConditions(head);

            if(CheckCondition(parameters, "CheckConditionTwo", i)) expressions->trueOrFalse = true;
            else expressions->trueOrFalse = false;
            

        }
        i++;
        expressions = expressions->next;
    }
    return head;
}

Condition* Filtering(Condition* condition) {
    string result;
    Condition* head = condition;
    while(condition != nullptr) {
        if(condition->oper == "AND" && condition->trueOrFalse == false) {
            RemoveConditionByIndex(head, condition->index);
            int tempCond = condition->index + 1;
            RemoveConditionByIndex(head, tempCond);
        }
        else if(condition->oper == "AND" && condition->trueOrFalse == true) {
            if(condition->next->trueOrFalse == false) {
                RemoveConditionByIndex(head, condition->index);
                int tempCond = condition->index + 1;
                RemoveConditionByIndex(head, tempCond);
            }
        }
        condition = condition->next;
    }
    ofstream outFile("finalFile.tmp");
    while(head != nullptr) {
        //далее нужно сделать обработку по приоритету AND и OR
    }

}
