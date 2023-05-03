#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct
{
    char op, registro, segmento, celda;
    short int posmem, offset;
    int valor, inst;
} datos;

typedef struct
{
    short int base, size;
} TRTDS;

typedef char texto[5];

typedef void (*t_dis) (datos V[], int REG[], TRTDS TDS[], texto registro[16][4]);

typedef void (*t_func)(datos V[], char MEM[], int REG[], TRTDS TDS[]);

void cargaMatriz(texto registro[16][4])
{
    strcpy(registro[0][0],"cs");
    strcpy(registro[1][0],"ds");
    strcpy(registro[2][0],"ks");
    strcpy(registro[3][0],"es");
    strcpy(registro[4][0],"ss");
    strcpy(registro[5][0],"ip");
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

void iniciaRegistros(int REG[], int indice[], int RAM)
{
    REG[0] = indice[0];       //CS
    REG[1] = indice[2];       //DS
    REG[2] = indice[1];       //KS
    REG[3] = indice[3];       //ES
    REG[4] = indice[4];       //SS
    REG[5] = 0;               //IP
    if(REG[4] == -1)          //SP
        REG[6] = -1;
    else
        REG[6] = RAM + 1;
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
    int i, j = 0;

    TDS[0].base = 0;
    TDS[0].size = indice[0];
    indice[0] = 0;
    for (i = 1 ; i < 5 ; i++)
    {
        if (indice[i] != 0)
        {
            TDS[j].base = TDS[j-1].base + TDS[j-1].size;
            if(indice[i] > 0)
                TDS[j].size = indice[i];
            else
                TDS[j].size = 1024;
            indice[i] = j++;
            indice[i] = indice[i]<<16;
        }
        else
            indice[i] = -1;
    }
    //VER QUE HACER CON LA RAM :)
    iniciaRegistros (REG, indice, RAM);
}

short int base(int seg, TRTDS TDS[], int REG[])
{
    return TDS[(REG[seg])>>16].base;
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
    int i, aux = 0, ds = REG[1];
    *var = 0;
    for(i = 1 ; i >= 0 ; i--)
    {
        if(REG[5] > base(ds, TDS, REG))
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
    int aux = 0, ds = REG[1];
    if(REG[5] > base(ds, TDS, REG))
    {
        printf("Fallo de segmento\n");
        exit(1);
    }
    aux = MEM[REG[5]++];
    mascaras(&aux, 0);
    *var = aux;
}

void leedemem(datos V[], int k, char MEM[], int REG[], TRTDS TDS[])
{
    int i, j = 3, aux2 = 0, aux = 0, ds = REG[1];



    for(i = 0 ; i < 4 ; i++)
    {
        if(V[k].posmem+V[k].offset+j < base(ds,TDS,REG))
        {
            printf("Fallo de segmento\n");
            exit(1);
        }
        else
        {
            aux = MEM[V[k].posmem+V[k].offset+j--];
            aux = aux<<(8*i);
            mascaras(&aux, i);
            aux2 |= aux;
        }
    }
    V[k].valor = aux2;
}

void leemem(datos V[], int i, char MEM[], int REG[], TRTDS TDS[])
{
    char segreg = 0;
    int ds = REG[1];
    short int offset = 0;

    lee1byte(&segreg, MEM, REG, TDS);
    V[i].celda = segreg>>6;
    V[i].celda &= 0x3;
    V[i].segmento = segreg>>4; // siempre = 0;
    V[i].registro = segreg&0xF;

    lee2byte(&offset, MEM, REG, TDS);
    V[i].offset = offset;

    REG[V[i].registro] &= 0xFFFF;
    REG[V[i].registro] |= ds;

    V[i].posmem = base(ds, TDS, REG) + (REG[V[i].registro]&0xFFFF);

    leedemem(V, i, MEM, REG, TDS);

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

void lectura(char MEM[], int REG[], int TDS[], int RAM, char DirArchivo[], char infoMV[])
{
    FILE *arch;
    char encabezado[6], version, c, str[6];
    arch = fopen(DirArchivo, "rb");
    int i, aux = 0, TAMCS = 0, TAMKS = 0, TAMDS = 0, TAMES = 0, TAMSS = 0, indice[5];

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

//lectura y carga en memoria
        for (i=0 ; i < TAMCS ; i++)
        {
            fread(&c,sizeof(char),1,arch);
            MEM[i] = c;
        }
        fclose(arch);

        strcpy(infoMV,encabezado);
        strcat(infoMV," VER: ");
        version += 48;
        infoMV[11] = version;
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
        printf("No se pudo abrir el archivo\n");
}

void codigosDis(int inst, char MEM[], int REG[], TRTDS TDS[])
{
    int op1 = 0, op2 = 0, i, IP;

    op1 = (inst>>6);
    op1 = (~op1)&0x03;
    op2 = inst>>4;
    op2 = (~op2)&0x03;
    IP = REG[5]-1;
    i = 0;
    printf("[%04X] ",IP);
    while(i < op1)
    {
        printf("%02X ",(MEM[IP])&0xFF);
        IP++;
        i++;
    }
    i = 0;
    while(i <= op2)
    {
        printf("%02X ",(MEM[IP])&0xFF);
        IP++;
        i++;
    }
    for(i = 0 ; i < (20-op1*3-op2*3) ; i++)
        printf(" ");
    printf("|     ");
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

void setCC(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    if(V[0].valor > 0)
        REG[8] = 0;
    else
        if(V[0].valor < 0)
            REG[8] = 0x80000000;
        else //V[0].valor == 0
            REG[8] = 0x40000000;
}

void MOV(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int i = 0, aux = 0 , aux1 = 0;
    if(V[0].op == 0) //op1 es memoria
        if (V[1].op == 0) //op2 es memoria
            for (i = 0 ; i < 4 ; i++)
                MEM[V[0].posmem+V[0].offset+i] = MEM[V[1].posmem+V[1].offset+i];
        else
        {
            if (V[1].op == 1) // op2 es inmediato
                aux = corrimiento(V[1].valor, 16, 16);
            else // op2 es registro
                aux = V[1].valor;
            MEM[V[0].posmem+V[0].offset] = aux>>24;
            MEM[V[0].posmem+V[0].offset+1] = aux>>16;
            MEM[V[0].posmem+V[0].offset+2] = aux>>8;
            MEM[V[0].posmem+V[0].offset+3] = aux;
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
                    //aux = corrimiento(V[1].valor, 24, 24);
                    REG[V[0].registro] |= aux&0xFF;
                break;
                case 2:
                    REG[V[0].registro] &= 0xFFFF00FF;
                    //aux = corrimiento(V[1].valor, 24, 24);
                    REG[V[0].registro] |= (aux<<8)&0xFF00;
                break;
                case 3:
                    REG[V[0].registro] &= 0xFFFF0000;
                    //aux = corrimiento(V[1].valor, 16, 16);
                    REG[V[0].registro] |= aux&0xFFFF;
            }
            V[0].valor = REG[V[0].registro];
        }
    }
}

void ADD(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int aux1 = 0;
    if(V[0].op == 0) //op1 es memoria
    {
        V[0].valor += V[1].valor;
        MEM[V[0].posmem+V[0].offset] = V[0].valor>>24;
        MEM[V[0].posmem+V[0].offset+1] = V[0].valor>>16;
        MEM[V[0].posmem+V[0].offset+2] = V[0].valor>>8;
        MEM[V[0].posmem+V[0].offset+3] = V[0].valor;
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
    setCC(V, MEM, REG, TDS);
}

void SUB(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int aux1 = 0;
    if(V[0].op == 0) //op1 es memoria
    {
        V[0].valor -= V[1].valor;
        MEM[V[0].posmem+V[0].offset] = V[0].valor>>24;
        MEM[V[0].posmem+V[0].offset+1] = V[0].valor>>16;
        MEM[V[0].posmem+V[0].offset+2] = V[0].valor>>8;
        MEM[V[0].posmem+V[0].offset+3] = V[0].valor;
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
    setCC(V, MEM, REG, TDS);
}

void SWAP(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    char auxV1[4], auxV2[4];
    int i = 0, aux = 0, aux0 = 0, aux1 = 0, auxreg = 0;
    if(V[0].op == 0) // op1 es memoria
        if (V[1].op == 0) // op2 es memoria
        {
            for(i = 0 ; i < 4 ; i++)
            {
                auxV1[i] = MEM[V[0].posmem+V[0].offset+i];
                auxV2[i] = MEM[V[1].posmem+V[1].offset+i];
            }
            for(i = 0 ; i < 4 ; i++)
            {
                MEM[V[1].posmem+V[1].offset+i] = auxV1[i];
                MEM[V[0].posmem+V[0].offset+i] = auxV2[i];
            }
            for(i = 0 ; i < 4 ; i++)
            {
                V[0].valor = V[0].valor<<8;
                V[0].valor = MEM[V[0].posmem+V[0].offset+i];
            }
            for(i = 0 ; i < 4 ; i++)
            {
                V[1].valor = V[1].valor<<8;
                V[1].valor = MEM[V[1].posmem+V[1].offset+i];
            }
        }
        else // op2 es registro
        {
            for(i = 0 ; i < 4 ; i++)
                auxV1[i] = MEM[V[0].posmem+V[0].offset+i];
            MEM[V[0].posmem+V[0].offset] = (V[1].valor>>24)&0xFF;
            MEM[V[0].posmem+V[0].offset+1] = (V[1].valor>>16)&0xFF;
            MEM[V[0].posmem+V[0].offset+2] = (V[1].valor>>8)&0xFF;
            MEM[V[0].posmem+V[0].offset+3] = (V[1].valor)&0xFF;

            switch(V[1].segmento)
            {
                case 0:
                    for(i = 0 ; i < 4 ; i++)
                    {
                        REG[V[1].registro] = REG[V[1].registro]<<8;
                        REG[V[1].registro] = auxV1[i];
                    }
                break;
                case 1:
                    REG[V[1].registro] &= 0xFFFFFF00;
                    REG[V[1].registro] |= (auxV1[3])&0xFF;
                break;
                case 2:
                    REG[V[1].registro] &= 0xFFFF00FF;
                    aux = auxV1[2];
                    REG[V[1].registro] |= (aux<<8)&0xFF00;
                break;
                case 3:
                    REG[V[1].registro] &= 0xFFFF0000;
                    for (i = 2 ; i < 4 ; i++)
                    {
                        aux = aux<<8;
                        aux = (auxV1[i]);
                    }
                    REG[V[1].registro] |= aux&0xFFFF;
                break;
            }
            leemem(V, 1, MEM, REG, TDS);
        }
    else // op1 es registro
        if (V[1].op == 0) // op2 es memoria
        {
            for(i = 0 ; i < 4 ; i++)
                auxV1[i] = MEM[V[1].posmem+V[1].offset+i];
            MEM[V[1].posmem+V[1].offset] = (V[0].valor>>24)&0xFF;
            MEM[V[1].posmem+V[1].offset+1] = (V[0].valor>>16)&0xFF;
            MEM[V[1].posmem+V[1].offset+2] = (V[0].valor>>8)&0xFF;
            MEM[V[1].posmem+V[1].offset+3] = (V[0].valor)&0xFF;

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
            V[1].valor = REG[V[1].registro];
            V[0].valor = REG[V[0].registro];
        }
}

void MUL(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int aux1 = 0;
    if(V[0].op == 0) //op1 es memoria
    {
        V[0].valor *= V[1].valor;
        MEM[V[0].posmem+V[0].offset] = V[0].valor>>24;
        MEM[V[0].posmem+V[0].offset+1] = V[0].valor>>16;
        MEM[V[0].posmem+V[0].offset+2] = V[0].valor>>8;
        MEM[V[0].posmem+V[0].offset+3] = V[0].valor;
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
    setCC(V, MEM, REG, TDS);
}

void DIV(datos V[], char MEM[], int REG[], TRTDS TDS[])
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
            MEM[V[0].posmem+V[0].offset] = V[0].valor>>24;
            MEM[V[0].posmem+V[0].offset+1] = V[0].valor>>16;
            MEM[V[0].posmem+V[0].offset+2] = V[0].valor>>8;
            MEM[V[0].posmem+V[0].offset+3] = V[0].valor;
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
        setCC(V, MEM, REG, TDS);
    }
}

void CMP(datos V[], char MEM[], int REG[], TRTDS TDS[])
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

void SHL(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    V[0].valor = V[0].valor<<V[1].valor;
    if(V[0].op == 0) //op1 es memoria
    {
        MEM[V[0].posmem+V[0].offset] = V[0].valor>>24;
        MEM[V[0].posmem+V[0].offset+1] = V[0].valor>>16;
        MEM[V[0].posmem+V[0].offset+2] = V[0].valor>>8;
        MEM[V[0].posmem+V[0].offset+3] = V[0].valor;
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
    setCC(V, MEM, REG, TDS);
}

void SHR(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    V[0].valor = V[0].valor>>V[1].valor;
    if(V[0].op == 0) //op1 es memoria
    {
        MEM[V[0].posmem+V[0].offset] = V[0].valor>>24;
        MEM[V[0].posmem+V[0].offset+1] = V[0].valor>>16;
        MEM[V[0].posmem+V[0].offset+2] = V[0].valor>>8;
        MEM[V[0].posmem+V[0].offset+3] = V[0].valor;
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
    setCC(V, MEM, REG, TDS);
}

void AND(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int aux1 = 0;
    if(V[0].op == 0) //op1 es memoria
    {
        V[0].valor &= V[1].valor;
        MEM[V[0].posmem+V[0].offset] = V[0].valor>>24;
        MEM[V[0].posmem+V[0].offset+1] = V[0].valor>>16;
        MEM[V[0].posmem+V[0].offset+2] = V[0].valor>>8;
        MEM[V[0].posmem+V[0].offset+3] = V[0].valor;
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
    setCC(V, MEM, REG, TDS);
}

void OR(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int aux1 = 0;
    if(V[0].op == 0) //op1 es memoria
    {
        V[0].valor |= V[1].valor;
        MEM[V[0].posmem+V[0].offset] = V[0].valor>>24;
        MEM[V[0].posmem+V[0].offset+1] = V[0].valor>>16;
        MEM[V[0].posmem+V[0].offset+2] = V[0].valor>>8;
        MEM[V[0].posmem+V[0].offset+3] = V[0].valor;
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
    setCC(V, MEM, REG, TDS);
}

void XOR(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int aux1 = 0;
    if(V[0].op == 0) //op1 es memoria
    {
        V[0].valor ^= V[1].valor;
        MEM[V[0].posmem+V[0].offset] = V[0].valor>>24;
        MEM[V[0].posmem+V[0].offset+1] = V[0].valor>>16;
        MEM[V[0].posmem+V[0].offset+2] = V[0].valor>>8;
        MEM[V[0].posmem+V[0].offset+3] = V[0].valor;
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
    setCC(V, MEM, REG, TDS);
}

void SYS(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int pos = 0, i, j, d = 0, c = 0, o = 0, x = 0;
    short int al = 0, cl = 0, ch = 0;
    int aux = 0, aux2 = 0, dx = REG[13], ds = REG[1];
    al = REG[10]&0xFF;
    ch = (REG[12]>>8) & 0xFF;
    cl = REG[12] & 0xFF;
    pos = base(dx, TDS, REG) + (dx&0xFFFF);
    switch(V[0].valor)
    {
        case 1: //lectura
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
                        scanf("%c", &aux);
                        for(j = 0 ; j < ch ; j++)
                        {
                            MEM[pos+j] = aux;
                            aux = aux>>8;
                        }
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
        case 2: // imprimir
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
                    printf("[%04X]: %d\n", pos-base(ds, TDS, REG), aux);
                    pos += ch;
                    aux = 0;
                }
                printf("\n");
            }
            pos = base(dx, TDS, REG) + (REG[13]&0xFFFF);
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
                    printf("[%04X]: %c\n", pos-base(ds, TDS, REG), aux);
                    pos += ch;
                    aux = 0;
                }
                printf("\n");
            }
            pos = base(dx, TDS, REG) + (REG[13]&0xFFFF);
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
                    printf("[%04X]: %011o\n", pos-base(ds, TDS, REG), aux);
                    pos += ch;
                    aux = 0;
                }
                printf("\n");
            }
            pos = base(dx, TDS, REG) + (REG[13]&0xFFFF);
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
                    printf("[%04X]: %08X\n", pos-base(ds, TDS, REG), aux);
                    pos += ch;
                    aux = 0;
                }
                printf("\n");
            }
        break;
    }
}

