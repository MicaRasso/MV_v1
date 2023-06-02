#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct
{
    char op, registro, segmento, celda, nombrevmi[120];
    short int posmem, offset, RAM;
    int valor, inst, breakpoint;
} datos;

typedef struct
{
    short int base, size;
} TRTDS;

typedef char texto[5];

typedef char nombreArch[120];

typedef struct
{
    nombreArch ruta; //"disco.vdd"
    char nUnidad;
    unsigned int cCilindros, cCabezas, cSectores, tamSector;
} TRD;

typedef void (*t_dis) (datos V[], int REG[], TRTDS TDS[], texto registro[16][4]);

typedef void (*t_func)(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos);

void cargaMatriz(texto registro[16][4])
{
    strcpy(registro[0][0],"cs");
    strcpy(registro[1][0],"ds");
    strcpy(registro[2][0],"ks");
    strcpy(registro[3][0],"es");
    strcpy(registro[4][0],"ss");
    strcpy(registro[5][0],"ip");
    strcpy(registro[6][0],"sp");
    strcpy(registro[7][0],"bp");
    strcpy(registro[8][0],"cc");
    strcpy(registro[9][0],"ac");
    strcpy(registro[10][0],"eax");
    strcpy(registro[10][1],"al");
    strcpy(registro[10][2],"ah");
    strcpy(registro[10][3],"ax");
    strcpy(registro[11][0],"ebx");
    strcpy(registro[11][1],"bl");
    strcpy(registro[11][2],"bh");
    strcpy(registro[11][3],"bx");
    strcpy(registro[12][0],"ecx");
    strcpy(registro[12][1],"cl");
    strcpy(registro[12][2],"ch");
    strcpy(registro[12][3],"cx");
    strcpy(registro[13][0],"edx");
    strcpy(registro[13][1],"dl");
    strcpy(registro[13][2],"dh");
    strcpy(registro[13][3],"dx");
    strcpy(registro[14][0],"eex");
    strcpy(registro[14][1],"el");
    strcpy(registro[14][2],"eh");
    strcpy(registro[14][3],"ex");
    strcpy(registro[15][0],"efx");
    strcpy(registro[15][1],"fl");
    strcpy(registro[15][2],"fh");
    strcpy(registro[15][3],"fx");
}

void cargaNombres(texto funciones[])
{
    strcpy(funciones[0], "MOV ");
    strcpy(funciones[1], "ADD ");
    strcpy(funciones[2], "SUB ");
    strcpy(funciones[3], "SWAP");
    strcpy(funciones[4], "MUL ");
    strcpy(funciones[5], "DIV ");
    strcpy(funciones[6], "CMP ");
    strcpy(funciones[7], "SHL ");
    strcpy(funciones[8], "SHR ");
    strcpy(funciones[9], "AND ");
    strcpy(funciones[10], "OR  ");
    strcpy(funciones[11], "XOR ");
    strcpy(funciones[48], "SYS ");
    strcpy(funciones[49], "JMP ");
    strcpy(funciones[50], "JZ  ");
    strcpy(funciones[51], "JP  ");
    strcpy(funciones[52], "JN  ");
    strcpy(funciones[53], "JNZ ");
    strcpy(funciones[54], "JNP ");
    strcpy(funciones[55], "JNN ");
    strcpy(funciones[56], "LDL ");
    strcpy(funciones[57], "LDH ");
    strcpy(funciones[58], "RND ");
    strcpy(funciones[59], "NOT ");
    strcpy(funciones[60], "PUSH");
    strcpy(funciones[61], "POP ");
    strcpy(funciones[62], "CALL");
    strcpy(funciones[240], "STOP");
    strcpy(funciones[241], "RET ");
}

short int base(int seg, TRTDS TDS[], int REG[])
{
    return TDS[seg>>16].base;
}

short int size(int seg, TRTDS TDS[], int REG[])
{
       return TDS[seg>>16].size;
}

void escribeArch(int dato, int tam, FILE *arch)
{
    char aux;
    int i, j = tam;

    if(arch)
        for (i = 0 ; i < tam ; i++)
        {
            aux = dato>>8*(--j);
            fwrite(&aux, sizeof(char), 1, arch);
        }
}

void completaArch(FILE* arch, short int posArch, short int tamArch, char al)
{
    int i = 1 , tamInicial = 0;

    fseek(arch, 0, SEEK_END); //puede que lo eliminemos
    tamInicial = ftell(arch);
    if(tamInicial < posArch)  //escritura hasta llegar a posArch
        escribeArch(0, (posArch-tamInicial), arch);
    else
        i += (tamInicial + posArch) / 512;
    while(i <= al && (posArch+i*512 <= tamArch))
    {
        escribeArch(0, 512, arch);
        i++;
    }
}

void iniciaRegistros(int REG[], int valoresTDS[], TRTDS TDS[])
{
    REG[0] = valoresTDS[0];   //CS
    REG[1] = valoresTDS[2];   //DS
    REG[2] = valoresTDS[1];   //KS
    REG[3] = valoresTDS[3];   //ES
    REG[4] = valoresTDS[4];   //SS
    REG[5] = 0;               //IP
    if(REG[4] == -1)          //SP
        REG[6] = -1;
    else{
        REG[6] = base(REG[4], TDS, REG) + size(REG[4], TDS, REG) + 1;
        REG[6] |= REG[4];
    }
    REG[7] = 0;               //BP
    REG[8] = 0;               //CC
    REG[9] = 0;               //AC
    REG[10] = 0;              //A
    REG[11] = 0;              //B
    REG[12] = 0;              //C
    REG[13] = 0;              //D
    REG[14] = 0;              //E
    REG[15] = 0;              //F
}

void iniciaTablaDeSegmentos(TRTDS TDS[], int REG[], int RAM, int indice[])
{
    int i, j = 1, tamActual = 0;

    for(i = 1 ; i < 8 ; i++)
        TDS[i].base = -1;
    TDS[0].base = 0;
    TDS[0].size = indice[0];
    tamActual = indice[0];
    indice[0] = 0;
    for (i = 1 ; i < 5 ; i++)
    {
        if(indice[i] < 0)
            indice[i] = 1024;
        if(tamActual + indice[i] <= RAM)
            if (indice[i] != 0)
            {
                TDS[j].base = TDS[j-1].base + TDS[j-1].size;
                TDS[j].size = indice[i];
                tamActual += indice[i];
                indice[i] = j++;
                indice[i] = indice[i]<<16;
            }
            else
                indice[i] = -1;
        else
        {
            printf("Memoria insuficiente\n");
            exit(1);
        }
    }
    iniciaRegistros(REG, indice, TDS);
}

int corrimiento (int aux, int izq, int der)
{
    aux = (aux<<8)>>8;
    return (aux<<izq)>>der;
}

void mascaras(int *valor, char tipo)
{
    switch(tipo)
    {
        case 0: //byte 4
            (*valor) &= 0xFF;
        break;
        case 1: //bytes 4 y 3
            (*valor) &= 0xFFFF;
        break;
        case 2: //bytes 4, 3 y 2
            (*valor) &= 0xFFFFFF;
        break;
        case 3: //bytes 4, 3, 2 y 1
            (*valor) &= 0xFFFFFFFF;
        break;
        case 4: //byte 3 - EXTRA
            (*valor) &= 0xFF00;
        break;
    }
}

void lee2byte(short int *var, char MEM[], int REG[], TRTDS TDS[])
{
    int i, aux = 0, ds = 1;
    *var = 0;
    for(i = 1 ; i >= 0 ; i--)
    {
        if(REG[5] > base(REG[ds], TDS, REG))
        {
            printf("Fallo de segmento\n");
            exit(1);
        }
        aux = MEM[REG[5]++];
        mascaras(&aux, 0);
        *var |= aux<<8*i;
    }
}

void lee1byte(char *var, char MEM[], int REG[], TRTDS TDS[])
{
    int aux = 0, ds = 1;
    if(REG[5] > base(REG[ds], TDS, REG))
    {
        printf("Fallo de segmento\n");
        exit(1);
    }
    aux = MEM[REG[5]++];
    mascaras(&aux, 0);
    *var = aux;
}

void lee2bytedis(short int *var, char MEM[], int REG[], TRTDS TDS[])
{
    int i, aux = 0;

    *var = 0;
    for(i = 1 ; i >= 0 ; i--)
    {
        aux = MEM[REG[5]++];
        mascaras(&aux, 0);
        *var |= aux<<8*i;
    }
}

void lee1bytedis(char *var, char MEM[], int REG[], TRTDS TDS[])
{
    int aux = 0;

    aux = MEM[REG[5]++];
    mascaras(&aux, 0);
    *var = aux;
}

