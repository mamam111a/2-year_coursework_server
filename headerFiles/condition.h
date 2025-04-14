
#ifndef CONDITION_H
#define CONDITION_H

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

Condition* SplitExpressionForStruct(string& filter);
bool SelectFromManyTables(vector<string> parameters, const int& tmpFileCount);
bool CheckCondition(vector<string>& parameters, const string &tmpFileName, const int& tmpFileCount, bool& hasCartezian);
Condition* ReplacingConditionsWithBool(Condition* expressions);
bool FilteringForOneFile(Condition* condition);
bool FindByCriteria(string& expression, bool& parameter);

#endif