#include <iostream>
using json = nlohmann::json;
using namespace std;


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
void CreatePkSequence(const string& filename) {
    string name = filename.substr(0, 5);
    ofstream outFile("./" + name + "/" + filename + "_pk_sequence.csv");
    outFile << "0";
    outFile.close();
}
void CreateListCSV(const string& filename) {
    string name = filename.substr(0, 5);
    ofstream outFile("./" + name + "/" + name + "_list_CSV.csv");
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
    if (FileExist("./" + name + "/" + tableName + ".csv")) return;

    auto structure = jsonData["structure"];
    string columnKey = structure.begin().key();

    vector<string> columns = structure[columnKey];
    ofstream outFile("./" + name + "/" + tableName + ".csv");
    outFile << tableName + "_pk;";
    for (int i = 0; i < columns.size(); i++) {
        outFile << columns[i];
        if (i != columns.size() - 1) outFile << ";";
    }
    outFile << endl;
    outFile.close();

    CreatePkSequence(tableName);
    CreateListCSV(tableName);
}
