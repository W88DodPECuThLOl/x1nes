#include "cpu6502.h"
#include "emulator.h"
// #include "utils.h"

static u16 get_address(c6502* ctx);
static u16 read_abs_address(Memory* mem, u16 offset);
static void set_ZN(c6502* ctx, u8 value);
static void fast_set_ZN(c6502* ctx, u8 value);
static u8 shift_l(c6502* ctx, u8 val);
static u8 shift_r(c6502* ctx, u8 val);
static u8 rot_l(c6502* ctx, u8 val);
static u8 rot_r(c6502* ctx, u8 val);
static void push(c6502* ctx, u8 value);
static void push_address(c6502* ctx, u16 address);
static u8 pop(c6502* ctx);
static u16 pop_address(c6502* ctx);
static void branch(c6502* ctx, u8 mask, u8 predicate);
static void prep_branch(c6502* ctx);
static u8 has_page_break(u16 addr1, u16 addr2);
static void interrupt_(c6502* ctx);
static void poll_interrupt(c6502* ctx);
static void rti(c6502* ctx);

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
    cpu->emulator = emulator;
    cpu->interrupt = NOI;
    set_cpu_mode(cpu, CPU_EXEC); // Normal execution
    cpu->memory = &emulator->mem;
    cpu->sr_started = 0;
    cpu->NMI_hook = nullptr;

    cpu->ac = cpu->x = cpu->y = cpu->state = 0;
    cpu->cycles = cpu->dma_cycles = 0;
    cpu->odd_cycle = 0; //cpu->t_cycles = 0;
    cpu->sr = 0x24;
    cpu->sp = 0xfd;
#if TRACER == 1 && PROFILE == 0
    cpu->pc = 0xC000;
#else
    cpu->pc = read_abs_address(cpu->memory, RESET_ADDRESS);
#endif
}

void reset_cpu(c6502* cpu){
    cpu->sr |= INTERRUPT;
    set_cpu_mode(cpu, CPU_EXEC);; // Normal execution
    cpu->sp -= 3;
    cpu->pc = read_abs_address(cpu->memory, RESET_ADDRESS);
    cpu->cycles = 0;
    cpu->dma_cycles = 0;
    cpu->sr_started = 0;
    cpu->NMI_hook = nullptr;
}

u8 run_cpu_subroutine(c6502* ctx, u16 address) {
    if (ctx->mode & CPU_SR_ANY) {
        // unable to begin subroutine because there is an executing subroutine and/or ISR
        LOG(TRACE, "Unable to start subroutine $%4x, pending subroutine %x",address, ctx->sub_address);
        return CPU_SR;
    }
    ctx->sub_address = address;
    set_cpu_mode(ctx, ctx->mode | CPU_SR);
    LOG(TRACE, "Running subroutine: $%4x",ctx->sub_address);
    return 0;
}

void set_cpu_mode(c6502* ctx, CPUMode mode) {
    ctx->mode = mode;
    LOG(TRACE, "CPU mode: %d",mode);
}

static void interrupt_(c6502* ctx){
    // handle interrupt
    u16 addr;
    u8 set_brk = 0;

    if(ctx->interrupt & BRK_I) {
        // BRK instruction is 2 bytes
        ctx->pc++;
        if(ctx->state & NMI_HIJACK) {
            addr = NMI_ADDRESS;
            ctx->interrupt &= ~NMI;
        }else {
            addr = IRQ_ADDRESS;
        }
        ctx->interrupt &= ~BRK_I;
        // re-apply Break flag
        set_brk = 1;
    }
    else if(ctx->interrupt & NMI) {
        addr = NMI_ADDRESS;
        // NMI is edge triggered so clear it
        ctx->interrupt &= ~NMI;
    }
    else if(ctx->interrupt & RSI){
        addr = RESET_ADDRESS;
        // not used but just in case
        interrupt_clear(ctx, RSI);
    }
    else if(ctx->interrupt & IRQ) {
        addr = IRQ_ADDRESS;
        // ctx->sr |= BREAK;
    } else {
        LOG(ERROR, "No interrupt set");
        return;
    }

    push_address(ctx, ctx->pc);
    // bit 5 always set, bit 4 set on BRK
    push(ctx, ctx->sr & ~(1<<4) | (1<<5) | (set_brk ? (1<<4) : 0));
    ctx->sr |= INTERRUPT;
    ctx->pc = read_abs_address(ctx->memory, addr);
    if (addr == NMI_ADDRESS && ctx->NMI_hook != nullptr) {
        // call NMI hook for phase 0 (pre ISR)
        push_address(ctx, ctx->sub_address);
        ctx->NMI_hook(ctx, 0);
        ctx->sr_started = 0;
        LOG(TRACE, "NMI wrapper at $%4x", ctx->sub_address);
        set_cpu_mode(ctx, ctx->mode | CPU_NMI_SR);
    }
}

