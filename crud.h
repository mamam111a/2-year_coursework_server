#include <iostream>
using namespace std;
using json = nlohmann::json;

class ConceptTable {
private:
    string filePath;
    json jsonData;
    string name;
    int tupleLimit;
    int LastLine;
    int lastCSV;

public:

    int FindLastCSV() {
        ifstream inFile(name + "/" + name + "_list_CSV.txt");
        int count;
        inFile >> count;
        return count;
    }
    int FindLastLine() {
        ifstream inFile(name + "/" + name + "_last_Line.txt");
        int count;
        inFile >> count;
        return count;
    }
    ConceptTable(const string& path) : filePath(path), jsonData(ReadSchema(path)) {
        name = jsonData["name"];
        LastLine = FindLastLine();
        lastCSV = FindLastCSV();
        tupleLimit = jsonData["tuples_limit"];
    };
   int GetRowID(const string& str) {
        int pos = str.find(';'); 
        string temp = str.substr(0, pos);
        return stoi(temp);    
    }
    bool InsertLastRow(vector<string> newLine) {
        string currFilepath = name + "/" + name + "_" + to_string(lastCSV) + ".csv";
        if (LastLine >= tupleLimit) {
            lastCSV++;
            ofstream listCSV(name + "/" + name + "_list_CSV.txt");
            listCSV << to_string(lastCSV);
            listCSV.close();
    
            currFilepath = name + "/" + name + "_" + to_string(lastCSV) + ".csv";
            LastLine = 0; 
        }
        int newIndex = 1;
        if (LastLine != 0) {
            ifstream inFile(currFilepath);
            string temp, lastRow;
            while (getline(inFile, temp)) {
                lastRow = temp;
            }
            inFile.close();
            newIndex = GetRowID(lastRow) + 1;
        }

        ofstream outFile(currFilepath, ios::app);
        outFile << to_string(newIndex) + ";";
        for (int i = 0; i < newLine.size(); i++) {
            outFile << newLine[i];
            if (i < newLine.size() - 1) {
                outFile << ";";
            }
        }
        outFile << endl;
        outFile.close();
    
        LastLine = newIndex;
    
        ofstream pkFile(name + "/" + name + "_last_Line.txt");
        pkFile << to_string(LastLine);
        pkFile.close();
        return 1;
    }

    bool DeleteLastRow() {
        string currFilepath = name + "/" + name + "_" + to_string(lastCSV) + ".csv";
        if (LastLine == 0) {
            return false; 
        }
        else if (LastLine == 1) {
            filesystem::remove(currFilepath);
            LastLine = tupleLimit;
            lastCSV--;
            ofstream pkFile(name + "/" + name + "_last_Line.txt");
            pkFile << LastLine;
            pkFile.close();

            ofstream listCSV(name + "/" + name + "_list_CSV.txt");
            listCSV << lastCSV;
            listCSV.close();
            return true;
        }
        string currLine;
        string prevLine;
        string tempFilepath = "DeleteLastRow.tmp";
        ifstream inFile(currFilepath);
        ofstream tempFile(tempFilepath);
    
        bool isFirst = true;
        while (getline(inFile, currLine)) {
            if (isFirst==false) {
                tempFile << prevLine << endl;
            }
            prevLine = currLine;
            isFirst = false;
        }
        inFile.close();
        tempFile.close();

        filesystem::remove(currFilepath);
        filesystem::rename(tempFilepath, currFilepath);
    
        LastLine--;
        ofstream pkFile(name + "/" + name + "_last_Line.txt");
        pkFile << LastLine;
        pkFile.close();
        return 1;
    }


};