void escribeenmem(datos V[], int i, int aux, char MEM[])
{
    switch (V[i].celda)
    {
        case 0:
            MEM[V[i].posmem + V[i].offset] = (aux>>24)&0xFF;
            MEM[V[i].posmem + V[i].offset+1] = (aux>>16)&0xFF;
            MEM[V[i].posmem + V[i].offset+2] = (aux>>8)&0xFF;
            MEM[V[i].posmem + V[i].offset+3] = aux&0xFF;
        break;
        case 2:
            MEM[V[i].posmem+V[i].offset] = (aux>>8)&0xFF;
            MEM[V[i].posmem+V[i].offset+1] = aux&0xFF;
        break;
        case 3:
            MEM[V[i].posmem+V[i].offset] = aux&0xFF;
        break;
    }
}

void leedemem(datos V[], int k, int seg, char MEM[], int REG[], TRTDS TDS[])
{
    int i, j = 3, l = 0, aux2 = 0, aux = 0;
    switch(V[k].celda)
    {
        case 0:
            j = 3;
            l = 4;
        break;
        case 2:
            j = 1;
            l = 2;
        break;
        case 3:
            j = 0;
            l = 1;
        break;
    }
    for(i = 0 ; i < l ; i++)
    {

        if(seg == 1 && V[k].posmem + V[k].offset + j < base(REG[seg], TDS, REG))
        {
            printf("Fallo de segmento\n");
            exit(1);
        }
        else
        {
            if(seg == 4 && V[k].posmem + V[k].offset + j < base(REG[seg], TDS, REG))
            {
                printf("Stack underflow\n");
                exit(1);
            }
            else
            {
                aux = MEM[V[k].posmem+V[k].offset+j];
                aux = aux<<(8*i);
                mascaras(&aux, i);
                aux2 |= aux;
                j--;
            }
        }
    }
    switch(V[k].celda)
    {
        case 0: //4 byte
            V[k].valor = aux2;
        break;
        case 2: //2 byte
            V[k].valor = corrimiento(aux2, 16, 16);
        break;
        case 3: //1 byte
            V[k].valor = corrimiento(aux2, 24, 24);
        break;
    }
}

void leedememdis(datos V[], int k, int seg, char MEM[], int REG[], TRTDS TDS[])
{
    int i, j = 3, l = 0, aux2 = 0, aux = 0;
    switch(V[k].celda)
    {
        case 0:
            j = 3;
            l = 4;
        break;
        case 2:
            j = 1;
            l = 2;
        break;
        case 3:
            j = 0;
            l = 1;
        break;
    }
    for(i = 0 ; i < l ; i++)
    {
        aux = MEM[V[k].posmem+V[k].offset+j];
        aux = aux<<(8*i);
        mascaras(&aux, i);
        aux2 |= aux;
        j--;
    }
    V[k].valor = aux2;
}

void leemem(datos V[], int i, char MEM[], int REG[], TRTDS TDS[])
{
    char segreg = 0;
    short int offset = 0;

    lee1byte(&segreg, MEM, REG, TDS);
    V[i].celda = segreg>>6;
    V[i].celda &= 0x3;
    V[i].segmento = (segreg>>4)&0x3; // siempre = 0;
    V[i].registro = segreg&0xF;

    lee2byte(&offset, MEM, REG, TDS);
    V[i].offset = offset;
    V[i].posmem = base(REG[V[i].registro], TDS, REG) + (REG[V[i].registro]&0xFFFF);
    leedemem(V, i, V[i].registro, MEM, REG, TDS);
}

void leeinm(datos V[], int i, char MEM[], int REG[], TRTDS TDS[])
{
    short int inmediato = 0;
    int aux = 0;

    lee2byte(&inmediato, MEM, REG, TDS);
    aux = inmediato;
    V[i].valor = corrimiento(aux, 16, 16);
}

void leereg(datos V[], int i, char MEM[], int REG[], TRTDS TDS[])
{
    char segreg = 0;

    lee1byte(&segreg, MEM, REG, TDS);
    V[i].segmento = segreg>>4;
    V[i].registro = segreg&0xF;
    switch (V[i].segmento)
    {
        case 0:
            V[i].valor = REG[V[i].registro];
        break;
        case 1:
            V[i].valor = corrimiento(REG[V[i].registro], 24, 24);
        break;
        case 2:
            V[i].valor = corrimiento(REG[V[i].registro], 16, 24);
        break;
        case 3:
            V[i].valor = corrimiento(REG[V[i].registro], 16, 16);
        break;
    }
}

void leememdis(datos V[], int i, char MEM[], int REG[], TRTDS TDS[])
{
    char segreg = 0;
    short int offset = 0;

    lee1bytedis(&segreg, MEM, REG, TDS);
    V[i].celda = segreg>>6;
    V[i].celda &= 0x3;
    V[i].segmento = (segreg>>4)&0x3; // siempre = 0;
    V[i].registro = segreg&0xF;

    lee2bytedis(&offset, MEM, REG, TDS);
    V[i].offset = offset;
    V[i].posmem = base(REG[V[i].registro], TDS, REG) + (REG[V[i].registro]&0xFFFF);
    leedememdis(V, i, V[i].registro, MEM, REG, TDS);
}

void leeinmdis(datos V[], int i, char MEM[], int REG[], TRTDS TDS[])
{
    short int inmediato = 0;
    int aux = 0;

    lee2bytedis(&inmediato, MEM, REG, TDS);
    aux = inmediato;
    V[i].valor = corrimiento(aux, 16, 16);
}

void leeregdis(datos V[], int i, char MEM[], int REG[], TRTDS TDS[])
{
    char segreg = 0;

    lee1bytedis(&segreg, MEM, REG, TDS);
    V[i].segmento = segreg>>4;
    V[i].registro = segreg&0xF;
    switch (V[i].segmento)
    {
        case 0:
            V[i].valor = REG[V[i].registro];
        break;
        case 1:
            V[i].valor = corrimiento(REG[V[i].registro], 24, 24);
        break;
        case 2:
            V[i].valor = corrimiento(REG[V[i].registro], 16, 24);
        break;
        case 3:
            V[i].valor = corrimiento(REG[V[i].registro], 16, 16);
        break;
    }
}

void lectura(char MEM[], int REG[], TRTDS TDS[], int RAM, char DirArchivo[], char infoMV[], int indice[])
{
    FILE *arch;
    char encabezado[6], version, c, str[6];
    arch = fopen(DirArchivo, "rb");
    int i, aux = 0, TAMCS = 0, TAMKS = 0, TAMDS = 0, TAMES = 0, TAMSS = 0;

    if(arch != NULL)
    {
        fread(encabezado, sizeof(char), 5, arch);
        encabezado[5] = '\0';
        fread(&version, sizeof(char), 1, arch);

        //lectura de tamaño CS
        fread(&c, sizeof(char), 1, arch);
        aux = c;
        mascaras(&aux, 0);
        aux = (aux<<8);
        TAMCS |= aux;

        fread(&c, sizeof(char), 1, arch);
        aux = c;
        mascaras(&aux, 0);
        TAMCS |= aux;

        //lectura de tamaño KS
        fread(&c, sizeof(char), 1, arch);
        aux = c;
        mascaras(&aux, 0);
        aux = (aux<<8);
        TAMKS |= aux;

        fread(&c, sizeof(char), 1, arch);
        aux = c;
        mascaras(&aux, 0);
        TAMKS |= aux;

        //lectura de tamaño DS
        fread(&c, sizeof(char), 1, arch);
        aux = c;
        mascaras(&aux, 0);
        aux = (aux<<8);
        TAMDS |= aux;

        fread(&c, sizeof(char), 1, arch);
        aux = c;
        mascaras(&aux, 0);
        TAMDS |= aux;

        //lectura de tamaño ES
        fread(&c, sizeof(char), 1, arch);
        aux = c;
        mascaras(&aux, 0);
        aux = (aux<<8);
        TAMES |= aux;

        fread(&c, sizeof(char), 1, arch);
        aux = c;
        mascaras(&aux, 0);
        TAMES |= aux;

        //lectura de tamaño SS
        fread(&c, sizeof(char), 1, arch);
        aux = c;
        mascaras(&aux, 0);
        aux = (aux<<8);
        TAMSS |= aux;

        fread(&c, sizeof(char), 1, arch);
        aux = c;
        mascaras(&aux, 0);
        TAMSS |= aux;

        //lectura y carga en memoria CS
        for (i = 0 ; i < TAMCS ; i++)
        {
            fread(&c, sizeof(char), 1, arch);
            MEM[i] = c;
        }
        //lectura y carga en memoria KS
        for (i = TAMCS ; i < TAMCS+TAMKS ; i++)
        {
            fread(&c, sizeof(char), 1, arch);
            MEM[i] = c;
        }

        fclose(arch);
        strcpy(infoMV,encabezado);
        strcat(infoMV," VER: ");
        version += 48;
        infoMV[11] = version;
        infoMV[12] = '\0';
        strcat(infoMV," TAMCS: ");
        sprintf(str, "%d", TAMCS);
        strcat(infoMV, str);
        strcat(infoMV," TAMKS: ");
        sprintf(str, "%d", TAMKS);
        strcat(infoMV, str);
        strcat(infoMV," TAMDS: ");
        sprintf(str, "%d", TAMDS);
        strcat(infoMV, str);
        strcat(infoMV," TAMES: ");
        sprintf(str, "%d", TAMES);
        strcat(infoMV, str);
        strcat(infoMV," TAMSS: ");
        sprintf(str, "%d", TAMSS);
        strcat(infoMV, str);
        indice[0]= TAMCS;
        indice[1]= TAMKS;
        indice[2]= TAMDS;
        indice[3]= TAMES;
        indice[4]= TAMSS;
        iniciaTablaDeSegmentos(TDS, REG, RAM, indice);
    }
    else
    {
        printf("No se pudo abrir el archivo\n");
        exit(1);
    }
}

