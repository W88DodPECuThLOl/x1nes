#include "cpu6502.h"
#include "emulator.h"
// #include "utils.h"

extern Emulator gEmulator;

static u16 get_address();
static u16 read_abs_address(u16 offset);
static void set_ZN(u8 value);
static void fast_set_ZN(u8 value);
static u8 shift_l(u8 val);
static u8 shift_r(u8 val);
static u8 rot_l(u8 val);
static u8 rot_r(u8 val);
static void push(u8 value);
static void push_address(u16 address);
static u8 pop();
static u16 pop_address();
static void branch(u8 mask, u8 predicate);
static void prep_branch();
static u8 has_page_break(u16 addr1, u16 addr2);
static void interrupt_();
static void poll_interrupt();
static void rti();

static const Instruction instructionLookup[256] =
{
//  HI\LO        0x0          0x1          0x2             0x3           0x4             0x5         0x6           0x7           0x8           0x9           0xA        0xB            0xC             0xD          0xE           0xF
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*  0x0  */  {BRK, IMPL},{ORA, IDX_IND}, NIL_OP,     {SLO, IDX_IND}, {NOP, ZPG},   {ORA, ZPG},   {ASL, ZPG},   {SLO, ZPG},   {PHP, IMPL}, {ORA, IMT},   {ASL, ACC},  {ANC, IMT},   {NOP, ABS},   {ORA, ABS},   {ASL, ABS},   {SLO, ABS},
/*  0x1  */  {BPL, REL}, {ORA, IND_IDX}, NIL_OP,     {SLO, IND_IDX}, {NOP, ZPG_X}, {ORA, ZPG_X}, {ASL, ZPG_X}, {SLO, ZPG_X}, {CLC, IMPL}, {ORA, ABS_Y}, {NOP, IMPL}, {SLO, ABS_Y}, {NOP, ABS_X}, {ORA, ABS_X}, {ASL, ABS_X}, {SLO, ABS_X},
/*  0x2  */  {JSR, ABS}, {AND, IDX_IND}, NIL_OP,     {RLA, IDX_IND}, {BIT, ZPG},   {AND, ZPG},   {ROL, ZPG},   {RLA, ZPG},   {PLP, IMPL}, {AND, IMT},   {ROL, ACC},  {ANC, IMT},   {BIT, ABS},   {AND, ABS},   {ROL, ABS},   {RLA, ABS},
/*  0x3  */  {BMI, REL}, {AND, IND_IDX}, NIL_OP,     {RLA, IND_IDX}, {NOP, ZPG_X}, {AND, ZPG_X}, {ROL, ZPG_X}, {RLA, ZPG_X}, {SEC, IMPL}, {AND, ABS_Y}, {NOP, IMPL}, {RLA, ABS_Y}, {NOP, ABS_X}, {AND, ABS_X}, {ROL, ABS_X}, {RLA, ABS_X},
/*  0x4  */  {RTI, IMPL},{EOR, IDX_IND}, NIL_OP,     {SRE, IDX_IND}, {NOP, ZPG},   {EOR, ZPG},   {LSR, ZPG},   {SRE, ZPG},   {PHA, IMPL}, {EOR, IMT},   {LSR, ACC},  {ALR, IMT},   {JMP, ABS},   {EOR, ABS},   {LSR, ABS},   {SRE, ABS},
/*  0x5  */  {BVC, REL}, {EOR, IND_IDX}, NIL_OP,     {SRE, IND_IDX}, {NOP, ZPG_X}, {EOR, ZPG_X}, {LSR, ZPG_X}, {SRE, ZPG_X}, {CLI, IMPL}, {EOR, ABS_Y}, {NOP, IMPL}, {SRE, ABS_Y}, {NOP, ABS_X}, {EOR, ABS_X}, {LSR, ABS_X}, {SRE, ABS_X},
/*  0x6  */  {RTS, IMPL},{ADC, IDX_IND}, NIL_OP,     {RRA, IDX_IND}, {NOP, ZPG},   {ADC, ZPG},   {ROR, ZPG},   {RRA, ZPG},   {PLA, IMPL}, {ADC, IMT},   {ROR, ACC},  {ARR, IMT},   {JMP, IND},   {ADC, ABS},   {ROR, ABS},   {RRA, ABS},
/*  0x7  */  {BVS, REL}, {ADC, IND_IDX}, NIL_OP,     {RRA, IND_IDX}, {NOP, ZPG_X}, {ADC, ZPG_X}, {ROR, ZPG_X}, {RRA, ZPG_X}, {SEI, IMPL}, {ADC, ABS_Y}, {NOP, IMPL}, {RRA, ABS_Y}, {NOP, ABS_X}, {ADC, ABS_X}, {ROR, ABS_X}, {RRA, ABS_X},
/*  0x8  */  {NOP, IMT}, {STA, IDX_IND}, {NOP, IMT}, {SAX, IDX_IND}, {STY, ZPG},   {STA, ZPG},   {STX, ZPG},   {SAX, ZPG},   {DEY, IMPL}, {NOP, IMT},   {TXA, IMPL}, {ANE, IMT},   {STY, ABS},   {STA, ABS},   {STX, ABS},   {SAX, ABS},
/*  0x9  */  {BCC, REL}, {STA, IND_IDX}, NIL_OP,     {SHA, IND_IDX}, {STY, ZPG_X}, {STA, ZPG_X}, {STX, ZPG_Y}, {SAX, ZPG_Y}, {TYA, IMPL}, {STA, ABS_Y}, {TXS, IMPL}, {SHS, ABS_Y}, {SHY, ABS_X}, {STA, ABS_X}, {SHX, ABS_Y}, {SHA, ABS_Y},
/*  0xA  */  {LDY, IMT}, {LDA, IDX_IND}, {LDX, IMT}, {LAX, IDX_IND}, {LDY, ZPG},   {LDA, ZPG},   {LDX, ZPG},   {LAX, ZPG},   {TAY, IMPL}, {LDA, IMT},   {TAX, IMPL}, {LAX, IMT},   {LDY, ABS},   {LDA, ABS},   {LDX, ABS},   {LAX, ABS},
/*  0xB  */  {BCS, REL}, {LDA, IND_IDX}, NIL_OP,     {LAX, IND_IDX}, {LDY, ZPG_X}, {LDA, ZPG_X}, {LDX, ZPG_Y}, {LAX, ZPG_Y}, {CLV, IMPL}, {LDA, ABS_Y}, {TSX, IMPL}, {LAS, ABS_Y}, {LDY, ABS_X}, {LDA, ABS_X}, {LDX, ABS_Y}, {LAX, ABS_Y},
/*  0xC  */  {CPY, IMT}, {CMP, IDX_IND}, {NOP, IMT}, {DCP, IDX_IND}, {CPY, ZPG},   {CMP, ZPG},   {DEC, ZPG},   {DCP, ZPG},   {INY, IMPL}, {CMP, IMT},   {DEX, IMPL}, {AXS, IMT},   {CPY, ABS},   {CMP, ABS},   {DEC, ABS},   {DCP, ABS},
/*  0xD  */  {BNE, REL}, {CMP, IND_IDX}, NIL_OP,     {DCP, IND_IDX}, {NOP, ZPG_X}, {CMP, ZPG_X}, {DEC, ZPG_X}, {DCP, ZPG_X}, {CLD, IMPL}, {CMP, ABS_Y}, {NOP, IMPL}, {DCP, ABS_Y}, {NOP, ABS_X}, {CMP, ABS_X}, {DEC, ABS_X}, {DCP, ABS_X},
/*  0xE  */  {CPX, IMT}, {SBC, IDX_IND}, {NOP, IMT}, {ISB, IDX_IND}, {CPX, ZPG},   {SBC, ZPG},   {INC, ZPG},   {ISB, ZPG},   {INX, IMPL}, {SBC, IMT},   NIL_OP,      {SBC, IMT},   {CPX, ABS},   {SBC, ABS},   {INC, ABS},   {ISB, ABS},
/*  0xF  */  {BEQ, REL}, {SBC, IND_IDX}, NIL_OP,     {ISB, IND_IDX}, {NOP, ZPG_X}, {SBC, ZPG_X}, {INC, ZPG_X}, {ISB, ZPG_X}, {SED, IMPL}, {SBC, ABS_Y}, {NOP, IMPL}, {ISB, ABS_Y}, {NOP, ABS_X}, {SBC, ABS_X}, {INC, ABS_X}, {ISB, ABS_X}
};


