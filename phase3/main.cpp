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
    int count = 40;
    while ((instruction_map[currentPC_str] != "Terminate"))
    {
        print();
        clockCycle++;
        write_back();
        memory_access();
        execute();
        if (!isFlushingDone)
        {
            decode();
            if (numStallNeeded == 0 && !stallingWhileDataForwarding)
            {
                fetch();
            }
        }
        else
        {
            isFlushingDone = false;
        }

        // we do stalling here
        if (numStallNeeded != 0)
        {
            cout << "Nummer of stalls needed " << numStallNeeded << endl;
        }
        bool is_one_stall = false;

        if (numStallNeeded == 2)
        {
            // cout << "Clock: " << clockCycle << endl;
            // for (auto it : R)
            // {
            //     cout << it << " ";
            // }
            // cout << endl;
            // cout << "----------------------------------------------------------------------------------------------------\n";
            print();
            clockCycle++;
            write_back();
            memory_access();
            print();
            clockCycle++;
            write_back();
            decode(); // decoding same instruction again
            fetch();

            // what if last fetched instruction terminate : that is handled at end of loop
            numStallNeeded = 0;
            // i want to update only do fetch, decode and execute in next cycle
            ex_mem.valid = false;
            mem_wb.valid = false;
        }
        else if (numStallNeeded == 1)
        {
            print();

            clockCycle++;
            write_back();
            memory_access();
            decode(); // decoding same instruction again
            fetch();
            numStallNeeded = 0;
            // doing write back for next cycle in this one only and keeping flow same for next cycles
            ex_mem.valid = false;
            is_one_stall = true;
        }

        if (is_one_stall || stallingWhileDataForwarding)
        {
            write_back();
            mem_wb.valid = false;
            is_one_stall = false;
            stallingWhileDataForwarding = false;
        }

        if (pipelineEnd && !jumped_to_terminate)
        {
            print();
            clockCycle++;
            write_back();
            memory_access();
            execute(); // a jump can be there in this execute

            if (pipelineEnd == false)
                continue;
            print();
            clockCycle++;
            // if jump is there in prev cycle in execute stage then go to while loop
            write_back();
            memory_access();
            print();
            clockCycle++;
            write_back();

            break;
        }
        else if (jumped_to_terminate)
        {
            break;
        }
        isFlushingDone = false;
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
    totalCycles=clockCycle;
    
    cout << "Total Stalls are " << stallCount+stallsDueToControlHazard << endl;
    cout << "Clock: " << clockCycle << endl;










    ofstream stat_file("output_stat.txt");
    if (!stat_file)
    {
        cerr << "Error: Unable to create output_stat.txt" << endl;
        return 1;
    }
    
    stat_file << "Execution Statistics\n";
    stat_file << "----------------------\n";
    stat_file << "Total Clock Cycles       : " << clockCycle << "\n";
    stat_file << "Total Instructions       : " <<totalInstructions << "\n";
    stat_file << "CPI       : " <<(float)clockCycle/(float)(totalInstructions) << "\n";
    stat_file << "Data Transfer Instructions    : " << dataTransferCount.size() << "\n";
    stat_file << "ALU Instructions    : " << aluInstructionCount.size() << "\n";
    stat_file << "Control Instructions    : " << controlInstructionCount.size() << "\n";
    stat_file << "Total Stalls             : " << stallCount + stallsDueToControlHazard << "\n";
    stat_file << "Data Hazards Count   : " << dataHazardCount << "\n";
    stat_file << "Control Hazards Count    : " << controlHazardCount<< "\n";
    stat_file << "Branch Misprediction Count  : " << branchMispredictionCount << "\n";


    stat_file << "Stalls Due to Data Hazards    : " << stallCount << "\n";
    stat_file << "Stalls Due to Control Hazards   : " << stallsDueToControlHazard << "\n";
    
    stat_file.close();





    return 0;
}