void JMP(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    REG[5] = 0;
    REG[5] |= (V[0].valor)&0xFFFF;
}

void JZ(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int Z = (REG[8]>>30) & 0x01;

    if(Z == 1)
    {
        REG[5] = 0;
        REG[5] = (V[0].valor)&0xFFFF;
    }
}

void JP(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int N = (REG[8]>>31) & 0x01;
    if(N == 0)
    {
        REG[5] = 0;
        REG[5] = (V[0].valor)&0xFFFF;
    }
}

void JN(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int N = (REG[8]>>31) & 0x01;

    if(N == 1)
    {
        REG[5] = 0;
        REG[5] = (V[0].valor)&0xFFFF;
    }
}

void JNZ(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int Z = (REG[8]>>30) & 0x01;

    if(Z == 0)
    {
        REG[5] = 0;
        REG[5] = (V[0].valor)&0xFFFF;
    }
}

void JNP(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int N = (REG[8]>>31) & 0x01, Z = (REG[8]>>30) & 0x01;

    if(N == 1 || Z == 1 )
    {
        REG[5] = 0;
        REG[5] = (V[0].valor)&0xFFFF;
    }
}

void JNN(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int N = (REG[8]>>31) & 0x01;

    if(N == 0)
    {
        REG[5] = 0;
        REG[5] = (V[0].valor)&0xFFFF;
    }
}