static const u8 cycleLookup_frozen[256] = {
// HI/LO 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
// -------------------------------------------------------
/* 0 */  7, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 0, 4, 6, 0,
/* 1 */  2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
/* 2 */  6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0,
/* 3 */  2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
/* 4 */  6, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 3, 4, 6, 0,
/* 5 */  2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
/* 6 */  6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0,
/* 7 */  2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
/* 8 */  0, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0,
/* 9 */  2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0,
/* A */  2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0,
/* B */  2, 5, 0, 0, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0,
/* C */  2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0,
/* D */  2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
/* E */  2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 2, 4, 4, 6, 0,
/* F */  2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0,
};

static const u8 cycleLookup[256] = {
// HI/LO 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
// -------------------------------------------------------
/* 0 */  7, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
/* 1 */  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/* 2 */  6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
/* 3 */  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/* 4 */  6, 6, 0, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
/* 5 */  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/* 6 */  6, 6, 0, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
/* 7 */  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/* 8 */  2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
/* 9 */  2, 6, 0, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
/* A */  2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
/* B */  2, 5, 0, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
/* C */  2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
/* D */  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/* E */  2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
/* F */  2, 5, 0, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
};

void init_cpu(struct Emulator* emulator){
    struct c6502* cpu = &emulator->cpu;
    gEmulator.cpu.emulator = emulator;
    gEmulator.cpu.interrupt = NOI;
    set_cpu_mode(CPU_EXEC); // Normal execution
    gEmulator.cpu.memory = &emulator->mem;
    gEmulator.cpu.sr_started = 0;
    gEmulator.cpu.NMI_hook = nullptr;

    gEmulator.cpu.ac = gEmulator.cpu.x = gEmulator.cpu.y = gEmulator.cpu.state = 0;
    gEmulator.cpu.cycles = gEmulator.cpu.dma_cycles = 0;
    gEmulator.cpu.odd_cycle = 0;
    gEmulator.cpu.t_cycles = 0;
    gEmulator.cpu.sr = 0x24;
    gEmulator.cpu.sp = 0xfd;
#if TRACER == 1 && PROFILE == 0
    gEmulator.cpu.pc = 0xC000;
#else
    gEmulator.cpu.pc = read_abs_address(RESET_ADDRESS);
#endif
}

void reset_cpu(){
    gEmulator.cpu.sr |= INTERRUPT;
    set_cpu_mode(CPU_EXEC);; // Normal execution
    gEmulator.cpu.sp -= 3;
    gEmulator.cpu.pc = read_abs_address(RESET_ADDRESS);
    gEmulator.cpu.cycles = 0;
    gEmulator.cpu.dma_cycles = 0;
    gEmulator.cpu.sr_started = 0;
    gEmulator.cpu.NMI_hook = nullptr;
}

u8 run_cpu_subroutine(u16 address) {
    if (gEmulator.cpu.mode & CPU_SR_ANY) {
        // unable to begin subroutine because there is an executing subroutine and/or ISR
        LOG(TRACE, "Unable to start subroutine $%4x, pending subroutine %x",address, gEmulator.cpu.sub_address);
        return CPU_SR;
    }
    gEmulator.cpu.sub_address = address;
    set_cpu_mode( gEmulator.cpu.mode | CPU_SR);
    LOG(TRACE, "Running subroutine: $%4x",gEmulator.cpu.sub_address);
    return 0;
}

