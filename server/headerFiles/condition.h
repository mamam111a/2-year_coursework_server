
#ifndef CONDITION_H
#define CONDITION_H

#include <iostream>
using namespace std;

struct Condition {
    vector<int> targetColumns;
    int index;
    int maxCount;

    bool trueOrFalse;
    string condition;
    string oper;
    Condition* next;
};

Condition* SplitExpressionForStruct(string& filter);
bool CheckCondition(vector<string>& parameters, const string &tmpFileName, const int& tmpFileCount, string &username);
Condition* ReplacingConditionsWithBool(Condition* expressions, string &username);
bool FilteringForOneFile(Condition* condition, const string& username);
bool FindByCriteria(string& expression, string &username);

#endif