void codigosDis(int inst, int *codop, datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int op0 = 0, op1 = 0, i, IP;

    op0 = inst>>6;
    op0 = (~op0)&0x03;
    op1 = inst>>4;
    op1 = (~op1)&0x03;

    IP = REG[5]-1;
    printf("[%04X] ", IP);
    i = 0;
    while(i < op0)
    {
        printf("%02X ", (MEM[IP])&0xFF);
        IP++;
        i++;
    }
    i = 0;
    while(i <= op1)
    {
        printf("%02X ", (MEM[IP])&0xFF);
        IP++;
        i++;
    }
    for(i = 0 ; i < (20-op0*3-op1*3) ; i++)
        printf(" ");
    printf("|     ");

    V[0].op = (inst>>0x6)&0x03;
    V[0].op &= 0xFF;
    V[1].op = (inst>>0x4)&0x03;
    V[1].op &= 0xFF;

    i = 0;
    if (V[0].op == 3)
        *codop = inst&0xFF;
    else
        if (V[1].op == 3)
            *codop = inst&0x3F;
        else
            *codop = inst&0x0F;
    while(i < 2)
    {
        switch (V[i].op)
        {
            case 0: //op es memoria
                leememdis(V, i, MEM, REG, TDS);
            break;
            case 1: //op es inmediato
                leeinmdis(V, i, MEM, REG, TDS);
            break;
            case 2: //op es registro
                leeregdis(V, i, MEM, REG, TDS);
            break;
        }
        i++;
    }
}

void codigos(int inst, int *codop, datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int i = 0;
    V[0].op = (inst>>0x6)&0x03;
    V[0].op &= 0xFF;
    V[1].op = (inst>>0x4)&0x03;
    V[1].op &= 0xFF;
    if (V[0].op == 3)
        *codop = inst&0xFF;
    else
        if (V[1].op == 3)
            *codop = inst&0x3F;
        else
            *codop = inst&0x0F;
    while(i < 2)
    {
        switch (V[i].op)
        {
            case 0: //op es memoria
                leemem(V, i, MEM, REG, TDS);
            break;
            case 1: //op es inmediato
                leeinm(V, i, MEM, REG, TDS);
            break;
            case 2: //op es registro
                leereg(V, i, MEM, REG, TDS);
            break;
        }
        i++;
    }
}

void setCC(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    if(V[0].valor > 0)
        REG[8] = 0;
    else
        if(V[0].valor < 0)
            REG[8] = 0x80000000;
        else //V[0].valor == 0
            REG[8] = 0x40000000;
}

void MOV(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int aux = 0 , aux1 = 0;
    if(V[0].op == 0) //op1 es memoria
    {
        if(V[1].op == 0) //op2 es memoria
        {
           switch (V[1].celda)
           {
                case 0:
                    aux = V[1].valor;
                break;
                case 2:
                    aux = corrimiento(V[1].valor, 16, 16);
                break;
                case 3:
                    aux = corrimiento(V[1].valor, 24, 24);
                break;
            }
        }
        else
            if (V[1].op == 1) // op2 es inmediato
                aux = corrimiento(V[1].valor, 16, 16);
            else // op2 es registro
                aux = V[1].valor;
        escribeenmem(V, 0, aux, MEM);
    }
    else //op1 es registro
    {
        if(V[1].op == 2) //op2 es registro
        {
            switch(V[1].segmento)
            {
                case 0:
                    aux1 = REG[V[1].registro];
                break;
                case 1:
                    aux1 = (REG[V[1].registro])&0xFF;
                break;
                case 2:
                    aux1 = ((REG[V[1].registro])&0xFF00)>>8;
                break;
                case 3:
                    aux1 = (REG[V[1].registro])&0xFFFF;
                break;
            }
            switch(V[0].segmento)
            {
                case 0:
                    REG[V[0].registro] = aux1;
                break;
                case 1:
                    REG[V[0].registro] &= 0xFFFFFF00;
                    REG[V[0].registro] |= aux1&0xFF;
                break;
                case 2:
                    REG[V[0].registro] &= 0xFFFF00FF;
                    REG[V[0].registro] |= (aux1<<8)&0xFF00;
                break;
                case 3:
                    REG[V[0].registro] &= 0xFFFF0000;
                    REG[V[0].registro] |= aux1&0xFFFF;
                break;
            }
        }
        else // op2 es inmediato
        {
            aux = V[1].valor;
            switch(V[0].segmento)
            {
                case 0:
                    REG[V[0].registro] = V[1].valor;
                break;
                case 1:
                    REG[V[0].registro] &= 0xFFFFFF00;
                    REG[V[0].registro] |= aux&0xFF;
                break;
                case 2:
                    REG[V[0].registro] &= 0xFFFF00FF;
                    REG[V[0].registro] |= (aux<<8)&0xFF00;
                break;
                case 3:
                    REG[V[0].registro] &= 0xFFFF0000;
                    REG[V[0].registro] |= aux&0xFFFF;
            }
            V[0].valor = REG[V[0].registro];
        }
    }
}

void ADD(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int aux1 = 0;
    if(V[0].op == 0) //op1 es memoria
    {
        V[0].valor += V[1].valor;
        escribeenmem(V, 0, V[0].valor, MEM);
    }
    else //op1 es registro
    {
        aux1 = REG[V[0].registro];
        switch(V[0].segmento)
        {
            case 0:
                REG[V[0].registro] += V[1].valor;
            break;
            case 1:
                aux1 = (aux1+V[1].valor)&0xFF;
                REG[V[0].registro] &= 0xFFFFFF00;
                REG[V[0].registro] |= aux1;
            break;
            case 2:
                aux1 = ((aux1+(V[1].valor))<<8)&0xFF00;
                REG[V[0].registro] &= 0xFFFF00FF;
                REG[V[0].registro] |= aux1;
            break;
            case 3:
                aux1 = (aux1+V[1].valor)&0xFFFF;
                REG[V[0].registro] &= 0xFFFF0000;
                REG[V[0].registro] |= aux1;
            break;
        }
        V[0].valor = REG[V[0].registro];
    }
    setCC(V, MEM, REG, TDS, Discos, cantDiscos);
}

void SUB(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int aux1 = 0;
    if(V[0].op == 0) //op1 es memoria
    {
        V[0].valor -= V[1].valor;
        escribeenmem(V, 0, V[0].valor, MEM);
    }
    else //op1 es registro
    {
        aux1 = REG[V[0].registro];
        switch(V[0].segmento)
        {
            case 0:
                REG[V[0].registro] -= V[1].valor;
            break;
            case 1:
                aux1 = (aux1-V[1].valor)&0xFF;
                REG[V[0].registro] &= 0xFFFFFF00;
                REG[V[0].registro] |= aux1;
            break;
            case 2:
                aux1 = ((aux1-(V[1].valor))<<8)&0xFF00;
                REG[V[0].registro] &= 0xFFFF00FF;
                REG[V[0].registro] |= aux1;
            break;
            case 3:
                aux1 = (aux1-V[1].valor)&0xFFFF;
                REG[V[0].registro] &= 0xFFFF0000;
                REG[V[0].registro] |= aux1;
            break;
        }
        V[0].valor = REG[V[0].registro];
    }
    setCC(V, MEM, REG, TDS, Discos, cantDiscos);
}