void set_cpu_mode(CPUMode mode) {
    gEmulator.cpu.mode = mode;
    LOG(TRACE, "CPU mode: %d",mode);
}

static void interrupt_(){
    // handle interrupt
    u16 addr;
    u8 set_brk = 0;

    if(gEmulator.cpu.interrupt & BRK_I) {
        // BRK instruction is 2 bytes
        gEmulator.cpu.pc++;
        if(gEmulator.cpu.state & NMI_HIJACK) {
            addr = NMI_ADDRESS;
            gEmulator.cpu.interrupt &= ~NMI;
        }else {
            addr = IRQ_ADDRESS;
        }
        gEmulator.cpu.interrupt &= ~BRK_I;
        // re-apply Break flag
        set_brk = 1;
    }
    else if(gEmulator.cpu.interrupt & NMI) {
        addr = NMI_ADDRESS;
        // NMI is edge triggered so clear it
        gEmulator.cpu.interrupt &= ~NMI;
    }
    else if(gEmulator.cpu.interrupt & RSI){
        addr = RESET_ADDRESS;
        // not used but just in case
        interrupt_clear( RSI);
    }
    else if(gEmulator.cpu.interrupt & IRQ) {
        addr = IRQ_ADDRESS;
        // gEmulator.cpu.sr |= BREAK;
    } else {
        LOG(ERROR, "No interrupt set");
        return;
    }

    push_address( gEmulator.cpu.pc);
    // bit 5 always set, bit 4 set on BRK
    push( gEmulator.cpu.sr & ~(1<<4) | (1<<5) | (set_brk ? (1<<4) : 0));
    gEmulator.cpu.sr |= INTERRUPT;
    gEmulator.cpu.pc = read_abs_address(addr);
    if (addr == NMI_ADDRESS && gEmulator.cpu.NMI_hook != nullptr) {
        // call NMI hook for phase 0 (pre ISR)
        push_address( gEmulator.cpu.sub_address);
        gEmulator.cpu.NMI_hook(0);
        gEmulator.cpu.sr_started = 0;
        LOG(TRACE, "NMI wrapper at $%4x", gEmulator.cpu.sub_address);
        set_cpu_mode( gEmulator.cpu.mode | CPU_NMI_SR);
    }
}

static void rti() {
    // ignore bit 4 and 5
    gEmulator.cpu.sr &= ((1<<4) | (1<<5));
    gEmulator.cpu.sr |= pop() & ~((1<<4) | (1<<5));
    gEmulator.cpu.pc = pop_address();
    // hopefully intrinsic 6502 x-tics will allow this to be enough
    // i.e. no ISR nesting
    if (gEmulator.cpu.mode & CPU_ISR) {
        LOG(TRACE, "ISR completed");
        set_cpu_mode( gEmulator.cpu.mode & ~CPU_ISR);
    }
}

void interrupt(const Interrupt code) {
    if(code == NMI) {
        if(!gEmulator.cpu.NMI_line) {
            gEmulator.cpu.interrupt |= NMI;
        }
        gEmulator.cpu.NMI_line = 1;
    }else {
        gEmulator.cpu.interrupt |= code;
    }
}
void interrupt_clear(const Interrupt code) {
    if(code == NMI) gEmulator.cpu.NMI_line = 0;
    else gEmulator.cpu.interrupt &= ~code;
}

void do_DMA(u16 cycles) {
    gEmulator.cpu.dma_cycles += cycles;
    gEmulator.cpu.state |= DMA_OCCURRED;
}

static void branch(u8 mask, u8 predicate) {
    if(((gEmulator.cpu.sr & mask) > 0) == predicate){
        // increment cycles if branching to a different page
        gEmulator.cpu.cycles += has_page_break(gEmulator.cpu.pc, gEmulator.cpu.address);
        gEmulator.cpu.cycles++;
        // tell the cpu to perform a branch when it is ready
        gEmulator.cpu.state |= BRANCH_STATE;
    }else {
        // remove branch state
        gEmulator.cpu.state &= ~BRANCH_STATE;
    }
}

static void prep_branch(){
    switch(gEmulator.cpu.instruction->opcode){
        case BCC:
            branch( CARRY, 0);
            break;
        case BCS:
            branch( CARRY, 1);
            break;
        case BEQ:
            branch( ZERO, 1);
            break;
        case BMI:
            branch( NEGATIVE, 1);
            break;
        case BNE:
            branch( ZERO, 0);
            break;
        case BPL:
            branch( NEGATIVE, 0);
            break;
        case BVC:
            branch( OVERFLW, 0);
            break;
        case BVS:
            branch( OVERFLW, 1);
            break;
        default:
            gEmulator.cpu.state &= ~BRANCH_STATE;
    }
}

static void poll_interrupt() {
    gEmulator.cpu.state |= INTERRUPT_POLLED;
    if(gEmulator.cpu.interrupt & ~IRQ || (gEmulator.cpu.interrupt & IRQ && !(gEmulator.cpu.sr & INTERRUPT))){
        // prepare for interrupts
        gEmulator.cpu.state |= INTERRUPT_PENDING;
        // check if NMI is asserted at poll time
        gEmulator.cpu.state |= gEmulator.cpu.interrupt & NMI ? NMI_ASSERTED : 0;
    }
}

