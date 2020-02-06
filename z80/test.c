#include "float16.h"
#include "comp.h"
#include <z80ex/z80ex.h>
#include <z80ex/z80ex_dasm.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned char memory[65536];
static Z80EX_CONTEXT *ctx; 
static int print_cpu_state;

static Z80EX_BYTE pread(Z80EX_CONTEXT *cpu, Z80EX_WORD port, void *user_data)
{
    return 0;
}
static void pwrite(Z80EX_CONTEXT *cpu, Z80EX_WORD port, Z80EX_BYTE val, void *user_data)
{
}

static Z80EX_BYTE mread(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, int m1_state, void *user_data)
{
    return memory[addr];
}

static void mwrite(Z80EX_CONTEXT *cpu, Z80EX_WORD addr, Z80EX_BYTE value, void *user_data)
{
    memory[addr]=value;
}

static Z80EX_BYTE intread(Z80EX_CONTEXT *cpu, void *user_data)
{
    return 0;
}

struct DEF_ADDR {
    unsigned short add;
    long add_states;
    int add_calls;
    unsigned short sub;
    long sub_states;
    int sub_calls;
    unsigned short mul;
    long mul_states;
    int mul_calls;
    unsigned short div;
    long div_states;
    int div_calls;
} addrs;

void read_def()
{
    FILE *f=fopen("float_z80.def","rb");
    char line[256];
    char *def,*name, *hex;
    while(fgets(line,sizeof(line),f)) {
        if((def=strtok(line," $=")) && (name=strtok(0," $=")) && (hex=strtok(0," $="))) {
            unsigned short addr = strtol(hex,0,16);
            if(strcmp(name,"_f16_add_hl_de")==0)
                addrs.add = addr;
            else if(strcmp(name,"_f16_sub_hl_de")==0)
                addrs.sub = addr;
            else if(strcmp(name,"_f16_mul_hl_de")==0)
                addrs.mul = addr;
            else if(strcmp(name,"_f16_div_hl_de")==0)
                addrs.div = addr;
            else
                continue;
            printf("%s=%x\n",name,addr);
        }
    }
    fclose(f);
}

void init()
{
    FILE *f=fopen("float_z80.bin","rb");
    if(!f) {
        perror("fopen");
        exit(1);
    }
    if(fread(memory+32768,1,32768,f) <= 0) {
        perror("Failed reading code\n");
        exit(1);
    }
    fclose(f);
    ctx = z80ex_create(mread,NULL,mwrite,NULL,pread,NULL,pwrite,NULL,intread,NULL);
    if(!ctx) {
        fprintf(stderr,"Failed to create contex!\n");
        exit(1);
    }
}

unsigned short read_word(int addr)
{
    return memory[addr&0xFFFF] + 256*memory[(addr+1)&0xFFFF];
}

void print_regs()
{
    printf("AF:%04x   AF':%04x\n",z80ex_get_reg(ctx,regAF),z80ex_get_reg(ctx,regAF_));
    printf("HL:%04x   HL':%04x\n",z80ex_get_reg(ctx,regHL),z80ex_get_reg(ctx,regHL_));
    printf("DE:%04x   DE':%04x\n",z80ex_get_reg(ctx,regDE),z80ex_get_reg(ctx,regDE_));
    printf("BC:%04x   BC':%04x\n",z80ex_get_reg(ctx,regBC),z80ex_get_reg(ctx,regBC_));
    printf("IX:%04x   IY :%04x  SP: %04x  PC: %04x\n",z80ex_get_reg(ctx,regIX),z80ex_get_reg(ctx,regIY),z80ex_get_reg(ctx,regSP),z80ex_get_reg(ctx,regPC));
    int sp = z80ex_get_reg(ctx,regSP);
    printf("S0:%04x   S1 :%04x  S2: %04x  S3: %04x\n",read_word(sp),read_word(sp+2),read_word(sp+4),read_word(sp+6));
}