void LDL(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    REG[9] &= 0xFFFF0000;
    REG[9] |= (V[0].valor & 0x0000FFFF);
}

void LDH(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    int aux = 0;
    REG[9] &= 0x0000FFFF;
    aux = V[0].valor;
    REG[9] |= ((aux<<16)&0xFFFF0000);
}

void RND(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    srand(time(NULL));
    REG[9] = rand() % V[0].valor;
}

void NOT(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    V[0].valor = ~V[0].valor;
    if(V[0].op == 0) //op1 es memoria
    {
        MEM[V[0].posmem+V[0].offset+3] = V[0].valor;
        MEM[V[0].posmem+V[0].offset+2] = V[0].valor>>8;
        MEM[V[0].posmem+V[0].offset+1] = V[0].valor>>16;
        MEM[V[0].posmem+V[0].offset] = V[0].valor>>24;
    }
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
    setCC(V, MEM, REG, TDS);
}

void PUSH(datos V[], char MEM[], int REG[], TRTDS TDS[]){}

void POP(datos V[], char MEM[], int REG[], TRTDS TDS[]){}

void CALL(datos V[], char MEM[], int REG[], TRTDS TDS[]){}

void STOP(datos V[], char MEM[], int REG[], TRTDS TDS[])
{
    printf("\n");
}

