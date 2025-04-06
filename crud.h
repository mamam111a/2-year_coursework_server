#include <iostream>
#include <string>
#include <functional>
#include <sstream>
#include <fstream>
#include <vector>
#include "json.hpp"
using json = nlohmann::json;
using namespace std;

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
        ifstream inFile("./" + name + "/" + name + "_list_CSV.csv");
        int count;
        inFile >> count;
        return count;
    }
    int FindLastLine() {
        ifstream inFile("./" + name + "/" + name + "_pk_sequence.csv");
        int count;
        inFile >> count;
        return count;
    }
    json ConvertTablesToJson() {
        json combinedJson;
        int fileIndex = 1;
        int globalLineIndex = 1;

        while (true) {
            string filepath = "./" + name + "/" + name + "_" + to_string(fileIndex) + ".csv";
            ifstream inFile(filepath);
            if (!inFile.is_open()) break;

            string headerLine;
            getline(inFile, headerLine);  
            vector<string> headers;
            stringstream headerStream(headerLine);
            string header;

            while (getline(headerStream, header, ';')) {
                headers.push_back(header);  
            }

            string line;
            bool firstLine = true; 

            while (getline(inFile, line)) {
                if (firstLine) {
                    firstLine = false; 
                    continue;
                }

                stringstream lineStream(line);
                string value;
                json lineJson;
                int count = 0;

                while (getline(lineStream, value, ';')) {
                    lineJson[headers[count]] = value;
                    count++;
                }
                combinedJson[to_string(globalLineIndex)] = lineJson;
                globalLineIndex++;
            }

            inFile.close();
            fileIndex++;
        }
       
        return combinedJson;
    }
    ConceptTable(const string& path) : filePath(path), jsonData(ReadSchema(path)) {
        name = jsonData["name"];
        LastLine = FindLastLine();
        lastCSV = FindLastCSV();
        tupleLimit = jsonData["tuples_limit"];
        jsonData = ConvertTablesToJson();
    };

};