Z80EX_BYTE dasm_read(Z80EX_WORD addr, void *user_data)
{
    return memory[addr];
}

void print_dasm()
{
    char buf[256];
    int t1,t2;
    int pc=z80ex_get_reg(ctx,regPC);
    z80ex_dasm(buf,sizeof(buf),0,&t1,&t2,dasm_read,pc,NULL);
    printf("%04x: %s\n",pc,buf);
}

int run_programm(int addr,short *hl,short *de,short *bc)
{
    if(print_cpu_state)
        printf("CALL:............................\n");
    unsigned short saddr = addr;
    z80ex_set_reg(ctx,regHL,*hl);
    z80ex_set_reg(ctx,regDE,*de);
    z80ex_set_reg(ctx,regBC,*bc);
    z80ex_set_reg(ctx,regSP,0xFFF0);
    z80ex_set_reg(ctx,regPC,0);
    memory[0]=0xCD;
    memcpy(memory+1,&saddr,2);
    memory[3]=0;
    int tstates = 0;
    int limit = 1000000;
    if(print_cpu_state)
        print_regs();
    while(z80ex_get_reg(ctx,regPC)!=3 || tstates > limit) {
        if(print_cpu_state)
            print_dasm();
        do {
            tstates+=z80ex_step(ctx);
        } while(z80ex_last_op_type(ctx)!=0 && tstates <= limit);
        if(print_cpu_state)
            print_regs();
    }
    if(tstates > limit) {
        fprintf(stderr,"Looks like a loop, pc=%x\n",z80ex_get_reg(ctx,regPC));
        exit(1);
    }
    *hl=z80ex_get_reg(ctx,regHL);
    *de=z80ex_get_reg(ctx,regDE);
    *bc=z80ex_get_reg(ctx,regDE);
    return tstates;
}

short run_2s1s(int addr,short a,short b,long *states)
{
    short hl=a;
    short de=b;
    short bc=0;
    if(addr==0) {
        fprintf(stderr,"Got call for addr 0\n");
        exit(1);
    }
    *states += run_programm(addr,&hl,&de,&bc);
    return hl;
}

static short zx_add(short a,short b)
{
    addrs.add_calls++;
    return run_2s1s(addrs.add,a,b,&addrs.add_states);
}
static short zx_sub(short a,short b)
{
    addrs.sub_calls++;
    return run_2s1s(addrs.sub,a,b,&addrs.sub_states);
}
static short zx_mul(short a,short b)
{
    addrs.mul_calls++;
    return run_2s1s(addrs.mul,a,b,&addrs.mul_states);
}
static short zx_div(short a,short b)
{
    addrs.div_calls++;
    return run_2s1s(addrs.div,a,b,&addrs.div_states);
}

int ratio(long states,int calls)
{
    if(calls == 0)
        return 0;
    return (int)(states/calls);
}

void print_stats()
{
    printf("add %d T states\n",ratio(addrs.add_states,addrs.add_calls));
    printf("sub %d T states\n",ratio(addrs.sub_states,addrs.sub_calls));
    printf("mul %d T states\n",ratio(addrs.mul_states,addrs.mul_calls));
    printf("div %d T states\n",ratio(addrs.div_states,addrs.div_calls));
}

int main()
{
    F16Comp cmp = {
        .add = zx_add,
        .sub = zx_sub,
        .mul = zx_mul,
        .div = zx_div,
        .neg = f16_neg,
        .to_int = f16_int,
        .from_int = f16_from_int,
        .gte = f16_gte,
        .lte = f16_lte,
        .gt = f16_gt,
        .lt = f16_lt,
        .eq = f16_eq,
        .neq = f16_neq
    };
    init();
    read_def();
    atexit(print_stats);
    //
    // print_cpu_state=1;
    // zx_mul(0xFC00,0x8001);
    // return 0;
    //
    run_test(&cmp);
    return 0;
}

