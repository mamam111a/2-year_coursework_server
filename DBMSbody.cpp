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
#include "DBMSbody.h"

using json = nlohmann::json;
using namespace std;
#include <sys/socket.h>

std::string toLower(const std::string& input) {
    // Устанавливаем русскую локаль для правильной работы с кириллицей
    std::locale loc("ru_RU.UTF-8");

    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::wstring wstr = conv.from_bytes(input);

    for (wchar_t& c : wstr) {
        c = std::tolower(c, loc); // теперь учитывается кириллица

        // заменяем Ё/ё на е
        if (c == L'ё' || c == L'Ё') {
            c = L'е';
        }
    }

    return conv.to_bytes(wstr);
}
bool DBMS_Queries(int& clientSocket, string& command, ostringstream& toClient) {
    //section|author|name|publisher|publisher_year
    //  0    // 1   // 2 // 3      // 4           
    string query;
    command = toLower(command);
    vector<string> elements;
    stringstream ss(command);
    string word;
    while (getline(ss, word, '|')) {
        elements.push_back(word);
    }

    int intListShops = elements.size() - 1;
    string strListShops = elements[intListShops]; 
    if(strListShops.empty() == false) {
            if(stoi(strListShops) != 0) {
            stringstream sss(elements[intListShops]);
            string number;
            while (getline(sss, number, ',')) {
                query += "books.shop_id = '" + number + "' OR ";
            }
            query.erase(query.size() - 3);
            query += "AND ";
        }
    }
    elements.pop_back();
    map<int, string> orElements;
    bool isFoundOR = false;
    for (int i = 0; i < elements.size(); i++) {
        if (elements[i].find("или") != string::npos) {
            orElements[i] = elements[i];
            isFoundOR = true;
        }
    }
    int countWords = 0;
    vector<int> toSkip;
    if(isFoundOR) {
        for (auto& x : orElements) {
            int pos = 0;
            while ((pos = x.second.find(u8"или", pos)) != std::string::npos) {
                x.second.replace(pos, std::string(u8"или").size(), "|");
                pos += 1;
            }

            int start = x.second.find_first_not_of(" \t");
            int end = x.second.find_last_not_of(" \t");
            if (start != string::npos && end != string::npos)
                x.second = x.second.substr(start, end - start + 1);
            
            pos = 0;
            while ((pos = x.second.find("  ", pos)) != string::npos) {
                x.second.replace(pos, 2, " ");
            }

            vector<string> words;
            stringstream ss(x.second);
            string word;
            while (getline(ss, word, '|')) {
                int start = word.find_first_not_of(" \t");
                int end = word.find_last_not_of(" \t");
                if (start != string::npos && end != string::npos) 
                    word = word.substr(start, end - start + 1);  
                words.push_back(word);
            }
            for(auto& y : words) {
                query += "books.";
                if (x.first == 0) query += "section = '" + words[countWords] + "' OR ";
                else if (x.first == 1) query += "author = '" + words[countWords] + "' OR ";
                else if (x.first == 2) query += "title = '" + words[countWords] + "' OR ";
                else if (x.first == 3) query += "publisher = '" + words[countWords] + "' OR ";
                else if (x.first == 4) query += "publishing_year = '" + words[countWords] + "' OR ";
                toSkip.push_back(x.first);
                countWords++;
            }
            

        }
        query.erase(query.size() - 3); //удаление лишнего OR
    }
    bool hasAND = false;
    for(int i = 0; i < elements.size(); i++) {
        if(elements[i].empty()) continue;
        bool letsGo = false;
        for(auto& j : toSkip) {
            if(i == j) letsGo = true;
            break;
        }
        if(letsGo) continue;
        string category;
        if(i == 0) category = "section";
        if(i == 1) category = "author";
        if(i == 2) category = "title";
        if(i == 3) category = "publisher";
        if(i == 4) category = "publishing_year";
        if(isFoundOR) query += "AND books." + category + " = '" + elements[i] + "'";
        else query += "books." + category + " = '" + elements[i] + "' AND ";
        hasAND = true;
    }
    if(hasAND) query.erase(query.size() - 4);
    toClient.str("");
    toClient.clear();
    cout << endl << query << endl;
    if (FindByCriteria(query)) {
        ifstream finalFile("finalFile.tmp");
        ostringstream oss;
        oss << finalFile.rdbuf(); 
        finalFile.close();
    
        string message = oss.str(); 
        toClient << message;    
    
        return true;
    } else {
        toClient << "!Ничего не найдено :( ))";
        return false;
    }


}
/*
    vector<string> v1 = {"4", "Фантастика", "Лев Толстой", "Война и хехе", "Издательство А", "2013", "400", "50", "Хорошее состояние"};
    vector<string> v2 = {"9", "Фэнтези", "Александр Пушкин", "Евгений хехе", "Издательство Б", "2005", "400", "20", "Как новая"};
    vector<string> v3 = {"2", "Фэнтези", "Иван Тургенев", "Отцы и дети", "Издательство В", "2021", "500", "20", "С повреждениями"};
    vector<string> v4 = {"6", "Фэнтези", "Фёдор Достоевский", "Преступление и наказание", "Издательство Г", "2012", "100", "10", "Бестселлер"};
    vector<string> v5 = {"3", "Фэнтези", "Лев Толстой", "Война и мир", "Издательство Д", "2003", "300", "40", "Как новая"};
    vector<string> v6 = {"2", "Фэнтези", "Фёдор Достоевский", "Преступление и наказание", "Издательство А", "2018", "500", "30", "С повреждениями"};
    vector<string> v7 = {"8", "Фантастика", "Антон Чехов", "Чеховские рассказы", "Издательство Б", "2002", "500", "30", "Как новая"};
    vector<string> v8 = {"1", "Фэнтези", "мамамия", "Война и мир", "Издательство В", "2002", "100", "50", "Бестселлер"};
    vector<string> v9 = {"1", "Нон-фикшн", "Лев Толстой", "Война и лол", "Издательство В", "2002", "100", "50", "Бестселлер"};
    vector<string> v10 = {"3", "Фэнтези", "Александр Пушкин", "Евгений Онегин", "Издательство А", "2007", "100", "30", "Как новая"};

    tableBooks.InsertLastRow(v1);
    tableBooks.InsertLastRow(v2);
    tableBooks.InsertLastRow(v3);
    tableBooks.InsertLastRow(v4);
    tableBooks.InsertLastRow(v5);
    tableBooks.InsertLastRow(v6);
    tableBooks.InsertLastRow(v7);
    tableBooks.InsertLastRow(v8);
   tableBooks.InsertLastRow(v9);
    tableBooks.InsertLastRow(v10);

    vector<string> v11 = {"1", "Магазин 1", "Москва, ул. Ленина, 1", "9:00 - 21:00"};
    vector<string> v21 = {"2", "Магазин 2", "Санкт-Петербург, пр. Невский, 10", "10:00 - 20:00"};
    vector<string> v31 = {"3", "Магазин 3", "Новосибирск, ул. Кузнецова, 5", "9:30 - 19:30"};
    vector<string> v41 = {"4", "Магазин 4", "Екатеринбург, ул. Революции, 3", "9:00 - 18:00"};
    vector<string> v51 = {"5", "Магазин 5", "Казань, ул. Баки Урманче, 7", "10:00 - 22:00"};
    vector<string> v61 = {"6", "Магазин 6", "Самара, ул. Красноармейская, 12", "9:00 - 20:00"};
    vector<string> v71 = {"7", "Магазин 7", "Ростов-на-Дону, ул. Садовая, 14", "10:00 - 21:00"};
    vector<string> v81 = {"8", "Магазин 8", "Красноярск, ул. Дзержинского, 9", "9:00 - 19:00"};
    vector<string> v91 = {"9", "Магазин 9", "Воронеж, ул. Пушкинская, 15", "10:00 - 18:00"};
    vector<string> v101 = {"10", "Магазин 10", "Томск, ул. Советская, 20", "9:00 - 22:00"};

    /*tableShops.InsertLastRow(v11);
    tableShops.InsertLastRow(v21);
    tableShops.InsertLastRow(v31);
    tableShops.InsertLastRow(v41);
    tableShops.InsertLastRow(v51);
    tableShops.InsertLastRow(v61);
    tableShops.InsertLastRow(v71);
    tableShops.InsertLastRow(v81);
    tableShops.InsertLastRow(v91);
    tableShops.InsertLastRow(v101);

    string tempstrA = "books.author = 'Лев Толстой'";
    string nameColumn = "shop_id";
    string value = "666";
    //tableBooks.Correction(tempstrA, nameColumn, value);
    //string tempstrb = "books.title = 'Евгений хехе'";
    //tableBooks.DeleteRowByCriteria(tempstrA);
    //tableBooks.DeleteRowByCriteria(tempstrb);
    return true;
*/