void SWAP(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    char auxV1[4];
    int i = 0, aux = 0, aux0 = 0, aux1 = 0;
    if(V[0].op == 0) // op1 es memoria
        if (V[1].op == 0) // op2 es memoria
        {
            escribeenmem(V, 0, V[1].valor, MEM);
            escribeenmem(V, 1, V[0].valor, MEM);
        }
        else // op2 es registro
        {
            escribeenmem(V, 0, V[1].valor, MEM);
            switch(V[1].segmento)
            {
                case 0:
                    REG[V[1].registro] = V[0].valor;
                break;
                case 1:
                    REG[V[1].registro] &= 0xFFFFFF00;
                    REG[V[1].registro] |= (V[0].valor)&0xFF;
                break;
                case 2:
                    REG[V[1].registro] &= 0xFFFF00FF;
                    REG[V[1].registro] |= (V[0].valor<<8)&0xFF00;
                break;
                case 3:
                    REG[V[1].registro] &= 0xFFFF0000;
                    REG[V[1].registro] |= V[0].valor&0xFFFF;
                break;
            }
        }
    else // op1 es registro
        if (V[1].op == 0) // op2 es memoria
        {
            for(i = 0 ; i < 4 ; i++)
                auxV1[i] = MEM[V[1].posmem+V[1].offset+i];
            escribeenmem(V,1,V[0].valor,MEM);
            switch(V[0].segmento)
            {
                case 0:
                    for(i = 0 ; i < 4 ; i++)
                    {
                        REG[V[0].registro] = REG[V[0].registro]<<8;
                        REG[V[0].registro] = auxV1[i];
                    }
                break;
                case 1:
                    REG[V[0].registro] &= 0xFFFFFF00;
                    REG[V[0].registro] |= (auxV1[3])&0xFF;
                break;
                case 2:
                    REG[V[0].registro] &= 0xFFFF00FF;
                    aux = auxV1[2];
                    REG[V[0].registro] |= (aux<<8)&0xFF00;
                break;
                case 3:
                    REG[V[0].registro] &= 0xFFFF0000;
                    for (i = 2 ; i < 4 ; i++)
                    {
                        aux = aux<<8;
                        aux = (auxV1[i]);
                    }
                    REG[V[0].registro] |= aux&0xFFFF;
                break;
            }
            V[1].valor = REG[V[1].registro];
            leemem(V, 0, MEM, REG, TDS);
        }
        else // op2 es registro
        {
            switch(V[0].segmento)
            {
                case 0:
                    aux0 = REG[V[0].registro];
                case 1:
                    aux0 = (REG[V[0].registro])&0xFF;
                break;
                case 2:
                    aux0 = ((REG[V[0].registro])&0xFF00)>>8;
                break;
                case 3:
                    aux0 = (REG[V[0].registro])&0xFFFF;
                break;
            }
            switch(V[1].segmento)
            {
                case 0:
                    aux1 = REG[V[1].registro];
                case 1:
                    aux1 = (REG[V[1].registro])&0xFF;
                break;
                case 2:
                    aux1 = ((REG[V[1].registro])&0xFF00)>>8;
                break;
                case 3:
                    aux1 = (REG[V[1].registro])&0xFFFF;
                break;
            }
            switch(V[1].segmento)
            {
                case 0:
                    REG[V[1].registro] = aux0;
                case 1:
                    REG[V[1].registro] &= 0xFFFFFF00;
                    REG[V[1].registro] |= aux0&0xFF;
                break;
                case 2:
                    REG[V[1].registro] &= 0xFFFF00FF;
                    REG[V[1].registro] |= (aux0<<8)&0xFF00;
                break;
                case 3:
                    REG[V[1].registro] &= 0xFFFF0000;
                    REG[V[1].registro] |= aux0&0xFFFF;
                break;
            }
            switch(V[0].segmento)
            {
                case 0:
                    REG[V[0].registro] = aux1;
                case 1:
                    REG[V[0].registro] &= 0xFFFFFF00;
                    REG[V[0].registro] |= aux1&0xFF;
                break;
                case 2:
                    REG[V[0].registro] &= 0xFFFF00FF;
                    REG[V[0].registro] |= (aux1<<8)&0xFF00;
                break;
                case 3:
                    REG[V[0].registro] &= 0xFFFF0000;
                    REG[V[0].registro] |= aux1&0xFFFF;
                break;
            }
        }
}

void MUL(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int aux1 = 0;
    if(V[0].op == 0) //op1 es memoria
    {
        V[0].valor *= V[1].valor;
        escribeenmem(V, 0, V[0].valor, MEM);
    }
    else //op1 es registro
    {
        aux1 = REG[V[0].registro];
        switch(V[0].segmento)
        {
            case 0:
                REG[V[0].registro] *= V[1].valor;
            break;
            case 1:
                aux1 = (aux1*V[1].valor)&0xFF;
                REG[V[0].registro] &= 0xFFFFFF00;
                REG[V[0].registro] |= aux1;
            break;
            case 2:
                aux1 = (aux1*(V[1].valor)<<8)&0xFF00;
                REG[V[0].registro] &= 0xFFFF00FF;
                REG[V[0].registro] |= aux1;
            break;
            case 3:
                aux1 = (aux1*V[1].valor)&0xFFFF;
                REG[V[0].registro] &= 0xFFFF0000;
                REG[V[0].registro] |= aux1;
            break;
        }
        V[0].valor = REG[V[0].registro];
    }
    setCC(V, MEM, REG, TDS, Discos, cantDiscos);
}

void DIV(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int aux1 = 0;
    if (V[1].valor == 0)
    {
        printf("Division por cero\n");
        exit(1);
    }
    else
    {
        if(V[0].op == 0) //op1 es memoria
        {
            REG[9] = V[0].valor % V[1].valor;
            V[0].valor /= V[1].valor;

            escribeenmem(V, 0,  V[0].valor, MEM);
        }
        else //op1 es registro
        {
            aux1 = REG[V[0].registro];
            switch(V[0].segmento)
            {
                case 0:
                    REG[9] = V[0].valor % V[1].valor;
                    REG[V[0].registro] /= V[1].valor;
                break;
                case 1:
                    REG[9] = (V[0].valor)&0xFF % V[1].valor;
                    aux1 = (aux1/V[1].valor)&0xFF;
                    REG[V[0].registro] &= 0xFFFFFF00;
                    REG[V[0].registro] |= aux1;
                break;
                case 2:
                    REG[9] = (((V[0].valor)&0xFF00)>>8) % V[1].valor;
                    aux1 = (aux1/(V[1].valor)<<8)&0xFF00;
                    REG[V[0].registro] &= 0xFFFF00FF;
                    REG[V[0].registro] |= aux1;
                break;
                case 3:
                    REG[9] = (V[0].valor)&0xFFFF % V[1].valor;
                    aux1 = (aux1/V[1].valor)&0xFFFF;
                    REG[V[0].registro] &= 0xFFFF0000;
                    REG[V[0].registro] |= aux1;
                break;
            }
            V[0].valor = REG[V[0].registro];
        }
        setCC(V, MEM, REG, TDS, Discos, cantDiscos);
    }
}

void CMP(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int aux = 0;
    aux = V[0].valor - V[1].valor;
    if(aux > 0)
        REG[8] = 0;
    else
        if(aux < 0)
            REG[8] = 0x80000000;
        else //aux == 0
            REG[8] = 0x40000000;
}

void SHL(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    V[0].valor = V[0].valor<<V[1].valor;
    if(V[0].op == 0) //op1 es memoria
    {
        escribeenmem(V, 0, V[0].valor, MEM);
    }
    else //op1 es registro
    {
        switch(V[0].segmento)
        {
            case 0:
                REG[V[0].registro] = V[0].valor;
            break;
            case 1:
                REG[V[0].registro] = V[0].valor&0xFF;
            break;
            case 2:
                REG[V[0].registro] = (V[0].valor&0xFF)<<8;
            break;
            case 3:
                REG[V[0].registro] = V[0].valor&0xFFFF;
        }
        V[0].valor = REG[V[0].registro];
    }
    setCC(V, MEM, REG, TDS, Discos, cantDiscos);
}

void SHR(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    V[0].valor = V[0].valor>>V[1].valor;
    if(V[0].op == 0) //op1 es memoria
    {
        escribeenmem(V, 0, V[0].valor, MEM);
    }
    else //op1 es registro
    {
        switch(V[0].segmento)
        {
            case 0:
                REG[V[0].registro] = V[0].valor;
            break;
            case 1:
                REG[V[0].registro] = V[0].valor&0xFF;
            break;
            case 2:
                REG[V[0].registro] = (V[0].valor&0xFF)<<8;
            break;
            case 3:
                REG[V[0].registro] = V[0].valor&0xFFFF;
        }
        V[0].valor = REG[V[0].registro];
    }
    setCC(V, MEM, REG, TDS, Discos, cantDiscos);
}

