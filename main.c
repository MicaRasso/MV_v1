#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//tipo tabla de descriptores de segmento
typedef struct
{
    short int base, size;
} TRTDS;

typedef char texto[5];

typedef void (*t_dis) (int V[], int REG[], TRTDS TDS[], texto registro[16][4]);

typedef void (*t_func)(int V[], char MEM[], int REG[], TRTDS TDS[]);

void iniciaTablaDeSegmentos(TRTDS TDS[], int RAM, int TAM)
{
    TDS[0].base = 0;
    TDS[0].size = TAM;
    TDS[1].base = TAM;
    TDS[1].size = RAM - TAM;
}

void iniciaRegistros(int REG[])
{
    REG[0] = 0;        //CS
    REG[1] = 0x10000;  //DS
//  REG[2] = ;
//  REG[3] = ;
//  REG[4] = ;
    REG[5] = REG[0];   //IP
//  REG[6] = ;
//  REG[7] = ;
    REG[8] = 0;        //CC
//  REG[9] =             AC
//  REG[10] =            A
//  REG[11] =            B
//  REG[12] =            C
//  REG[13] =            D
//  REG[14] =            E
//  REG[15] =            F
}

short int baseds(TRTDS TDS[], int REG[])
{
    return TDS[(REG[1])>>16].base;
}

short int corrimiento (int aux, int izq, int der)
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

void lee4byte(int *var, char MEM[], int REG[], TRTDS TDS[])
{
    int i , aux = 0;
    for(i = 3 ; i >= 0 ; i--)
    {
        if(REG[5] > baseds(TDS, REG))
        {
            printf("Fallo de segmento\n");
            exit(1);
        }
        aux = MEM[REG[5]++];
        aux |= aux<<8*i;
        mascaras(&aux, i);
    }
    *var = aux;
}

