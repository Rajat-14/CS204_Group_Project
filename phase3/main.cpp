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
    bool wr = false;
    cout << "----------------------------------------------------------------------------------------------------\n";

    while (instruction_map[currentPC_str] != "Terminate" || if_id.isValid || id_ex.valid || ex_mem.valid || mem_wb.valid)
    {
        if (exitLoop)
        {
            break;
        }
        clockCycle++;
        if (stalled || stalledM)
        {
            cout << numStallNeeded << endl;
        }
        write_back();
        if (wr == true)
        {

            wr = 0;
            id_ex = {0, 0, 0, 0, 0, 0, 0, 0, 0, "", "", 0, false, false, "", false, false, 0, false, 0};
            ex_mem = {0, 0, false, false, false, 0, false, 0, "", false, false};
            mem_wb = {0, 0, false, false};
            controlForIAG = 0;
            isFlushingDone = 0;
        }
        mem_wb.valid = false;
        if (stalled)
        {
            if (numStallNeeded == 1)
            {
                mem_wb = {0, 0, false, false};

                numStallNeeded--;
            }
        }

        memory_access();
        if (isFlushingDone)
        {
            stopFetch = false;
            wr = true;
        }

        if (stalled)
        {
            if (numStallNeeded == 2)
            {
                ex_mem = {0, 0, false, false, false, 0, false, 0, "", false, false};
                id_ex.valid = false;
                if_id.isValid = false;
                isFlushingDone = 0;
                numStallNeeded--;
            }
        }
        if (stalledM)
        {
            if (numStallNeeded == 1)
            {
                ex_mem = {0, 0, false, false, false, 0, false, 0, "", false, false};
                id_ex.valid = false;
                if_id.isValid = false;
                numStallNeeded--;
            }
        }
        execute();

        if (numStallNeeded == 0 && stalled)
        {
            id_ex = {0, 0, 0, 0, 0, 0, 0, 0, 0, "", "", 0, false, false, "", false, false, 0, false, 0};
            ex_mem = {0, 0, false, false, false, 0, false, 0, "", false, false};
            mem_wb = {0, 0, false, false};
            stalled = false;
            if_id.isValid = true;
        }
        if (numStallNeeded == 0 && stalledM)
        {
            id_ex = {0, 0, 0, 0, 0, 0, 0, 0, 0, "", "", 0, false, false, "", false, false, 0, false, 0};
            ex_mem = {0, 0, false, false, false, 0, false, 0, "", false, false};

            stalledM = false;
            if_id.isValid = true;
        }
        decode();

        if (!stalled && !stalledM)
        {
            fetch();
        }

        cout << "Clock: " << clockCycle << endl;
        for (auto it : R)
        {
            cout << it << " ";
        }
        cout << endl;
        cout << "----------------------------------------------------------------------------------------------------\n";
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