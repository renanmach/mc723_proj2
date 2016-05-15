/**
 * @file      mips_isa.cpp
 * @author    Sandro Rigo
 *            Marcus Bartholomeu
 *            Alexandro Baldassin (acasm information)
 *
 *            The ArchC Team
 *            http://www.archc.org/
 *
 *            Computer Systems Laboratory (LSC)
 *            IC-UNICAMP
 *            http://www.lsc.ic.unicamp.br/
 *
 * @version   1.0
 * @date      Mon, 19 Jun 2006 15:50:52 -0300
 * 
 * @brief     The ArchC i8051 functional model.
 * 
 * @attention Copyright (C) 2002-2006 --- The ArchC Team
 *
 */

#include  "mips_isa.H"
#include  "mips_isa_init.cpp"
#include  "mips_bhv_macros.H"


/* ***************************************************************** */
// Renan:

// References
// [1] Slides of the book "Computer Architecture and Design". Chapter 4
// [2] http://www.ece.ucsb.edu/~strukov/ece154aFall2013/viewgraphs/pipelinedMIPS.pdf

// GLOBAL VARIABLES

// Number of cycles 
// STILL WITHOUT STALLS CAUSED BY CACHE MISS
unsigned long long CYCLES_SCALAR = 0; // pipeline 1 stage

    // Scalar
unsigned long long DYNAMIC_CYCLES_SCALAR_5 = 0;
unsigned long long DYNAMIC_CYCLES_SCALAR_7 = 0;

unsigned long long STATIC_CYCLES_SCALAR_5 = 0;
unsigned long long STATIC_CYCLES_SCALAR_7 = 0;

    // Superscalar
//unsigned long long CYCLES_SUPERSCALAR_5 = 0;
//unsigned long long CYCLES_SUPERSCALAR_7 = 0


// Number of instructions
unsigned long long INSTRUCTIONS = 0;

// Trace file (for dineroIV)
ofstream file;

// Result file
ofstream fresults;

// Branch predictiors

    // Number of branches instructions
unsigned long long NBRANCHES_INTR = 0;
unsigned long long NJUMPS = 0;

unsigned long long BRANCHES_TAKEN = 0;
unsigned long long BRANCHES_NOT_TAKEN = 0;
unsigned long long DYNAMIC_HITS = 0;
unsigned long long DYNAMIC_MISSES = 0;
unsigned long long STATIC_HITS = 0;
unsigned long long STATIC_MISSES = 0;

// Control hazards for pipelines of size 5 and 7
unsigned long long DYNAMIC_CONTROL_HAZARDS_5 = 0;
unsigned long long DYNAMIC_CONTROL_HAZARDS_7 = 0;
unsigned long long STATIC_CONTROL_HAZARDS_5 = 0;
unsigned long long STATIC_CONTROL_HAZARDS_7 = 0;

// Data hazards
unsigned long long DATA_HAZARDS_5 = 0;
unsigned long long DATA_HAZARDS_7 = 0;

// Bubbles

// Load-use hazard
#define NBUBBLES_LOAD_USE_5 1 
#define NBUBBLES_LOAD_USE_7 2 

//If a comparison register is a destination of
// preceding ALU instruction, insert 1 buble [1] pag. 88 and 89
#define NBUBBLES_DATA_HAZ_ALU_PREC_BRANCH_5 1 
#define NBUBBLES_DATA_HAZ_ALU_PREC_BRANCH_7 1

// If a comparison register is a destination of
// immediately preceding load instruction [1] pag. 88 and 89
#define NBUBBLES_DATA_HAZ_LOAD_PREC_BRANCH_5 2 
#define NBUBBLES_DATA_HAZ_LOAD_PREC_BRANCH_7 3

// If a comparison register is a destination of
// immediately 2nd preceding load instruction [1] pag. 88 and 89
#define NBUBBLES_DATA_HAZ_LOAD_2_PREC_BRANCH_5 1 
#define NBUBBLES_DATA_HAZ_LOAD_2_PREC_BRANCH_7 2


// if the branch is taken and the prediction is right we will have a bubble
// we are considering the case that the branch target is buffed when instruction is
// fetched. [1] p. 93
//#define NBUBBLES_RIGHT_PREDICT_BTAKEN 0

// Branch occurs at the end of stage EX/MEM
#define NBUBBLES_BRANCH_5 2

// We "lose" 3 instructions IT, IF and ID
#define NBUBBLES_BRANCH_7 3

// Same for all cases
// Jumps are not decoded until ID 
#define NBUBBLES_JUMP 1 

// Flag
#define NOT_USED_REG 999


// Branch Predictors

// States:
// 0 Predict taken strong (if miss, go to state 1, but keep the same prediction)
// 1 Predict taken weak (if miss again, change prediction)
// 2 Predict not weak (if miss, change prediction)
// 3 Predict not taken strong (can miss once)

class DynamicBranchPredictorTwoBits {
private: 
    int currState;
    bool prediction;
    
    // FSM. [1] p. 92
    void twoBitsFSM(bool taken) {
        if(currState == 0) {
            if(!taken) 
                currState=1;
        }
        
        else if(currState == 1) {
            if(taken) currState--;
            else currState++;
        }
        
        else if(currState == 2) {
            if(taken) currState--;
            else currState++;
        }
        
         else {
            if(taken) currState--;
        }
        
        if(currState < 0 || currState > 3)
            fresults << "ERROR DYNAMIC BRANCH PREDICTOR. currState=" << currState << endl;
    }
    
public:
    bool getPrediction() {
        return prediction;
    }
    
    // constructor
    DynamicBranchPredictorTwoBits() {
        prediction=true;
        currState=0;
    }
    
    // updates the prediction
    void updatePredict(bool taken) {
        twoBitsFSM(taken);
        if(currState >= 2) prediction = false;
        else prediction = true;
    }
};


// Always NOT taken
class StaticBranchPredictor {
public:
    bool getPrediction() {
        return false;
    }
};

class Instruction {
public:
    unsigned int rt;
    unsigned int rs;
    unsigned int rd; 
    bool memRead;
    bool branch;
    
    Instruction() {
        memRead = false;
        branch = false;
        rd = NOT_USED_REG;
        rs = NOT_USED_REG;
        rt = NOT_USED_REG;
    }
    
    Instruction(bool memRead1, bool branch1, unsigned int rd1, unsigned int rt1, unsigned int rs1) {
        memRead = memRead1;
        branch = branch1;
        rd = rd1;
        rt = rt1;
        rs = rs1;
    }
};

