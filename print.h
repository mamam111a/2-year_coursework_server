#include <iostream>
using namespace std;

void PrintFinalFile() {
    ifstream file("finalFile.tmp");
    string line;
    while (getline(file, line)) {
        int columnCount = count(line.begin(), line.end(), ';') + 1;
        stringstream ss(line);
        string cell;
        if (columnCount == 10) {
            // Книга
            cout << "\n| Жанр        | Автор              | Название          | Издательство     | Год  | Цена | Кол-во | Описание           | Магазин ID |\n";
            cout << "|-------------|--------------------|--------------------|------------------|------|------|--------|---------------------|------------|\n";

            int index = 0;
            while (getline(ss, cell, ';')) {
                if (index == 0) { index++; continue; } 
                cout << "| " << cell << " ";
                index++;
            }
            cout << "|\n";

        } else if (columnCount == 5) {
            cout << "\n| ID  | Название магазина | Адрес                     | Время работы     |\n";
            cout << "|-----|--------------------|---------------------------|------------------|\n";

            int index = 0;
            while (getline(ss, cell, ';')) {
                if (index == 1) { cout << "| " << cell << " "; } // ID
                else if (index > 1) { cout << "| " << cell << " "; }
                index++;
            }
            cout << "|\n";

        } else if (columnCount == 15) {
            cout << "\n| Жанр        | Автор              | Название          | Издательство     | Год  | Цена | Кол-во | Описание           | Магазин ID | Название магазина | Адрес                     | Время работы     |\n";
            cout << "|-------------|--------------------|--------------------|------------------|------|------|--------|---------------------|------------|--------------------|---------------------------|------------------|\n";

            int index = 0;
            while (getline(ss, cell, ';')) {
                if (index == 0) { index++; continue; } 
                cout << "| " << cell << " ";
                index++;
            }
            cout << "|\n";

        } 
    }
    file.close();
}