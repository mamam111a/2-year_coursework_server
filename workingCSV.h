#include <iostream>
using namespace std;
using json = nlohmann::json;

json ReadSchema(const string& filepath) {
    ifstream inFile(filepath);
    if (!inFile.is_open()) {
        cout << "Ошибка открытия файла!" << endl;
        return json();
    }

    json jsonData;
    inFile >> jsonData;
    return jsonData;
}
void CreateLastLine(const string& filename) {
    string name = filename.substr(0, 5);
    ofstream outFile(name + "/" + name + "_last_Line.txt");
    outFile << "0";
    outFile.close();
}
void CreateListCSV(const string& filename) {
    string name = filename.substr(0, 5);
    ofstream outFile(name + "/" + name + "_list_CSV.txt");
    outFile << "1";
    outFile.close();
}
bool FileExist(const string& filepath) {
    ifstream inFile(filepath);
    if (!inFile.is_open()) {
        return 0;
    }
    inFile.close();
    return 1;
}
void CreateTableFromJson(const string& filepath) {
    json jsonData = ReadSchema(filepath);
    string tableName = jsonData["name"];
    tableName += "_1";
    string name = jsonData["name"];
    if (FileExist(name + "/" + tableName + ".csv")) return;
    ofstream outFile(name + "/" + tableName + ".csv");
    outFile.close();
    CreateLastLine(tableName);
    CreateListCSV(tableName);
}