void AND(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int aux1 = 0;
    if(V[0].op == 0) //op1 es memoria
    {
        V[0].valor &= V[1].valor;
        escribeenmem(V, 0, V[0].valor, MEM);
   }
    else //op1 es registro
    {
        aux1 = REG[V[0].registro];
        switch(V[0].segmento)
        {
            case 0:
                REG[V[0].registro] &= V[1].valor;
            break;
            case 1:
                aux1 = (aux1&V[1].valor)&0xFF;
                REG[V[0].registro] &= 0xFFFFFF00;
                REG[V[0].registro] |= aux1;
            break;
            case 2:
                aux1 = (aux1&(V[1].valor)<<8)&0xFF00;
                REG[V[0].registro] &= 0xFFFF00FF;
                REG[V[0].registro] |= aux1;
            break;
            case 3:
                aux1 = (aux1&V[1].valor)&0xFFFF;
                REG[V[0].registro] &= 0xFFFF0000;
                REG[V[0].registro] |= aux1;
            break;
        }
        V[0].valor = REG[V[0].registro];
    }
    setCC(V, MEM, REG, TDS, Discos, cantDiscos);
}

void OR(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int aux1 = 0;
    if(V[0].op == 0) //op1 es memoria
    {
        V[0].valor |= V[1].valor;
        escribeenmem(V, 0, V[0].valor, MEM);
    }
    else //op1 es registro
    {
        aux1 = REG[V[0].registro];
        switch(V[0].segmento)
        {
            case 0:
                REG[V[0].registro] |= V[1].valor;
            break;
            case 1:
                aux1 = (aux1|V[1].valor)&0xFF;
                REG[V[0].registro] &= 0xFFFFFF00;
                REG[V[0].registro] |= aux1;
            break;
            case 2:
                aux1 = (aux1|(V[1].valor)<<8)&0xFF00;
                REG[V[0].registro] &= 0xFFFF00FF;
                REG[V[0].registro] |= aux1;
            break;
            case 3:
                aux1 = (aux1|V[1].valor)&0xFFFF;
                REG[V[0].registro] &= 0xFFFF0000;
                REG[V[0].registro] |= aux1;
            break;
        }
        V[0].valor = REG[V[0].registro];
    }
    setCC(V, MEM, REG, TDS, Discos, cantDiscos);
}

void XOR(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int aux1 = 0;
    if(V[0].op == 0) //op1 es memoria
    {
        V[0].valor ^= V[1].valor;
        escribeenmem(V, 0, V[0].valor, MEM);
    }
    else //op1 es registro
    {
        aux1 = REG[V[0].registro];
        switch(V[0].segmento)
        {
            case 0:
                REG[V[0].registro] ^= V[1].valor;
            break;
            case 1:
                aux1 = (aux1^V[1].valor)&0xFF;
                REG[V[0].registro] &= 0xFFFFFF00;
                REG[V[0].registro] |= aux1;
            break;
            case 2:
                aux1 = (aux1^(V[1].valor)<<8)&0xFF00;
                REG[V[0].registro] &= 0xFFFF00FF;
                REG[V[0].registro] |= aux1;
            break;
            case 3:
                aux1 = (aux1^V[1].valor)&0xFFFF;
                REG[V[0].registro] &= 0xFFFF0000;
                REG[V[0].registro] |= aux1;
            break;
        }
        V[0].valor = REG[V[0].registro];
    }
    setCC(V, MEM, REG, TDS, Discos, cantDiscos);
}