static void rti(c6502* ctx) {
    // ignore bit 4 and 5
    ctx->sr &= ((1<<4) | (1<<5));
    ctx->sr |= pop(ctx) & ~((1<<4) | (1<<5));
    ctx->pc = pop_address(ctx);
    // hopefully intrinsic 6502 x-tics will allow this to be enough
    // i.e. no ISR nesting
    if (ctx->mode & CPU_ISR) {
        LOG(TRACE, "ISR completed");
        set_cpu_mode(ctx, ctx->mode & ~CPU_ISR);
    }
}

void interrupt(c6502* ctx, const Interrupt code) {
    if(code == NMI) {
        if(!ctx->NMI_line) {
            ctx->interrupt |= NMI;
        }
        ctx->NMI_line = 1;
    }else {
        ctx->interrupt |= code;
    }
}
void interrupt_clear(c6502* ctx, const Interrupt code) {
    if(code == NMI) ctx->NMI_line = 0;
    else ctx->interrupt &= ~code;
}

void do_DMA(c6502* ctx, u16 cycles) {
    ctx->dma_cycles += cycles;
    ctx->state |= DMA_OCCURRED;
}

static void branch(c6502* ctx, u8 mask, u8 predicate) {
    if(((ctx->sr & mask) > 0) == predicate){
        // increment cycles if branching to a different page
        ctx->cycles += has_page_break(ctx->pc, ctx->address);
        ctx->cycles++;
        // tell the cpu to perform a branch when it is ready
        ctx->state |= BRANCH_STATE;
    }else {
        // remove branch state
        ctx->state &= ~BRANCH_STATE;
    }
}

static void prep_branch(c6502* ctx){
    switch(ctx->instruction->opcode){
        case BCC:
            branch(ctx, CARRY, 0);
            break;
        case BCS:
            branch(ctx, CARRY, 1);
            break;
        case BEQ:
            branch(ctx, ZERO, 1);
            break;
        case BMI:
            branch(ctx, NEGATIVE, 1);
            break;
        case BNE:
            branch(ctx, ZERO, 0);
            break;
        case BPL:
            branch(ctx, NEGATIVE, 0);
            break;
        case BVC:
            branch(ctx, OVERFLW, 0);
            break;
        case BVS:
            branch(ctx, OVERFLW, 1);
            break;
        default:
            ctx->state &= ~BRANCH_STATE;
    }
}

static void poll_interrupt(c6502* ctx) {
    ctx->state |= INTERRUPT_POLLED;
    if(ctx->interrupt & ~IRQ || (ctx->interrupt & IRQ && !(ctx->sr & INTERRUPT))){
        // prepare for interrupts
        ctx->state |= INTERRUPT_PENDING;
        // check if NMI is asserted at poll time
        ctx->state |= ctx->interrupt & NMI ? NMI_ASSERTED : 0;
    }
}