void lee2byte(short int *var, char MEM[], int REG[], TRTDS TDS[])
{
    int i, aux = 0;
    *var = 0;
    for(i = 1 ; i >= 0 ; i--)
    {
        if(REG[5] > baseds(TDS, REG))
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
    int aux = 0;
    if(REG[5] > baseds(TDS, REG))
    {
        printf("Fallo de segmento\n");
        exit(1);
    }
    aux = MEM[REG[5]++];
    mascaras(&aux, 0);
    *var = aux;
}

void leedemem(int *var, char MEM[], short int posmem, TRTDS TDS[])
{
    int i, j = 3, aux2 = 0, aux = 0;

    for(i = 0 ; i < 4 ; i++)
    {
        if(posmem+j > TDS[1].size){
            printf("Fallo de segmento\n");
            exit(1);
        }
        else{
            aux = MEM[posmem+j--];
            aux = aux<<(8*i);
            mascaras(&aux, i);
            aux2 |= aux;
        }
    }
    *var = aux2;
}

void leemem(short int *posmem, char MEM[], int REG[], TRTDS TDS[])
{
    char registro = 0;
    short int offset = 0;
    int aux = 0;

    lee1byte(&registro, MEM, REG, TDS);
    lee2byte(&offset, MEM, REG, TDS);
    *posmem = baseds(TDS, REG) + offset;
    if(registro != 1)
    {
        aux = registro;
        (*posmem) += REG[aux];
    }
}

void leeinm(short int *inmediato, char MEM[], int REG[], TRTDS TDS[])
{
    short int aux = 0;
    lee2byte(&aux, MEM, REG, TDS);
    *inmediato = aux;
}

void leereg(char *registro, char *segmento, char MEM[], int REG[], TRTDS TDS[])
{
    char aux = 0;
    lee1byte(&aux, MEM, REG, TDS);
    *registro = aux&0x0F;
    *segmento = (aux>>4)&0x03;
}

void lectura(char MEM[], int *TAM, char DirArchivo[], char infoMV[])
{
    FILE *arch;
    char encabezado[6], version, c;
    arch = fopen(DirArchivo, "rb");
    int i, aux = 0;

    if(arch != NULL)
    {
        fread(encabezado, sizeof(char), 5, arch);
        encabezado[5] = '\0';

        fread(&version, sizeof(char), 1, arch);

        *TAM = 0;

        fread(&c, sizeof(char), 1, arch);
        aux = c;
        mascaras(&aux, 0);
        aux = (aux<<8);
        *TAM |= aux;

        fread(&c, sizeof(char), 1, arch);
        aux = c;
        mascaras(&aux, 0);
        *TAM |= aux;

        for (i=0 ; i < *TAM ; i++)
        {
            fread(&c,sizeof(char),1,arch);
            MEM[i] = c;
        }
        fclose(arch);

        strcpy(infoMV,encabezado);
        strcat(infoMV," VER: ");
        version += 48;
        infoMV[11] = version;
        infoMV[12] = '\0';
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
    printf("[%04d] ",IP);
    while(i < op1)
    {
        printf("%02x ",(MEM[IP])&0xFF);
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

void codigos(int inst, int *codop, int V[], char MEM[], int REG[], TRTDS TDS[])
{
    char registro, segmento;
    short int posmem = 0, inmediato = 0;
    int aux, i = 0, valor;
    V[0] = (inst>>0x6)&0x03;
    V[0] &= 0xFF;
    V[4] = (inst>>0x4)&0x03;
    V[4] &= 0xFF;
    if (V[0] == 3) // probar 0x03
        *codop = inst&0xFF;
    else
        if (V[4] == 3)
            *codop = inst&0x3F;
        else
            *codop = inst&0x0F;
    while(i < 5)
    {
        switch (V[i])
        {
            case 0: // op1 es memoria
                leemem(&posmem, MEM, REG, TDS);
                V[1+i] = posmem;
                leedemem(&valor, MEM, posmem,TDS);
                V[3+i] = valor;
            break;
            case 1:
                leeinm(&inmediato, MEM, REG, TDS);
                aux = inmediato&0xFFFF;
                V[3+i] = aux;
            break;
            case 2: // op1 es registro
                leereg(&registro, &segmento, MEM, REG, TDS);
                if (registro == 1)
                {
                    V[1+i] = baseds(TDS, REG);
                    V[2+i] = 0;
                    leedemem(&valor, MEM, V[1+i], TDS);
                    V[1+i] = 1;
                    V[3+i] = valor;
                }
                else
                {
                    V[1+i] = registro;
                    V[2+i] = segmento;
                    switch (V[2+i])
                    {
                        case 0:
                            V[3+i] = REG[V[1+i]];
                        break;
                        case 1:
                            V[3+i] = corrimiento(REG[V[1+i]], 24, 24);
                        break;
                        case 2:
                            V[3+i] = corrimiento(REG[V[1+i]], 16, 24);
                        break;
                        case 3:
                            V[3+i] = corrimiento(REG[V[1+i]], 16, 16);
                        break;
                    }
                }
            break;
        }
        i += 4;
    }
}

void setCC(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    if(V[3] > 0)
        REG[8] = 0;
    else
        if(V[3] < 0)
            REG[8] = 0x80000000;
        else //V[3] == 0
            REG[8] = 0x40000000;
}

void MOV(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    int i = 0;

    if(V[0] == 0) //op1 es memoria
        if (V[4] == 0) //op2 es memoria
            for (i = 0 ; i < 4 ; i++)
                MEM[V[1]+i] = MEM[V[5]+i];
        else
            if (V[4] == 1) // op2 es inmediato
            {
                MEM[V[1]+2] = V[7]>>8;
                MEM[V[1]+3] = V[7];
            }
            else // op2 es registro
            {
                MEM[V[1]] = V[7]>>24;
                MEM[V[1]+1] = V[7]>>16;
                MEM[V[1]+2] = V[7]>>8;
                MEM[V[1]+3] = V[7];
            }
    else //op1 es registro
    {
        switch(V[2])
        {
            case 0:
                REG[V[1]] = V[7];
            break;
            case 1:
                REG[V[1]] &= 0xFFFFFF00;
                if (V[4] != 2)
                {
                    if (((V[7]>>8) & 0x01) == 1)
                        V[7] |= 0xFFFFFF00;
                    else
                        V[7] &= 0x000000FF;
                }
                REG[V[1]] |= V[7];
            break;
            case 2:
                REG[V[1]] &= 0xFFFF00FF;
                if (V[4] != 2)
                {
                    if ((((V[7]>>8)&0x01) == 1))
                        V[7] |= 0xFFFFFF00;
                    else
                        V[7] &= 0x000000FF;
                }
                REG[V[1]] |= V[7]<<8;
            break;
            case 3:
                REG[V[1]] &= 0xFFFF0000;
                if (V[4] != 2)
                {
                    if ((((V[7]>>16)&0x01) == 1))
                        V[7] |= 0xFFFF0000;
                    else
                        V[7] &= 0x0000FFFF;
                }
                REG[V[1]] |= V[7];
        }
    }
}

void ADD(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    V[3] += V[7];
    if(V[0] == 0) //op1 es memoria
    {
        MEM[V[1]+3] = V[3];
        MEM[V[1]+2] = V[3]>>8;
        MEM[V[1]+1] = V[3]>>16;
        MEM[V[1]] = V[3]>>24;
    }
    else //op1 es registro
    {
        switch(V[2])
        {
            case 0:
                REG[V[1]] += V[7];
            break;
            case 1:
                REG[V[1]] += V[7];
            break;
            case 2:
                REG[V[1]] += V[7]<<8;
            break;
            case 3:
                REG[V[1]] += V[7];
        }
    }

    setCC(V, MEM, REG, TDS);
}

void SUB(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    V[3] -= V[7];
    if(V[0] == 0) //op1 es memoria
    {
        MEM[V[1]+3] = V[3];
        MEM[V[1]+2] = V[3]>>8;
        MEM[V[1]+1] = V[3]>>16;
        MEM[V[1]] = V[3]>>24;
    }
    else //op1 es registro
    {
        switch(V[2])
        {
            case 0:
                REG[V[1]] -= V[7];
            break;
            case 1:
                REG[V[1]] -=  V[7];
            break;
            case 2:
                REG[V[1]] -=  V[7]<<8;
            break;
            case 3:
                REG[V[1]] -=  V[7];
        }
    }

    setCC(V, MEM, REG, TDS);
}

void SWAP(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    char auxV1[4], auxV2[4];
    int i = 0;

    if(V[0] == 0) // op1 es memoria
        if (V[4] == 0) // op2 es memoria
        {
            for(i = 0 ; i < 4 ; i++)
            {
                auxV1[i] = MEM[V[1]+i];
                auxV2[i] = MEM[V[5]+i];
            }
            for(i = 0 ; i < 4 ; i++)
            {
                MEM[V[5]+i] = auxV1[i];
                MEM[V[1]+i] = auxV2[i];
            }
            for(i = 0 ; i < 4 ; i++)
            {
                V[3] = V[3]<<8;
                V[3] = MEM[V[1]+i];
            }
            for(i = 0 ; i < 4 ; i++)
            {
                V[7] = V[7]<<8;
                V[7] = MEM[V[5]+i];
            }
        }
        else // op2 es registro
        {
            for(i = 0 ; i < 4 ; i++)
                auxV1[i] = MEM[V[1]+i];
            switch(V[6])
            {
                case 0:
                    V[7] = REG[V[5]];
                break;
                case 1:
                    V[7] = corrimiento(REG[V[5]], 24, 24);
                break;
                case 2:
                    V[7] = corrimiento(REG[V[5]], 16, 24);
                break;
                case 3:
                    V[7] = corrimiento(REG[V[5]], 16, 16);
                break;
            }
            MEM[V[1]] = (V[7]>>24)&0xFF;
            MEM[V[1]+1] = (V[7]>>16)&0xFF;
            MEM[V[1]+2] = (V[7]>>8)&0xFF;
            MEM[V[1]+3] = (V[7])&0xFF;

            for(i = 0 ; i < 4 ; i++)
            {
                REG[V[5]]<<8;
                REG[V[5]] = auxV1[i];
            }
        }
    else // op1 es registro
        if (V[4] == 0) // op2 es memoria
        {
            for(i = 0 ; i < 4 ; i++)
                auxV2[i] = MEM[V[5]+i];
            switch(V[2])
            {
                case 0:
                    V[3] = REG[V[1]];
                break;
                case 1:
                    V[3] = corrimiento(REG[V[1]], 24, 24);
                break;
                case 2:
                    V[3] = corrimiento(REG[V[1]], 16, 24);
                break;
                case 3:
                    V[3] = corrimiento(REG[V[1]], 16, 16);
                break;
            }
            MEM[V[5]] = (V[3]>>24)&0xFF;
            MEM[V[5]+1] = (V[3]>>16)&0xFF;
            MEM[V[5]+2] = (V[3]>>8)&0xFF;
            MEM[V[5]+3] = (V[3])&0xFF;
            for(i = 0 ; i < 4 ; i++)
            {
                REG[V[1]]<<8;
                REG[V[1]] = auxV2[i];
            }
        }
        else // op2 es registro
        {
            V[3] = REG[V[1]];
            switch(V[2])
            {
                case 1:
                    V[3] = corrimiento(V[3], 24, 24);
                break;
                case 2:
                    V[3] = corrimiento(V[3], 16, 24);
                break;
                case 3:
                    V[3] = corrimiento(V[3], 16, 16);
                break;
            }
            V[7] = REG[V[5]];
            switch(V[6])
            {
                case 0:
                    REG[V[5]] = V[3];
                case 1:
                    V[7] = corrimiento(V[7], 24, 24);
                    REG[V[5]] &= 0xFFFFFF00;
                    REG[V[5]] |= V[3];
                break;
                case 2:
                    V[7] = corrimiento(V[7], 16, 24);
                    REG[V[5]] &= 0xFFFF00FF;
                    REG[V[5]] |= (V[3]<<8);
                break;
                case 3:
                    V[7] = corrimiento(V[7], 16, 16);
                    REG[V[5]] &= 0xFFFF0000;
                    REG[V[5]] |= (V[3]);
                break;
            }
            switch(V[2])
            {
                case 0:
                    REG[V[1]] = V[7];
                case 1:
                    REG[V[1]] &= 0xFFFFFF00;
                    REG[V[1]] |= V[7];
                break;
                case 2:
                    REG[V[1]] &= 0xFFFF00FF;
                    REG[V[1]] |= (V[7]<<8);
                break;
                case 3:
                    REG[V[1]] &= 0xFFFF0000;
                    REG[V[1]] |= (V[7]);
                break;
            }
        }
}

void MUL(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    V[3] *= V[7];
    if(V[0] == 0) //op1 es memoria
    {
        MEM[V[1]+3] = V[3];
        MEM[V[1]+2] = V[3]>>8;
        MEM[V[1]+1] = V[3]>>16;
        MEM[V[1]] = V[3]>>24;
    }
    else //op1 es registro
    {
        switch(V[2])
        {
            case 0:
                REG[V[1]] *= V[7];
            break;
            case 1:
                REG[V[1]] *= V[7];
            break;
            case 2:
                REG[V[1]] *= V[7]<<8;
            break;
            case 3:
                REG[V[1]] *= V[7];
        }
    }

    setCC(V, MEM, REG, TDS);
}

void DIV(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    if (V[7] == 0)
    {
        printf("Division por cero\n");
        exit(1);
    }
    else
    {
        REG[9] = V[3] % V[7];
        V[3] /= V[7];
        if(V[0] == 0) //op1 es memoria
        {
            MEM[V[1]+3] = V[3];
            MEM[V[1]+2] = V[3]>>8;
            MEM[V[1]+1] = V[3]>>16;
            MEM[V[1]] = V[3]>>24;
        }
        else //op1 es registro
        {
            REG[9] = REG[V[1]] % V[7];
            switch(V[2])
            {
                case 0:
                    REG[V[1]] /= V[7];
                break;
                case 1:
                    REG[V[1]] /= V[7];
                break;
                case 2:
                    REG[V[1]] /= V[7]<<8;
                break;
                case 3:
                    REG[V[1]] /= V[7];
            }
        }
    }

    setCC(V, MEM, REG, TDS);
}

void CMP(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    V[3] -= V[7];
    setCC(V, MEM, REG, TDS);
}

void SHL(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    V[3] = V[3]<<V[7];
    if(V[0] == 0) //op1 es memoria
    {
        MEM[V[1]+3] = V[3];
        MEM[V[1]+2] = V[3]>>8;
        MEM[V[1]+1] = V[3]>>16;
        MEM[V[1]] = V[3]>>24;
    }
    else //op1 es registro
    {
        switch(V[2])
        {
            case 0:
                REG[V[1]] = V[3];
            break;
            case 1:
                REG[V[1]] = V[3]&0xFF;
            break;
            case 2:
                REG[V[1]] = (V[7]&0xFF)<<8;
            break;
            case 3:
                REG[V[1]] = V[7]&0xFFFF;
        }
    }

    setCC(V, MEM, REG, TDS);
}

void SHR(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    V[3] = V[3]>>V[7];
    if(V[0] == 0) //op1 es memoria
    {
        MEM[V[1]+3] = V[3];
        MEM[V[1]+2] = V[3]>>8;
        MEM[V[1]+1] = V[3]>>16;
        MEM[V[1]] = V[3]>>24;
    }
    else //op1 es registro
    {
        switch(V[2])
        {
            case 0:
                REG[V[1]] = V[3];
            break;
            case 1:
                REG[V[1]] = V[3]&0xFF;
            break;
            case 2:
                REG[V[1]] = (V[7]&0xFF)<<8;
            break;
            case 3:
                REG[V[1]] = V[7]&0xFFFF;
        }
    }

    setCC(V, MEM, REG, TDS);
}

void AND(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    V[3] &= V[7];
    if(V[0] == 0) //V[0] es memoria
    {
        MEM[V[1]+3] = V[3];
        MEM[V[1]+2] = V[3]>>8;
        MEM[V[1]+1] = V[3]>>16;
        MEM[V[1]] = V[3]>>24;
    }
    else //V[0] es registro
    {
        switch(V[2])
        {
            case 0:
                REG[V[1]] &= V[7];
            break;
            case 1:
                REG[V[1]] &= V[7];
            break;
            case 2:
                REG[V[1]] &= V[7]<<8;
            break;
            case 3:
                REG[V[1]] &= V[7];
        }
    }

    setCC(V, MEM, REG, TDS);
}

void OR(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    V[3] |= V[7];
    if(V[0] == 0) //op1 es memoria
    {
        MEM[V[1]+3] = V[3];
        MEM[V[1]+2] = V[3]>>8;
        MEM[V[1]+1] = V[3]>>16;
        MEM[V[1]] = V[3]>>24;
    }
    else //op1 es registro
    {
        switch(V[2])
        {
            case 0:
                REG[V[1]] |= V[7];
            break;
            case 1:
                REG[V[1]] |= V[7];
            break;
            case 2:
                REG[V[1]] |= V[7]<<8;
            break;
            case 3:
                REG[V[1]] |= V[7];
        }
    }

    setCC(V, MEM, REG, TDS);
}

void XOR(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    V[3] ^= V[7];
    if(V[0] == 0) //op1 es memoria
    {
        MEM[V[1]+3] = V[3];
        MEM[V[1]+2] = V[3]>>8;
        MEM[V[1]+1] = V[3]>>16;
        MEM[V[1]] = V[3]>>24;
    }
    else //op1 es registro
    {
        switch(V[2])
        {
            case 0:
                REG[V[1]] ^= V[7];
            break;
            case 1:
                REG[V[1]] ^= V[7];
            break;
            case 2:
                REG[V[1]] ^= V[7]<<8;
            break;
            case 3:
                REG[V[1]] ^= V[7];
        }
    }

    setCC(V, MEM, REG, TDS);
}

void SYS(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    int posmem = 0, i, j;
    short int al = 0, cl = 0, ch = 0;
    int aux = 0, aux2 = 0;

    al = REG[10]&0xFF;
    ch = (REG[12]>>8) & 0xFF;
    cl = REG[12] & 0xFF;
    posmem = baseds(TDS, REG) + (REG[13]&0xFFFF); //dx

    switch(V[3])
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
                            MEM[posmem+j] = aux;
                            aux = aux>>8;
                        }
                        posmem += ch;
                    }
                break;
                case 2: // caracteres
                    for(i = 0 ; i < cl ; i++)
                    {
                        printf("Ingrese datos (caracter): ");
                        scanf("%c", &aux);
                        for(j = 0 ; j < ch ; j++)
                        {
                            MEM[posmem+j] = aux;
                            aux = aux>>8;
                        }
                        posmem += ch;
                    }
                break;
                case 4: // octal
                    for(i = 0 ; i < cl ; i++)
                    {
                        printf("Ingrese datos (octal): ");
                        scanf("%o", &aux);
                        for(j = 0 ; j < ch ; j++)
                        {
                            MEM[posmem+j] = aux;
                            aux = aux>>8;
                        }
                        posmem += ch;
                    }
                break;
                case 8: // hexadecimal
                    for(i = 0 ; i < cl ; i++)
                    {
                        printf("Ingrese datos (hexadecimal): ");
                        scanf("%x", &aux);
                        for(j = 0 ; j < ch ; j++)
                        {
                            MEM[posmem+j] = aux;
                            aux = aux>>8;
                        }
                        posmem += ch;
                    }
                break;
            }
        break;
        case 2: // imprimir
            switch(al)
            {
                case 1: // decimal
                    for(i = 0 ; i < cl ; i++)
                    {
                        for(j = 0 ; j < ch ; j++)
                        {
                            aux2 = MEM[posmem+j];
                            mascaras(&aux2, 0);
                            aux |= aux2<<8*(ch-j-1);
                        }
                        printf("[%04d]: %d\n", posmem-baseds(TDS, REG), aux);
                        posmem += ch;
                    }
                break;
                case 2: // caracteres
                    for(i = 0 ; i < cl ; i++)
                    {
                        for(j = 0 ; j < ch ; j++)
                        {
                            aux2 = MEM[posmem+j];
                            mascaras(&aux2, 0);
                            aux |= aux2<<8*(ch-j-1);
                        }
                        printf("[%04d]: %c\n", posmem-baseds(TDS, REG), aux);
                        posmem += ch;
                    }
                break;
                case 4: // octal
                    for(i = 0 ; i < cl ; i++)
                    {
                        for(j = 0 ; j < ch ; j++)
                        {
                            aux2 = MEM[posmem+j];
                            mascaras(&aux2, 0);
                            aux |= aux2<<8*(ch-j-1);
                        }
                        printf("[%04d]: %o\n", posmem-baseds(TDS, REG), aux);
                        posmem += ch;
                    }
                break;
                case 8: // hexadecimal
                    for(i = 0 ; i < cl ; i++)
                    {
                        for(j = 0 ; j < ch ; j++)
                        {
                            aux2 = MEM[posmem+j];
                            mascaras(&aux2, 0);
                            aux |= aux2<<8*(ch-j-1);
                        }
                        printf("[%04d]: %x\n", posmem-baseds(TDS, REG), aux);
                        posmem += ch;
                    }
                break;
            }
        break;
    }
}

