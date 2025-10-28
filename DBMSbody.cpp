#include <iostream>
#include <functional>
#include <sstream>
#include <fstream>
#include <dirent.h>
#include <vector>
#include <set>
#include "json.hpp"
#include "headerFiles/authorization.h"
#include "headerFiles/workingCSV.h"
#include "headerFiles/crud.h"
#include "headerFiles/condition.h"
#include "headerFiles/condition_additional.h"
#include "headerFiles/DBMSbody.h"
#include <sys/socket.h>
using json = nlohmann::json;
using namespace std;

string toLower(const string& input) {

    locale loc("ru_RU.UTF-8");
    wstring_convert<codecvt_utf8<wchar_t>> conv;
    wstring wstr = conv.from_bytes(input);
    for (wchar_t& c : wstr) {
        c = tolower(c, loc); 
        if (c == L'ё' || c == L'Ё') {
            c = L'е';
        }
    }
    return conv.to_bytes(wstr);
}
bool DBMS_Queries(int& clientSocket, string& command, ostringstream& toClient, string& role, string& username) {
    command = toLower(command);
    vector<string> elements;
    stringstream ss(command);
    string word;

    while (getline(ss, word, '|')) {
        elements.push_back(word);
    }
    ConceptTable tableBooks("books/books.json", username);
    ConceptTable tableShops("shops/shops.json", username);
    if(role == "user" ) {
        int intListShops = elements.size() - 1;
        string strListShops = elements[intListShops];
        elements.pop_back();

        vector<string> andElements;        
        vector<vector<string>> orElements;  
        for (int i = 0; i < elements.size(); i++) {
            if (elements[i].empty()) continue;

            if (elements[i].find(u8"или") != string::npos) {
                string tmp = elements[i];
                int pos = 0;
                while ((pos = tmp.find(u8"или", pos)) != string::npos) {
                    tmp.replace(pos, string(u8"или").size(), "|");
                    pos += 1;
                }

                stringstream ssTmp(tmp);
                string wordOr;
                vector<string> orConds;

                while (getline(ssTmp, wordOr, '|')) {
                    int start = wordOr.find_first_not_of(" \t");
                    int end = wordOr.find_last_not_of(" \t");
                    if (start != string::npos && end != string::npos)
                        wordOr = wordOr.substr(start, end - start + 1);

                    string category;
                    if(i == 0) category = "section";
                    else if(i == 1) category = "author";
                    else if(i == 2) category = "title";
                    else if(i == 3) category = "publisher";
                    else if(i == 4) category = "publishing_year";

                    orConds.push_back("books." + category + " = '" + wordOr + "'");
                }
                orElements.push_back(orConds);
            } else {
                string category;
                if(i == 0) category = "section";
                else if(i == 1) category = "author";
                else if(i == 2) category = "title";
                else if(i == 3) category = "publisher";
                else if(i == 4) category = "publishing_year";

                andElements.push_back("books." + category + " = '" + elements[i] + "'");
            }
        }

        if (!strListShops.empty() && stoi(strListShops) != 0) {
            stringstream sss(strListShops);
            string number;
            vector<string> shopConds;
            while (getline(sss, number, ',')) {
                shopConds.push_back("books.shop_id = '" + number + "'");
            }
            orElements.insert(orElements.begin(), shopConds); 
        }
        vector<string> combined;
        combined.push_back("");

        for (auto& orBlock : orElements) {
            vector<string> newCombined;
            for (auto& prefix : combined) {
                for (auto& cond : orBlock) {
                    string tmp = prefix;
                    if (!tmp.empty()) tmp += " AND ";
                    tmp += cond;
                    newCombined.push_back(tmp);
                }
            }
            combined = newCombined;
        }

        for (auto& andEl : andElements) {
            for (auto& s : combined) {
                if (!s.empty()) s = andEl + " AND " + s;
                else s = andEl;
            }
        }
        string query = "";
        for (auto& s : combined) query += s + " OR ";
        query.erase(query.size() - 4); 
        toClient.str("");
        toClient.clear();
        cout << endl << query << endl;

        if (FindByCriteria(query, username)) {
            ifstream finalFile("finalFile_" + username + ".tmp");
            ostringstream oss;
            oss << finalFile.rdbuf(); 
            finalFile.close();

            string message = oss.str(); 
            toClient << "#";
            toClient << message;    

            return true;
        } else {
            toClient << "!Ничего не найдено :( ))";
            return false;
        }
    }
    else {
        if((elements[0] == "addshops")) {
            elements.erase(elements.begin());
            if(tableShops.InsertLastRow(elements)) toClient << "!Строка успешно добавлена!";
            else toClient << "!Ошибка добавления строки!";
        }
        else if (elements[0] == "addbooks") {
            elements.erase(elements.begin());
            if(tableBooks.InsertLastRow(elements)) toClient << "!Строка успешно добавлена!";
            else toClient << "!Ошибка добавления строки!";
        }
        else if (elements[0] == "deletebooks") { 
            string query;
            elements.erase(elements.begin());
            for(int i = 0; i < elements.size(); i++) {
                if(elements[i].empty()) continue;
                string category;
                if(i==0) category = "shop_id";
                else if(i == 1) category = "section";
                else if(i == 2) category = "author";
                else if(i == 3) category = "title";
                else if(i == 4) category = "publisher";
                else if(i == 5) category = "publishing_year";
                else if(i == 6) category = "quantity";
                else if(i == 7) category = "price";
                else if(i == 8) category = "additional_info";
                query += "books." + category + " = '" + elements[i] + "' AND ";
            }
            query.erase(query.size() - 5);
            cout << endl << query;
            if(tableBooks.DeleteRowByCriteria(query,username)) toClient << "!Успешное удаление!";
            else toClient << "!Ошибка удаления!";
        }
        else if (elements[0] == "deleteshops") { 
            string query;
            elements.erase(elements.begin());
            for(int i = 0; i < elements.size(); i++) {
                if(elements[i].empty()) continue;
                string category;
                if(i==0) category = "name";
                else if(i == 1) category = "address";
                else if(i == 2) category = "working_hours";
                query += "shops." + category + " = '" + elements[i] + "' AND ";
            }
            query.erase(query.size() - 5);
            cout << endl << query;
            if(tableShops.DeleteRowByCriteria(query,username)) toClient << "!Успешное удаление!";
            else toClient << "!Ошибка удаления!";
        }
        //string& criteria, string& nameColumn, string newValue
        else if (elements[0] == "updatebooks") {
            string query, nameColumn, newValue;
            elements.erase(elements.begin());
            newValue = elements.back();
            elements.pop_back();
            nameColumn = elements.back();
            elements.pop_back();
            for(int i = 0; i < elements.size(); i++) {
                if(elements[i].empty()) continue;
                string category;
                if(i==0) category = "shop_id";
                else if(i == 1) category = "section";
                else if(i == 2) category = "author";
                else if(i == 3) category = "title";
                else if(i == 4) category = "publisher";
                else if(i == 5) category = "publishing_year";
                else if(i == 6) category = "quantity";
                else if(i == 7) category = "price";
                else if(i == 8) category = "additional_info";
                query += "books." + category + " = '" + elements[i] + "' AND ";
            }
            query.erase(query.size() - 5);
            cout << endl << query;
            if(tableBooks.Correction(query,nameColumn,newValue, username)) toClient << "!Успешное обновление!";
            else toClient << "!Ошибка обновления!";
        }
        else if (elements[0] == "updateshops") {
            string query, nameColumn, newValue;
            elements.erase(elements.begin());
            newValue = elements.back();
            elements.pop_back();
            nameColumn = elements.back();
            elements.pop_back();

            for(int i = 0; i < elements.size(); i++) {
                if(elements[i].empty()) continue;
                string category;
                if(i==0) category = "name";
                else if(i == 1) category = "address";
                else if(i == 2) category = "working_hours";
                query += "shops." + category + " = '" + elements[i] + "' AND ";
            }
            query.erase(query.size() - 5);
            cout << endl << query;
            if(tableShops.Correction(query,nameColumn,newValue,username)) toClient << "!Успешное обновление!";
            else toClient << "!Ошибка обновления!";
        }
        else if (elements[0] == "findbooks") { 
            string query;
            elements.erase(elements.begin());
            for(int i = 0; i < elements.size(); i++) {
                if(elements[i].empty()) continue;
                string category;
                if(i==0) category = "shop_id";
                else if(i == 1) category = "section";
                else if(i == 2) category = "author";
                else if(i == 3) category = "title";
                else if(i == 4) category = "publisher";
                else if(i == 5) category = "publishing_year";
                else if(i == 6) category = "quantity";
                else if(i == 7) category = "price";
                else if(i == 8) category = "additional_info";
                query += "books." + category + " = '" + elements[i] + "' AND ";
            }
            query.erase(query.size() - 5);
            cout << endl << query;
            if (FindByCriteria(query,username)) {
                ifstream finalFile("finalFile_" + username + ".tmp");
                ostringstream oss;
                oss << finalFile.rdbuf(); 
                finalFile.close();
                string message = oss.str();
                toClient << "#";
                toClient << message;    
    
                return true;
            } else {
                toClient << "!Ничего не найдено :( ))";
                return false;
            }
        }
        else if (elements[0] == "findshops") { 
            string query;
            elements.erase(elements.begin());
            for(int i = 0; i < elements.size(); i++) {
                if(elements[i].empty()) continue;
                string category;
                if(i==0) category = "name";
                else if(i == 1) category = "address";
                else if(i == 2) category = "working_hours";
                query += "shops." + category + " = '" + elements[i] + "' AND ";
            }
            query.erase(query.size() - 5);
            cout << endl << query;

            if (FindByCriteria(query,username)) {
                ifstream finalFile("finalFile_" + username + ".tmp");
                ostringstream oss;
                oss << finalFile.rdbuf(); 
                finalFile.close();
                string message = oss.str();
                toClient << "%";
                toClient << message;    
    
                return true;
            } else {
                toClient << "!Ничего не найдено :( ))";
                return false;
            }
            
        }

    }
    return true;
}