void execute(c6502* ctx){
    ctx->odd_cycle ^= 1;
//    ctx->t_cycles++;
    if (ctx->dma_cycles != 0){
        // DMA CPU suspend
        ctx->dma_cycles--;
        return;
    }
    if(ctx->cycles == 0) {
        if (!ctx->mode) {
            // cpu wait mode
            return;
        }
        // if there is a pending ISR, we wait before we start the subroutine unless it is NMI
        if ((ctx->mode & CPU_NMI_SR || (ctx->mode & (CPU_SR | CPU_ISR)) == CPU_SR) && ctx->sr_started == 0) {
            // we just started cpu subroutine mode
            // let's do a manual JSR to sub_address
            push_address(ctx, ctx->pc);
            push_address(ctx, SENTINEL_ADDR);
            ctx->pc = ctx->sub_address;
            ctx->sr_started = 1;
        }

#if TRACER == 1
        print_cpu_trace(ctx);
#endif
        if(!(ctx->state & INTERRUPT_POLLED)) {
            poll_interrupt(ctx);
        }
        ctx->state &= ~(INTERRUPT_POLLED | NMI_HIJACK | DMA_OCCURRED);
        if(ctx->state & INTERRUPT_PENDING) {
            // takes 7 cycles and this is one of them so only 6 are left
            LOG(TRACE, "Starting ISR: $%2x", ctx->interrupt);
            ctx->cycles = 6;
            set_cpu_mode(ctx, ctx->mode | CPU_ISR);
            return;
        }
        if (!(ctx->mode & CPU_EXEC_ANY)) {
            // busy wait if not in any execution mode
            return;
        }

        u8 opcode = read_mem(ctx->memory, ctx->pc++);
        ctx->instruction = &instructionLookup[opcode];
        ctx->address = get_address(ctx);
        if(ctx->instruction->opcode == BRK) {
            // set BRK interrupt
            ctx->interrupt |= BRK_I;
            ctx->state |= INTERRUPT_PENDING;
            ctx->cycles = 6;
            return;
        }
        ctx->cycles += cycleLookup[opcode];
        // prepare for branching and adjust cycles accordingly
        prep_branch(ctx);
        ctx->cycles--;
        return;
    }

    if(ctx->cycles == 1){
        ctx->cycles--;
        // proceed to execution
    } else{
        // delay execution
        if(ctx->state & INTERRUPT_PENDING && ctx->cycles == 4) {
            if(ctx->interrupt & NMI) ctx->state |= NMI_HIJACK;
        }
        ctx->cycles--;
        return;
    }

    // handle interrupt
    if(ctx->state & INTERRUPT_PENDING) {
        interrupt_(ctx);
        ctx->state &= ~INTERRUPT_PENDING;
        return;
    }

    u16 address = ctx->address;

    switch (ctx->instruction->opcode) {

        // Load and store opcodes

        case LDA:
            ctx->ac = read_mem(ctx->memory, address);
            set_ZN(ctx, ctx->ac);
            break;
        case LDX:
            ctx->x = read_mem(ctx->memory, address);
            set_ZN(ctx, ctx->x);
            break;
        case LDY:
            ctx->y = read_mem(ctx->memory, address);
            set_ZN(ctx, ctx->y);
            break;
        case STA:
            write_mem(ctx->memory, address, ctx->ac);
            break;
        case STY:
            write_mem(ctx->memory, address, ctx->y);
            break;
        case STX:
            write_mem(ctx->memory, address, ctx->x);
            break;

        // Register transfer opcodes

        case TAX:
            ctx->x = ctx->ac;
            set_ZN(ctx, ctx->x);
            break;
        case TAY:
            ctx->y = ctx->ac;
            set_ZN(ctx, ctx->y);
            break;
        case TXA:
            ctx->ac = ctx->x;
            set_ZN(ctx, ctx->ac);
            break;
        case TYA:
            ctx->ac = ctx->y;
            set_ZN(ctx, ctx->ac);
            break;

        // stack opcodes

        case TSX:
            ctx->x = ctx->sp;
            set_ZN(ctx, ctx->x);
            break;
        case TXS:
            ctx->sp = ctx->x;
            break;
        case PHA:
            push(ctx, ctx->ac);
            break;
        case PHP:
            // 6502 quirk bit 4 and 5 are always set by this instruction: see also BRK
            push(ctx, ctx->sr | (1<<4) | (1<<5));
            break;
        case PLA:
            ctx->ac = pop(ctx);
            set_ZN(ctx, ctx->ac);
            break;
        case PLP:
            // poll before updating I flag
            poll_interrupt(ctx);
            ctx->sr = pop(ctx);
            break;

        // logical opcodes

        case AND:
            ctx->ac &= read_mem(ctx->memory, address);
            set_ZN(ctx, ctx->ac);
            break;
        case EOR:
            ctx->ac ^= read_mem(ctx->memory, address);
            set_ZN(ctx, ctx->ac);
            break;
        case ORA:
            ctx->ac |= read_mem(ctx->memory, address);
            set_ZN(ctx, ctx->ac);
            break;
        case BIT: {
            u8 opr = read_mem(ctx->memory, address);
            ctx->sr &= ~(NEGATIVE | OVERFLW | ZERO);
            ctx->sr |= (!(opr & ctx->ac) ? ZERO: 0);
            ctx->sr |= (opr & (NEGATIVE | OVERFLW));
            break;
        }

        // Arithmetic opcodes

        case ADC: {
            u8 opr = read_mem(ctx->memory, address);
            u16 sum = ctx->ac + opr + ((ctx->sr & CARRY) != 0);
            ctx->sr &= ~(CARRY | OVERFLW | NEGATIVE | ZERO);
            ctx->sr |= (sum & 0xFF00 ? CARRY: 0);
            ctx->sr |= ((ctx->ac ^ sum) & (opr ^ sum) & 0x80) ? OVERFLW: 0;
            ctx->ac = sum;
            fast_set_ZN(ctx, ctx->ac);
            break;
        }
        case SBC: {
            u8 opr = read_mem(ctx->memory, address);
            u16 diff = ctx->ac - opr - ((ctx->sr & CARRY) == 0);
            ctx->sr &= ~(CARRY | OVERFLW | NEGATIVE | ZERO);
            ctx->sr |= (!(diff & 0xFF00)) ? CARRY : 0;
            ctx->sr |= ((ctx->ac ^ diff) & (~opr ^ diff) & 0x80) ? OVERFLW: 0;
            ctx->ac = diff;
            fast_set_ZN(ctx, ctx->ac);
            break;
        }
        case CMP: {
            u16 diff = ctx->ac - read_mem(ctx->memory, address);
            ctx->sr &= ~(CARRY | NEGATIVE | ZERO);
            ctx->sr |= !(diff & 0xFF00) ? CARRY: 0;
            fast_set_ZN(ctx, diff);
            break;
        }
        case CPX: {
            u16 diff = ctx->x - read_mem(ctx->memory, address);
            ctx->sr &= ~(CARRY | NEGATIVE | ZERO);
            ctx->sr |= !(diff & 0x100) ? CARRY: 0;
            fast_set_ZN(ctx, diff);
            break;
        }
        case CPY: {
            u16 diff = ctx->y - read_mem(ctx->memory, address);
            ctx->sr &= ~(CARRY | NEGATIVE | ZERO);
            ctx->sr |= !(diff & 0xFF00) ? CARRY: 0;
            fast_set_ZN(ctx, diff);
            break;
        }

        // Increments and decrements opcodes

        case DEC:{
            u8 m = read_mem(ctx->memory, address);
            // dummy write
            write_mem(ctx->memory, address, m--);
            write_mem(ctx->memory, address, m);
            set_ZN(ctx, m);
            break;
        }
        case DEX:
            ctx->x--;
            set_ZN(ctx, ctx->x);
            break;
        case DEY:
            ctx->y--;
            set_ZN(ctx, ctx->y);
            break;
        case INC:{
            u8 m = read_mem(ctx->memory, address);
            // dummy write
            write_mem(ctx->memory, address, m++);
            write_mem(ctx->memory, address, m);
            set_ZN(ctx, m);
            break;
        }
        case INX:
            ctx->x++;
            set_ZN(ctx, ctx->x);
            break;
        case INY:
            ctx->y++;
            set_ZN(ctx, ctx->y);
            break;

        // shifts and rotations opcodes

        case ASL:
            if(ctx->instruction->mode == ACC) {
                ctx->ac = shift_l(ctx, ctx->ac);
            }else{
                u8 m = read_mem(ctx->memory, address);
                // dummy write
                write_mem(ctx->memory, address, m);
                write_mem(ctx->memory, address, shift_l(ctx, m));
            }
            break;
        case LSR:
            if(ctx->instruction->mode == ACC) {
                ctx->ac = shift_r(ctx, ctx->ac);
            }else{
                u8 m = read_mem(ctx->memory, address);
                // dummy write
                write_mem(ctx->memory, address, m);
                write_mem(ctx->memory, address, shift_r(ctx, m));
            }
            break;
        case ROL:
            if(ctx->instruction->mode == ACC){
                ctx->ac = rot_l(ctx, ctx->ac);
            }else{
                u8 m = read_mem(ctx->memory, address);
                // dummy write
                write_mem(ctx->memory, address, m);
                write_mem(ctx->memory, address, rot_l(ctx, m));
            }
            break;
        case ROR:
            if(ctx->instruction->mode == ACC){
                ctx->ac = rot_r(ctx, ctx->ac);
            }else{
                u8 m = read_mem(ctx->memory, address);
                // dummy write
                write_mem(ctx->memory, address, m);
                write_mem(ctx->memory, address, rot_r(ctx, m));
            }
            break;

        // jumps and calls

        case JMP:
            ctx->pc = address;
            ctx->memory->bus = ctx->pc >> 8;
            break;
        case JSR:
            push_address(ctx, ctx->pc - 1);
            ctx->pc = address;
            ctx->memory->bus = ctx->pc >> 8;
            break;
        case RTS:
            ctx->pc = pop_address(ctx) + 1;
            ctx->memory->bus = ctx->pc >> 8;
            // Keep track of external subroutine
            if ((ctx->mode & CPU_SR_ANY) && ctx->pc == SENTINEL_ADDR + 1) {
                ctx->pc = pop_address(ctx);
                ctx->memory->bus = ctx->pc >> 8;
                if (ctx->mode & CPU_NMI_SR) {
                    if (ctx->NMI_hook != nullptr) ctx->NMI_hook(ctx, 1);
                    LOG(TRACE, "NMI wrapper at $%4x ended", ctx->sub_address);
                    ctx->sub_address = pop_address(ctx);
                    set_cpu_mode(ctx, ctx->mode & ~CPU_NMI_SR);
                    if (!(ctx->mode & CPU_SR)) ctx->sr_started = 0;
                    // end NMI
                    rti(ctx);
                }else {
                    LOG(TRACE, "Exiting subroutine $%4x", ctx->sub_address);
                    set_cpu_mode(ctx, ctx->mode & ~CPU_SR);
                    ctx->sr_started = 0;
                }
            }
            break;

        // branching opcodes

        case BCC:case BCS:case BEQ:case BMI:case BNE:case BPL:case BVC:case BVS:
            if(ctx->state & BRANCH_STATE) {
                ctx->pc = ctx->address;
                ctx->state &= ~BRANCH_STATE;
            }
            break;

        // status flag changes

        case CLC:
            ctx->sr &= ~CARRY;
            break;
        case CLD:
            ctx->sr &= ~DECIMAL_;
            break;
        case CLI:
            // poll before updating I flag
            poll_interrupt(ctx);
            ctx->sr &= ~INTERRUPT;
            break;
        case CLV:
            ctx->sr &= ~OVERFLW;
            break;
        case SEC:
            ctx->sr |= CARRY;
            break;
        case SED:
            ctx->sr |= DECIMAL_;
            break;
        case SEI:
            poll_interrupt(ctx);
            ctx->sr |= INTERRUPT;
            break;

        // system functions

        case BRK:
            // Handled in interrupt_()
            break;
        case RTI:
            rti(ctx);
            break;
        case NOP:
            if(ctx->instruction->mode == ABS) {
                // dummy read edge case for unofficial opcode Ox0C
                read_mem(ctx->memory, address);
            }
            break;

        // unofficial

        case ALR:
            // Unstable instruction.
            // A <- A & (const | M), A <- LSR A
            // magic const set to 0x00 but, could also be 0xff, 0xee, ..., 0x11 depending on analog effects
            ctx->ac &= read_mem(ctx->memory, address);
            ctx->ac = shift_r(ctx, ctx->ac);
            break;
        case ANC:
            ctx->ac = ctx->ac & read_mem(ctx->memory, address);
            ctx->sr &= ~(CARRY | ZERO | NEGATIVE);
            ctx->sr |= (ctx->ac & NEGATIVE) ? (CARRY | NEGATIVE): 0;
            ctx->sr |= ((!ctx->ac)? ZERO: 0);
            break;
        case ANE:
            // Unstable instruction.
            // A <- (A | const) & X & M
            // magic const set to 0xff but, could also be 0xee, 0xdd, ..., 0x00 depending on analog effects
            ctx->ac = ctx->x & read_mem(ctx->memory, address);
            set_ZN(ctx, ctx->ac);
            break;
        case ARR: {
            // Unstable instruction.
            // A <- A & (const | M), ROR A
            // magic const set to 0x00 but, could also be 0xff, 0xee, ..., 0x11 depending on analog effects
            u8 val = ctx->ac & read_mem(ctx->memory, address);
            u8 rotated = val >> 1;
            rotated |= (ctx->sr & CARRY) << 7;
            ctx->sr &= ~(CARRY | ZERO | NEGATIVE | OVERFLW);
            ctx->sr |= (rotated & (1<<6)) ? CARRY: 0;
            ctx->sr |= (((rotated & (1<<6)) >> 1) ^ (rotated & (1<<5))) ? OVERFLW: 0;
            fast_set_ZN(ctx, rotated);
            ctx->ac = rotated;
            break;
        }
        case AXS: {
            u8 opr = read_mem(ctx->memory, address);
            ctx->x = ctx->x & ctx->ac;
            u16 diff = ctx->x - opr;
            ctx->sr &= ~(CARRY | NEGATIVE | ZERO);
            ctx->sr |= (!(diff & 0xFF00)) ? CARRY : 0;
            ctx->x = diff;
            fast_set_ZN(ctx, ctx->x);
            break;
        }
        case LAX:
            // Unstable instruction.
            // A,X <- (A | const) & M
            // magic const set to 0xff but, could also be 0xee, 0xdd, ..., 0x00 depending on analog effects
            ctx->ac = read_mem(ctx->memory, address);
            ctx->x = ctx->ac;
            set_ZN(ctx, ctx->ac);
            break;
        case SAX: {
            write_mem(ctx->memory, address, ctx->ac & ctx->x);
            break;
        }
        case DCP: {
            u8 m = read_mem(ctx->memory, address);
            // dummy write
            write_mem(ctx->memory, address, m--);
            write_mem(ctx->memory, address, m);
            u16 diff = ctx->ac - read_mem(ctx->memory, address);
            ctx->sr &= ~(CARRY | NEGATIVE | ZERO);
            ctx->sr |= !(diff & 0xFF00) ? CARRY: 0;
            fast_set_ZN(ctx, diff);
            break;
        }
        case ISB: {
            u8 m = read_mem(ctx->memory, address);
            // dummy write
            write_mem(ctx->memory, address, m++);
            write_mem(ctx->memory, address, m);
            u16 diff = ctx->ac - m - ((ctx->sr & CARRY) == 0);
            ctx->sr &= ~(CARRY | OVERFLW | NEGATIVE | ZERO);
            ctx->sr |= (!(diff & 0xFF00)) ? CARRY : 0;
            ctx->sr |= ((ctx->ac ^ diff) & (~m ^ diff) & 0x80) ? OVERFLW: 0;
            ctx->ac = diff;
            fast_set_ZN(ctx, ctx->ac);
            break;
        }
        case RLA: {
            u8 m = read_mem(ctx->memory, address);
            // dummy write
            write_mem(ctx->memory, address, m);
            m = rot_l(ctx, m);
            write_mem(ctx->memory, address, m);
            ctx->ac &= m;
            set_ZN(ctx, ctx->ac);
            break;
        }
        case RRA: {
            u8 m = read_mem(ctx->memory, address);
            // dummy write
            write_mem(ctx->memory, address, m);
            m = rot_r(ctx, m);
            write_mem(ctx->memory, address, m);
            u16 sum = ctx->ac + m + ((ctx->sr & CARRY) != 0);
            ctx->sr &= ~(CARRY | OVERFLW | NEGATIVE | ZERO);
            ctx->sr |= (sum & 0xFF00 ? CARRY : 0);
            ctx->sr |= ((ctx->ac ^ sum) & (m ^ sum) & 0x80) ? OVERFLW : 0;
            ctx->ac = sum;
            fast_set_ZN(ctx, sum);
            break;
        }
        case SLO: {
            u8 m = read_mem(ctx->memory, address);
            // dummy write
            write_mem(ctx->memory, address, m);
            m = shift_l(ctx, m);
            write_mem(ctx->memory, address, m);
            ctx->ac |= m;
            set_ZN(ctx, ctx->ac);
            break;
        }
        case SRE: {
            u8 m = read_mem(ctx->memory, address);
            // dummy write
            write_mem(ctx->memory, address, m);
            m = shift_r(ctx, m);
            write_mem(ctx->memory, address, m);
            ctx->ac ^= m;
            set_ZN(ctx, ctx->ac);
            break;
        }

        // The next 4 instructions are extremely unstable with weird sometimes inexplicable behavior
        // I have tried to emulate them to the best of my ability
        case SHY: {
            // if RDY line goes low 2 cycles before the value write, the high byte of target address is ignored
            // By target address I mean the unindexed address as read from memory referred to here as raw_address
            // I have equated this behaviour with DMA occurring in the course of the instruction
            // This also applies to SHX, SHA and SHS
            const u8 val = ctx->state & DMA_OCCURRED ? ctx->y: ctx->y & ((ctx->raw_address >> 8) + 1);
            if (has_page_break(ctx->raw_address, address))
                // Instruction unstable, corrupt high byte of address
                address = (address & 0xff) | (val << 8);
            write_mem(ctx->memory, address, val);
            break;
        }
        case SHX: {
            const u8 val = ctx->state & DMA_OCCURRED ? ctx->x: ctx->x & ((ctx->raw_address >> 8) + 1);
            if (has_page_break(ctx->raw_address, address))
                // Instruction unstable, corrupt high byte of address
                address = (address & 0xff) | (val << 8);
            write_mem(ctx->memory, address, val);
            break;
        }
        case SHA: {
            const u8 val = ctx->state & DMA_OCCURRED ? ctx->x & ctx->ac: ctx->x & ctx->ac & ((ctx->raw_address >> 8) + 1);
            if (has_page_break(ctx->raw_address, address))
                // Instruction unstable, corrupt high byte of address
                address = (address & 0xff) | (val << 8);
            write_mem(ctx->memory, address, val);
            break;
        }
        case SHS:
            ctx->sp = ctx->ac & ctx->x;
            const u8 val = ctx->state & DMA_OCCURRED ? ctx->x & ctx->ac: ctx->x & ctx->ac & ((ctx->raw_address >> 8) + 1);
            if (has_page_break(ctx->raw_address, address))
                // Instruction unstable, corrupt high byte of address
                address = (address & 0xff) | (val << 8);
            write_mem(ctx->memory, address, val);
            break;
        case LAS:
            ctx->ac = ctx->x = ctx->sp = read_mem(ctx->memory, address) & ctx->sp;;
            set_ZN(ctx, ctx->ac);
            break;
        default:
            break;
    }
}