void JMP(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    REG[5] = V[3];
}

void JZ(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    int Z = (REG[8]>>30) & 0x01;

    if(Z == 1)
        REG[5] = V[3];
}

void JP(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    int N = (REG[8]>>31) & 0x01;

    if(N == 0)
        REG[5] = V[3];
}

void JN(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    int N = (REG[8]>>31) & 0x01;

    if(N == 1)
        REG[5] = V[3];
}

void JNZ(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    int Z = (REG[8]>>30) & 0x01;

    if(Z == 0)
        REG[5] = V[3];
}

void JNP(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    int N = (REG[8]>>31) & 0x01, Z = (REG[8]>>30) & 0x01;

    if(N == 1 || Z == 1 )
        REG[5] = V[3];
}

void JNN(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    int N = (REG[8]>>31) & 0x01;

    if(N == 0)
        REG[5] = V[3];
}

void LDL(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    REG[9] &= 0xFFFF0000;
    REG[9] |= V[3] & 0x0000FFFF;
}

void LDH(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    REG[9] &= 0x0000FFFF;
    REG[9] |= V[3] & 0xFFFF0000;
}

void RND(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    srand(time(NULL));
    REG[9] = rand() % V[3];
}

void NOT(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    if(V[0] == 0) //op1 es memoria
    {
        V[3] = ~V[3];
        MEM[V[1]+3] = V[3];
        MEM[V[1]+2] = V[3]>>8;
        MEM[V[1]+1] = V[3]>>16;
        MEM[V[1]] = V[3]>>24;
    }
    else //op1 es registro
    {
        switch(V[2])
        {
            case 0: // ex
                REG[V[1]] = V[3];
            break;
            case 1: // l
                V[3] &= 0xFF;
                REG[V[1]] &= 0xFF;
                REG[V[1]] |= V[3];
            break;
            case 2: // h
                V[3] &= 0xFF00;
                REG[V[1]] &= 0xFF00;
                REG[V[1]] |= V[3];
            break;
            case 3: // x
                V[3] &= 0xFFFF;
                REG[V[1]] &= 0xFFFF;
                REG[V[1]] |= V[3];
        }
        V[3] = REG[V[1]];
    }

    setCC(V, MEM, REG, TDS);
}

