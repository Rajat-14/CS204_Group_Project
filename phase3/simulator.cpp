#include "simulator.h"
#include "utils.h" // Include header only

//---------------- Global Variables Definitions (Phase 2) ----------------
map<string, string> instruction_map;
map<string, string> data_map;
string instructionRegister;
string currentPC_str = "0x0";
unsigned int returnAddress = 0;
long long int RM;
long long int aluResult;
long long int RY;
long long int memoryData;
bool is_PCupdated_while_execution = false;
int R[32] = {0};
long long int clockCycle = 0;
int numStallNeeded = 0;
bool pipelineEnd = false;
instruction_register ir;

//---------------- Pipeline Registers (Phase 3) ----------------
IF_ID if_id = {"", "", false};
ID_EX id_ex ={0, 0, 0, 0, 0, 0, 0, 0, 0, "", "", 0, false, false, "", false, false, 0, false, 0};
EX_MEM ex_mem = {0, 0, false, false, false, 0, false, 0};
MEM_WB mem_wb = {0, 0, false, false};

//---------------- Knobs (Default Settings) ----------------
bool enablePipelining = false;
bool enableDataForwarding = false;
bool printRegisterFile = false;
bool printPipelineTrace = false;
int traceInstructionNumber = 0; // 0 means disabled
bool printBranchPredictor = false;

//---------------- Statistics Counters ----------------
unsigned long totalCycles = 0;
unsigned long totalInstructions = 0;
unsigned long aluInstructionCount = 0;
unsigned long dataTransferCount = 0;
unsigned long controlInstructionCount = 0;
unsigned long stallCount = 0;
unsigned long dataHazardCount = 0;
unsigned long controlHazardCount = 0;
unsigned long branchMispredictionCount = 0;

void load_mc_file(const string &filename)
{
    ifstream file(filename);
    if (!file)
    {
        cerr << "Error: Unable to open file " << filename << "\n";
        return;
    }

    string line;
    bool foundTextSegment = true;

    while (getline(file, line))
    {
        if (line.find("Memory:") != string::npos)
        {
            foundTextSegment = false;
            break;
        }
    }

    // Process Data Segment lines (address-value pairs)
    while (getline(file, line))
    {
        if (line.find("Text Segment:") != string::npos)
        {
            foundTextSegment = true;
            break;
        }

        if (line.find("->") != string::npos)
        {
            istringstream iss(line);
            string address, arrow, value;
            if (iss >> address >> arrow >> value && arrow == "->")
            {
                data_map[address] = value;
            }
        }
    }

    // Process the Text Segment lines (PC and machine code)
    if (foundTextSegment)
    {
        while (getline(file, line))
        {
            if (line.empty())
                continue;
            istringstream iss(line);
            string pc, code;

            if (iss >> pc >> code)
            {

                if (code == "Terminate")
                {
                    instruction_map[pc] = code;
                    break;
                }
                instruction_map[pc] = code;
            }
        }
    }

    file.close();
}
