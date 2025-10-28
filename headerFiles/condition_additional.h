#ifndef CONDITION_ADDITIONAL_H
#define CONDITION_ADDITIONAL_H

#include <iostream>
#include "../json.hpp"
#include "condition.h"
using namespace std;
using json = nlohmann::json;


void DeleteTmpInDirectory(const string& path);
void RemoveConditionByIndex(Condition*& head, int index);
string GetCellByIndex(const string& row, int index);
string CleanString(const string& str);
int GetColumnIndex(json& schema, string& nameTable, string& nameColumn);
Condition* FindConditionOper(Condition* head, const string& oper);
Condition* ConstFindConditionOper(const Condition* head, const string& oper);
int GetSizeCondition(Condition* head);
bool isLineInFile(const string& filename, const string& lineToCheck);

#endif