void STOP(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    printf("\n");
}

void cargaMatriz(texto registro[16][4])
{
    strcpy(registro[0][0],"cs");
    strcpy(registro[1][0],"ds");
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

void dis0(texto nombre)
{
    printf("%s\n", nombre);
}

void dis1(texto nombre, int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0) // memoria
        printf("%s  [%d]\n", nombre, V[1]-baseds(TDS,REG));
    else
        if(V[0] == 1) // inmediato
            printf("%s  %d\n", nombre, V[3]);
        else // registro
            printf("%s  %s\n", nombre, registro[V[1]][V[2]]);
}

void dis2(texto nombre, int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0)
        if(V[4] == 0) //memoria a memoria
            printf("%s  [%d], [%d] \n", nombre, V[1]-baseds(TDS,REG), V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a memoria
                printf("%s  [%d], %d \n", nombre, V[1]-baseds(TDS,REG), V[7]);
            else // registro a memoria
                printf("%s  [%d], %s \n", nombre, V[1]-baseds(TDS,REG), registro[V[5]][V[6]]);
    else
        if(V[4] == 0) //memoria a registro
            printf("%s  %s, [%d] \n", nombre, registro[V[1]][V[2]], V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a registro
                printf("%s  %s, %d \n", nombre, registro[V[1]][V[2]], V[7]);
            else // registro a registro
                printf("%s  %s, %s \n", nombre, registro[V[1]][V[2]], registro[V[5]][V[6]]);
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
    strcpy(funciones[240], "STOP");
}

void disassembler(int codop, int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    texto Nombres[256];
    cargaNombres(Nombres);
    if (V[0] == 3)
        dis0(Nombres[codop]);
    else
        if (V[4] == 3)
            dis1(Nombres[codop], V, REG, TDS, registro);
        else
            dis2(Nombres[codop], V, REG, TDS, registro);
}

void procesoDatos(char MEM[], int REG[], TRTDS TDS[], t_func funciones[], int ejecutar, char infoVM[])
{
    char inst;
    int codop = 0, V[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    texto registro[16][4];
    cargaMatriz (registro);
    if(ejecutar)
        printf("\n%s TAM: %d\n\n", infoVM, baseds(TDS,REG));
    while(ejecutar == 1 && REG[5] < baseds(TDS, REG))
    {
        inst = MEM[REG[5]++];
        V[9] = inst&0xFF;
        codigosDis(V[9],MEM,REG,TDS);
        codigos(V[9], &codop, V, MEM, REG, TDS);

        if( (codop < 0) || (codop > 12 && codop < 48) || (codop > 59 && codop < 240) || (codop > 240))
        {
            printf("\nInstruccion invalida\n");
            exit(1);
        }
        disassembler(codop, V, REG, TDS, registro);
    }
    printf("\n");
    iniciaRegistros(REG);
    codop = 0;
    while(codop != 240 && REG[5] < baseds(TDS, REG))
    {
        inst = MEM[REG[5]++];
        V[9] = inst&0xFF;
        codigos(V[9], &codop, V, MEM, REG, TDS);
        if( (codop < 0) || (codop > 12 && codop < 48) || (codop > 59 && codop < 240) || (codop > 240))
        {
            printf("\nInstruccion invalida\n");
            exit(1);
        }
        funciones[codop](V, MEM, REG, TDS);
    }
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
    funciones[240] = STOP;
}

int main(int argc, char *argv[])
{
    int TAM, RAM = 16384, REG[16];
    //( 0 = CS / 1 = DS / 5 = IP / 8 = CC / 9 = AC )
    char MEM[RAM], DirArchivo[120], infoMV[15];
    TRTDS TDS[8];

    //declaracion de funciones
    t_func funciones[256];
    cargaFunciones(funciones);

    //inicializacion de variables y carga de memoria
    strcpy(DirArchivo, argv[1]);

    lectura(MEM, &TAM, DirArchivo,infoMV);
    iniciaTablaDeSegmentos(TDS, RAM, TAM);
    iniciaRegistros(REG);

    if( (argc == 3) && strcmp(argv[2], "-d") == 0 )
        procesoDatos(MEM, REG, TDS, funciones, 1, infoMV);
    else
        procesoDatos(MEM, REG, TDS, funciones, 0, infoMV);

    return 0;
}