void execute(){
    gEmulator.cpu.odd_cycle ^= 1;
    gEmulator.cpu.t_cycles++;
    if (gEmulator.cpu.dma_cycles != 0){
        // DMA CPU suspend
        gEmulator.cpu.dma_cycles--;
        return;
    }
    if(gEmulator.cpu.cycles == 0) {
        if (!gEmulator.cpu.mode) {
            // cpu wait mode
            return;
        }
        // if there is a pending ISR, we wait before we start the subroutine unless it is NMI
        if ((gEmulator.cpu.mode & CPU_NMI_SR || (gEmulator.cpu.mode & (CPU_SR | CPU_ISR)) == CPU_SR) && gEmulator.cpu.sr_started == 0) {
            // we just started cpu subroutine mode
            // let's do a manual JSR to sub_address
            push_address( gEmulator.cpu.pc);
            push_address( SENTINEL_ADDR);
            gEmulator.cpu.pc = gEmulator.cpu.sub_address;
            gEmulator.cpu.sr_started = 1;
        }

#if TRACER == 1
        print_cpu_trace();
#endif
        if(!(gEmulator.cpu.state & INTERRUPT_POLLED)) {
            poll_interrupt();
        }
        gEmulator.cpu.state &= ~(INTERRUPT_POLLED | NMI_HIJACK | DMA_OCCURRED);
        if(gEmulator.cpu.state & INTERRUPT_PENDING) {
            // takes 7 cycles and this is one of them so only 6 are left
            LOG(TRACE, "Starting ISR: $%2x", gEmulator.cpu.interrupt);
            gEmulator.cpu.cycles = 6;
            set_cpu_mode( gEmulator.cpu.mode | CPU_ISR);
            return;
        }
        if (!(gEmulator.cpu.mode & CPU_EXEC_ANY)) {
            // busy wait if not in any execution mode
            return;
        }

        u8 opcode = read_mem(gEmulator.cpu.pc++);
        gEmulator.cpu.instruction = &instructionLookup[opcode];
        gEmulator.cpu.address = get_address();
        if(gEmulator.cpu.instruction->opcode == BRK) {
            // set BRK interrupt
            gEmulator.cpu.interrupt |= BRK_I;
            gEmulator.cpu.state |= INTERRUPT_PENDING;
            gEmulator.cpu.cycles = 6;
            return;
        }
        gEmulator.cpu.cycles += cycleLookup[opcode];
        // prepare for branching and adjust cycles accordingly
        prep_branch();
        gEmulator.cpu.cycles--;
        return;
    }

    if(gEmulator.cpu.cycles == 1){
        gEmulator.cpu.cycles--;
        // proceed to execution
    } else{
        // delay execution
        if(gEmulator.cpu.state & INTERRUPT_PENDING && gEmulator.cpu.cycles == 4) {
            if(gEmulator.cpu.interrupt & NMI) gEmulator.cpu.state |= NMI_HIJACK;
        }
        gEmulator.cpu.cycles--;
        return;
    }

    // handle interrupt
    if(gEmulator.cpu.state & INTERRUPT_PENDING) {
        interrupt_();
        gEmulator.cpu.state &= ~INTERRUPT_PENDING;
        return;
    }

    u16 address = gEmulator.cpu.address;

    switch (gEmulator.cpu.instruction->opcode) {

        // Load and store opcodes

        case LDA:
            gEmulator.cpu.ac = read_mem(address);
            set_ZN( gEmulator.cpu.ac);
            break;
        case LDX:
            gEmulator.cpu.x = read_mem(address);
            set_ZN( gEmulator.cpu.x);
            break;
        case LDY:
            gEmulator.cpu.y = read_mem(address);
            set_ZN( gEmulator.cpu.y);
            break;
        case STA:
            write_mem(address, gEmulator.cpu.ac);
            break;
        case STY:
            write_mem(address, gEmulator.cpu.y);
            break;
        case STX:
            write_mem(address, gEmulator.cpu.x);
            break;

        // Register transfer opcodes

        case TAX:
            gEmulator.cpu.x = gEmulator.cpu.ac;
            set_ZN( gEmulator.cpu.x);
            break;
        case TAY:
            gEmulator.cpu.y = gEmulator.cpu.ac;
            set_ZN( gEmulator.cpu.y);
            break;
        case TXA:
            gEmulator.cpu.ac = gEmulator.cpu.x;
            set_ZN( gEmulator.cpu.ac);
            break;
        case TYA:
            gEmulator.cpu.ac = gEmulator.cpu.y;
            set_ZN( gEmulator.cpu.ac);
            break;

        // stack opcodes

        case TSX:
            gEmulator.cpu.x = gEmulator.cpu.sp;
            set_ZN( gEmulator.cpu.x);
            break;
        case TXS:
            gEmulator.cpu.sp = gEmulator.cpu.x;
            break;
        case PHA:
            push( gEmulator.cpu.ac);
            break;
        case PHP:
            // 6502 quirk bit 4 and 5 are always set by this instruction: see also BRK
            push( gEmulator.cpu.sr | (1<<4) | (1<<5));
            break;
        case PLA:
            gEmulator.cpu.ac = pop();
            set_ZN( gEmulator.cpu.ac);
            break;
        case PLP:
            // poll before updating I flag
            poll_interrupt();
            gEmulator.cpu.sr = pop();
            break;

        // logical opcodes

        case AND:
            gEmulator.cpu.ac &= read_mem(address);
            set_ZN( gEmulator.cpu.ac);
            break;
        case EOR:
            gEmulator.cpu.ac ^= read_mem(address);
            set_ZN( gEmulator.cpu.ac);
            break;
        case ORA:
            gEmulator.cpu.ac |= read_mem(address);
            set_ZN( gEmulator.cpu.ac);
            break;
        case BIT: {
            u8 opr = read_mem(address);
            gEmulator.cpu.sr &= ~(NEGATIVE | OVERFLW | ZERO);
            gEmulator.cpu.sr |= (!(opr & gEmulator.cpu.ac) ? ZERO: 0);
            gEmulator.cpu.sr |= (opr & (NEGATIVE | OVERFLW));
            break;
        }

        // Arithmetic opcodes

        case ADC: {
            u8 opr = read_mem(address);
            u16 sum = gEmulator.cpu.ac + opr + ((gEmulator.cpu.sr & CARRY) != 0);
            gEmulator.cpu.sr &= ~(CARRY | OVERFLW | NEGATIVE | ZERO);
            gEmulator.cpu.sr |= (sum & 0xFF00 ? CARRY: 0);
            gEmulator.cpu.sr |= ((gEmulator.cpu.ac ^ sum) & (opr ^ sum) & 0x80) ? OVERFLW: 0;
            gEmulator.cpu.ac = sum;
            fast_set_ZN( gEmulator.cpu.ac);
            break;
        }
        case SBC: {
            u8 opr = read_mem(address);
            u16 diff = gEmulator.cpu.ac - opr - ((gEmulator.cpu.sr & CARRY) == 0);
            gEmulator.cpu.sr &= ~(CARRY | OVERFLW | NEGATIVE | ZERO);
            gEmulator.cpu.sr |= (!(diff & 0xFF00)) ? CARRY : 0;
            gEmulator.cpu.sr |= ((gEmulator.cpu.ac ^ diff) & (~opr ^ diff) & 0x80) ? OVERFLW: 0;
            gEmulator.cpu.ac = diff;
            fast_set_ZN( gEmulator.cpu.ac);
            break;
        }
        case CMP: {
            u16 diff = gEmulator.cpu.ac - read_mem(address);
            gEmulator.cpu.sr &= ~(CARRY | NEGATIVE | ZERO);
            gEmulator.cpu.sr |= !(diff & 0xFF00) ? CARRY: 0;
            fast_set_ZN( diff);
            break;
        }
        case CPX: {
            u16 diff = gEmulator.cpu.x - read_mem(address);
            gEmulator.cpu.sr &= ~(CARRY | NEGATIVE | ZERO);
            gEmulator.cpu.sr |= !(diff & 0x100) ? CARRY: 0;
            fast_set_ZN( diff);
            break;
        }
        case CPY: {
            u16 diff = gEmulator.cpu.y - read_mem(address);
            gEmulator.cpu.sr &= ~(CARRY | NEGATIVE | ZERO);
            gEmulator.cpu.sr |= !(diff & 0xFF00) ? CARRY: 0;
            fast_set_ZN( diff);
            break;
        }

        // Increments and decrements opcodes

        case DEC:{
            u8 m = read_mem(address);
            // dummy write
            write_mem(address, m--);
            write_mem(address, m);
            set_ZN( m);
            break;
        }
        case DEX:
            gEmulator.cpu.x--;
            set_ZN( gEmulator.cpu.x);
            break;
        case DEY:
            gEmulator.cpu.y--;
            set_ZN( gEmulator.cpu.y);
            break;
        case INC:{
            u8 m = read_mem(address);
            // dummy write
            write_mem(address, m++);
            write_mem(address, m);
            set_ZN( m);
            break;
        }
        case INX:
            gEmulator.cpu.x++;
            set_ZN( gEmulator.cpu.x);
            break;
        case INY:
            gEmulator.cpu.y++;
            set_ZN( gEmulator.cpu.y);
            break;

        // shifts and rotations opcodes

        case ASL:
            if(gEmulator.cpu.instruction->mode == ACC) {
                gEmulator.cpu.ac = shift_l( gEmulator.cpu.ac);
            }else{
                u8 m = read_mem(address);
                // dummy write
                write_mem(address, m);
                write_mem(address, shift_l( m));
            }
            break;
        case LSR:
            if(gEmulator.cpu.instruction->mode == ACC) {
                gEmulator.cpu.ac = shift_r( gEmulator.cpu.ac);
            }else{
                u8 m = read_mem(address);
                // dummy write
                write_mem(address, m);
                write_mem(address, shift_r( m));
            }
            break;
        case ROL:
            if(gEmulator.cpu.instruction->mode == ACC){
                gEmulator.cpu.ac = rot_l( gEmulator.cpu.ac);
            }else{
                u8 m = read_mem(address);
                // dummy write
                write_mem(address, m);
                write_mem(address, rot_l( m));
            }
            break;
        case ROR:
            if(gEmulator.cpu.instruction->mode == ACC){
                gEmulator.cpu.ac = rot_r( gEmulator.cpu.ac);
            }else{
                u8 m = read_mem(address);
                // dummy write
                write_mem(address, m);
                write_mem(address, rot_r( m));
            }
            break;

        // jumps and calls

        case JMP:
            gEmulator.cpu.pc = address;
            gEmulator.cpu.memory->bus = gEmulator.cpu.pc >> 8;
            break;
        case JSR:
            push_address( gEmulator.cpu.pc - 1);
            gEmulator.cpu.pc = address;
            gEmulator.cpu.memory->bus = gEmulator.cpu.pc >> 8;
            break;
        case RTS:
            gEmulator.cpu.pc = pop_address() + 1;
            gEmulator.cpu.memory->bus = gEmulator.cpu.pc >> 8;
            // Keep track of external subroutine
            if ((gEmulator.cpu.mode & CPU_SR_ANY) && gEmulator.cpu.pc == SENTINEL_ADDR + 1) {
                gEmulator.cpu.pc = pop_address();
                gEmulator.cpu.memory->bus = gEmulator.cpu.pc >> 8;
                if (gEmulator.cpu.mode & CPU_NMI_SR) {
                    if (gEmulator.cpu.NMI_hook != nullptr) gEmulator.cpu.NMI_hook( 1);
                    LOG(TRACE, "NMI wrapper at $%4x ended", gEmulator.cpu.sub_address);
                    gEmulator.cpu.sub_address = pop_address();
                    set_cpu_mode( gEmulator.cpu.mode & ~CPU_NMI_SR);
                    if (!(gEmulator.cpu.mode & CPU_SR)) gEmulator.cpu.sr_started = 0;
                    // end NMI
                    rti();
                }else {
                    LOG(TRACE, "Exiting subroutine $%4x", gEmulator.cpu.sub_address);
                    set_cpu_mode( gEmulator.cpu.mode & ~CPU_SR);
                    gEmulator.cpu.sr_started = 0;
                }
            }
            break;

        // branching opcodes

        case BCC:case BCS:case BEQ:case BMI:case BNE:case BPL:case BVC:case BVS:
            if(gEmulator.cpu.state & BRANCH_STATE) {
                gEmulator.cpu.pc = gEmulator.cpu.address;
                gEmulator.cpu.state &= ~BRANCH_STATE;
            }
            break;

        // status flag changes

        case CLC:
            gEmulator.cpu.sr &= ~CARRY;
            break;
        case CLD:
            gEmulator.cpu.sr &= ~DECIMAL_;
            break;
        case CLI:
            // poll before updating I flag
            poll_interrupt();
            gEmulator.cpu.sr &= ~INTERRUPT;
            break;
        case CLV:
            gEmulator.cpu.sr &= ~OVERFLW;
            break;
        case SEC:
            gEmulator.cpu.sr |= CARRY;
            break;
        case SED:
            gEmulator.cpu.sr |= DECIMAL_;
            break;
        case SEI:
            poll_interrupt();
            gEmulator.cpu.sr |= INTERRUPT;
            break;

        // system functions

        case BRK:
            // Handled in interrupt_()
            break;
        case RTI:
            rti();
            break;
        case NOP:
            if(gEmulator.cpu.instruction->mode == ABS) {
                // dummy read edge case for unofficial opcode Ox0C
                read_mem(address);
            }
            break;

        // unofficial

        case ALR:
            // Unstable instruction.
            // A <- A & (const | M), A <- LSR A
            // magic const set to 0x00 but, could also be 0xff, 0xee, ..., 0x11 depending on analog effects
            gEmulator.cpu.ac &= read_mem(address);
            gEmulator.cpu.ac = shift_r( gEmulator.cpu.ac);
            break;
        case ANC:
            gEmulator.cpu.ac = gEmulator.cpu.ac & read_mem(address);
            gEmulator.cpu.sr &= ~(CARRY | ZERO | NEGATIVE);
            gEmulator.cpu.sr |= (gEmulator.cpu.ac & NEGATIVE) ? (CARRY | NEGATIVE): 0;
            gEmulator.cpu.sr |= ((!gEmulator.cpu.ac)? ZERO: 0);
            break;
        case ANE:
            // Unstable instruction.
            // A <- (A | const) & X & M
            // magic const set to 0xff but, could also be 0xee, 0xdd, ..., 0x00 depending on analog effects
            gEmulator.cpu.ac = gEmulator.cpu.x & read_mem(address);
            set_ZN( gEmulator.cpu.ac);
            break;
        case ARR: {
            // Unstable instruction.
            // A <- A & (const | M), ROR A
            // magic const set to 0x00 but, could also be 0xff, 0xee, ..., 0x11 depending on analog effects
            u8 val = gEmulator.cpu.ac & read_mem(address);
            u8 rotated = val >> 1;
            rotated |= (gEmulator.cpu.sr & CARRY) << 7;
            gEmulator.cpu.sr &= ~(CARRY | ZERO | NEGATIVE | OVERFLW);
            gEmulator.cpu.sr |= (rotated & (1<<6)) ? CARRY: 0;
            gEmulator.cpu.sr |= (((rotated & (1<<6)) >> 1) ^ (rotated & (1<<5))) ? OVERFLW: 0;
            fast_set_ZN( rotated);
            gEmulator.cpu.ac = rotated;
            break;
        }
        case AXS: {
            u8 opr = read_mem(address);
            gEmulator.cpu.x = gEmulator.cpu.x & gEmulator.cpu.ac;
            u16 diff = gEmulator.cpu.x - opr;
            gEmulator.cpu.sr &= ~(CARRY | NEGATIVE | ZERO);
            gEmulator.cpu.sr |= (!(diff & 0xFF00)) ? CARRY : 0;
            gEmulator.cpu.x = diff;
            fast_set_ZN( gEmulator.cpu.x);
            break;
        }
        case LAX:
            // Unstable instruction.
            // A,X <- (A | const) & M
            // magic const set to 0xff but, could also be 0xee, 0xdd, ..., 0x00 depending on analog effects
            gEmulator.cpu.ac = read_mem(address);
            gEmulator.cpu.x = gEmulator.cpu.ac;
            set_ZN( gEmulator.cpu.ac);
            break;
        case SAX: {
            write_mem(address, gEmulator.cpu.ac & gEmulator.cpu.x);
            break;
        }
        case DCP: {
            u8 m = read_mem(address);
            // dummy write
            write_mem(address, m--);
            write_mem(address, m);
            u16 diff = gEmulator.cpu.ac - read_mem(address);
            gEmulator.cpu.sr &= ~(CARRY | NEGATIVE | ZERO);
            gEmulator.cpu.sr |= !(diff & 0xFF00) ? CARRY: 0;
            fast_set_ZN( diff);
            break;
        }
        case ISB: {
            u8 m = read_mem(address);
            // dummy write
            write_mem(address, m++);
            write_mem(address, m);
            u16 diff = gEmulator.cpu.ac - m - ((gEmulator.cpu.sr & CARRY) == 0);
            gEmulator.cpu.sr &= ~(CARRY | OVERFLW | NEGATIVE | ZERO);
            gEmulator.cpu.sr |= (!(diff & 0xFF00)) ? CARRY : 0;
            gEmulator.cpu.sr |= ((gEmulator.cpu.ac ^ diff) & (~m ^ diff) & 0x80) ? OVERFLW: 0;
            gEmulator.cpu.ac = diff;
            fast_set_ZN( gEmulator.cpu.ac);
            break;
        }
        case RLA: {
            u8 m = read_mem(address);
            // dummy write
            write_mem(address, m);
            m = rot_l( m);
            write_mem(address, m);
            gEmulator.cpu.ac &= m;
            set_ZN( gEmulator.cpu.ac);
            break;
        }
        case RRA: {
            u8 m = read_mem(address);
            // dummy write
            write_mem(address, m);
            m = rot_r( m);
            write_mem(address, m);
            u16 sum = gEmulator.cpu.ac + m + ((gEmulator.cpu.sr & CARRY) != 0);
            gEmulator.cpu.sr &= ~(CARRY | OVERFLW | NEGATIVE | ZERO);
            gEmulator.cpu.sr |= (sum & 0xFF00 ? CARRY : 0);
            gEmulator.cpu.sr |= ((gEmulator.cpu.ac ^ sum) & (m ^ sum) & 0x80) ? OVERFLW : 0;
            gEmulator.cpu.ac = sum;
            fast_set_ZN( sum);
            break;
        }
        case SLO: {
            u8 m = read_mem(address);
            // dummy write
            write_mem(address, m);
            m = shift_l( m);
            write_mem(address, m);
            gEmulator.cpu.ac |= m;
            set_ZN( gEmulator.cpu.ac);
            break;
        }
        case SRE: {
            u8 m = read_mem(address);
            // dummy write
            write_mem(address, m);
            m = shift_r( m);
            write_mem(address, m);
            gEmulator.cpu.ac ^= m;
            set_ZN( gEmulator.cpu.ac);
            break;
        }

        // The next 4 instructions are extremely unstable with weird sometimes inexplicable behavior
        // I have tried to emulate them to the best of my ability
        case SHY: {
            // if RDY line goes low 2 cycles before the value write, the high byte of target address is ignored
            // By target address I mean the unindexed address as read from memory referred to here as raw_address
            // I have equated this behaviour with DMA occurring in the course of the instruction
            // This also applies to SHX, SHA and SHS
            const u8 val = gEmulator.cpu.state & DMA_OCCURRED ? gEmulator.cpu.y: gEmulator.cpu.y & ((gEmulator.cpu.raw_address >> 8) + 1);
            if (has_page_break(gEmulator.cpu.raw_address, address))
                // Instruction unstable, corrupt high byte of address
                address = (address & 0xff) | (val << 8);
            write_mem(address, val);
            break;
        }
        case SHX: {
            const u8 val = gEmulator.cpu.state & DMA_OCCURRED ? gEmulator.cpu.x: gEmulator.cpu.x & ((gEmulator.cpu.raw_address >> 8) + 1);
            if (has_page_break(gEmulator.cpu.raw_address, address))
                // Instruction unstable, corrupt high byte of address
                address = (address & 0xff) | (val << 8);
            write_mem(address, val);
            break;
        }
        case SHA: {
            const u8 val = gEmulator.cpu.state & DMA_OCCURRED ? gEmulator.cpu.x & gEmulator.cpu.ac: gEmulator.cpu.x & gEmulator.cpu.ac & ((gEmulator.cpu.raw_address >> 8) + 1);
            if (has_page_break(gEmulator.cpu.raw_address, address))
                // Instruction unstable, corrupt high byte of address
                address = (address & 0xff) | (val << 8);
            write_mem(address, val);
            break;
        }
        case SHS:
            gEmulator.cpu.sp = gEmulator.cpu.ac & gEmulator.cpu.x;
            const u8 val = gEmulator.cpu.state & DMA_OCCURRED ? gEmulator.cpu.x & gEmulator.cpu.ac: gEmulator.cpu.x & gEmulator.cpu.ac & ((gEmulator.cpu.raw_address >> 8) + 1);
            if (has_page_break(gEmulator.cpu.raw_address, address))
                // Instruction unstable, corrupt high byte of address
                address = (address & 0xff) | (val << 8);
            write_mem(address, val);
            break;
        case LAS:
            gEmulator.cpu.ac = gEmulator.cpu.x = gEmulator.cpu.sp = read_mem(address) & gEmulator.cpu.sp;;
            set_ZN( gEmulator.cpu.ac);
            break;
        default:
            break;
    }
}

