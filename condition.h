#include <iostream>
using namespace std;

struct Condition {
    bool trueOrFalse;
    string condition;
    string oper;
    Condition* next;
};

string CleanString(string& input) {
    string output = input;
    output.erase(remove(output.begin(), output.end(), ' '), output.end());
    return output;
}
Condition* SplitExpressionForStruct(string& filter) {
    Condition* firstElement = nullptr;
    Condition* lastElement = nullptr;
    int begin = 0;
    int end;

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

/*bool CheckCondition(vector<string>& parameters) {
    if(parameters.size() == 3) {\


    }
    else {

    }
}*/

Condition* ReplacingConditionsWithBool(Condition* expressions) {
    string condition = expressions->condition;

    while(expressions != nullptr) {
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
            string target = condition.substr(equalPos + 1, condition.length() - (equalPos + 1));

            nameTable = CleanString(nameTable);
            nameColumn = CleanString(nameColumn);
            target = CleanString(target);

            vector<string> parameters = {nameTable, nameColumn, target};
            //

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
            //
        }
    }
    condition = expressions->next->condition;
}