void RET(datos V[], char MEM[], int REG[], TRTDS TDS[]){}

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

void dis1(texto nombre, datos V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0].op == 0) // memoria
        printf("%s  [%s+%d]\n", nombre, registro[V[0].registro][V[0].segmento], V[0].offset);
    else
        if(V[0].op == 1) // inmediato
            printf("%s  %d\n", nombre, V[0].valor&0xFFFF);
        else // registro
            printf("%s  %s\n", nombre, registro[V[0].registro][V[0].segmento]);
}

void dis2(texto nombre, datos V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0].op == 0) //op1 es memoria
        if(V[1].op == 0) //memoria y memoria
            printf("%s  [%s+%d], [%s+%d] \n", nombre, registro[V[0].registro][V[0].segmento], V[0].offset, registro[V[1].registro][V[1].segmento], V[1].offset);
        else
            if(V[1].op == 1) // memoria e inmediato
                printf("%s  [%s+%d], %d \n", nombre, registro[V[0].registro][V[0].segmento], V[0].offset, V[1].valor);
            else //memoria y registro
                printf("%s  [%s+%d], %s \n", nombre, registro[V[0].registro][V[0].segmento], V[0].offset, registro[V[1].registro][V[1].segmento]);
    else
        if(V[0].op == 1) //op1 es inmediato
            if(V[1].op == 0) //inmediato y memoria
                printf("%s  %d, [%s+%d] \n", nombre, V[0].valor, registro[V[1].registro][V[1].segmento], V[1].offset);
            else
                if(V[1].op == 1) //inmediato e inmediato
                    printf("%s  %d, %d \n", nombre, V[0].valor, V[1].valor);
                else //inmediato y registro
                    printf("%s  %d, %s \n", nombre, V[0].valor, registro[V[1].registro][V[1].segmento]);
        else //op1 es registro
            if(V[1].op == 0) //registro y memoria
                printf("%s  %s, [%s+%d] \n", nombre, registro[V[0].registro][V[0].segmento], registro[V[1].registro][V[1].segmento], V[1].offset);
            else
                if(V[1].op == 1) //registro e inmediato
                    printf("%s  %s, %d \n", nombre, registro[V[0].registro][V[0].segmento], V[1].valor);
                else //registro y registro
                    printf("%s  %s, %s \n", nombre, registro[V[0].registro][V[0].segmento], registro[V[1].registro][V[1].segmento]);
}