static u16 read_abs_address(Memory* mem, u16 offset){
    // 16-bit address is little endian so read lo then hi
    u16 lo = (u16)read_mem(mem, offset);
    u16 hi = (u16)read_mem(mem, offset + 1);
    return (hi << 8) | lo;
}

static u16 get_address(c6502* ctx){
    u16 addr, hi,lo;
    switch (ctx->instruction->mode) {
        case IMPL:
        case ACC:
            // dummy read
            read_mem(ctx->memory, ctx->pc);
        case NONE:
            ctx->raw_address = 0;
            return 0;
        case REL: {
            s8 offset = (s8)read_mem(ctx->memory, ctx->pc++);
            addr = ctx->raw_address = ctx->pc + offset;
            return addr;
        }
        case IMT:
            ctx->raw_address = ctx->pc + 1;
            return ctx->pc++;
        case ZPG:
            ctx->raw_address = ctx->pc + 1;
            return read_mem(ctx->memory, ctx->pc++);
        case ZPG_X:
            addr = ctx->raw_address = ctx->raw_address = read_mem(ctx->memory, ctx->pc++);
            return (addr + ctx->x) & 0xFF;
        case ZPG_Y:
            addr = ctx->raw_address = read_mem(ctx->memory, ctx->pc++);
            return (addr + ctx->y) & 0xFF;
        case ABS:
            addr = ctx->raw_address = read_abs_address(ctx->memory, ctx->pc);
            ctx->pc += 2;
            return addr;
        case ABS_X:
            addr = ctx->raw_address = read_abs_address(ctx->memory, ctx->pc);
            ctx->pc += 2;
            switch (ctx->instruction->opcode) {
                // these don't take into account absolute x page breaks
                case STA:case ASL:case DEC:case INC:case LSR:case ROL:case ROR:
                // unofficial
                case SLO:case RLA:case SRE:case RRA:case DCP:case ISB: case SHY:
                    // invalid read
                    read_mem(ctx->memory, (addr & 0xff00) | ((addr + ctx->x) & 0xff));
                    break;
                default:
                    if(has_page_break(addr, addr + ctx->x)) {
                        // invalid read
                        read_mem(ctx->memory, (addr & 0xff00) | ((addr + ctx->x) & 0xff));
                        ctx->cycles++;
                    }
            }
            return addr + ctx->x;
        case ABS_Y:
            addr = ctx->raw_address = read_abs_address(ctx->memory, ctx->pc);
            ctx->pc += 2;
            switch (ctx->instruction->opcode) {
                case STA:case SLO:case RLA:case SRE:case RRA:case DCP:case ISB: case NOP:
                case SHX:case SHA:case SHS:
                    // invalid read
                    read_mem(ctx->memory, (addr & 0xff00) | ((addr + ctx->y) & 0xff));
                    break;
                default:
                    if(has_page_break(addr, addr + ctx->y)) {
                        // invalid read
                        read_mem(ctx->memory, (addr & 0xff00) | ((addr + ctx->y) & 0xff));
                        ctx->cycles++;
                    }
            }
            return addr + ctx->y;
        case IND:
            addr = ctx->raw_address = read_abs_address(ctx->memory, ctx->pc);
            ctx->pc += 2;
            lo = read_mem(ctx->memory, addr);
            // handle a bug in 6502 hardware where if reading from $xxFF (page boundary) the
            // LSB is read from $xxFF as expected but the MSB is fetched from xx00
            hi = read_mem(ctx->memory, (addr & 0xFF00) | (addr + 1) & 0xFF);
            return (hi << 8) | lo;
        case IDX_IND:
            addr = (read_mem(ctx->memory, ctx->pc++) + ctx->x) & 0xFF;
            lo = read_mem(ctx->memory, addr & 0xFF);
            hi = read_mem(ctx->memory, (addr + 1) & 0xFF);
            addr = ctx->raw_address = (hi << 8) | lo;
            return addr;
        case IND_IDX:
            addr = read_mem(ctx->memory, ctx->pc++);
            lo = read_mem(ctx->memory, addr & 0xFF);
            hi = read_mem(ctx->memory, (addr + 1) & 0xFF);
            addr = ctx->raw_address = (hi << 8) | lo;
            switch (ctx->instruction->opcode) {
                case STA:case SLO:case RLA:case SRE:case RRA:case DCP:case ISB: case NOP:
                case SHA:
                    // invalid read
                    read_mem(ctx->memory, (addr & 0xff00) | ((addr + ctx->y) & 0xff));
                    break;
                default:
                    if(has_page_break(addr, addr + ctx->y)) {
                        // invalid read
                        read_mem(ctx->memory, (addr & 0xff00) | ((addr + ctx->y) & 0xff));
                        ctx->cycles++;
                    }
            }
            return addr + ctx->y;
    }
    return 0;
}

