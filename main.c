#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//tipo tabla de descriptores de segmento
typedef struct
{
    short int base, size;
} TRTDS;

typedef char texto[4];

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
    int i , j, aux = 0;
    j = 4;
    for(i = 3 ; i >= 0 ; i--)
    {
        if(REG[5] > baseds(TDS, REG))
        {
            printf("Fallo de segmento\n");
            exit(1);
        }
        aux = MEM[REG[5]++];
        aux |= aux<<8*j--;
        mascaras(&aux, i);
    }
    *var = aux;
}

void lee2byte(short int *var, char MEM[], int REG[], TRTDS TDS[])
{
    int i , j, aux = 0;
    j = 2;
    for(i = 1 ; i >= 0 ; i--)
    {
        if(REG[5] > baseds(TDS, REG))
        {
            printf("Fallo de segmento\n");
            exit(1);
        }
        aux = MEM[REG[5]++];
        aux |= aux<<8*j--;
        mascaras(&aux, i);
    }
    *var = aux;
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

void leedemem(int *var, char MEM[], short int posmem)
{
    int i, j = 3, aux2 = 0, aux = 0;

    for(i = 0 ; i < 4 ; i++)
    {
        aux = MEM[posmem+j--];
        aux = aux<<(8*i);
        mascaras(&aux, i);
        aux2 |= aux;
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

void lectura(char MEM[], int *TAM, char DirArchivo[])
{
    FILE *arch;
    char encabezado[6], version, c;
    arch = fopen(DirArchivo, "rb");
    int i;

    if(arch != NULL)
    {
        fread(encabezado, sizeof(char), 5, arch);
        encabezado[5] = '\0';
        printf("%s ", encabezado);

        fread(&version, sizeof(char), 1, arch);
        printf("Ver: %d ", version);

        /*tomo los caracteres uno a uno y los opero bit a bit para pasarlos a (int)tamaño*/
        *TAM=0x00;
        fread(&c,sizeof(char),1,arch);
        *TAM = c;
        *TAM = (*TAM<<0X8);
        fread(&c,sizeof(char),1,arch);
        *TAM |= c;
        printf("TAM: %d\n",*TAM);

        for (i=0 ; i < *TAM ; i++)
        {
            //printf("posicion: %d, tamanio: %d    memoria: ",i,*TAM);
            fread(&c,sizeof(char),1,arch);
            MEM[i] = c;
            //printf("%d  %c\n", MEM[i], MEM[i]);
        }
        fclose(arch);
    }
    else
        printf("No se pudo abrir el archivo\n");
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
                leedemem(&valor, MEM, posmem);
                V[3+i] = valor;
            break;
            case 1:
                leeinm(&inmediato, MEM, REG, TDS);
                aux = inmediato&0xFFFF;
                V[3+i] = aux;
            break;
            case 2: // op1 es registro
                leereg(&registro, &segmento, MEM, REG, TDS);
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
            break;
        }
        i += 4;
    }
}

void disMOV(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0)
        if(V[4] == 0) //memoria a memoria
            printf("MOV [%d], [%d] \n",V[1]-baseds(TDS,REG), V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a memoria
                printf("MOV [%d], %d \n",V[1]-baseds(TDS,REG), V[7]);
            else // registro a memoria
                printf("MOV [%d], %s \n",V[1]-baseds(TDS,REG), registro[V[5]][V[6]]);
    else
        if(V[4] == 0) //memoria a registro
            printf("MOV %s, [%d] \n", registro[V[1]][V[2]], V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a registro
                printf("MOV %s, %d \n", registro[V[1]][V[2]], V[7]);
            else // registro a registro
                printf("MOV %s, %d \n", registro[V[1]][V[2]], V[5]);
}

void disADD(int V[], int REG[],  TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0)
        if(V[4] == 0) //memoria a memoria
            printf("ADD [%d], [%d] \n",V[1]-baseds(TDS,REG), V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a memoria
                printf("ADD [%d], %d \n",V[1]-baseds(TDS,REG), V[7]);
            else // registro a memoria
                printf("ADD [%d], %s \n",V[1]-baseds(TDS,REG), registro[V[5]][V[6]]);
    else
        if(V[4] == 0) //memoria a registro
            printf("ADD %s, [%d] \n", registro[V[1]][V[2]], V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a registro
                printf("ADD %s, %d \n", registro[V[1]][V[2]], V[7]);
            else // registro a registro
                printf("ADD %s, %d \n", registro[V[1]][V[2]], V[5]);
}

void disSUB(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0)
        if(V[4] == 0) //memoria a memoria
            printf("SUB [%d], [%d] \n",V[1]-baseds(TDS,REG), V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a memoria
                printf("SUB [%d], %d \n",V[1]-baseds(TDS,REG), V[7]);
            else // registro a memoria
                printf("SUB [%d], %s \n",V[1]-baseds(TDS,REG), registro[V[5]][V[6]]);
    else
        if(V[4] == 0) //memoria a registro
            printf("SUB %s, [%d] \n", registro[V[1]][V[2]], V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a registro
                printf("SUB %s, %d \n", registro[V[1]][V[2]], V[7]);
            else // registro a registro
                printf("SUB %s, %d \n", registro[V[1]][V[2]], V[5]);
}

void disSWAP(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0)
        if(V[4] == 0) //memoria a memoria
            printf("SWAP [%d], [%d] \n",V[1]-baseds(TDS,REG), V[5]-baseds(TDS,REG));
        else // registro a memoria
            printf("SWAP [%d], %s \n",V[1]-baseds(TDS,REG), registro[V[5]][V[6]]);
    else
        if(V[4] == 0) //memoria a registro
            printf("SWAP %s, [%d] \n", registro[V[1]][V[2]], V[5]-baseds(TDS,REG));
        else// registro a registro
            printf("SWAP %s, %d \n", registro[V[1]][V[2]], V[5]);
}

void disMUL(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0)
        if(V[4] == 0) //memoria a memoria
            printf("MUL [%d], [%d] \n",V[1]-baseds(TDS,REG), V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a memoria
                printf("MUL [%d], %d \n",V[1]-baseds(TDS,REG), V[7]);
            else // registro a memoria
                printf("MUL [%d], %s \n",V[1]-baseds(TDS,REG), registro[V[5]][V[6]]);
    else
        if(V[4] == 0) //memoria a registro
            printf("MUL %s, [%d] \n", registro[V[1]][V[2]], V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a registro
                printf("MUL %s, %d \n", registro[V[1]][V[2]], V[7]);
            else // registro a registro
                printf("MUL %s, %d \n", registro[V[1]][V[2]], V[5]);
}

void disDIV(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0)
        if(V[4] == 0) //memoria a memoria
            printf("DIV [%d], [%d] \n",V[1]-baseds(TDS,REG), V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a memoria
                printf("DIV [%d], %d \n",V[1]-baseds(TDS,REG), V[7]);
            else // registro a memoria
                printf("DIV [%d], %s \n",V[1]-baseds(TDS,REG), registro[V[5]][V[6]]);
    else
        if(V[4] == 0) //memoria a registro
            printf("DIV %s, [%d] \n", registro[V[1]][V[2]], V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a registro
                printf("DIV %s, %d \n", registro[V[1]][V[2]], V[7]);
            else // registro a registro
                printf("DIV %s, %d \n", registro[V[1]][V[2]], V[5]);
}

void disCMP(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0)
        if(V[4] == 0) //memoria - memoria
            printf("CMP [%d], [%d] \n",V[1]-baseds(TDS,REG), V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato - memoria
                printf("CMP [%d], %d \n",V[1]-baseds(TDS,REG), V[7]);
            else // registro - memoria
                printf("CMP [%d], %s \n",V[1]-baseds(TDS,REG), registro[V[5]][V[6]]);
    else
        if(V[0] == 1)
            if(V[4] == 0) //memoria - inmediato
                printf("CMP [%d], [%d] \n",V[4], V[5]-baseds(TDS,REG));
            else
                if(V[4] == 1) // inmediato - inmediato
                    printf("CMP [%d], %d \n",V[4], V[7]);
                else // registro - inmediato
                    printf("CMP [%d], %s \n",V[4], registro[V[5]][V[6]]);
        else
            if(V[4] == 0) //memoria - registro
                printf("CMP %s, [%d] \n", registro[V[1]][V[2]], V[5]-baseds(TDS,REG));
            else
                if(V[4] == 1) // inmediato - registro
                    printf("CMP %s, %d \n", registro[V[1]][V[2]], V[7]);
                else // registro - registro
                    printf("CMP %s, %d \n", registro[V[1]][V[2]], V[5]);
}

void disSHL(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0)
        if(V[4] == 0) //memoria a memoria
            printf("SHL [%d], [%d] \n",V[1]-baseds(TDS,REG), V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a memoria
                printf("SHL [%d], %d \n",V[1]-baseds(TDS,REG), V[7]);
            else // registro a memoria
                printf("SHL [%d], %s \n",V[1]-baseds(TDS,REG), registro[V[5]][V[6]]);
    else
        if(V[4] == 0) //memoria a registro
            printf("SHL %s, [%d] \n", registro[V[1]][V[2]], V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a registro
                printf("SHL %s, %d \n", registro[V[1]][V[2]], V[7]);
            else // registro a registro
                printf("SHL %s, %d \n", registro[V[1]][V[2]], V[5]);
}

void disSHR(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0)
        if(V[4] == 0) //memoria a memoria
            printf("SHR [%d], [%d] \n",V[1]-baseds(TDS,REG), V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a memoria
                printf("SHR [%d], %d \n",V[1]-baseds(TDS,REG), V[7]);
            else // registro a memoria
                printf("SHR [%d], %s \n",V[1]-baseds(TDS,REG), registro[V[5]][V[6]]);
    else
        if(V[4] == 0) //memoria a registro
            printf("SHR %s, [%d] \n", registro[V[1]][V[2]], V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a registro
                printf("SHR %s, %d \n", registro[V[1]][V[2]], V[7]);
            else // registro a registro
                printf("SHR %s, %d \n", registro[V[1]][V[2]], V[5]);
}

void disAND(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0)
        if(V[4] == 0) //memoria a memoria
            printf("AND [%d], [%d] \n",V[1]-baseds(TDS,REG), V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a memoria
                printf("AND [%d], %d \n",V[1]-baseds(TDS,REG), V[7]);
            else // registro a memoria
                printf("AND [%d], %s \n",V[1]-baseds(TDS,REG), registro[V[5]][V[6]]);
    else
        if(V[4] == 0) //memoria a registro
            printf("AND %s, [%d] \n", registro[V[1]][V[2]], V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a registro
                printf("AND %s, %d \n", registro[V[1]][V[2]], V[7]);
            else // registro a registro
                printf("AND %s, %d \n", registro[V[1]][V[2]], V[5]);

}

void disOR(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0)
        if(V[4] == 0) //memoria a memoria
            printf("OR [%d], [%d] \n",V[1]-baseds(TDS,REG), V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a memoria
                printf("OR [%d], %d \n",V[1]-baseds(TDS,REG), V[7]);
            else // registro a memoria
                printf("OR [%d], %s \n",V[1]-baseds(TDS,REG), registro[V[5]][V[6]]);
    else
        if(V[4] == 0) //memoria a registro
            printf("OR %s, [%d] \n", registro[V[1]][V[2]], V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a registro
                printf("OR %s, %d \n", registro[V[1]][V[2]], V[7]);
            else // registro a registro
                printf("OR %s, %d \n", registro[V[1]][V[2]], V[5]);
}

void disXOR(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0)
        if(V[4] == 0) //memoria a memoria
            printf("XOR [%d], [%d] \n",V[1]-baseds(TDS,REG), V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a memoria
                printf("XOR [%d], %d \n",V[1]-baseds(TDS,REG), V[7]);
            else // registro a memoria
                printf("XOR [%d], %s \n",V[1]-baseds(TDS,REG), registro[V[5]][V[6]]);
    else
        if(V[4] == 0) //memoria a registro
            printf("XOR %s, [%d] \n", registro[V[1]][V[2]], V[5]-baseds(TDS,REG));
        else
            if(V[4] == 1) // inmediato a registro
                printf("XOR %s, %d \n", registro[V[1]][V[2]], V[7]);
            else // registro a registro
                printf("XOR %s, %d \n", registro[V[1]][V[2]], V[5]);
}

void disSYS(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    printf("SYS %d\n",V[3]);
}

void disJMP(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0) // memoria
        printf("JMP [%d]\n",V[1]-baseds(TDS,REG));
    else
        if(V[0] == 1) // inmediato
            printf("JMP %d\n", V[3]);
        else // registro
            printf("JMP %s\n",registro[V[1]][V[2]]);
}

void disJZ(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0) // memoria
        printf("JZ [%d]\n",V[1]-baseds(TDS,REG));
    else
        if(V[0] == 1) // inmediato
            printf("JZ %d\n", V[3]);
        else // registro
            printf("JZ %s\n",registro[V[1]][V[2]]);
}

void disJP(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0) // memoria
        printf("JP [%d]\n",V[1]-baseds(TDS,REG));
    else
        if(V[0] == 1) // inmediato
            printf("JP %d\n", V[3]);
        else // registro
            printf("JP %s\n",registro[V[1]][V[2]]);
}

void disJN(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0) // memoria
        printf("JN [%d]\n",V[1]-baseds(TDS,REG));
    else
        if(V[0] == 1) // inmediato
            printf("JN %d\n", V[3]);
        else // registro
            printf("JN %s\n",registro[V[1]][V[2]]);
}

void disJNZ(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0) // memoria
        printf("JNZ [%d]\n",V[1]-baseds(TDS,REG));
    else
        if(V[0] == 1) // inmediato
            printf("JNZ %d\n", V[3]);
        else // registro
            printf("JNZ %s\n",registro[V[1]][V[2]]);
}

void disJNP(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0) // memoria
        printf("JNP [%d]\n",V[1]-baseds(TDS,REG));
    else
        if(V[0] == 1) // inmediato
            printf("JNP %d\n", V[3]);
        else // registro
            printf("JNP %s\n",registro[V[1]][V[2]]);
}

void disJNN(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    if(V[0] == 0) // memoria
        printf("JNN [%d]\n",V[1]-baseds(TDS,REG));
    else
        if(V[0] == 1) // inmediato
            printf("JNN %d\n", V[3]);
        else // registro
            printf("JNN %s\n",registro[V[1]][V[2]]);
}

void disLDL(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    printf("LDL %d\n", V[3]);// inmediato
}

void disLDH(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    printf("LDH %d\n", V[3]);// inmediato
}

void disRND(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    printf("RND %d\n", V[3]);// inmediato
}

void disNOT(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
        if(V[0] == 0) // memoria
        printf("NOT [%d]\n",V[1]-baseds(TDS,REG));
    else
        if(V[0] == 1) // inmediato
            printf("NOT %d\n", V[3]);
        else // registro
            printf("NOT %s\n",registro[V[1]][V[2]]);
}

void disSTOP(int V[], int REG[], TRTDS TDS[], texto registro[16][4])
{
    printf("STOP\n");
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

    if(V[3] > 0)
        REG[8] = 0;
    else
        if(V[3] < 0)
            REG[8] = 0x80000000;
        else //valor1 == 0
            REG[8] = 0x40000000;
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

    if( V[3] > 0)
        REG[8] = 0;
    else
        if( V[3] < 0)
            REG[8] = 0x80000000;
        else //V[3] == 0
            REG[8] = 0x40000000;
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

    if(V[3] > 0)
        REG[8] = 0;
    else
        if(V[3] < 0)
            REG[8] = 0x80000000;
        else //V[3] == 0
            REG[8] = 0x40000000;
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

    if(V[3] > 0)
        REG[8] = 0;
    else
        if(V[3] < 0)
            REG[8] = 0x80000000;
        else //V[3] == 0
            REG[8] = 0x40000000;
}

void CMP(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    V[3] -= V[7];
    if(V[3] > 0)
        REG[8] = 0;
    else
        if(V[3] < 0)
            REG[8] = 0x80000000;
        else //V[3] == 0
            REG[8] = 0x40000000;
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

    if(V[3] > 0)
        REG[8] = 0;
    else
        if(V[3] < 0)
            REG[8] = 0x80000000;
        else //V[3] == 0
            REG[8] = 0x40000000;
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

    if(V[3] > 0)
        REG[8] = 0;
    else
        if(V[3] < 0)
            REG[8] = 0x80000000;
        else //V[3] == 0
            REG[8] = 0x40000000;
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

    if(V[3] > 0)
        REG[8] = 0;
    else
        if(V[3] < 0)
            REG[8] = 0x80000000;
        else //V[3] == 0
            REG[8] = 0x40000000;
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
                        posmem += 4;
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
                        posmem += 4;
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
                        posmem += 4;
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
                        posmem += 4;
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
                        posmem += 4;
                        printf("%d\n", aux);
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
                        posmem += 4;
                        printf("%c\n", aux);
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
                        posmem += 4;
                        printf("%o\n", aux);
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
                        posmem += 4;
                        printf("%x\n", aux);
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

    if(V[3] > 0)
        REG[8] = 0;
    else
        if(V[3] < 0)
            REG[8] = 0x80000000;
        else //V[3] == 0
            REG[8] = 0x40000000;
}

void STOP(int V[], char MEM[], int REG[], TRTDS TDS[])
{
    printf("\n");
}

void cargaMatriz(texto registro[16][4])
{
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

void procesoDatos(char MEM[], int REG[], TRTDS TDS[], t_func funciones[], t_dis disassembler[], int ejecutar)
{
    char inst;
    int codop = 0, V[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    texto registro[16][4];
    cargaMatriz (registro);
    while(ejecutar == 1 && codop != 240)
    {
        if(REG[5] > baseds(TDS, REG))
        {
            printf("Fallo de segmento\n");
            exit(1);
        }
        inst = MEM[REG[5]++];
        V[9] = inst&0xFF;
        codigos(V[9], &codop, V, MEM, REG, TDS);
        if( (codop < 0) || (codop > 12 && codop < 48) || (codop > 59 && codop < 240) || (codop > 240))
        {
            printf("Instruccion invalida dis");
            exit(1);
        }
        disassembler[codop](V, REG, TDS, registro);
    }

    iniciaRegistros(REG);
    codop = 0;
    while(codop != 240)
    {
        if(REG[5] > baseds(TDS, REG))
        {
            printf("Fallo de segmento\n");
            exit(1);
        }
        inst = MEM[REG[5]++];
        V[9] = inst&0xFF;
        codigos(V[9], &codop, V, MEM, REG, TDS);
        if( (codop < 0) || (codop > 12 && codop < 48) || (codop > 59 && codop < 240) || (codop > 240))
        {
            printf("Instruccion invalida eje");
            exit(1);
        }
        funciones[codop](V, MEM, REG, TDS);
    }
    scanf("");
}

void cargaFuncionesDis (t_dis funciones[])
{
    funciones[0] = disMOV;
    funciones[1] = disADD;
    funciones[2] = disSUB;
    funciones[3] = disSWAP;
    funciones[4] = disMUL;
    funciones[5] = disDIV;
    funciones[6] = disCMP;
    funciones[7] = disSHL;
    funciones[8] = disSHR;
    funciones[9] = disAND;
    funciones[10] = disOR;
    funciones[11] = disXOR;
    funciones[48] = disSYS;
    funciones[49] = disJMP;
    funciones[50] = disJZ;
    funciones[51] = disJP;
    funciones[52] = disJN;
    funciones[53] = disJNZ;
    funciones[54] = disJNP;
    funciones[55] = disJNN;
    funciones[56] = disLDL;
    funciones[57] = disLDH;
    funciones[58] = disRND;
    funciones[59] = disNOT;
    funciones[240] = disSTOP;
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
    char MEM[RAM], DirArchivo[120];
    TRTDS TDS[8];

    t_dis disassembler[256];
    cargaFuncionesDis(disassembler);

    //declaracion de funciones
    t_func funciones[256];
    cargaFunciones(funciones);

    //inicializacion de variables y carga de memoria
    strcpy(DirArchivo, argv[1]);

    lectura(MEM, &TAM, DirArchivo);
    iniciaTablaDeSegmentos(TDS, RAM, TAM);
    iniciaRegistros(REG);

    if( (argc == 3) && strcmp(argv[2], "-d") == 0 )
        procesoDatos(MEM, REG, TDS, funciones, disassembler , 1);
    else
        procesoDatos(MEM, REG, TDS, funciones, disassembler, 0);

    return 0;
}
