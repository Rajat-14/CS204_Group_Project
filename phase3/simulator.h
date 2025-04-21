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
#include<set>

using namespace std;

//---------------- Global Variables (Phase 2 Globals) ----------------
extern map<string, string> instruction_map;
extern map<string, string> data_map;
extern string instructionRegister;
extern string currentPC_str;
extern bool first_ever_fetch;
extern int branch_control;
extern bool jumped_to_terminate;
extern unsigned int returnAddress;
extern long long int RM;
extern long long int aluResult;
extern long long int RY;
extern long long int memoryData;
extern int R[32];
extern long long int clockCycle;
extern bool pipelineEnd;
extern bool isFlushingDone;
extern bool stallingWhileDataForwarding;
extern bool flush1;
extern bool flush2;
extern bool fetching_in_flushing;

extern bool fetchFlag;
extern bool decodeFlag;
extern bool executeFlag;
extern bool memoryFlag;
extern bool writebackFlag;



static const int BTB_SIZE = 256;

// the data structures:
extern bool        PHT[BTB_SIZE];
extern unsigned int BTB_target[BTB_SIZE];
extern bool        BTB_valid[BTB_SIZE];
extern bool        prediction_used[BTB_SIZE];


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
    string pc;
};

struct MEM_WB
{
    int writeData;
    int rd;
    bool valid;
    bool writeBack;
    string pc;
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
extern bool printBranchPredictor; 
extern bool printSpecificInstruction;
 // Knob6

//---------------- Statistics Counters ----------------
extern unsigned long totalCycles;
extern unsigned long totalInstructions;
extern set<string> aluInstructionCount;
extern set<string> dataTransferCount;
extern set<string> controlInstructionCount;
extern unsigned long stallCount;
extern unsigned long dataHazardCount;
extern unsigned long controlHazardCount;
extern unsigned long branchMispredictionCount;
extern unsigned long stallsDueToControlHazard;

//---------------- Function Prototypes ----------------
void load_mc_file(const string &filename);

// Execution functions (phase 3)
void IAG();
void fetch();
void decode();
void execute();
void memory_access();
void write_back();

//----------------Pipeline Functions----------------
void DataHazard(); // Detect data hazards and handle them
void print();
void logPipelineRegisters();
void printBTB();
void checkNthInstruction(int n);

#endif // SIMULATOR_H