static void set_ZN(c6502* ctx, u8 value){
    ctx->sr &= ~(NEGATIVE | ZERO);
    ctx->sr |= ((!value)? ZERO: 0);
    ctx->sr |= (value & NEGATIVE);
}

static void fast_set_ZN(c6502* ctx, u8 value){
    // this assumes the necessary flags (Z & N) have been cleared
    ctx->sr |= ((!value)? ZERO: 0);
    ctx->sr |= (value & NEGATIVE);
}

static void push(c6502* ctx, u8 value){
    write_mem(ctx->memory, STACK_START + ctx->sp--, value);
}

static void push_address(c6502* ctx, u16 address){
    write_mem(ctx->memory, STACK_START + ctx->sp--, address >> 8);
    write_mem(ctx->memory, STACK_START + ctx->sp--, address & 0xFF);
}

static u8 pop(c6502* ctx){
    return read_mem(ctx->memory, STACK_START + ++ctx->sp);
}

static u16 pop_address(c6502* ctx){
    u16 addr = read_mem(ctx->memory, STACK_START + ++ctx->sp);
    return addr | ((u16)read_mem(ctx->memory, STACK_START + ++ctx->sp)) << 8;
}

static u8 shift_l(c6502* ctx, u8 val){
    ctx->sr &= ~(CARRY | ZERO | NEGATIVE);
    ctx->sr |= (val & NEGATIVE) ? CARRY: 0;
    val <<= 1;
    fast_set_ZN(ctx, val);
    return val;
}

static u8 shift_r(c6502* ctx, u8 val){
    ctx->sr &= ~(CARRY | ZERO | NEGATIVE);
    ctx->sr |= (val & 0x1) ? CARRY: 0;
    val >>= 1;
    fast_set_ZN(ctx, val);
    return val;
}

static u8 rot_l(c6502* ctx, u8 val){
    u8 rotated = val << 1;
    rotated |= ctx->sr & CARRY;
    ctx->sr &= ~(CARRY | ZERO | NEGATIVE);
    ctx->sr |= val & NEGATIVE ? CARRY: 0;
    fast_set_ZN(ctx, rotated);
    return rotated;
}

static u8 rot_r(c6502* ctx, u8 val){
    u8 rotated = val >> 1;
    rotated |= (ctx->sr &  CARRY) << 7;
    ctx->sr &= ~(CARRY | ZERO | NEGATIVE);
    ctx->sr |= val & CARRY;
    fast_set_ZN(ctx, rotated);
    return rotated;
}

static u8 has_page_break(u16 addr1, u16 addr2){
    return (addr1 & 0xFF00) != (addr2 & 0xFF00);
}