static u16 read_abs_address(u16 offset){
    // 16-bit address is little endian so read lo then hi
    u16 lo = (u16)read_mem(offset);
    u16 hi = (u16)read_mem(offset + 1);
    return (hi << 8) | lo;
}

static u16 get_address(){
    u16 addr, hi,lo;
    switch (gEmulator.cpu.instruction->mode) {
        case IMPL:
        case ACC:
            // dummy read
            read_mem(gEmulator.cpu.pc);
        case NONE:
            gEmulator.cpu.raw_address = 0;
            return 0;
        case REL: {
            s8 offset = (s8)read_mem(gEmulator.cpu.pc++);
            addr = gEmulator.cpu.raw_address = gEmulator.cpu.pc + offset;
            return addr;
        }
        case IMT:
            gEmulator.cpu.raw_address = gEmulator.cpu.pc + 1;
            return gEmulator.cpu.pc++;
        case ZPG:
            gEmulator.cpu.raw_address = gEmulator.cpu.pc + 1;
            return read_mem(gEmulator.cpu.pc++);
        case ZPG_X:
            addr = gEmulator.cpu.raw_address = gEmulator.cpu.raw_address = read_mem(gEmulator.cpu.pc++);
            return (addr + gEmulator.cpu.x) & 0xFF;
        case ZPG_Y:
            addr = gEmulator.cpu.raw_address = read_mem(gEmulator.cpu.pc++);
            return (addr + gEmulator.cpu.y) & 0xFF;
        case ABS:
            addr = gEmulator.cpu.raw_address = read_abs_address(gEmulator.cpu.pc);
            gEmulator.cpu.pc += 2;
            return addr;
        case ABS_X:
            addr = gEmulator.cpu.raw_address = read_abs_address(gEmulator.cpu.pc);
            gEmulator.cpu.pc += 2;
            switch (gEmulator.cpu.instruction->opcode) {
                // these don't take into account absolute x page breaks
                case STA:case ASL:case DEC:case INC:case LSR:case ROL:case ROR:
                // unofficial
                case SLO:case RLA:case SRE:case RRA:case DCP:case ISB: case SHY:
                    // invalid read
                    read_mem((addr & 0xff00) | ((addr + gEmulator.cpu.x) & 0xff));
                    break;
                default:
                    if(has_page_break(addr, addr + gEmulator.cpu.x)) {
                        // invalid read
                        read_mem((addr & 0xff00) | ((addr + gEmulator.cpu.x) & 0xff));
                        gEmulator.cpu.cycles++;
                    }
            }
            return addr + gEmulator.cpu.x;
        case ABS_Y:
            addr = gEmulator.cpu.raw_address = read_abs_address(gEmulator.cpu.pc);
            gEmulator.cpu.pc += 2;
            switch (gEmulator.cpu.instruction->opcode) {
                case STA:case SLO:case RLA:case SRE:case RRA:case DCP:case ISB: case NOP:
                case SHX:case SHA:case SHS:
                    // invalid read
                    read_mem((addr & 0xff00) | ((addr + gEmulator.cpu.y) & 0xff));
                    break;
                default:
                    if(has_page_break(addr, addr + gEmulator.cpu.y)) {
                        // invalid read
                        read_mem((addr & 0xff00) | ((addr + gEmulator.cpu.y) & 0xff));
                        gEmulator.cpu.cycles++;
                    }
            }
            return addr + gEmulator.cpu.y;
        case IND:
            addr = gEmulator.cpu.raw_address = read_abs_address(gEmulator.cpu.pc);
            gEmulator.cpu.pc += 2;
            lo = read_mem(addr);
            // handle a bug in 6502 hardware where if reading from $xxFF (page boundary) the
            // LSB is read from $xxFF as expected but the MSB is fetched from xx00
            hi = read_mem((addr & 0xFF00) | (addr + 1) & 0xFF);
            return (hi << 8) | lo;
        case IDX_IND:
            addr = (read_mem(gEmulator.cpu.pc++) + gEmulator.cpu.x) & 0xFF;
            lo = read_mem(addr & 0xFF);
            hi = read_mem((addr + 1) & 0xFF);
            addr = gEmulator.cpu.raw_address = (hi << 8) | lo;
            return addr;
        case IND_IDX:
            addr = read_mem(gEmulator.cpu.pc++);
            lo = read_mem(addr & 0xFF);
            hi = read_mem((addr + 1) & 0xFF);
            addr = gEmulator.cpu.raw_address = (hi << 8) | lo;
            switch (gEmulator.cpu.instruction->opcode) {
                case STA:case SLO:case RLA:case SRE:case RRA:case DCP:case ISB: case NOP:
                case SHA:
                    // invalid read
                    read_mem((addr & 0xff00) | ((addr + gEmulator.cpu.y) & 0xff));
                    break;
                default:
                    if(has_page_break(addr, addr + gEmulator.cpu.y)) {
                        // invalid read
                        read_mem((addr & 0xff00) | ((addr + gEmulator.cpu.y) & 0xff));
                        gEmulator.cpu.cycles++;
                    }
            }
            return addr + gEmulator.cpu.y;
    }
    return 0;
}

