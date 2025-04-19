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
        if (pipelineEnd && id_ex.signal != "100" && id_ex.signal != "111")
        {
            clockCycle++;
            write_back();
            memory_access();
            execute();
            cout << "Clock: " << clockCycle << endl;
            for (auto it : R)
            {
                cout << it << " ";
            }
            cout << endl;
            cout << "----------------------------------------------------------------------------------------------------\n";
            clockCycle++;
            write_back();
            memory_access();
            cout << "Clock: " << clockCycle << endl;
            for (auto it : R)
            {
                cout << it << " ";
            }
            cout << endl;
            cout << "----------------------------------------------------------------------------------------------------\n";
            clockCycle++;
            write_back();
            cout << "Clock: " << clockCycle << endl;
            for (auto it : R)
            {
                cout << it << " ";
            }
            cout << endl;
            cout << "----------------------------------------------------------------------------------------------------\n";
            break;
        }

        clockCycle++;
        write_back();
        memory_access();
        execute();
        if (isFlushingDone == false && !pipelineEnd)
        {
            decode();
            if (numStallNeeded == 0 && !stallingWhileDataForwarding)
            {
                fetch();
            }
            else if (stallingWhileDataForwarding)
            {
                stallingWhileDataForwarding = false;
            }
        }
        else
        {
            // if last fetched is terminate then we have write_back its prev instruction in control hazard()
            // so last instruction is WB and current PC ="terminate"
            // just break while loop
            if (pipelineEnd && (id_ex.signal == "100" || id_ex.signal == "111"))
            {
                cout << "Clock: " << clockCycle << endl;
                for (auto it : R)
                {
                    cout << it << " ";
                }
                cout << endl;
                cout << "----------------------------------------------------------------------------------------------------\n";
                break;
            }
            isFlushingDone = false;
        }

        if (numStallNeeded != 0)
        {
            cout << "Nummer of stalls needed " << numStallNeeded << endl;
        }

        if (numStallNeeded == 2)
        {
            clockCycle++;
            write_back();
            memory_access();
            cout << "Clock: " << clockCycle << endl;
            for (auto it : R)
            {
                cout << it << " ";
            }
            cout << endl;
            cout << "----------------------------------------------------------------------------------------------------\n";
            clockCycle++;
            write_back();
            decode();
            fetch();
            cout << "Clock: " << clockCycle << endl;
            for (auto it : R)
            {
                cout << it << " ";
            }
            cout << endl;
            cout << "----------------------------------------------------------------------------------------------------\n";
            numStallNeeded = 0;
            if (is_PCupdated_while_execution == false)
            {
                currentPC_str = IAG();
            }
            else
            {
                is_PCupdated_while_execution = false;
            }
            // i want to update only do fetch, decode and execute in next cycle
            ex_mem.valid = false;
            mem_wb.valid = false;
            continue;
        }
        else if (numStallNeeded == 1)
        {
            clockCycle++;
            write_back();
            memory_access();
            decode();
            fetch();
            cout << "Clock: " << clockCycle << endl;
            for (auto it : R)
            {
                cout << it << " ";
            }
            cout << endl;
            cout << "----------------------------------------------------------------------------------------------------\n";
            numStallNeeded = 0;
            if (is_PCupdated_while_execution == false)
            {
                currentPC_str = IAG();
            }
            else
            {
                is_PCupdated_while_execution = false;
            }
            // doing write back for next cycle in this one only and keeping flow same for next cycles
            write_back();
            ex_mem.valid = false;
            mem_wb.valid = false;
            continue;
        }

        if (is_PCupdated_while_execution == false)
        {
            currentPC_str = IAG();
        }
        else
        {
            is_PCupdated_while_execution = false;
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