class PipelineFiveStages {
public:    
    void addInstruction(Instruction inst) {
        mem_wb = ex_mem;
        ex_mem = id_ex;
        id_ex = if_id;
        if_id = inst;
        
        checkDataHazards();
    }

private:   
    Instruction if_id;
    Instruction id_ex;
    Instruction ex_mem;
    Instruction mem_wb;
    
    // we are considering that most of the data hazards are going to 
    // be resolved by forwards.
    // We only need to check load-use hazards and data hazards for
    // branches.
    void checkDataHazards() {        
        // Load use
        if(id_ex.memRead) {
            if((id_ex.rt == if_id.rs) || (id_ex.rt == if_id.rt)) {
                DATA_HAZARDS_5 += NBUBBLES_LOAD_USE_5;
            }
        }
        
        // data hazards for branches
        else if(id_ex.branch) {
            // load is the preceding branch instr
            if(ex_mem.memRead && (ex_mem.rt != NOT_USED_REG)) {
                if((id_ex.rt == ex_mem.rt) || (id_ex.rs == ex_mem.rt)) {
                    DATA_HAZARDS_5 += NBUBBLES_DATA_HAZ_LOAD_PREC_BRANCH_5;
                } 
            }
            
            // load is the 2nd preceding branch instr
            else if(mem_wb.memRead && (mem_wb.rt != NOT_USED_REG)) {
                 if((id_ex.rt == mem_wb.rt) || (id_ex.rs == mem_wb.rt)) {
                     DATA_HAZARDS_5 += NBUBBLES_DATA_HAZ_LOAD_2_PREC_BRANCH_5;
                 }
            }
            
            // comparison register is a destination of preceding ALU instruction
            else if(ex_mem.rd != NOT_USED_REG) {
                if((id_ex.rt == ex_mem.rd) || (id_ex.rs == ex_mem.rd)) {
                    DATA_HAZARDS_5 += NBUBBLES_DATA_HAZ_ALU_PREC_BRANCH_5;
                }
            }
            
            else if(ex_mem.rt != NOT_USED_REG) {
                if((id_ex.rt == ex_mem.rt) || (id_ex.rs == ex_mem.rt)) {
                    DATA_HAZARDS_5 += NBUBBLES_DATA_HAZ_ALU_PREC_BRANCH_5;
                }
            }
        }
    }
};

class PipelineSevenStages {
public:
    void addInstruction(Instruction inst) {
        mem_wb = mem1_mem2;
        mem1_mem2 = ex_mem1;
        ex_mem1 = id_ex;
        id_ex = if2_id;
        if2_id = if1_if2;
        if1_if2 = inst;
        
        checkDataHazards();
    }

private:    
    Instruction if1_if2;
    Instruction if2_id;
    Instruction id_ex;
    Instruction ex_mem1;
    Instruction mem1_mem2;
    Instruction mem_wb;
    
    // we are considering that most of the data hazards are going to 
    // be resolved by forwards.
    // We only need to check load-use hazards and data hazards for
    // branches.
    void checkDataHazards() {        
        // Load use
        if(id_ex.memRead) {
            if((id_ex.rt == if2_id.rs) || (id_ex.rt == if2_id.rt)) {
                DATA_HAZARDS_7 += NBUBBLES_LOAD_USE_7;
            }
        }
        
        // data hazards for branches
        else if(id_ex.branch) {
            // load is the preceding branch instr
            if(ex_mem1.memRead && (ex_mem1.rt != NOT_USED_REG)) {
                if((id_ex.rt == ex_mem1.rt) || (id_ex.rs == ex_mem1.rt)) {
                    DATA_HAZARDS_7 += NBUBBLES_DATA_HAZ_LOAD_PREC_BRANCH_7;
                } 
            }
            
            // load is the 2nd preceding branch instr
            else if(mem1_mem2.memRead && (mem1_mem2.rt != NOT_USED_REG)) {
                 if((id_ex.rt == mem1_mem2.rt) || (id_ex.rs == mem1_mem2.rt)) {
                     DATA_HAZARDS_7 += NBUBBLES_DATA_HAZ_LOAD_2_PREC_BRANCH_7;
                 }
            }
            
            // comparison register is a destination of preceding ALU instruction
            else if(ex_mem1.rd != NOT_USED_REG) {
                if((id_ex.rt == ex_mem1.rd) || (id_ex.rs == ex_mem1.rd)) {
                    DATA_HAZARDS_7 += NBUBBLES_DATA_HAZ_ALU_PREC_BRANCH_7;
                }
            }
            
            else if(ex_mem1.rt != NOT_USED_REG) {
                if((id_ex.rt == ex_mem1.rt) || (id_ex.rs == ex_mem1.rt)) {
                    DATA_HAZARDS_7 += NBUBBLES_DATA_HAZ_ALU_PREC_BRANCH_7;
                }
            }
        }
    }
};

DynamicBranchPredictorTwoBits dynamicBP;
StaticBranchPredictor staticBP;
PipelineFiveStages pl5;
PipelineSevenStages pl7;


/* ***************************************************************** */

//If you want debug information for this model, uncomment next line
//#define DEBUG_MODEL
#include "ac_debug_model.H"

//!User defined macros to reference registers.
#define Ra 31
#define Sp 29

// 'using namespace' statement to allow access to all
// mips-specific datatypes
using namespace mips_parms;

static int processors_started = 0;
#define DEFAULT_STACK_SIZE (256*1024)

//!Generic instruction behavior method.
void ac_behavior( instruction )
{ 
  dbg_printf("----- PC=%#x ----- %lld\n", (int) ac_pc, ac_instr_counter);
  //  dbg_printf("----- PC=%#x NPC=%#x ----- %lld\n", (int) ac_pc, (int)npc, ac_instr_counter);
#ifndef NO_NEED_PC_UPDATE
  ac_pc = npc;
  npc = ac_pc + 4;
#endif 

/* ***************************************************************** */
 // renan:
 file << 2 << " " << std::hex <<((int) ac_pc) << "\n";
 INSTRUCTIONS++;
/* ***************************************************************** */
};
 
//! Instruction Format behavior methods.
void ac_behavior( Type_R ){}
void ac_behavior( Type_I ){}
void ac_behavior( Type_J ){}

//!Behavior called before starting simulation
void ac_behavior(begin)
{
  // renan: abre o arquivo de traces
  file.open("/home/renan/semestre_13/mc723/mc723_proj2/mips-1.0.2/traces/trace.txt");
  fresults.open("/home/renan/semestre_13/mc723/mc723_proj2/mips-1.0.2/results/result.txt");
  
  dbg_printf("@@@ begin behavior @@@\n");
  RB[0] = 0;
  npc = ac_pc + 4;

  // Is is not required by the architecture, but makes debug really easier
  for (int regNum = 0; regNum < 32; regNum ++)
    RB[regNum] = 0;
  hi = 0;
  lo = 0;

  RB[29] =  AC_RAM_END - 1024 - processors_started++ * DEFAULT_STACK_SIZE;
}

