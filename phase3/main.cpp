#include "simulator.h"
#include "utils.h" // For conversion functions.
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace std;

void load_mc_file(const string &filename); // Prototype declared in simulator.h

int main()
{
    // Initializing the register file
    R[2] = 2147483612; // Stack pointer
    R[3] = 268435456;  // Frame pointer
    R[10] = 1;
    R[11] = 2147483612;
    load_mc_file("input.mc");

    cout << "Instruction Memory:\n";
    for (map<string, string>::const_iterator it = instruction_map.begin();
         it != instruction_map.end(); ++it)
    {
        cout << "PC: " << it->first << " -> " << it->second << "\n";
    }

    cout << "\nData Memory:\n";
    for (map<string, string>::const_iterator it = data_map.begin(); it != data_map.end(); ++it)
    {
        cout << "Address: " << it->first
             << " -> Value: " << it->second << "\n";
    }

    cout << "----------------------------------------------------------------------------------------------------\n";
    while (instruction_map[currentPC_str] != "Terminate" || if_id.isValid || id_ex.valid || ex_mem.valid || mem_wb.valid)
    {
        if (pipelineEnd)
        {
            write_back();
            memory_access();
            execute();
            clockCycle++;
            write_back();
            memory_access();
            clockCycle++;
            write_back();
            clockCycle++;
            break;
        }
        write_back();
        memory_access();
        execute();
        decode();
        fetch();
        if (numStallNeeded == 2)
        {
            memory_access();
            write_back();
            write_back();
            stallNeeded = false;
            numStallNeeded = 0;
            if (is_PCupdated_while_execution == false)
            {
                currentPC_str = IAG();
            }
            clockCycle += 2;
            continue;
        }
        else if (numStallNeeded == 1)
        {
            write_back();
            stallNeeded = false;
            numStallNeeded = 0;
            if (is_PCupdated_while_execution == false)
            {
                currentPC_str = IAG();
            }
            clockCycle += 1;
            continue;
        }

        clockCycle++;
        if (is_PCupdated_while_execution == false)
        {
            currentPC_str = IAG();
        }
        // cout << "Clock: " << clockCycle << endl;
        // for (auto it : R)
        // {
        //     cout << it << " ";
        // }
        // cout << endl;
        // cout << "----------------------------------------------------------------------------------------------------\n";
    }

    // write data memory in a file named data_memory.txt
    ofstream data_memory_file("data_memory.txt");
    if (!data_memory_file)
    {
        cerr << "Error: Unable to create data_memory.txt" << endl;
        return 1;
    }
    data_memory_file << "Data Memory:\n";
    for (map<string, string>::const_iterator it = data_map.begin();
         it != data_map.end(); ++it)
    {
        data_memory_file << "Address: " << it->first
                         << " -> Value: " << it->second << "\n";
    }

    cout << "Total Stalls are " << stallCount << endl;
    cout << "Clock: " << clockCycle << endl;

    return 0;
}