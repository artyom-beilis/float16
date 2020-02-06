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

typedef struct zfunc {
    unsigned short addr;
    long states;
    int calls;
    char const *name;
 } zfunc;

struct DEF_ADDR {
    zfunc add;
    zfunc sub;
    zfunc mul;
    zfunc div;
    zfunc neg;
    zfunc toint;
    zfunc from_int;
    zfunc gt;
    zfunc lt;
    zfunc gte;
    zfunc lte;
    zfunc eq;
    zfunc neq;
} addrs;

#define ADDR(n) else if(strcmp(name,"_f16_" #n "_hl_de") == 0) { addrs.n.addr = addr; addrs.n.name = #n; }

void read_def()
{
    FILE *f=fopen("float_z80.def","rb");
    char line[256];
    char *def,*name, *hex;
    while(fgets(line,sizeof(line),f)) {
        if((def=strtok(line," $=")) && (name=strtok(0," $=")) && (hex=strtok(0," $="))) {
            unsigned short addr = strtol(hex,0,16);
            if(strcmp(name,"_f16_int")==0) {
                addrs.toint.addr = addr;
                addrs.toint.name = "int";
            }
            else if(strcmp(name,"_f16_neg")==0) {
                addrs.neg.addr = addr;
                addrs.neg.name = "neg";
            }
            else if(strcmp(name,"_f16_from_int")==0) {
                addrs.from_int.addr = addr;
                addrs.from_int.name = "from_int";
            }
            ADDR(add)
            ADDR(sub)
            ADDR(mul)
            ADDR(div)
            ADDR(gt)
            ADDR(gte)
            ADDR(lt)
            ADDR(lte)
            ADDR(eq)
            ADDR(neq)
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

void run_programm(zfunc *func,short *hl,short *de,short *bc)
{
    if(print_cpu_state)
        printf("CALL:............................\n");
    unsigned short saddr = func->addr;
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
    func->states+=tstates;
    func->calls++;
}

short run_2s1s(zfunc *func,short a,short b)
{
    short hl=a;
    short de=b;
    short bc=0;
    if(func->addr==0) {
        fprintf(stderr,"Got call for addr 0\n");
        exit(1);
    }
    run_programm(func,&hl,&de,&bc);
    return hl;
}

#define STDFUNC(x) static short zx_##x(short a,short b) { return run_2s1s(&addrs.x,a,b); }
#define CPFUNC(x) static int zx_##x(short a,short b) { return run_2s1s(&addrs.x,a,b); }
STDFUNC(add)
STDFUNC(sub)
STDFUNC(mul)
STDFUNC(div)
CPFUNC(gte)
CPFUNC(lte)
CPFUNC(gt)
CPFUNC(lt)
CPFUNC(eq)
CPFUNC(neq)

static short zx_neg(short a)
{
    return run_2s1s(&addrs.neg,a,1234);
}

static int32_t zx_int(short a)
{
    short hl=a;
    short de=0;
    short bc=0;
    if(addrs.toint.addr==0) {
        fprintf(stderr,"Got call for addr 0\n");
        exit(1);
    }
    run_programm(&addrs.toint,&hl,&de,&bc);
    unsigned res = (unsigned short)hl + 256u*(unsigned short)de;
    return res;
}

static short zx_from_int(int32_t a)
{
    short hl=a & 0xFFFF;
    short de=a >> 16;
    short bc=0;
    if(addrs.from_int.addr==0) {
        fprintf(stderr,"Got call for addr 0\n");
        exit(1);
    }
    run_programm(&addrs.from_int,&hl,&de,&bc);
    return hl;
}


void ratio(zfunc *func,char const *name)
{
    if(func->calls==0)
        return;
    printf("%s %ld Tstats\n",name,func->states/func->calls);
}

void print_stats()
{
    zfunc *f = &addrs.add;
    for(size_t i=0;i<sizeof(addrs)/sizeof(zfunc);i++) {
        ratio(f+i,f[i].name);
    }
}


int main()
{
    F16Comp cmp = {
        .add = zx_add,
        .sub = zx_sub,
        .mul = zx_mul,
        .div = zx_div,
        .neg = zx_neg,
        .to_int = zx_int,
        .from_int = zx_from_int,
        .gte = zx_gte,
        .lte = zx_lte,
        .gt = zx_gt,
        .lt = zx_lt,
        .eq = zx_eq,
        .neq = zx_neq
    };
    init();
    read_def();
    atexit(print_stats);
    //
    // print_cpu_state=1;
    // zx_gte(0xfc00,0xbc00);
    // return 0;
    //
    print_cpu_state=0;
    run_test(&cmp);
    return 0;
}