static void set_ZN(u8 value){
    gEmulator.cpu.sr &= ~(NEGATIVE | ZERO);
    gEmulator.cpu.sr |= ((!value)? ZERO: 0);
    gEmulator.cpu.sr |= (value & NEGATIVE);
}

static void fast_set_ZN(u8 value){
    // this assumes the necessary flags (Z & N) have been cleared
    gEmulator.cpu.sr |= ((!value)? ZERO: 0);
    gEmulator.cpu.sr |= (value & NEGATIVE);
}

static void push(u8 value){
    write_mem(STACK_START + gEmulator.cpu.sp--, value);
}

static void push_address(u16 address){
    write_mem(STACK_START + gEmulator.cpu.sp--, address >> 8);
    write_mem(STACK_START + gEmulator.cpu.sp--, address & 0xFF);
}

static u8 pop(){
    return read_mem(STACK_START + ++gEmulator.cpu.sp);
}

static u16 pop_address(){
    u16 addr = read_mem(STACK_START + ++gEmulator.cpu.sp);
    return addr | ((u16)read_mem(STACK_START + ++gEmulator.cpu.sp)) << 8;
}

static u8 shift_l(u8 val){
    gEmulator.cpu.sr &= ~(CARRY | ZERO | NEGATIVE);
    gEmulator.cpu.sr |= (val & NEGATIVE) ? CARRY: 0;
    val <<= 1;
    fast_set_ZN( val);
    return val;
}

static u8 shift_r(u8 val){
    gEmulator.cpu.sr &= ~(CARRY | ZERO | NEGATIVE);
    gEmulator.cpu.sr |= (val & 0x1) ? CARRY: 0;
    val >>= 1;
    fast_set_ZN( val);
    return val;
}

static u8 rot_l(u8 val){
    u8 rotated = val << 1;
    rotated |= gEmulator.cpu.sr & CARRY;
    gEmulator.cpu.sr &= ~(CARRY | ZERO | NEGATIVE);
    gEmulator.cpu.sr |= val & NEGATIVE ? CARRY: 0;
    fast_set_ZN( rotated);
    return rotated;
}

static u8 rot_r(u8 val){
    u8 rotated = val >> 1;
    rotated |= (gEmulator.cpu.sr &  CARRY) << 7;
    gEmulator.cpu.sr &= ~(CARRY | ZERO | NEGATIVE);
    gEmulator.cpu.sr |= val & CARRY;
    fast_set_ZN( rotated);
    return rotated;
}

static u8 has_page_break(u16 addr1, u16 addr2){
    return (addr1 & 0xFF00) != (addr2 & 0xFF00);
}