void disassembler(int codop, datos V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    texto Nombres[256];
    cargaNombres(Nombres);
    if (V[0].op == 3)
        dis0(Nombres[codop]);
    else
        if (V[1].op == 3)
            dis1(Nombres[codop], V, REG, TDS, registro);
        else
            dis2(Nombres[codop], V, REG, TDS, registro);
}

void procesoDatos(char MEM[], int REG[], TRTDS TDS[], t_func funciones[], int ejecutar, char infoVM[])
{
    char inst;
    int codop = 0, ds = REG[1];
    datos V[2];
    texto registro[16][4];
    cargaMatriz (registro);
    if(ejecutar)
        printf("\n%s TAM: %d\n\n", infoVM, base(ds, TDS, REG));
    while(ejecutar == 1 && REG[5] < base(ds, TDS, REG))
    {
        inst = MEM[REG[5]++];
        V[0].inst = inst&0xFF;
        codigosDis(V[0].inst, MEM, REG, TDS);
        codigos(V[0].inst, &codop, V, MEM, REG, TDS);

        if( (codop < 0) || (codop > 12 && codop < 48) || (codop > 59 && codop < 240) || (codop > 240))
        {
            printf("\nInstruccion invalidadis\n");
            exit(1);
        }
        disassembler(codop, V, REG, TDS, registro);
    }
    printf("\n");
    iniciaRegistros(REG);
    codop = 0;
    while(codop != 240 && REG[5] < base(ds, TDS, REG))
    {
        inst = MEM[REG[5]++];
        V[0].inst = inst&0xFF;
        codigos(V[0].inst, &codop, V, MEM, REG, TDS);
        if( (codop < 0) || (codop > 12 && codop < 48) || (codop > 59 && codop < 240) || (codop > 240))
        {
            printf("\nInstruccion invalidarun\n");
            exit(1);
        }
        funciones[codop](V, MEM, REG, TDS);
    }
}

int main(int argc, char *argv[])
{
    int RAM = 16384, REG[16], dissa=0;
    char DirArchivo[120], DirArchivoDebugger[120], infoMV[60], info[30];
    TRTDS TDS[8];
    t_func funciones[256];

    strcpy(DirArchivo, argv[1]);

    for(int i = 2; i < argc ; i++)
    {
        strcpy(info,argv[i]);
        if(info[0]=='m') //es el tamaño de la RAM
        {
        //funcion para convertir el string a int
        memmove(info, info + 2, strlen(info)-1);
        RAM = atoi(info);
        }
        else
            if(strcmp(info,"-d")==0) // ejecuta o no el dissasembler
                dissa = 1;
            else//es el nombre del archivo
                strcpy(DirArchivoDebugger, info);
    }
    char MEM[RAM];

    cargaFunciones(funciones);

    lectura(MEM, REG, TDS, RAM, DirArchivo, infoMV);
    procesoDatos(MEM, REG, TDS, funciones, dissa, infoMV);

    return 0;
}
