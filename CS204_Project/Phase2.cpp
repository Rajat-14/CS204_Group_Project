#include <iostream>
#include <map>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

map<string, string> instruction_map;
map<string, string> data_map;

void load_mc_file(const string& filename) {
    ifstream file(filename);
    if (!file) {
        cerr << "Error: Unable to open file " << filename << "\n";
        return;
    }
    
    string line;
    bool foundTextSegment = true;
    
    
    while (getline(file, line)) {
        if (line.find("Memory:") != string::npos) {
            foundTextSegment = false;
            break;
        }
    }
    
    // Process Data Segment lines (address-value pairs)
    while (getline(file, line)) {
        if (line.find("Text Segment:") != string::npos) {
            foundTextSegment = true;
            break;
        }
        
        if (line.find("->") != string::npos) {
            istringstream iss(line);
            string address, arrow, value;
            if (iss >> address >> arrow >> value && arrow == "->") {
                data_map[address] = value;
            }
        }
    }
    
    // Process the Text Segment lines (PC and machine code)
    if (foundTextSegment) {
        while (getline(file, line)) {
            if (line.empty())
                continue;
            istringstream iss(line);
            string pc, code;
           
            if (iss >> pc >> code) {
                
                if (code == "Terminate")
                    break;
                instruction_map[pc] = code;
            }
        }
    }
    
    file.close();
}

int main() {
    load_mc_file("input.mc");
    
    cout << "Instruction Memory:\n";
    for (map<string, string>::const_iterator it = instruction_map.begin();
         it != instruction_map.end(); ++it) {
        cout << "PC: " << it->first << " -> " << it->second << "\n";
    }
    
    cout << "\nData Memory:\n";
    for (map<string, string>::const_iterator it = data_map.begin();
         it != data_map.end(); ++it) {
        cout << "Address: " << it->first 
             << " -> Value: " << it->second << "\n";
    }
    
    return 0;
}