//!Behavior called after finishing simulation
void ac_behavior(end)
{
  dbg_printf("@@@ end behavior @@@\n");
  
  /* ******************************************************* */
  // RENAN:
  
  NBRANCHES_INTR = BRANCHES_NOT_TAKEN + BRANCHES_TAKEN;
  
  // calculates jumps control hazards
  DYNAMIC_CONTROL_HAZARDS_5 += (NJUMPS*NBUBBLES_JUMP);
  DYNAMIC_CONTROL_HAZARDS_7 += (NJUMPS*NBUBBLES_JUMP);
  
  DYNAMIC_CONTROL_HAZARDS_5 += (DYNAMIC_MISSES*NBUBBLES_BRANCH_5);
  DYNAMIC_CONTROL_HAZARDS_7 += (DYNAMIC_MISSES*NBUBBLES_BRANCH_7);
  
  STATIC_CONTROL_HAZARDS_5 += (NJUMPS*NBUBBLES_JUMP);
  STATIC_CONTROL_HAZARDS_7 += (NJUMPS*NBUBBLES_JUMP);
  
  STATIC_CONTROL_HAZARDS_5 += (STATIC_MISSES*NBUBBLES_BRANCH_5);
  STATIC_CONTROL_HAZARDS_7 += (STATIC_MISSES*NBUBBLES_BRANCH_7);
  
  CYCLES_SCALAR += INSTRUCTIONS;
  
  //adds the stalls caused by jump instr
  CYCLES_SCALAR += (NJUMPS*NBUBBLES_JUMP);

  DYNAMIC_CYCLES_SCALAR_5 += CYCLES_SCALAR + DATA_HAZARDS_5 + DYNAMIC_CONTROL_HAZARDS_5;
  STATIC_CYCLES_SCALAR_5 += CYCLES_SCALAR + DATA_HAZARDS_5 + STATIC_CONTROL_HAZARDS_5;
  
  DYNAMIC_CYCLES_SCALAR_7 += CYCLES_SCALAR + DATA_HAZARDS_7 + DYNAMIC_CONTROL_HAZARDS_7;
  STATIC_CYCLES_SCALAR_7 += CYCLES_SCALAR + DATA_HAZARDS_7 + STATIC_CONTROL_HAZARDS_7;

  fresults << "INSTRUCTIONS," << INSTRUCTIONS << endl;
  
  fresults << "NBRANCHES_INTR," << NBRANCHES_INTR << endl;
  fresults << "NJUMPS," << NJUMPS << endl;
  fresults << "BRANCHES_TAKEN," << BRANCHES_TAKEN << endl;
  fresults << "BRANCHES_NOT_TAKEN," << BRANCHES_NOT_TAKEN << endl;
  
  fresults << "DYNAMIC_HITS," << DYNAMIC_HITS << endl;
  fresults << "DYNAMIC_MISSES," << DYNAMIC_MISSES << endl;
  fresults << "STATIC_HITS," << STATIC_HITS << endl;
  fresults << "STATIC_MISSES," << STATIC_MISSES << endl;
  
  fresults << "STATIC_RATIO (MISSES/TOTAL)," << (double) STATIC_MISSES/NBRANCHES_INTR << endl;
  fresults << "DYNAMIC_RATIO (MISSES/TOTAL)," << (double) DYNAMIC_MISSES/NBRANCHES_INTR << endl;
  
  fresults << "STATIC_CONTROL_HAZARDS_5," << STATIC_CONTROL_HAZARDS_5 << endl;
  fresults << "DYNAMIC_CONTROL_HAZARDS_5," << DYNAMIC_CONTROL_HAZARDS_5 << endl;
  fresults << "DATA_HAZARDS_5," << DATA_HAZARDS_5 << endl;
  
  fresults << "DYNAMIC_CONTROL_HAZARDS_7," << DYNAMIC_CONTROL_HAZARDS_7 << endl;
  fresults << "STATIC_CONTROL_HAZARDS_7," << STATIC_CONTROL_HAZARDS_7 << endl;
  fresults << "DATA_HAZARDS_7," << DATA_HAZARDS_7 << endl;
  
  // Number of cycles **WITHOUT CACHE MISS STALLS**
  fresults << "CYCLES_SCALAR(without cache miss)," << CYCLES_SCALAR << endl;
  
  fresults << "STATIC_CYCLES_SCALAR_5(without cache miss)," << STATIC_CYCLES_SCALAR_5 << endl;
  fresults << "STATIC_CYCLES_SCALAR_7(without cache miss)," << STATIC_CYCLES_SCALAR_7 << endl;
  
  fresults << "DYNAMIC_CYCLES_SCALAR_5(without cache miss)," << DYNAMIC_CYCLES_SCALAR_5 << endl;
  fresults << "DYNAMIC_CYCLES_SCALAR_7(without cache miss)," << DYNAMIC_CYCLES_SCALAR_7 << endl;
  
  // renan: close traces and results files
  file.close();
  fresults.close();
  /* ******************************************************* */
}