void SYS(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int pos = 0, i = 0, j = 0, d = 0, c = 0, o = 0, x = 0;
    short int ax = 0, cx = 0, acumSegmentos = 0, posMEM = 0;
    int aux = 0, aux2 = 0, edx = 13, ds = 1, ebx = 0;
    unsigned int posArch = 0, tamArch = 0;
    char al = 0, ah = 0, cl = 0, ch = 0, dh = 0, dl = 0, car, cadena[base(REG[ds], TDS, REG) + size(REG[ds], TDS, REG) - pos], auxchar, ultSeg = 0, cantSegmentos = 0;
    FILE *arch;

    switch(V[0].valor)
    {
        case 1: //lectura
            al = REG[10]&0xFF;
            ch = (REG[12]>>8) & 0xFF;
            cl = REG[12] & 0xFF;
            pos = base(REG[edx], TDS, REG) + (REG[edx]&0xFFFF);
            switch(al)
            {
                case 1: // decimal
                    for(i = 0 ; i < cl ; i++)
                    {
                        printf("Ingrese datos (decimal): ");
                        scanf("%d", &aux);
                        for(j = (ch-1) ; j >= 0 ; j--)
                        {
                            MEM[pos+j] = aux;
                            aux = aux>>8;
                        }
                        pos += ch;
                    }
                break;
                case 2: // caracteres
                    for(i = 0 ; i < cl ; i++)
                    {
                        printf("Ingrese datos (caracter): ");
                        scanf("%c", &auxchar);
                        for(j = 0 ; j < ch-1 ; j++)
                            MEM[pos+j] = 0;
                        if(ch != 0)
                            MEM[pos+ch-1] = auxchar;
                        pos += ch;
                    }
                break;
                case 4: // octal
                    for(i = 0 ; i < cl ; i++)
                    {
                        printf("Ingrese datos (octal): ");
                        scanf("%o", &aux);
                        for(j = 0 ; j < ch ; j++)
                        {
                            MEM[pos+j] = aux;
                            aux = aux>>8;
                        }
                        pos += ch;
                    }
                break;
                case 8: // hexadecimal
                    for(i = 0 ; i < cl ; i++)
                    {
                        printf("Ingrese datos (hexadecimal): ");
                        scanf("%x", &aux);
                        for(j = 0 ; j < ch ; j++)
                        {
                            MEM[pos+j] = aux;
                            aux = aux>>8;
                        }
                        pos += ch;
                    }
                break;
            }
        break;
        case 2: //imprimir
            al = REG[10]&0xFF;
            ch = (REG[12]>>8) & 0xFF;
            cl = REG[12] & 0xFF;
            pos = base(REG[edx], TDS, REG) + (REG[edx]&0xFFFF);
            d = al&0x1;
            c = (al>>1)&0x1;
            o = (al>>2)&0x1;
            x = (al>>3)&0x1;
            if(d)
            {
                for(i = 0 ; i < cl ; i++)
                {
                    for(j = 0 ; j < ch ; j++)
                    {
                        aux2 = MEM[pos+j];
                        mascaras(&aux2, 0);
                        aux |= aux2<<8*(ch-j-1);
                    }
                    printf("[%04X]: %d\n", pos-base(REG[edx], TDS, REG),aux);
                    pos += ch;
                    aux = 0;
                }
                printf("\n");
            }
            pos = base(REG[edx], TDS, REG) + (REG[edx]&0xFFFF);
            if(c)
            {
                for(i = 0 ; i < cl ; i++)
                {
                    for(j = 0 ; j < ch ; j++)
                    {
                        aux2 = MEM[pos+j];
                        mascaras(&aux2, 0);
                        aux |= aux2<<8*(ch-j-1);
                    }
                    printf("[%04X]: %c\n", pos-base(REG[edx], TDS, REG), aux);
                    pos += ch;
                    aux = 0;
                }
                printf("\n");
            }
            pos = base(REG[edx], TDS, REG) + (REG[edx]&0xFFFF);
            if(o)
            {
                for(i = 0 ; i < cl ; i++)
                {
                    for(j = 0 ; j < ch ; j++)
                    {
                        aux2 = MEM[pos+j];
                        mascaras(&aux2, 0);
                        aux |= aux2<<8*(ch-j-1);
                    }
                    printf("[%04X]: %011o\n", pos-base(REG[edx], TDS, REG), aux);
                    pos += ch;
                    aux = 0;
                }
                printf("\n");
            }
            pos = base(REG[edx], TDS, REG) + (REG[edx]&0xFFFF);
            if(x)
            {
                for(i = 0 ; i < cl ; i++)
                {
                    for(j = 0 ; j < ch ; j++)
                    {
                        aux2 = MEM[pos+j];
                        mascaras(&aux2, 0);
                        aux |= aux2<<8*(ch-j-1);
                    }
                    printf("[%04X]: %08X\n", pos-base(REG[edx], TDS, REG), aux);
                    pos += ch;
                    aux = 0;
                }
                printf("\n");
            }
        break;
        case 3: //string read
            //cx:cant de caracteres a leer
            //dx: apunta a donde se escribe
            cx = REG[12]&0xFFFF;
            pos = base(REG[edx], TDS, REG) + (REG[edx]&0xFFFF);
            if (cx != -1)
                aux = cx + 1;
            else
                aux = base(REG[edx], TDS, REG) + size(REG[edx], TDS, REG) - pos;
            printf("Ingrese texto: ");
            scanf(" %s", cadena);
            if(strlen(cadena) > base(REG[edx], TDS, REG) + size(REG[edx], TDS, REG) - pos)
            {
                printf("Fallo de segmento\n");
                exit(1);
            }
            while(i < aux && cadena[i] != '\0')
            {
                MEM[pos+i] = cadena[i];
                i++;
            }
            cadena[i]='\0';
        break;
        case 4: //string write
            cx = REG[12]&0xFFFF;
            pos = base(REG[edx], TDS, REG) + (REG[edx]&0xFFFF);
            car = MEM[pos+i];
            printf("[%04X]: ",pos+i);
            while((i <= cx || cx == -1) && (i <= base(REG[edx], TDS, REG) + size(REG[edx], TDS, REG) - pos) && car != '\0')
            {
                if(car > 31 && car != 127 && car != 255)
                    printf("%c",car);
                else
                    printf(".");
                i++;
                car = MEM[pos+i];
            }
            printf("\n");
            if(i > base(REG[edx], TDS, REG) + size(REG[edx], TDS, REG) - pos)
            {
                printf("Fallo de segmento\n");
                exit(1);
            }
        break;
        case 7:
            system("cls");
        break;
        case 13: //discos
            ah = (REG[10]>>8) & 0xFF;
            al = REG[10] & 0xFF;
            ch = (REG[12]>>8) & 0xFF;
            cl = REG[12] & 0xFF;
            dh = (REG[14]>>8) & 0xFF;
            dl = REG[14] & 0xFF;
            ebx = REG[11];
            if (ah != 8)
            {
                REG[10] &= 0xFFFF00FF;
                while(i < cantDiscos && dl != Discos[i].nUnidad)
                        i++;
                if(dl == Discos[i].nUnidad)
                    if(ch < Discos[i].cCilindros)
                        if(cl < Discos[i].cCabezas)
                        {
                            if(dh > Discos[i].cSectores)
                                REG[10] |= 0x0D00;
                        }
                        else
                            REG[10] |= 0x0C00;
                    else
                        REG[10] |= 0x0B00;
                else
                    REG[10] |= 0x3100;
            }
            if(((REG[10]>>8)&0xFF) == 0)
                switch (ah)
                {
                    case 0: //consultar ultimo estado

                    break;
                    case 2: // leer del disco
                        if(size(REG[11], TDS, REG) > (REG[11]&0xFFFF) + al*512)
                        {
                            arch = fopen(Discos[i].ruta, "rb+");
                            if(arch)
                            {
                                posArch = 512 + ch * Discos[i].cCabezas * Discos[i].cSectores * 512 + cl * Discos[i].cSectores * 512 + dh * 512;
                                tamArch = Discos[i].cCilindros * Discos[i].cCabezas * Discos[i].cSectores * 512;
                                fseek(arch, 0, SEEK_END);
                                if(posArch+al*512 > ftell(arch)) //completa el archivo para su lectura
                                    completaArch(arch, posArch, tamArch, al);

                                fseek(arch, posArch, SEEK_SET);
                                posMEM = base(REG[11], TDS, REG) + (REG[11]&0xFFFF);
                                while(j < al && posArch+512*j < tamArch)
                                {
                                    for (i = 0 ; i < 512 ; i++)
                                    {
                                        auxchar = fgetc(arch);
                                        MEM[posMEM+i+j*512] = auxchar;
                                    }
                                    j++; //cant de sectores transferidos
                                }
                                REG[10] &= 0xFFFF0000;
                                REG[10] |= j;                   
                                fclose(arch);
                            }
                            else
                                REG[10] = 0xFFFFFFFF; 
                        }
                        else
                        {
                            REG[10] &= 0xFFFF00FF; 
                            REG[10] |= 0x400;
                        }
                    break;
                    case 3: //escribir en el disco
                        if(size(REG[11], TDS, REG) > (REG[11]&0xFFFF) + al*512)
                        {
                            arch = fopen(Discos[i].ruta, "rb+");
                            if(arch)
                            {
                                posArch = 512 + ch * Discos[i].cCabezas * Discos[i].cSectores * 512 + cl * Discos[i].cSectores * 512 + dh * 512;
                                tamArch = Discos[i].cCilindros * Discos[i].cCabezas * Discos[i].cSectores * 512;
                                fseek(arch, 0, SEEK_END);
                                if(posArch+al*512 > ftell(arch)) //completa el archivo para su lectura
                                    completaArch(arch, posArch, tamArch, 0);
                            
                                fseek(arch, posArch, SEEK_SET);
                                posMEM = base(REG[11], TDS, REG) + (REG[11]&0xFFFF);
                                while(j < al && posArch+512*j < tamArch)
                                {
                                    for (i = 0 ; i < 512 ; i++)
                                    {
                                        auxchar = MEM[posMEM+i+j*512];
                                        escribeArch(auxchar, 1, arch);
                                    }
                                    j++; //cant de sectores transferidos
                                }
                                if(j < al)
                                {
                                    REG[10] &= 0xFFFF00FF; 
                                    REG[10] |= 0xCC00;
                                }
                                fclose(arch);
                            }
                            else
                                REG[10] = 0xFFFFFFFF; 
                        }
                        else
                        {
                            REG[10] &= 0xFFFF00FF; 
                            REG[10] |= 0xCC00;
                        }    
                    break;
                    case 8: //obtener parametros
                        i = 0;
                        while(i < cantDiscos && dl != Discos[i].nUnidad)
                            i++;
                        if(dl == Discos[i].nUnidad)
                        {
                            REG[10] &= 0xFFFF00FF;
                            REG[12] &= 0xFFFF0000;
                            REG[12] |= (Discos[i].cCilindros)<<8;
                            REG[12] |= Discos[i].cCabezas;
                            REG[13] &= 0xFFFF00FF;
                            REG[13] |= (Discos[i].cSectores)<<8;
                        }
                        else
                        {
                            REG[10] &= 0xFFFF00FF;
                            REG[10] |= 0x3100;
                        }
                    break;
                    default: 
                        //funcion invalida
                        REG[10] &= 0xFFFF00FF;
                        REG[10] |= 0x100;
                    break;
                }
        break;
        case 14: //gestion dinamica de segmentos
            ax = REG[10];
            ebx = REG[11];
            cx = REG[12];   
            switch (ax)
            {
                case 0: //consultar segmento
                    i=0;
                    while(i < 8 && ebx != TDS[i].base)
                        i++;
                    if(TDS[i].base == ebx) //existe el segmento
                    {
                        REG[12] &= 0xFFFF0000;
                        REG[12] |= TDS[i].size;
                        REG[10] &= 0xFFFF0000; //operacion exitosa
                    }
                    else //no existe el segmento
                    {
                        REG[12] &= 0xFFFF0000;
                        REG[10] &= 0xFFFF0000;
                        REG[10] |= 0x0031;
                    }                
                break;
                case 1: //crear segmento
                    i = 0;
                    acumSegmentos = 0;
                    while(i < 8 && TDS[i].base == acumSegmentos)
                    {
                        acumSegmentos += TDS[i].size; 
                        i++;
                    }
                    if (i == 8) //no hay lugar en la TDS
                    {
                        //falla en la operacion
                        REG[10] &= 0xFFFF0000; 
                        REG[10] |= 0xFFFF;
                        REG[11] = -1;
                    }
                    else //si hay lugar en la TDS
                    {
                        if(acumSegmentos + cx > V[0].RAM) //no hay suficiente memoria
                        {
                            //no hay suficiente memoria
                            REG[10] &= 0xFFFF0000; 
                            REG[10] |= 0xCC;
                            REG[11] = -1;
                        }
                        else //si hay suficiente memoria
                        {
                            TDS[i].base = acumSegmentos;
                            TDS[i].size = cx;
                            REG[10] &= 0xFFFF0000; //operacion exitosa
                            REG[11] = TDS[i].base;
                        }
                    }
                break;
                default:
                    REG[10] = 1;
                break;
            }
        break;
        case 15:
            if(strcmp(V[0].nombrevmi,"") != 0)
            {
                arch = fopen(V[0].nombrevmi, "wb");
                //va a recibir la palabra a escribir y el tamaño del dato
                auxchar='V';
                escribeArch(auxchar, 1, arch);
                auxchar='M';
                escribeArch(auxchar, 1, arch);
                auxchar='I';
                escribeArch(auxchar, 1, arch);
                auxchar='2';
                escribeArch(auxchar, 1, arch);
                auxchar='3';
                escribeArch(auxchar, 1, arch);
                auxchar=1;
                escribeArch(auxchar, 1, arch);
                escribeArch(V[0].RAM, 2, arch);

                for(i = 0 ; i < 16 ; i++) //escribe registros
                    escribeArch(REG[i], 4, arch);

                for(i = 0 ; i < 8 ; i++) //escribe tds (segmentos activos)
                   if(TDS[i].base != -1)
                    {
                        cantSegmentos++;
                        escribeArch(TDS[i].base, 2, arch);
                        escribeArch(TDS[i].size, 2, arch);
                    }
                while (cantSegmentos < 8) //escribe tds (segmentos para completar los 8)
                {
                        cantSegmentos++;
                        escribeArch(0, 2, arch);
                        escribeArch(0, 2, arch);
                }
                ultSeg = 4;
                while (REG[ultSeg] == -1)
                    ultSeg--;

                for(i=0; i < base(REG[ultSeg], TDS, REG)+size(REG[ultSeg], TDS, REG) ; i++) //escritura de la totalidad de la memoria
                    escribeArch(MEM[i], 1, arch);

                fclose(arch);
            }
            car = getchar();
            while(car != 'q' && car != '\n')
                car = getchar();
            if(car == '\n')
                V[0].breakpoint = 1;
            else
                V[0].breakpoint = 0;
        break;
    }
}

