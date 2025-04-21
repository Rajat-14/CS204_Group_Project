#include "simulator.h"
#include "utils.h" // Include header only

//---------------- Global Variables Definitions (Phase 2) ----------------
map<string, string> instruction_map;
map<string, string> data_map;
string instructionRegister;
string currentPC_str = "0x0";
bool first_ever_fetch = true;
int branch_control = -1;
bool jumped_to_terminate = false;
unsigned int returnAddress = 0;
long long int RM;
long long int aluResult;
long long int RY;
long long int memoryData;
int R[32] = {0};
long long int clockCycle = 0;
int numStallNeeded = 0;
bool pipelineEnd = false;
instruction_register ir;
bool stallingWhileDataForwarding = false;
bool isFlushingDone = false;
bool flush1 = false;
bool flush2 = false;
bool fetching_in_flushing = false;


bool        PHT[BTB_SIZE]         = {false};
unsigned int BTB_target[BTB_SIZE] = {0};
bool        BTB_valid[BTB_SIZE]   = {false};
bool        prediction_used[BTB_SIZE] = {false};



//---------------- Pipeline Registers (Phase 3) ----------------

IF_ID if_id = {"", "", false};
ID_EX id_ex = {0, 0, 0, 0, 0, 0, 0, 0, 0, "", "", 0, false, false, "", false, false, 0, false, 0};
EX_MEM ex_mem = {0, 0, false, false, false, 0, false, 0,""};
MEM_WB mem_wb = {0, 0, false, false,""};

//---------------- Knobs (Default Settings) ----------------
bool enablePipelining = true;
bool enableDataForwarding = true;
bool printRegisterFile = true;
bool printPipelineTrace = false;
bool printSpecificInstruction=true; //knob 5
int traceInstructionNumber = 9; 
bool printBranchPredictor = true;

//---------------- Statistics Counters ----------------
unsigned long totalCycles = 0;
unsigned long totalInstructions = 0;
set<string> aluInstructionCount ;
set<string> dataTransferCount;
set<string> controlInstructionCount;
unsigned long stallCount = 0;
unsigned long dataHazardCount = 0;
unsigned long controlHazardCount = 0;
unsigned long branchMispredictionCount = 0;
unsigned long stallsDueToControlHazard=0;


  bool fetchFlag=false;
  bool decodeFlag=false;
  bool executeFlag=false;
  bool memoryFlag=false;
  bool writebackFlag=false;



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

void print()
{
    cout << "Clock: " << clockCycle << endl;
    if (printRegisterFile)
    {
        for (auto it : R)
        {
            cout << it << " ";
        }
        cout << endl;
    }
    cout << "----------------------------------------------------------------------------------------------------\n";
    logPipelineRegisters();
    printBTB();
}

void logPipelineRegisters()
{
    // if(printSpecificTrace){
    //   checkNthInstruction(traceInstructionNumber);
    // }

    if (printPipelineTrace)
    {
        std::ofstream ofs("pipeline_registers.txt", std::ios::app);
        if (!ofs.is_open())
            return;

        ofs << "Clock Cycle: " << clockCycle << "\n";

        // IF/ID stage
        ofs << "IF/ID Stage:\n";
        ofs << "  instruction: " << if_id.instruction << "\n";
        ofs << "  pc:          " << if_id.pc << "\n";
        ofs << "  isValid:     " << if_id.isValid << "\n\n";

        // ID/EX stage
        ofs << "ID/EX Stage:\n";
        ofs << "  opcode:      " << id_ex.opcode << "\n";
        ofs << "  rd:          " << id_ex.rd << "\n";
        ofs << "  rs1:         " << id_ex.rs1 << "\n";
        ofs << "  rs2:         " << id_ex.rs2 << "\n";
        ofs << "  funct3:      " << id_ex.funct3 << "\n";
        ofs << "  funct7:      " << id_ex.funct7 << "\n";
        ofs << "  imm:         " << id_ex.imm << "\n";
        ofs << "  valid:       " << id_ex.valid << "\n\n";

        // EX/MEM stage
        ofs << "EX/MEM Stage:\n";
        ofs << "  aluResult:   " << ex_mem.aluResult << "\n";
        ofs << "  rd:          " << ex_mem.rd << "\n";
        ofs << "  valid:       " << ex_mem.valid << "\n\n";

        // MEM/WB stage
        ofs << "MEM/WB Stage:\n";
        ofs << "  writeData:   " << mem_wb.writeData << "\n";
        ofs << "  rd:          " << mem_wb.rd << "\n";
        ofs << "  valid:       " << mem_wb.valid << "\n";

        ofs << "-------------------------------------------\n\n";
    }
}




void printBTB()
{
    if (printBranchPredictor)
    {
        for (int i = 0; i < BTB_SIZE; i++)
        {
            if (BTB_target[i] != 0)
            {
                cout << "PC is: " << convert_PC2hex(i * 4) << " Target Address is: " << BTB_target[i];
                if (PHT[i] == 1)
                {
                    cout << " The State of predictor is: TAKEN\n";
                }
                else
                {
                    cout << " The State of predictor is: NOT TAKEN\n";
                }
            }
        }
    }
}
