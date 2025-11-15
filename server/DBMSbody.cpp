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
#include "headerFiles/server_additional.h"
#include <sys/socket.h>
#include <mutex>
#include "headerFiles/filelocks.h"
using json = nlohmann::json;
using namespace std;

void ChangeStoreNumber(string& username) {
    ifstream shopsJsonFile("shops/shops.json");
    json shopsJson;
    shopsJsonFile >> shopsJson;
    int tuplesLimit = shopsJson["tuples_limit"];
    ifstream finalFile("finalFile_" + username + ".tmp");
    ofstream finalFileTemp("finalFile_with_shops_" + username + ".tmp");
    string line;
    while (getline(finalFile, line)) {
        string numberShop = GetCellByIndex(line,1);
        int numberShopFile = ((stoi(numberShop)) / tuplesLimit) + 1;
        lock_guard<recursive_mutex> lock (GetFileMutex("shops/shops_" + to_string(numberShopFile) + ".csv"));
        ifstream shopFile("shops/shops_" + to_string(numberShopFile) + ".csv");
        string shopLine;
        while(getline(shopFile, shopLine)) {
            string id;
            stringstream ss(shopLine);
            getline(ss, id, ';');
            if(id == numberShop) {
                int pos = shopLine.find(';');
                shopLine = shopLine.substr(pos + 1);
                stringstream sss(shopLine);
                string city, street, house, time, name;
                getline(sss, name, ';'); 
                getline(sss, city, ';');   
                getline(sss, street, ';');
                getline(sss, house, ';');
                getline(sss, time, ';');
                int pos1 = line.find(';');
                line = line.substr(pos1 + 1);
                int pos2 = line.find(';');
                line = line.substr(pos2 + 1);
                string address = city + "," + street + "," + house;
                finalFileTemp << name << ";" << address << ";" << time << ";" << line << endl;
            }
        }
        shopFile.close();
    }
    finalFile.close();
    finalFileTemp.close();
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
        if(elements[0] == "findshops") {
            string query;
            elements.erase(elements.begin());
            for(int i = 0; i < elements.size(); i++) {
                if(elements[i].empty()) continue;
                string category;
                if(i==0) category = "name";
                else if(i == 1) category = "city";
                else if(i == 2) category = "street";
                else if(i == 3) category = "house_number";
                else if(i == 4) category = "working_hours";
                query += "shops." + category + " = '" + elements[i] + "' AND ";
            }
            query.erase(query.size() - 5);
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
                toClient << "!Ничего не найдено :(";
                return false;
            }
        }
        else{
            string priceOt, priceDo, quantityOt, quantityDo;

            priceOt = elements[8];
            priceDo= elements[9];
            quantityOt = elements[6];
            quantityDo = elements[7];
            elements.pop_back();
            elements.pop_back();
            elements.pop_back();
            elements.pop_back();
            
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
                ChangeStoreNumber(username);
                ifstream finalFile("finalFile_with_shops_" + username + ".tmp");
                ofstream finalFinalFile("finalFileAfterChecking_" + username + ".tmp");
                string temp;
                bool rowAdded = false;
            
                while(getline(finalFile, temp)) {
                    string quantity = GetCellByIndex(temp, 8);
                    string price = GetCellByIndex(temp, 9);
                    if((stoi(quantity) >= stoi(quantityOt) && stoi(quantity) <= stoi(quantityDo)) && 
                    ( stoi(price)  >= stoi(priceOt) && stoi(price) <= stoi(priceDo))) { 
                        finalFinalFile << temp << endl; rowAdded = true; 
                    }
                }
            
                finalFinalFile.close();
                finalFile.close();
            
                if(!rowAdded) {
                    toClient << "!Ничего не найдено :(";
                    return false;
                }
            
                ifstream finalFinalFileForRead("finalFileAfterChecking_" + username + ".tmp");
                ostringstream oss;
                oss << finalFinalFileForRead.rdbuf(); 
                finalFinalFileForRead.close();
            
                string message = oss.str(); 
                toClient << "#";
                toClient << message;    
            
                return true;
            }
            else{
                toClient << "!Ничего не найдено :(";
                return false;
            }
        }
    }
    else {
        if((elements[0] == "addshops")) {
            elements.erase(elements.begin());
            
            if(tableShops.InsertLastRow(elements)) toClient << "!Строка успешно добавлена!";
            else toClient << "!Ошибка! Строка не была добавлена!";
        }
        else if (elements[0] == "addbooks") {
            elements.erase(elements.begin());
            int maxShop;
            lock_guard<recursive_mutex> lock3(GetFileMutex("shops/shops_last_Line.txt"));
            ifstream lastLineFile("shops/shops_last_Line.txt");
            lastLineFile >> maxShop;
            if(stoi(elements[0]) > maxShop) {
                toClient << "!Не существует магазина под таким номером!";
                return false;
            }

            if(tableBooks.InsertLastRow(elements)) toClient << "!Строка успешно добавлена!";
            else toClient << "!Ошибка! Строка не была добавлена!";
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
            if(tableBooks.DeleteRowByCriteria(query,username)) toClient << "!Успешное удаление!";
            else toClient << "!Ошибка! Строка не была удалена!";
        }
        else if (elements[0] == "deleteshops") { 
            string query;
            elements.erase(elements.begin());
            for(int i = 0; i < elements.size(); i++) {
                if(elements[i].empty()) continue;
                string category;
                if(i==0) category = "name";
                else if(i == 1) category = "city";
                else if(i == 2) category = "street";
                else if(i == 3) category = "house_number";
                else if(i == 4) category = "working_hours";
                query += "shops." + category + " = '" + elements[i] + "' AND ";
            }
            query.erase(query.size() - 5);
            if(tableShops.DeleteRowByCriteria(query,username)) toClient << "!Успешное удаление!";
            else toClient << "!Ошибка! Строка не была удалена!";
        }
        
        else if (elements[0] == "updatebooks") {
            string query, nameColumn, newValue;
            elements.erase(elements.begin());
            newValue = elements.back();
            elements.pop_back();
            nameColumn = elements.back();
            elements.pop_back();
            int maxShop;
            lock_guard<recursive_mutex> lock3(GetFileMutex("shops/shops_last_Line.txt"));
            ifstream lastLineFile("shops/shops_last_Line.txt");
            lastLineFile >> maxShop;
            if(nameColumn == "shop_id") {
                if(stoi(newValue) > maxShop) {
                    toClient << "!Не существует магазина под таким номером!";
                    return false;
                }
            }
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
            if(tableBooks.Correction(query,nameColumn,newValue, username)) toClient << "!Успешное обновление!";
            else toClient << "!Ошибка! Строка не была обновлена!";
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
                else if(i == 1) category = "city";
                else if(i == 2) category = "street";
                else if(i == 3) category = "house_number";
                else if(i == 4) category = "working_hours";
                query += "shops." + category + " = '" + elements[i] + "' AND ";
            }
            query.erase(query.size() - 5);
            if(tableShops.Correction(query,nameColumn,newValue,username)) toClient << "!Успешное обновление!";
            else toClient << "!Ошибка! Строка не была обновлена!";
        }
        else if (elements[0] == "findbooks") {
            elements.erase(elements.begin());
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
                        if(i == 0) category = "shop_id";
                        else if(i == 1) category = "section";
                        else if(i == 2) category = "author";
                        else if(i == 3) category = "title";
                        else if(i == 4) category = "publisher";
                        else if(i == 5) category = "publishing_year";
                        else if(i == 6) category = "quantity";
                        else if(i == 7) category = "price";
                        else if(i == 8) category = "additional_info";
        
                        orConds.push_back("books." + category + " = '" + wordOr + "'");
                    }
                    orElements.push_back(orConds);
                } else {
                    string category;
                    if(i == 0) category = "shop_id";
                    else if(i == 1) category = "section";
                    else if(i == 2) category = "author";
                    else if(i == 3) category = "title";
                    else if(i == 4) category = "publisher";
                    else if(i == 5) category = "publishing_year";
                    else if(i == 6) category = "quantity";
                    else if(i == 7) category = "price";
                    else if(i == 8) category = "additional_info";
        
                    andElements.push_back("books." + category + " = '" + elements[i] + "'");
                }
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
            string query;
            for (auto& s : combined) query += s + " OR ";
            if (!query.empty()) query.erase(query.size() - 4); 
        
            toClient.str("");
            toClient.clear();
            cout << endl << query << endl;
            if (FindByCriteria(query, username)) {
                ChangeStoreNumber(username);
                ifstream finalFile("finalFile_with_shops_" + username + ".tmp");
                ostringstream oss;
                oss << finalFile.rdbuf();
                finalFile.close();
                string message = oss.str();
                toClient << "#";
                toClient << message;
                return true;
            } else {
                toClient << "!Ничего не найдено :(";
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
                else if(i == 1) category = "city";
                else if(i == 2) category = "street";
                else if(i == 3) category = "house_number";
                else if(i == 4) category = "working_hours";
                query += "shops." + category + " = '" + elements[i] + "' AND ";
            }
            query.erase(query.size() - 5);
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
                toClient << "!Ничего не найдено :(";
                return false;
            }
            
        }

    }
    return true;
}


