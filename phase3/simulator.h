#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <iostream>
#include <map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <iomanip>

using namespace std;

//---------------- Global Variables (Phase 2 Globals) ----------------
extern map<string, string> instruction_map;
extern map<string, string> data_map;
extern string instructionRegister;
extern string currentPC_str;
extern unsigned int returnAddress;
extern long long int RM;
extern long long int aluResult;
extern long long int RY;
extern long long int memoryData;
extern bool is_PCupdated_while_execution;
extern int R[32];
extern long long int clockCycle;
extern bool pipelineEnd;
extern bool isFlushingDone;
extern bool stallingWhileDataForwarding;
extern map<string, pair<int, string>> branchPredictor;
extern bool controlForIAG;
extern bool exitLoop;
extern bool stopFetch;
extern bool start;
extern bool stalled;
extern bool stalledM;
//---------------- Instruction Register (Phase 2) ----------------
struct instruction_register
{
    string operation;
    int opcode;
    int rd;
    int funct3;
    int funct7;
    int rs1;
    int rs2;
    long long int imm;
};
extern instruction_register ir;

//---------------- Pipeline Register Structures (Phase 3) ----------------
struct IF_ID
{
    string instruction;
    string pc;
    bool isValid;
};

struct ID_EX
{
    int opcode;
    int rd;
    int rs1;
    int rs2;
    int funct3;
    int funct7;
    long long int imm;
    int operandA;
    int operandB;
    string operation;
    string pc;
    int aluOperation;
    bool useImmediateForB;
    bool valid;
    string signal;
    bool memoryAccess;
    bool memoryRequest;
    int size;
    bool writeBack;
    int controlMuxY;
};

struct EX_MEM
{
    int aluResult;
    int rd;
    bool valid;
    bool memoryAccess;
    bool memoryRequest;
    int size;
    bool writeBack;
    int controlMuxY;
    string end;
    bool isJal;
    bool isJalr;
};

struct MEM_WB
{
    int writeData;
    int rd;
    bool valid;
    bool writeBack;
};

extern IF_ID if_id;
extern ID_EX id_ex;
extern EX_MEM ex_mem;
extern MEM_WB mem_wb;

//---------------- Knobs / Configuration ----------------
extern int numStallNeeded;
extern bool enablePipelining;      // Knob1
extern bool enableDataForwarding;  // Knob2
extern bool printRegisterFile;     // Knob3
extern bool printPipelineTrace;    // Knob4
extern int traceInstructionNumber; // Knob5 (0 means disabled)
extern bool printBranchPredictor;  // Knob6

//---------------- Statistics Counters ----------------
extern unsigned long totalCycles;
extern unsigned long totalInstructions;
extern unsigned long aluInstructionCount;
extern unsigned long dataTransferCount;
extern unsigned long controlInstructionCount;
extern unsigned long stallCount;
extern unsigned long dataHazardCount;
extern unsigned long controlHazardCount;
extern unsigned long branchMispredictionCount;

//---------------- Function Prototypes ----------------
void load_mc_file(const string &filename);

// Execution functions (phase 3)
string IAG();
void fetch();
void decode();
void execute();
void memory_access();
void write_back();

//----------------Pipeline Functions----------------
void DataHazard(); // Detect data hazards and handle them

#endif // SIMULATOR_H