void JMP(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    REG[5] = 0;
    REG[5] |= (V[0].valor)&0xFFFF;
}

void JZ(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int Z = (REG[8]>>30) & 0x01;

    if(Z == 1)
    {
        REG[5] = 0;
        REG[5] = (V[0].valor)&0xFFFF;
    }
}

void JP(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int N = (REG[8]>>31) & 0x01;

    if(N == 0)
    {
        REG[5] = 0;
        REG[5] = (V[0].valor)&0xFFFF;
    }
}

void JN(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int N = (REG[8]>>31) & 0x01;

    if(N == 1)
    {
        REG[5] = 0;
        REG[5] = (V[0].valor)&0xFFFF;
    }
}

void JNZ(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int Z = (REG[8]>>30) & 0x01;

    if(Z == 0)
    {
        REG[5] = 0;
        REG[5] = (V[0].valor)&0xFFFF;
    }
}

void JNP(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int N = (REG[8]>>31) & 0x01, Z = (REG[8]>>30) & 0x01;

    if(N == 1 || Z == 1 )
    {
        REG[5] = 0;
        REG[5] = (V[0].valor)&0xFFFF;
    }
}

void JNN(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int N = (REG[8]>>31) & 0x01;

    if(N == 0)
    {
        REG[5] = 0;
        REG[5] = (V[0].valor)&0xFFFF;
    }
}

void LDL(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    REG[9] &= 0xFFFF0000;
    REG[9] |= (V[0].valor & 0x0000FFFF);
}

void LDH(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int aux = 0;

    REG[9] &= 0x0000FFFF;
    aux = V[0].valor;
    REG[9] |= ((aux<<16)&0xFFFF0000);
}

void RND(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    REG[9] = rand() % V[0].valor;
}

void NOT(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    V[0].valor = ~V[0].valor;
    if(V[0].op == 0) //op1 es memoria
        escribeenmem(V, 0, V[0].valor, MEM);
    else //op1 es registro
    {
        switch(V[0].segmento)
        {
            case 0: // ex
                REG[V[0].registro] = V[0].valor;
            break;
            case 1: // l
                V[0].valor &= 0xFF;
                REG[V[0].registro] &= 0xFFFFFF00;
                REG[V[0].registro] |= V[0].valor;
            break;
            case 2: // h
                V[0].valor &= 0xFF;
                REG[V[0].registro] &= 0xFFFF00FF;
                REG[V[0].registro] |= (V[0].valor)<<8;
            break;
            case 3: // x
                V[0].valor &= 0xFFFF;
                REG[V[0].registro] &= 0xFFFF0000;
                REG[V[0].registro] |= V[0].valor;
            break;
        }
        V[0].valor = REG[V[0].registro];
    }
    setCC(V, MEM, REG, TDS, Discos, cantDiscos);
}

void PUSH(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    REG[6] -= 4;
    if ((REG[6]&0xFFFF) > base(REG[4],TDS,REG) + size(REG[4],TDS,REG))
    {
        printf("Stack overflow");
        exit(1);
    }
    V[0].posmem = REG[6];
    V[0].offset = 0;
    V[0].celda = 0;
    escribeenmem(V, 0, V[0].valor, MEM);
}

void POP(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    int aux = 0, aux2 = 0;
    V[1].posmem = REG[6];
    V[1].offset = 0;
    V[1].celda = 0;

    aux = MEM[V[1].posmem+V[1].offset+3];
    mascaras(&aux, 0);
    aux2 |= aux;

    aux = MEM[V[1].posmem+V[1].offset+2];
    aux = aux <<(8*1);
    mascaras(&aux, 1);
    aux2 |= aux;

    aux = MEM[V[1].posmem+V[1].offset+1];
    aux = aux <<(8*2);
    mascaras(&aux, 2);
    aux2 |= aux;

    aux = MEM[V[1].posmem+V[1].offset];
    aux = aux <<(8*3);
    mascaras(&aux, 3);
    aux2 |= aux;

    V[1].valor = aux2;

    if(V[0].inst != 0xF1)
        MOV(V, MEM, REG, TDS, Discos, cantDiscos);

    REG[6] += 4;
}

void CALL(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    short int subrutina = 0;

    subrutina = V[0].valor;
    V[0].valor = REG[5];
    PUSH(V, MEM, REG, TDS, Discos, cantDiscos);
    V[0].valor = (subrutina)&0xFFFF;
    JMP(V, MEM, REG, TDS, Discos, cantDiscos);
}

void STOP(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    printf("\n");
}

void RET(datos V[], char MEM[], int REG[], TRTDS TDS[], TRD Discos[], char cantDiscos)
{
    POP(V, MEM, REG, TDS, Discos, cantDiscos);
    REG[5] = (V[1].valor)&0xFFFF;
}

void cargaFunciones(t_func funciones[])
{
    funciones[0] = MOV;
    funciones[1] = ADD;
    funciones[2] = SUB;
    funciones[3] = SWAP;
    funciones[4] = MUL;
    funciones[5] = DIV;
    funciones[6] = CMP;
    funciones[7] = SHL;
    funciones[8] = SHR;
    funciones[9] = AND;
    funciones[10] = OR;
    funciones[11] = XOR;
    funciones[48] = SYS;
    funciones[49] = JMP;
    funciones[50] = JZ;
    funciones[51] = JP;
    funciones[52] = JN;
    funciones[53] = JNZ;
    funciones[54] = JNP;
    funciones[55] = JNN;
    funciones[56] = LDL;
    funciones[57] = LDH;
    funciones[58] = RND;
    funciones[59] = NOT;
    funciones[60] = PUSH;
    funciones[61] = POP;
    funciones[62] = CALL;
    funciones[240] = STOP;
    funciones[241] = RET;
}

void dis0(texto nombre)
{
    printf("%s\n", nombre);
}

void dis1(texto nombre, char modificador[], datos V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    char signo1;

    if(V[0].offset >= 0)
        signo1 = '+';
    else
        signo1 = '\0';

    if(V[0].op == 0) // memoria
        printf("%s  %c[%s%c%d]\n", nombre, modificador[V[0].celda], registro[V[0].registro][V[0].segmento], signo1, V[0].offset);
    else
        if(V[0].op == 1) // inmediato
            if((48 < (V[0].inst&0x3F) && (V[0].inst&0x3F) < 56) || ((V[0].inst&0x3F) == 0x3E))
                printf("%s  %X\n", nombre, V[0].valor&0xFFFF);
            else
                printf("%s  %d\n", nombre, V[0].valor&0xFFFF);
        else // registro
            printf("%s  %s\n", nombre, registro[V[0].registro][V[0].segmento]);
}