//!Instruction lb behavior method.
void ac_behavior( lb )
{
  char byte;
  dbg_printf("lb r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
  byte = DM.read_byte(RB[rs]+ imm);
  RB[rt] = (ac_Sword)byte ;
  dbg_printf("Result = %#x\n", RB[rt]);
  
  // renan:
  file << 0 << " " << std::hex <<((RB[rs]+ imm)) << "\n";
  
  pl5.addInstruction(Instruction(true,false,NOT_USED_REG,rt,rs));
  pl7.addInstruction(Instruction(true,false,NOT_USED_REG,rt,rs));
};

//!Instruction lbu behavior method.
void ac_behavior( lbu )
{
  unsigned char byte;
  dbg_printf("lbu r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
  byte = DM.read_byte(RB[rs]+ imm);
  RB[rt] = byte ;
  dbg_printf("Result = %#x\n", RB[rt]);
  
    // renan:
    file << 0 << " " << std::hex <<((RB[rs]+ imm)) << "\n";
    
    pl5.addInstruction(Instruction(true,false,NOT_USED_REG,rt,rs));
    pl7.addInstruction(Instruction(true,false,NOT_USED_REG,rt,rs));
};

//!Instruction lh behavior method.
void ac_behavior( lh )
{
  short int half;
  dbg_printf("lh r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
  half = DM.read_half(RB[rs]+ imm);
  RB[rt] = (ac_Sword)half ;
  dbg_printf("Result = %#x\n", RB[rt]);
  
  pl5.addInstruction(Instruction(true,false,NOT_USED_REG,rt,rs));
  pl7.addInstruction(Instruction(true,false,NOT_USED_REG,rt,rs));
};

//!Instruction lhu behavior method.
void ac_behavior( lhu )
{
  unsigned short int  half;
  half = DM.read_half(RB[rs]+ imm);
  RB[rt] = half ;
  dbg_printf("Result = %#x\n", RB[rt]);
  
  // renan:
  file << 0 << " " << std::hex <<((RB[rs]+ imm)) << "\n";
  
  pl5.addInstruction(Instruction(true,false,NOT_USED_REG,rt,rs));
  pl7.addInstruction(Instruction(true,false,NOT_USED_REG,rt,rs));
};

//!Instruction lw behavior method.
void ac_behavior( lw )
{
  dbg_printf("lw r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
  RB[rt] = DM.read(RB[rs]+ imm);
  dbg_printf("Result = %#x\n", RB[rt]);
  
  // renan:
  file << 0 << " " << std::hex <<((RB[rs]+ imm)) << "\n";
  
  pl5.addInstruction(Instruction(true,false,NOT_USED_REG,rt,rs));
  pl7.addInstruction(Instruction(true,false,NOT_USED_REG,rt,rs));
};

//!Instruction lwl behavior method.
void ac_behavior( lwl )
{
  dbg_printf("lwl r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
  unsigned int addr, offset;
  ac_Uword data;

  addr = RB[rs] + imm;
  offset = (addr & 0x3) * 8;
  data = DM.read(addr & 0xFFFFFFFC);
  data <<= offset;
  data |= RB[rt] & ((1<<offset)-1);
  RB[rt] = data;
  dbg_printf("Result = %#x\n", RB[rt]);
  
  // renan:
  file << 0 << " " << std::hex <<((RB[rs]+ imm)) << "\n";
  
  pl5.addInstruction(Instruction(true,false,NOT_USED_REG,rt,rs));
  pl7.addInstruction(Instruction(true,false,NOT_USED_REG,rt,rs));
};

//!Instruction lwr behavior method.
void ac_behavior( lwr )
{
  dbg_printf("lwr r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
  unsigned int addr, offset;
  ac_Uword data;

  addr = RB[rs] + imm;
  offset = (3 - (addr & 0x3)) * 8;
  data = DM.read(addr & 0xFFFFFFFC);
  data >>= offset;
  data |= RB[rt] & (0xFFFFFFFF << (32-offset));
  RB[rt] = data;
  dbg_printf("Result = %#x\n", RB[rt]);
  
    file << 0 << " " << std::hex <<((RB[rs]+ imm)) << "\n";
    
    pl5.addInstruction(Instruction(true,false,NOT_USED_REG,rt,rs));
    pl7.addInstruction(Instruction(true,false,NOT_USED_REG,rt,rs));
};

//!Instruction sb behavior method.
void ac_behavior( sb )
{
  unsigned char byte;
  dbg_printf("sb r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
  byte = RB[rt] & 0xFF;
  DM.write_byte(RB[rs] + imm, byte);
  dbg_printf("Result = %#x\n", (int) byte);
  
    // renan:
    file << 1 << " " << std::hex <<((RB[rs]+ imm)) << "\n";
    
    pl5.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
    pl7.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
};

//!Instruction sh behavior method.
void ac_behavior( sh )
{
  unsigned short int half;
  dbg_printf("sh r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
  half = RB[rt] & 0xFFFF;
  DM.write_half(RB[rs] + imm, half);
  dbg_printf("Result = %#x\n", (int) half);
  
  // renan:
  file << 1 << " " << std::hex <<((RB[rs]+ imm)) << "\n";
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
};

//!Instruction sw behavior method.
void ac_behavior( sw )
{
  dbg_printf("sw r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
  DM.write(RB[rs] + imm, RB[rt]);
  dbg_printf("Result = %#x\n", RB[rt]);
  
  
  file << 1 << " " << std::hex <<((RB[rs]+ imm)) << "\n";
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
};

//!Instruction swl behavior method.
void ac_behavior( swl )
{
  dbg_printf("swl r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
  unsigned int addr, offset;
  ac_Uword data;

  addr = RB[rs] + imm;
  offset = (addr & 0x3) * 8;
  data = RB[rt];
  data >>= offset;
  data |= DM.read(addr & 0xFFFFFFFC) & (0xFFFFFFFF << (32-offset));
  DM.write(addr & 0xFFFFFFFC, data);
  dbg_printf("Result = %#x\n", data);
   
    // renan:
    file << 1 << " " << std::hex <<((RB[rs]+ imm)) << "\n";
    
    pl5.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
    pl7.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
};

//!Instruction swr behavior method.
void ac_behavior( swr )
{
  dbg_printf("swr r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
  unsigned int addr, offset;
  ac_Uword data;

  addr = RB[rs] + imm;
  offset = (3 - (addr & 0x3)) * 8;
  data = RB[rt];
  data <<= offset;
  data |= DM.read(addr & 0xFFFFFFFC) & ((1<<offset)-1);
  DM.write(addr & 0xFFFFFFFC, data);
  dbg_printf("Result = %#x\n", data);
  
    // renan:
    file << 1 << " " << std::hex <<((RB[rs]+ imm)) << "\n";
    
    pl5.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
    pl7.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
};

//!Instruction addi behavior method.
void ac_behavior( addi )
{
  dbg_printf("addi r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
  RB[rt] = RB[rs] + imm;
  dbg_printf("Result = %#x\n", RB[rt]);
  //Test overflow
  if ( ((RB[rs] & 0x80000000) == (imm & 0x80000000)) &&
       ((imm & 0x80000000) != (RB[rt] & 0x80000000)) ) {
    fprintf(stderr, "EXCEPTION(addi): integer overflow.\n"); exit(EXIT_FAILURE);
  }
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,rt,rs));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,rt,rs));
};

//!Instruction addiu behavior method.
void ac_behavior( addiu )
{
  dbg_printf("addiu r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
  RB[rt] = RB[rs] + imm;
  dbg_printf("Result = %#x\n", RB[rt]);
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,rt,rs));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,rt,rs));
};

//!Instruction slti behavior method.
void ac_behavior( slti )
{
  dbg_printf("slti r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
  // Set the RD if RS< IMM
  if( (ac_Sword) RB[rs] < (ac_Sword) imm )
    RB[rt] = 1;
  // Else reset RD
  else
    RB[rt] = 0;
  dbg_printf("Result = %#x\n", RB[rt]);
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,rt,rs));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,rt,rs));
};

//!Instruction sltiu behavior method.
void ac_behavior( sltiu )
{
  dbg_printf("sltiu r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
  // Set the RD if RS< IMM
  if( (ac_Uword) RB[rs] < (ac_Uword) imm )
    RB[rt] = 1;
  // Else reset RD
  else
    RB[rt] = 0;
  dbg_printf("Result = %#x\n", RB[rt]);
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,rt,rs));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,rt,rs));
};

//!Instruction andi behavior method.
void ac_behavior( andi )
{	
  dbg_printf("andi r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
  RB[rt] = RB[rs] & (imm & 0xFFFF) ;
  dbg_printf("Result = %#x\n", RB[rt]);
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,rt,rs));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,rt,rs));
};

//!Instruction ori behavior method.
void ac_behavior( ori )
{	
  dbg_printf("ori r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
  RB[rt] = RB[rs] | (imm & 0xFFFF) ;
  dbg_printf("Result = %#x\n", RB[rt]);
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,rt,rs));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,rt,rs));
};

//!Instruction xori behavior method.
void ac_behavior( xori )
{	
  dbg_printf("xori r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
  RB[rt] = RB[rs] ^ (imm & 0xFFFF) ;
  dbg_printf("Result = %#x\n", RB[rt]);
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,rt,rs));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,rt,rs));
};

//!Instruction lui behavior method.
void ac_behavior( lui )
{	
  dbg_printf("lui r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
  // Load a constant in the upper 16 bits of a register
  // To achieve the desired behaviour, the constant was shifted 16 bits left
  // and moved to the target register ( rt )
  RB[rt] = imm << 16;
  dbg_printf("Result = %#x\n", RB[rt]);
  
  pl5.addInstruction(Instruction(true,false,NOT_USED_REG,rt,rs));
  pl7.addInstruction(Instruction(true,false,NOT_USED_REG,rt,rs));
};

//!Instruction add behavior method.
void ac_behavior( add )
{
  dbg_printf("add r%d, r%d, r%d\n", rd, rs, rt);
  RB[rd] = RB[rs] + RB[rt];
  dbg_printf("Result = %#x\n", RB[rd]);
  //Test overflow
  if ( ((RB[rs] & 0x80000000) == (RB[rd] & 0x80000000)) &&
       ((RB[rd] & 0x80000000) != (RB[rt] & 0x80000000)) ) {
    fprintf(stderr, "EXCEPTION(add): integer overflow.\n"); exit(EXIT_FAILURE);
  }
    pl5.addInstruction(Instruction(false,false,rd,rt,rs));
    pl7.addInstruction(Instruction(false,false,rd,rt,rs));
};

//!Instruction addu behavior method.
void ac_behavior( addu )
{
  dbg_printf("addu r%d, r%d, r%d\n", rd, rs, rt);
  RB[rd] = RB[rs] + RB[rt];
  //cout << "  RS: " << (unsigned int)RB[rs] << " RT: " << (unsigned int)RB[rt] << endl;
  //cout << "  Result =  " <<  (unsigned int)RB[rd] <<endl;
  dbg_printf("Result = %#x\n", RB[rd]);
  
  pl5.addInstruction(Instruction(false,false,rd,rt,rs));
  pl7.addInstruction(Instruction(false,false,rd,rt,rs));
};

//!Instruction sub behavior method.
void ac_behavior( sub )
{
  dbg_printf("sub r%d, r%d, r%d\n", rd, rs, rt);
  RB[rd] = RB[rs] - RB[rt];
  dbg_printf("Result = %#x\n", RB[rd]);
  //TODO: test integer overflow exception for sub
  
  pl5.addInstruction(Instruction(false,false,rd,rt,rs));
  pl7.addInstruction(Instruction(false,false,rd,rt,rs));
};

//!Instruction subu behavior method.
void ac_behavior( subu )
{
  dbg_printf("subu r%d, r%d, r%d\n", rd, rs, rt);
  RB[rd] = RB[rs] - RB[rt];
  dbg_printf("Result = %#x\n", RB[rd]);
  
  pl5.addInstruction(Instruction(false,false,rd,rt,rs));
  pl7.addInstruction(Instruction(false,false,rd,rt,rs));
};

//!Instruction slt behavior method.
void ac_behavior( slt )
{	
  dbg_printf("slt r%d, r%d, r%d\n", rd, rs, rt);
  // Set the RD if RS< RT
  if( (ac_Sword) RB[rs] < (ac_Sword) RB[rt] )
    RB[rd] = 1;
  // Else reset RD
  else
    RB[rd] = 0;
  dbg_printf("Result = %#x\n", RB[rd]);
  
  pl5.addInstruction(Instruction(false,false,rd,rt,rs));
  pl7.addInstruction(Instruction(false,false,rd,rt,rs));
};

//!Instruction sltu behavior method.
void ac_behavior( sltu )
{
  dbg_printf("sltu r%d, r%d, r%d\n", rd, rs, rt);
  // Set the RD if RS < RT
  if( RB[rs] < RB[rt] )
    RB[rd] = 1;
  // Else reset RD
  else
    RB[rd] = 0;
  dbg_printf("Result = %#x\n", RB[rd]);
  
  pl5.addInstruction(Instruction(false,false,rd,rt,rs));
  pl7.addInstruction(Instruction(false,false,rd,rt,rs));
};

//!Instruction instr_and behavior method.
void ac_behavior( instr_and )
{
  dbg_printf("instr_and r%d, r%d, r%d\n", rd, rs, rt);
  RB[rd] = RB[rs] & RB[rt];
  dbg_printf("Result = %#x\n", RB[rd]);
  
  pl5.addInstruction(Instruction(false,false,rd,rt,rs));
  pl7.addInstruction(Instruction(false,false,rd,rt,rs));
};

//!Instruction instr_or behavior method.
void ac_behavior( instr_or )
{
  dbg_printf("instr_or r%d, r%d, r%d\n", rd, rs, rt);
  RB[rd] = RB[rs] | RB[rt];
  dbg_printf("Result = %#x\n", RB[rd]);
  
  pl5.addInstruction(Instruction(false,false,rd,rt,rs));
  pl7.addInstruction(Instruction(false,false,rd,rt,rs));
};

//!Instruction instr_xor behavior method.
void ac_behavior( instr_xor )
{
  dbg_printf("instr_xor r%d, r%d, r%d\n", rd, rs, rt);
  RB[rd] = RB[rs] ^ RB[rt];
  dbg_printf("Result = %#x\n", RB[rd]);
  
  pl5.addInstruction(Instruction(false,false,rd,rt,rs));
  pl7.addInstruction(Instruction(false,false,rd,rt,rs));
};

//!Instruction instr_nor behavior method.
void ac_behavior( instr_nor )
{
  dbg_printf("nor r%d, r%d, r%d\n", rd, rs, rt);
  RB[rd] = ~(RB[rs] | RB[rt]);
  dbg_printf("Result = %#x\n", RB[rd]);
  
  pl5.addInstruction(Instruction(false,false,rd,rt,rs));
  pl7.addInstruction(Instruction(false,false,rd,rt,rs));
};

//!Instruction nop behavior method.
void ac_behavior( nop )
{  
  dbg_printf("nop\n");
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
};

//!Instruction sll behavior method.
void ac_behavior( sll )
{  
  dbg_printf("sll r%d, r%d, %d\n", rd, rs, shamt);
  RB[rd] = RB[rt] << shamt;
  dbg_printf("Result = %#x\n", RB[rd]);
  
  pl5.addInstruction(Instruction(false,false,rd,rt,rs));
  pl7.addInstruction(Instruction(false,false,rd,rt,rs));
};

//!Instruction srl behavior method.
void ac_behavior( srl )
{
  dbg_printf("srl r%d, r%d, %d\n", rd, rs, shamt);
  RB[rd] = RB[rt] >> shamt;
  dbg_printf("Result = %#x\n", RB[rd]);
  
  pl5.addInstruction(Instruction(false,false,rd,rt,rs));
  pl7.addInstruction(Instruction(false,false,rd,rt,rs));
};

//!Instruction sra behavior method.
void ac_behavior( sra )
{
  dbg_printf("sra r%d, r%d, %d\n", rd, rs, shamt);
  RB[rd] = (ac_Sword) RB[rt] >> shamt;
  dbg_printf("Result = %#x\n", RB[rd]);
  
  pl5.addInstruction(Instruction(false,false,rd,rt,rs));
  pl7.addInstruction(Instruction(false,false,rd,rt,rs));
};

//!Instruction sllv behavior method.
void ac_behavior( sllv )
{
  dbg_printf("sllv r%d, r%d, r%d\n", rd, rt, rs);
  RB[rd] = RB[rt] << (RB[rs] & 0x1F);
  dbg_printf("Result = %#x\n", RB[rd]);
  
  pl5.addInstruction(Instruction(false,false,rd,rt,rs));
  pl7.addInstruction(Instruction(false,false,rd,rt,rs));
};

//!Instruction srlv behavior method.
void ac_behavior( srlv )
{
  dbg_printf("srlv r%d, r%d, r%d\n", rd, rt, rs);
  RB[rd] = RB[rt] >> (RB[rs] & 0x1F);
  dbg_printf("Result = %#x\n", RB[rd]);
  
  pl5.addInstruction(Instruction(false,false,rd,rt,rs));
  pl7.addInstruction(Instruction(false,false,rd,rt,rs));
};

//!Instruction srav behavior method.
void ac_behavior( srav )
{
  dbg_printf("srav r%d, r%d, r%d\n", rd, rt, rs);
  RB[rd] = (ac_Sword) RB[rt] >> (RB[rs] & 0x1F);
  dbg_printf("Result = %#x\n", RB[rd]);
  
  pl5.addInstruction(Instruction(false,false,rd,rt,rs));
  pl7.addInstruction(Instruction(false,false,rd,rt,rs));
};

//!Instruction mult behavior method.
void ac_behavior( mult )
{
  dbg_printf("mult r%d, r%d\n", rs, rt);

  long long result;
  int half_result;

  result = (ac_Sword) RB[rs];
  result *= (ac_Sword) RB[rt];

  half_result = (result & 0xFFFFFFFF);
  // Register LO receives 32 less significant bits
  lo = half_result;

  half_result = ((result >> 32) & 0xFFFFFFFF);
  // Register HI receives 32 most significant bits
  hi = half_result ;

  dbg_printf("Result = %#llx\n", result);
  
  pl5.addInstruction(Instruction(false,false,hi,lo,rs));
  pl7.addInstruction(Instruction(false,false,hi,lo,rs));
};

//!Instruction multu behavior method.
void ac_behavior( multu )
{
  dbg_printf("multu r%d, r%d\n", rs, rt);

  unsigned long long result;
  unsigned int half_result;

  result  = RB[rs];
  result *= RB[rt];

  half_result = (result & 0xFFFFFFFF);
  // Register LO receives 32 less significant bits
  lo = half_result;

  half_result = ((result>>32) & 0xFFFFFFFF);
  // Register HI receives 32 most significant bits
  hi = half_result ;

  dbg_printf("Result = %#llx\n", result);
  
  pl5.addInstruction(Instruction(false,false,hi,lo,NOT_USED_REG));
  pl7.addInstruction(Instruction(false,false,hi,lo,NOT_USED_REG));
};

//!Instruction div behavior method.
void ac_behavior( div )
{
  dbg_printf("div r%d, r%d\n", rs, rt);
  // Register LO receives quotient
  lo = (ac_Sword) RB[rs] / (ac_Sword) RB[rt];
  // Register HI receives remainder
  hi = (ac_Sword) RB[rs] % (ac_Sword) RB[rt];
  
  pl5.addInstruction(Instruction(false,false,hi,lo,NOT_USED_REG));
  pl7.addInstruction(Instruction(false,false,hi,lo,NOT_USED_REG));
};

//!Instruction divu behavior method.
void ac_behavior( divu )
{
  dbg_printf("divu r%d, r%d\n", rs, rt);
  // Register LO receives quotient
  lo = RB[rs] / RB[rt];
  // Register HI receives remainder
  hi = RB[rs] % RB[rt];
  
  pl5.addInstruction(Instruction(false,false,hi,lo,NOT_USED_REG));
  pl7.addInstruction(Instruction(false,false,hi,lo,NOT_USED_REG));
};

//!Instruction mfhi behavior method.
void ac_behavior( mfhi )
{
  dbg_printf("mfhi r%d\n", rd);
  RB[rd] = hi;
  dbg_printf("Result = %#x\n", RB[rd]);
  
  pl5.addInstruction(Instruction(false,false,rd,NOT_USED_REG,NOT_USED_REG));
  pl7.addInstruction(Instruction(false,false,rd,NOT_USED_REG,NOT_USED_REG));
};

//!Instruction mthi behavior method.
void ac_behavior( mthi )
{
  dbg_printf("mthi r%d\n", rs);
  hi = RB[rs];
  dbg_printf("Result = %#x\n", (unsigned int) hi);
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
};

//!Instruction mflo behavior method.
void ac_behavior( mflo )
{
  dbg_printf("mflo r%d\n", rd);
  RB[rd] = lo;
  dbg_printf("Result = %#x\n", RB[rd]);
  
  pl5.addInstruction(Instruction(false,false,rd,NOT_USED_REG,NOT_USED_REG));
  pl7.addInstruction(Instruction(false,false,rd,NOT_USED_REG,NOT_USED_REG));
};

//!Instruction mtlo behavior method.
void ac_behavior( mtlo )
{
  dbg_printf("mtlo r%d\n", rs);
  lo = RB[rs];
  dbg_printf("Result = %#x\n", (unsigned int) lo);
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
};

//!Instruction j behavior method.
void ac_behavior( j )
{
  dbg_printf("j %d\n", addr);
  addr = addr << 2;
#ifndef NO_NEED_PC_UPDATE
  npc =  (ac_pc & 0xF0000000) | addr;
#endif 
  dbg_printf("Target = %#x\n", (ac_pc & 0xF0000000) | addr );
  
  
  // renan: Control Hazard... add bubbles
  NJUMPS++;
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
};

//!Instruction jal behavior method.
void ac_behavior( jal )
{
  dbg_printf("jal %d\n", addr);
  // Save the value of PC + 8 (return address) in $ra ($31) and
  // jump to the address given by PC(31...28)||(addr<<2)
  // It must also flush the instructions that were loaded into the pipeline
  RB[Ra] = ac_pc+4; //ac_pc is pc+4, we need pc+8
	
  addr = addr << 2;
#ifndef NO_NEED_PC_UPDATE
  npc = (ac_pc & 0xF0000000) | addr;
#endif 
	
  dbg_printf("Target = %#x\n", (ac_pc & 0xF0000000) | addr );
  dbg_printf("Return = %#x\n", ac_pc+4);
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
};

//!Instruction jr behavior method.
void ac_behavior( jr )
{
  dbg_printf("jr r%d\n", rs);
  // Jump to the address stored on the register reg[RS]
  // It must also flush the instructions that were loaded into the pipeline
#ifndef NO_NEED_PC_UPDATE
  npc = RB[rs], 1;
#endif 
  dbg_printf("Target = %#x\n", RB[rs]);
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
};

//!Instruction jalr behavior method.
void ac_behavior( jalr )
{
  dbg_printf("jalr r%d, r%d\n", rd, rs);
  // Save the value of PC + 8(return address) in rd and
  // jump to the address given by [rs]

#ifndef NO_NEED_PC_UPDATE
  npc = RB[rs], 1;
#endif 
  dbg_printf("Target = %#x\n", RB[rs]);

  if( rd == 0 )  //If rd is not defined use default
    rd = Ra;
  RB[rd] = ac_pc+4;
  dbg_printf("Return = %#x\n", ac_pc+4);
  
  pl5.addInstruction(Instruction(false,false,rd,NOT_USED_REG,rs));
  pl7.addInstruction(Instruction(false,false,rd,NOT_USED_REG,rs));
};

//!Instruction beq behavior method.
void ac_behavior( beq )
{
  bool taken;
  
  dbg_printf("beq r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
  if( RB[rs] == RB[rt] ){
#ifndef NO_NEED_PC_UPDATE
    npc = ac_pc + (imm<<2);
#endif 
    dbg_printf("Taken to %#x\n", ac_pc + (imm<<2));
        
    // renan:
    BRANCHES_TAKEN++;
    taken = true;
  }	  
  
  // renan:
  else {
       taken = false;
       BRANCHES_NOT_TAKEN++;
   }
  
  if(dynamicBP.getPrediction() == taken) DYNAMIC_HITS++;
  else DYNAMIC_MISSES++;
  
  if(staticBP.getPrediction() == taken) STATIC_HITS++;
  else STATIC_MISSES++;
  
  dynamicBP.updatePredict(taken);
  
  pl5.addInstruction(Instruction(false,true,NOT_USED_REG,rt,rs));
  pl7.addInstruction(Instruction(false,true,NOT_USED_REG,rt,rs));
};

//!Instruction bne behavior method.
void ac_behavior( bne )
{	
  bool taken;
  dbg_printf("bne r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
  if( RB[rs] != RB[rt] ){
#ifndef NO_NEED_PC_UPDATE
    npc = ac_pc + (imm<<2);
#endif 
    dbg_printf("Taken to %#x\n", ac_pc + (imm<<2));
        
    // renan:
    BRANCHES_TAKEN++;
    taken = true;
  }	  
  
  // renan:
  else {
       taken = false;
       BRANCHES_NOT_TAKEN++;
   }
  
  if(dynamicBP.getPrediction() == taken) DYNAMIC_HITS++;
  else DYNAMIC_MISSES++;
  
  if(staticBP.getPrediction() == taken) STATIC_HITS++;
  else STATIC_MISSES++;
  
  dynamicBP.updatePredict(taken);
  
  pl5.addInstruction(Instruction(false,true,NOT_USED_REG,rt,rs));
  pl7.addInstruction(Instruction(false,true,NOT_USED_REG,rt,rs));
};

//!Instruction blez behavior method.
void ac_behavior( blez )
{
  bool taken;
  dbg_printf("blez r%d, %d\n", rs, imm & 0xFFFF);
  if( (RB[rs] == 0 ) || (RB[rs]&0x80000000 ) ){
#ifndef NO_NEED_PC_UPDATE
    npc = ac_pc + (imm<<2), 1;
#endif 
    dbg_printf("Taken to %#x\n", ac_pc + (imm<<2));
        
    // renan:
    BRANCHES_TAKEN++;
    taken = true;
  }	  
  
  // renan:
  else {
       taken = false;
       BRANCHES_NOT_TAKEN++;
   }
  
  if(dynamicBP.getPrediction() == taken) DYNAMIC_HITS++;
  else DYNAMIC_MISSES++;
  
  if(staticBP.getPrediction() == taken) STATIC_HITS++;
  else STATIC_MISSES++;
  
  dynamicBP.updatePredict(taken);
  
  pl5.addInstruction(Instruction(false,true,NOT_USED_REG,NOT_USED_REG,rs));
  pl7.addInstruction(Instruction(false,true,NOT_USED_REG,NOT_USED_REG,rs));
};

//!Instruction bgtz behavior method.
void ac_behavior( bgtz )
{
  bool taken;
  dbg_printf("bgtz r%d, %d\n", rs, imm & 0xFFFF);
  if( !(RB[rs] & 0x80000000) && (RB[rs]!=0) ){
#ifndef NO_NEED_PC_UPDATE
    npc = ac_pc + (imm<<2);
#endif 
    dbg_printf("Taken to %#x\n", ac_pc + (imm<<2));
        
    // renan:
    BRANCHES_TAKEN++;
    taken = true;
  }	  
  
  // renan:
  else {
       taken = false;
       BRANCHES_NOT_TAKEN++;
   }
  
  if(dynamicBP.getPrediction() == taken) DYNAMIC_HITS++;
  else DYNAMIC_MISSES++;
  
  if(staticBP.getPrediction() == taken) STATIC_HITS++;
  else STATIC_MISSES++;
  
  dynamicBP.updatePredict(taken);
  
  pl5.addInstruction(Instruction(false,true,NOT_USED_REG,NOT_USED_REG,rs));
  pl7.addInstruction(Instruction(false,true,NOT_USED_REG,NOT_USED_REG,rs));
};

//!Instruction bltz behavior method.
void ac_behavior( bltz )
{
  bool taken;
  dbg_printf("bltz r%d, %d\n", rs, imm & 0xFFFF);
  if( RB[rs] & 0x80000000 ){
#ifndef NO_NEED_PC_UPDATE
    npc = ac_pc + (imm<<2);
#endif 
    dbg_printf("Taken to %#x\n", ac_pc + (imm<<2));
        
    // renan:
    BRANCHES_TAKEN++;
    taken = true;
  }	  
  
  // renan:
  else {
       taken = false;
       BRANCHES_NOT_TAKEN++;
   }
  
  if(dynamicBP.getPrediction() == taken) DYNAMIC_HITS++;
  else DYNAMIC_MISSES++;
  
  if(staticBP.getPrediction() == taken) STATIC_HITS++;
  else STATIC_MISSES++;
  
  dynamicBP.updatePredict(taken);
  
  pl5.addInstruction(Instruction(false,true,NOT_USED_REG,NOT_USED_REG,rs));
  pl7.addInstruction(Instruction(false,true,NOT_USED_REG,NOT_USED_REG,rs));
};

//!Instruction bgez behavior method.
void ac_behavior( bgez )
{
  bool taken;
  dbg_printf("bgez r%d, %d\n", rs, imm & 0xFFFF);
  if( !(RB[rs] & 0x80000000) ){
#ifndef NO_NEED_PC_UPDATE
    npc = ac_pc + (imm<<2);
#endif 
    dbg_printf("Taken to %#x\n", ac_pc + (imm<<2));
        
    // renan:
    BRANCHES_TAKEN++;
    taken = true;
  }	  
  
  // renan:
  else {
       taken = false;
       BRANCHES_NOT_TAKEN++;
   }
  
  if(dynamicBP.getPrediction() == taken) DYNAMIC_HITS++;
  else DYNAMIC_MISSES++;
  
  if(staticBP.getPrediction() == taken) STATIC_HITS++;
  else STATIC_MISSES++;
  
  dynamicBP.updatePredict(taken);
  
  pl5.addInstruction(Instruction(false,true,NOT_USED_REG,NOT_USED_REG,rs));
  pl7.addInstruction(Instruction(false,true,NOT_USED_REG,NOT_USED_REG,rs));
};

//!Instruction bltzal behavior method.
void ac_behavior( bltzal )
{
  bool taken;
  dbg_printf("bltzal r%d, %d\n", rs, imm & 0xFFFF);
  RB[Ra] = ac_pc+4; //ac_pc is pc+4, we need pc+8
  if( RB[rs] & 0x80000000 ){
#ifndef NO_NEED_PC_UPDATE
    npc = ac_pc + (imm<<2);
#endif 
    dbg_printf("Taken to %#x\n", ac_pc + (imm<<2));
        
    // renan:
    BRANCHES_TAKEN++;
    taken = true;
  }	  
  
  // renan:
  else {
       taken = false;
       BRANCHES_NOT_TAKEN++;
   }
  
  if(dynamicBP.getPrediction() == taken) DYNAMIC_HITS++;
  else DYNAMIC_MISSES++;
  
  if(staticBP.getPrediction() == taken) STATIC_HITS++;
  else STATIC_MISSES++;
  
  dynamicBP.updatePredict(taken);
  
  dbg_printf("Return = %#x\n", ac_pc+4);
  
  pl5.addInstruction(Instruction(false,true,NOT_USED_REG,NOT_USED_REG,rs));
  pl7.addInstruction(Instruction(false,true,NOT_USED_REG,NOT_USED_REG,rs));
};

//!Instruction bgezal behavior method.
void ac_behavior( bgezal )
{
  bool taken;
  dbg_printf("bgezal r%d, %d\n", rs, imm & 0xFFFF);
  RB[Ra] = ac_pc+4; //ac_pc is pc+4, we need pc+8
  if( !(RB[rs] & 0x80000000) ){
#ifndef NO_NEED_PC_UPDATE
    npc = ac_pc + (imm<<2);
#endif 
    dbg_printf("Taken to %#x\n", ac_pc + (imm<<2));
    
    // renan:
    BRANCHES_TAKEN++;
    taken = true;
  }	  
  
  // renan:
  else {
       taken = false;
       BRANCHES_NOT_TAKEN++;
   }
  
  if(dynamicBP.getPrediction() == taken) DYNAMIC_HITS++;
  else DYNAMIC_MISSES++;
  
  if(staticBP.getPrediction() == taken) STATIC_HITS++;
  else STATIC_MISSES++;
  
  dynamicBP.updatePredict(taken);
  
  dbg_printf("Return = %#x\n", ac_pc+4);
  
  pl5.addInstruction(Instruction(false,true,NOT_USED_REG,NOT_USED_REG,rs));
  pl7.addInstruction(Instruction(false,true,NOT_USED_REG,NOT_USED_REG,rs));
};

//!Instruction sys_call behavior method.
void ac_behavior( sys_call )
{
  dbg_printf("syscall\n");
  stop();
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
}

//!Instruction instr_break behavior method.
void ac_behavior( instr_break )
{
  fprintf(stderr, "instr_break behavior not implemented.\n"); 
  exit(EXIT_FAILURE);
  
  pl5.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
  pl7.addInstruction(Instruction(false,false,NOT_USED_REG,NOT_USED_REG,NOT_USED_REG));
}