void dis2(texto nombre, char modificador[], datos V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    char signo1, signo2;

    if(V[0].offset >= 0)
        signo1 = '+';
    else
        signo1 = '\0';
    if(V[1].offset >= 0)
        signo2 = '+';
    else
        signo2 = '\0';

    if(V[0].op == 0) //op1 es memoria
        if(V[1].op == 0) //memoria y memoria
            printf("%s  %c[%s%c%d], %c[%s%c%d] \n", nombre, modificador[V[0].celda], registro[V[0].registro][V[0].segmento], signo1, V[0].offset, modificador[V[1].celda], registro[V[1].registro][V[1].segmento], signo2, V[1].offset);
        else
            if(V[1].op == 1) // memoria e inmediato
                printf("%s  %c[%s%c%d], %d \n", nombre, modificador[V[0].celda], registro[V[0].registro][V[0].segmento], signo1, V[0].offset, V[1].valor);
            else //memoria y registro
                printf("%s  %c[%s%c%d], %s \n", nombre, modificador[V[0].celda], registro[V[0].registro][V[0].segmento],signo1, V[0].offset, registro[V[1].registro][V[1].segmento]);
    else
        if(V[0].op == 1) //op1 es inmediato
            if(V[1].op == 0) //inmediato y memoria
                printf("%s  %d, %c[%s%c%d] \n", nombre, V[0].valor, modificador[V[1].celda], registro[V[1].registro][V[1].segmento], signo2, V[1].offset);
            else
                if(V[1].op == 1) //inmediato e inmediato
                    printf("%s  %d, %d \n", nombre, V[0].valor, V[1].valor);
                else //inmediato y registro
                    printf("%s  %d, %s \n", nombre, V[0].valor, registro[V[1].registro][V[1].segmento]);
        else //op1 es registro
            if(V[1].op == 0) //registro y memoria
                printf("%s  %s, %c[%s%c%d] \n", nombre, registro[V[0].registro][V[0].segmento], modificador[V[1].celda], registro[V[1].registro][V[1].segmento],signo2, V[1].offset);
            else
                if(V[1].op == 1) //registro e inmediato
                    printf("%s  %s, %d \n", nombre, registro[V[0].registro][V[0].segmento], V[1].valor);
                else //registro y registro
                    printf("%s  %s, %s \n", nombre, registro[V[0].registro][V[0].segmento], registro[V[1].registro][V[1].segmento]);
}

void disassembler(int codop, datos V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    texto Nombres[256];
    char Modificador[4] = {'l', ' ', 'w', 'b'};
    cargaNombres(Nombres);
    if (V[0].op == 3)
        dis0(Nombres[codop]);
    else
        if (V[1].op == 3)
            dis1(Nombres[codop], Modificador, V, REG, TDS, registro);
        else
            dis2(Nombres[codop], Modificador, V, REG, TDS, registro);
}

void procesoDatos(char MEM[], int REG[], TRTDS TDS[], t_func funciones[], int ejecutar, char infoVM[], int RAM, int indice[], char DirArchDebugger[], TRD Discos[], char cantDiscos)
{
    char inst;
    int codop = 0, ds = 1;
    datos V[2];
    texto registro[16][4];

    cargaMatriz (registro);
    V[0].RAM = RAM;
    if(ejecutar)
        printf("\n%s\n\n", infoVM);
    while(ejecutar == 1 && REG[5] < size(REG[0], TDS, REG))
    {
        inst = MEM[REG[5]++];
        V[0].inst = inst&0xFF;

        codigosDis(V[0].inst, &codop, V, MEM, REG, TDS);
        disassembler(codop, V, REG, TDS, registro);
    }

    printf("\n");
    iniciaRegistros(REG, indice, TDS);
    codop = 0;
    strcpy(V[0].nombrevmi, DirArchDebugger);
    V[0].breakpoint = 1;
    printf("\n");

    while(codop != 240 && REG[5] < base(REG[ds], TDS, REG))
    {
        inst = MEM[REG[5]++];
        V[0].inst = inst&0xFF;
        codigos(V[0].inst, &codop, V, MEM, REG, TDS);
        if( (codop < 0) || (12 < codop && codop < 48) || (62 < codop && codop < 240) || (241 < codop))
        {
            printf("\nInstruccion invalida\n");
            exit(1);
        }

        funciones[codop](V, MEM, REG, TDS, Discos, cantDiscos);
        if(V[0].breakpoint == 1)
        {
            V[0].valor = 15;
            SYS(V, MEM, REG, TDS, Discos, cantDiscos);
        }
    }
}

void generarIdentificador(char *identificador, int longitud) 
{
    int i;
    static const char caracteres[] = "0123456789ABCDEF";

    for (i = 0; i < longitud; i++) 
    {
        int indice = rand() % (sizeof(caracteres) - 1);
        identificador[i] = caracteres[indice];
    }

    identificador[longitud] = '\0';
}

void escribeFecha (FILE *arch)
{
    time_t currentTime;
    time(&currentTime);
    
    struct tm* timeInfo = localtime(&currentTime);
    
    int auxFecha; 

    //año    
    auxFecha = timeInfo->tm_year + 1900;
    escribeArch(auxFecha, 2, arch);
    //mes
    auxFecha = timeInfo->tm_mon+1;
    escribeArch(auxFecha, 1, arch);
    //dia
    auxFecha = timeInfo->tm_mday;
    escribeArch(auxFecha, 1, arch);
    //hora
    auxFecha = timeInfo->tm_hour;
    escribeArch(auxFecha, 1, arch);
    //minutos
    auxFecha = timeInfo->tm_min;
    escribeArch(auxFecha, 1, arch);
    //segundos
    auxFecha = timeInfo->tm_sec;
    escribeArch(auxFecha, 1, arch);
    //decimas //no encontramos una función que permita consultar decimas de segundo y que funcione en todos los SO
    auxFecha = 0;
    escribeArch(auxFecha, 1, arch);
    
}

void analizarDisco(TRD Disco)
{
    FILE *arch;
    int auxEscritura,i;
    char identificador[17];

    if(access(Disco.ruta, F_OK) == -1) //no existe el archivo
    {
        arch = fopen(Disco.ruta, "wb");
        //tipo de archivo
        auxEscritura = 'V';
        escribeArch(auxEscritura, 1, arch);
        auxEscritura = 'D';
        escribeArch(auxEscritura, 1, arch);
        auxEscritura = 'D';
        escribeArch(auxEscritura, 1, arch);
        auxEscritura = '0';
        escribeArch(auxEscritura, 1, arch);
        //numero de version
        auxEscritura = 1;
        escribeArch(auxEscritura, 4, arch);
        //identificador del disco
        generarIdentificador(identificador, 16);
        for(i = 0 ; i < 16 ; i++)
            escribeArch(identificador[i], 1, arch);
        //fecha
        escribeFecha(arch);
        //tipo
        auxEscritura = 1;
        escribeArch(auxEscritura, 1, arch);
        //cant de cilindros
        auxEscritura = 128;
        escribeArch(auxEscritura, 1, arch);
        Disco.cCilindros = 128;
        //cant de cabezas
        auxEscritura = 128;
        escribeArch(auxEscritura, 1, arch);
        Disco.cCabezas = 128;
        //cant de sectores
        auxEscritura = 128;
        escribeArch(auxEscritura, 1, arch);
        Disco.cSectores = 128;
        //tamSector
        auxEscritura = 512;
        escribeArch(auxEscritura, 4, arch);
        Disco.tamSector = 512;
        auxEscritura = 0;
        for(i = 0 ; i < 118 ; i++)
            escribeArch(auxEscritura, 4, arch);
        fclose(arch);
    }
    else
    {
        arch = fopen(Disco.ruta, "rb");
        fseek(arch, 33, SEEK_SET);
        Disco.cCilindros = fgetc(arch);
        Disco.cCabezas = fgetc(arch);
        Disco.cSectores = fgetc(arch);
        fclose(arch);
    }
}

int main(int argc, char *argv[])
{
    int RAM = 16384, REG[16], disa = 0, indice[5];
    char DirArchivo[120], DirArchivoDebugger[120], infoMV[100], info[120], tipoArch[5], cantDiscos = 0;
    TRD Discos[255];

    TRTDS TDS[8];
    t_func funciones[256];

    srand(time(NULL));

    strcpy(DirArchivo, argv[1]);
    strcpy(DirArchivoDebugger, "");

    for(int i = 2; i < argc ; i++)
    {
        strcpy(info, argv[i]);
        if(info[0] =='m') //es el tamaño de la RAM
        {
            memmove(info, info + 2, strlen(info)-1);
            RAM = atoi(info);
        }
        else
            if(strcmp(info, "-d") == 0) //ejecuta o no el dissasembler
                disa = 1;
            else //es archivo
            {
                strncpy(tipoArch, info + strlen(info) -4, 4);
                tipoArch[4] = '\0';
                printf("%s\n", tipoArch);
                if(strcmp(tipoArch, ".vmi") == 0)
                    strcpy(DirArchivoDebugger, info);
                else
                    if(strcmp(tipoArch, ".vdd") == 0)
                    {
                        strcpy(Discos[cantDiscos].ruta, info);
                        Discos[cantDiscos].nUnidad = cantDiscos + 1;
                        analizarDisco(Discos[cantDiscos]);
                        cantDiscos++;
                    }
            }
    }
    printf("%s\n", DirArchivoDebugger);
    char MEM[RAM];
    cargaFunciones(funciones);
    lectura(MEM, REG, TDS, RAM, DirArchivo, infoMV, indice);
    procesoDatos(MEM, REG, TDS, funciones, disa, infoMV, RAM, indice, DirArchivoDebugger, Discos, cantDiscos);
    return 0;
}
