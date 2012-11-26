/*=======================================================================
  A simple parser for "self" format

  The circuit format (called "self" format) is based on outputs of
  a ISCAS 85 format translator written by Dr. Sandeep Gupta.
  The format uses only integers to represent circuit information.
  The format is as follows:

1        2        3        4           5           6 ...
------   -------  -------  ---------   --------    --------
0 GATE   outline  0 IPT    #_of_fout   #_of_fin    inlines
                  1 BRCH
                  2 XOR(currently not implemented)
                  3 OR
                  4 NOR
                  5 NOT
                  6 NAND
                  7 AND


1 PI     outline  0        #_of_fout   0

2 FB     outline  1 BRCH   inline

3 PO     outline  2 - 7    0           #_of_fin    inlines




                                    Author: Chihang Chen
                                    Date: 9/16/94

=======================================================================*/

/*=======================================================================
  - Write your program as a subroutine under main().
    The following is an example to add another command 'lev' under main()

enum e_com {READ, PC, HELP, QUIT, LEV};
#define NUMFUNCS 5
int cread(), pc(), quit(), lev();
struct cmdstruc command[NUMFUNCS] = {
   {"READ", cread, EXEC},
   {"PC", pc, CKTLD},
   {"HELP", help, EXEC},
   {"QUIT", quit, EXEC},
   {"LEV", lev, CKTLD},
};

lev()
{
   ...
}
=======================================================================*/
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 81               /* Input buffer size */
#define MAXNAME 31               /* File name size */

#define V_0000 0
#define V_0001 1
#define V_0010 2
#define V_0011 3
#define V_0100 4
#define V_0101 5
#define V_0110 6
#define V_0111 7
#define V_1000 8
#define V_1001 9
#define V_1010 10
#define V_1011 11
#define V_1100 12
#define V_1101 13
#define V_1110 14
#define V_1111 15

#define Upcase(x) ((isalpha(x) && islower(x))? toupper(x) : (x))
#define Lowcase(x) ((isalpha(x) && isupper(x))? tolower(x) : (x))

enum e_com {READ, PC, HELP, QUIT, LVL, SLS, ATPG, PPC};
enum e_state {EXEC, CKTLD, CKTLVL};         /* Gstate values */
enum e_ntype {GATE, PI, FB, PO};    /* column 1 of circuit format */
enum e_gtype {IPT, BRCH, XOR, OR, NOR, NOT, NAND, AND};  /* gate types */

typedef struct lineVal{
    unsigned int indx;
    unsigned int val;
} lineVal;


struct cmdstruc {
   char name[MAXNAME];        /* command syntax */
   int (*fptr)();             /* function pointer of the commands */
   enum e_state state;        /* execution state sequence */
};

typedef struct n_struc {
   unsigned indx;             /* node index(from 0 to NumOfLine - 1 */
   unsigned num;              /* line number(May be different from indx */
   enum e_gtype type;         /* gate type */
   unsigned fin;              /* number of fanins */
   unsigned fout;             /* number of fanouts */
   struct n_struc **unodes;   /* pointer to array of up nodes */
   struct n_struc **dnodes;   /* pointer to array of down nodes */
   int numImpsReady;          /* number of inputs ready */
   int level;                 /* level of the gate output */
   int value;                 /* value implied at the line */
   char fault[3];             /* possible faults at the node after fault collapsing */
} NSTRUC;

typedef struct f_struc
{
    int indx;
    int saf;
    struct f_struc *next;
} FLIST;

typedef struct d_struc
{
    int indx;
    int posVal;
    struct d_struc *next;
} DLIST;

/*----------------- Command definitions ----------------------------------*/
#define NUMFUNCS 8

DLIST* genDFront();
void printCirc();
char *get16ValStr(unsigned int num);
int atpgRound(int indx);
int *getInVals();
char conv16ValTo3Val(int num);
int valAssign(lineVal assVal);
int implication(int index);
lineVal backtracing(int indx, unsigned int value) ;
int conv16ValInt(unsigned int dec);
char getValChar(int u, int v);
void restoreInVals(int *inpBackup);
int not_cover(int value);
int initFaultPath(int fault_indx);
int initialize(int fault_indx);
int isPowOfTwo(unsigned int x);
lineVal getPosVal(int indx, char *dir, unsigned int outVal);
char evaluateLogic(char *input, enum e_gtype op, int nInput);
int cread(), pc(), help(), quit(), lvl(), sls(), atpg(), ppc();
struct cmdstruc command[NUMFUNCS] = {
   {"READ", cread, EXEC},
   {"PC", pc, CKTLD},
   {"HELP", help, EXEC},
   {"QUIT", quit, EXEC},
   {"LVL", lvl, CKTLD},
   {"SLS", sls, CKTLVL},
   {"PPC", ppc, CKTLD},
   {"ATPG", atpg, CKTLD},
};

/*------------------------------------------------------------------------*/
enum e_state Gstate = EXEC;     /* global exectution sequence */
FLIST *faultList;               /* fault list */
int faultVal;                   /* fault value */
int faultIndx;                  /* fault index */
NSTRUC *Node;                   /* dynamic array of nodes */
NSTRUC **Pinput;                /* pointer to array of primary inputs */
NSTRUC **Poutput;               /* pointer to array of primary outputs */
int Nnodes;                     /* number of nodes */
int Npi;                        /* number of primary inputs */
int Npo;                        /* number of primary outputs */
int *ready;                     /* Ordered list of circut lines */
int Done = 0;                   /* status bit to terminate program */
/*------------------------------------------------------------------------*/
unsigned int (*cover)[4];
/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: shell
description:
  This is the main program of the simulator. It displays the prompt, reads
  and parses the user command, and calls the corresponding routines.
  Commands not reconized by the parser are passed along to the shell.
  The command is executed according to some pre-determined sequence.
  For example, we have to read in the circuit description file before any
  action commands.  The code uses "Gstate" to check the execution
  sequence.
  Pointers to functions are used to make function calls which makes the
  code short and clean.
-----------------------------------------------------------------------*/
main()
{
   enum e_com com;
   char cline[MAXLINE], wstr[MAXLINE], *cp;

   while(!Done) {
      printf("\nCommand>");
      fgets(cline, MAXLINE, stdin);
      if(sscanf(cline, "%s", wstr) != 1) continue;
      cp = wstr;
      while(*cp){
   *cp= Upcase(*cp);
   cp++;
      }
      cp = cline + strlen(wstr);
      com = READ;
      while(com < NUMFUNCS && strcmp(wstr, command[com].name)) com++;
      if(com < NUMFUNCS) {
         if(command[com].state <= Gstate) (*command[com].fptr)(cp);
         else{
            printf("Execution out of sequence!\n");
            printf("order: READ --> LVL --> SLS\n");
         }
      }
      else system(cline);
   }
}

/*-----------------------------------------------------------------------
input: circuit description file name
output: nothing
called by: main
description:
  This routine reads in the circuit description file and set up all the
  required data structure. It first checks if the file exists, then it
  sets up a mapping table, determines the number of nodes, PI's and PO's,
  allocates dynamic data arrays, and fills in the structural information
  of the circuit. In the ISCAS circuit description format, only upstream
  nodes are specified. Downstream nodes are implied. However, to facilitate
  forward implication, they are also built up in the data structure.
  To have the maximal flexibility, three passes through the circuit file
  are required: the first pass to determine the size of the mapping table
  , the second to fill in the mapping table, and the third to actually
  set up the circuit information. These procedures may be simplified in
  the future.
-----------------------------------------------------------------------*/
cread(cp)
char *cp;
{
   char buf[MAXLINE];
   int ntbl, *tbl, i, j, k, nd, tp, fo, fi, ni = 0, no = 0;
   FILE *fd;
   NSTRUC *np;

   sscanf(cp, "%s", buf);
   if((fd = fopen(buf,"r")) == NULL) {
      printf("File %s does not exist!\n", buf);
      return;
   }
   if(Gstate >= CKTLD) clear();
   Nnodes = Npi = Npo = ntbl = 0;
   while(fgets(buf, MAXLINE, fd) != NULL) {
      if(sscanf(buf,"%d %d", &tp, &nd) == 2) {
         if(ntbl < nd) ntbl = nd;
         Nnodes ++;
         if(tp == PI) Npi++;
         else if(tp == PO) Npo++;
      }
   }
   tbl = (int *) malloc(++ntbl * sizeof(int));

   fseek(fd, 0L, 0);
   i = 0;
   while(fgets(buf, MAXLINE, fd) != NULL) {
      if(sscanf(buf,"%d %d", &tp, &nd) == 2) tbl[nd] = i++;
   }
   allocate();

   fseek(fd, 0L, 0);
   while(fscanf(fd, "%d %d", &tp, &nd) != EOF) {
      np = &Node[tbl[nd]];
      strcpy(np->fault, "11\0");
      np->num = nd;
      if(tp == PI) Pinput[ni++] = np;
      else if(tp == PO) Poutput[no++] = np;
      switch(tp) {
         case PI:
         case PO:
         case GATE:
            fscanf(fd, "%d %d %d", &np->type, &np->fout, &np->fin);
            break;

         case FB:
            np->fout = np->fin = 1;
            fscanf(fd, "%d", &np->type);
            break;

         default:
            printf("Unknown node type!\n");
            exit(-1);
         }
      np->unodes = (NSTRUC **) malloc(np->fin * sizeof(NSTRUC *));
      np->dnodes = (NSTRUC **) malloc(np->fout * sizeof(NSTRUC *));
      if (np->fin == 0)
      {
          np->unodes = NULL;
      }
      if (np->fout == 0)
      {
          np->dnodes = NULL;
      }
      for(i = 0; i < np->fin; i++) {
         fscanf(fd, "%d", &nd);
         np->unodes[i] = &Node[tbl[nd]];
         }
      for(i = 0; i < np->fout; np->dnodes[i++] = NULL);
      }
   for(i = 0; i < Nnodes; i++) {
      for(j = 0; j < Node[i].fin; j++) {
         np = Node[i].unodes[j];
         k = 0;
         while(np->dnodes[k] != NULL) k++;
         np->dnodes[k] = &Node[i];
         }
      }
   fclose(fd);
   Gstate = CKTLD;
   printf("==> OK\n");
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: main
description:
  The routine prints out the circuit description from previous READ command.
-----------------------------------------------------------------------*/
pc(cp)
char *cp;
{
   int i, j;
   NSTRUC *np;
   char *gname();

   printf(" Node   Type \tIn     \t\t\tOut    \n");
   printf("------ ------\t-------\t\t\t-------\n");
   for(i = 0; i<Nnodes; i++) {
      np = &Node[i];
      printf("\t\t\t\t\t");
      for(j = 0; j<np->fout; j++) printf("%d ",np->dnodes[j]->num);
      printf("\r%5d  %s\t", np->num, gname(np->type));
      for(j = 0; j<np->fin; j++) printf("%d ",np->unodes[j]->num);
      printf("\n");
   }
   printf("Primary inputs:  ");
   for(i = 0; i<Npi; i++) printf("%d ",Pinput[i]->num);
   printf("\n");
   printf("Primary outputs: ");
   for(i = 0; i<Npo; i++) printf("%d ",Poutput[i]->num);
   printf("\n\n");
   printf("Number of nodes = %d\n", Nnodes);
   printf("Number of primary inputs = %d\n", Npi);
   printf("Number of primary outputs = %d\n", Npo);
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: main
description:
  The routine prints ot help inormation for each command.
-----------------------------------------------------------------------*/
help()
{
   printf("READ filename - ");
   printf("read in circuit file and creat all data structures\n");
   printf("PC - ");
   printf("print circuit information\n");
   printf("HELP - ");
   printf("print this help information\n");
   printf("QUIT - ");
   printf("stop and exit\n");
   printf("LVL - ");
   printf("levelize the circuit\n");
   printf("SLS - ");
   printf("run simple simulation on the circuit\n");
   printf("/nREAD --> LVL --> SLS\n");

}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: main
description:
  Set Done to 1 which will terminates the program.
-----------------------------------------------------------------------*/
quit()
{
   Done = 1;
}

/*======================================================================*/

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: cread
description:
  This routine clears the memory space occupied by the previous circuit
  before reading in new one. It frees up the dynamic arrays Node.unodes,
  Node.dnodes, Node.flist, Node, Pinput, Poutput, and Tap.
-----------------------------------------------------------------------*/
clear()
{
   int i;

   for(i = 0; i<Nnodes; i++) {
      free(Node[i].unodes);
      free(Node[i].dnodes);
   }
   free(Node);
   free(Pinput);
   free(Poutput);
   Gstate = EXEC;
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: cread
description:
  This routine allocatess the memory space required by the circuit
  description data structure. It allocates the dynamic arrays Node,
  Node.flist, Node, Pinput, Poutput, and Tap. It also set the default
  tap selection and the fanin and fanout to 0.
-----------------------------------------------------------------------*/
allocate()
{
   int i;

   Node = (NSTRUC *) malloc(Nnodes * sizeof(NSTRUC));
   Pinput = (NSTRUC **) malloc(Npi * sizeof(NSTRUC *));
   Poutput = (NSTRUC **) malloc(Npo * sizeof(NSTRUC *));
   for(i = 0; i<Nnodes; i++) {
      Node[i].indx = i;
      Node[i].fin = Node[i].fout = 0;
   }
}

/*-----------------------------------------------------------------------
input: gate type
output: string of the gate type
called by: pc
description:
  The routine receive an integer gate type and return the gate type in
  character string.
-----------------------------------------------------------------------*/
char *gname(tp)
int tp;
{
   switch(tp) {
      case 0: return("PI");
      case 1: return("BRANCH");
      case 2: return("XOR");
      case 3: return("OR");
      case 4: return("NOR");
      case 5: return("NOT");
      case 6: return("NAND");
      case 7: return("AND");
   }
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: main
description:
  The routine levelizes the input circuit
-----------------------------------------------------------------------*/
lvl()
{
   int i, j, ready_cnt;
   //int lvl_done, current_level;
   NSTRUC *np;
   NSTRUC *temp;

   //Gstate = EXEC;

   ready_cnt = 0;
   ready = (int *) malloc(Nnodes * sizeof(int));

   for (i=0; i<Nnodes;i++)
   {
      Node[i].level = -1;
      Node[i].numImpsReady = 0;
      //printf("debug: indx= %d, num= %d\n", Node[i].indx, Node[i].num);
   }

   for (i=0; i<Npi; i++)
   {
      Pinput[i]->level = 0;
      ready[ready_cnt] = Pinput[i]->indx;
      //printf("ready cnt = %d\n", ready_cnt);
      ready_cnt++;
   }
   i = 0;
   while(ready_cnt < Nnodes)
   {
      //printf("Ready: %d\n", ready[i]);
      np = &Node[ready[i]];

      //printf("current level nodes: %d\n", Node[ready[i]].num);
      for(j=0; j<np->fout; j++)
      {
         np->dnodes[j]->numImpsReady++;

         if (np->dnodes[j]->numImpsReady == np->dnodes[j]->fin)
         {
            np->dnodes[j]->level = np->level+1;
            ready[ready_cnt] = np->dnodes[j]->indx;
            ready_cnt++;
         }
      }
      i++;
   }
   printf("Levelization Done!\n\n");
   //for (i=0; i<Nnodes;i++)
   //{
   //   for(j=0; j
      //np = &Node[ready[i]];
      //printf("i: %d, num: %d, pin: %d, ready: %d\n",i, np->num, np->fin, ready[i]);
   //}

   Gstate = CKTLVL;
   //free(ready);
}



ppc()
{
    gen_circ_faults();
    gen_new_fault_list();
    Gstate = CKTLVL;
}

atpg()
{
    char buf[MAXLINE];
    char ch;
    int i;
    int fNum, fVal;
    FLIST *temp;
    FILE *faultFp;
    FILE *vectFp;
    lineVal test;
    int tempIndx;

    //printf("Please enter fault list filename: \n");


    if (Gstate != CKTLVL)
    {
        lvl();
    }

    if ((vectFp = fopen("vector_file.txt", "w")) == NULL)
    {
        printf("Cannot open vector_file.txt for write\n");
        return -1;
    }

    if ((faultFp = fopen("fault_list.txt", "r")) == NULL)
    {
        printf("Cannot open fault_list.txt for read\n");
        return -1;
    }
    while (fgets(buf, MAXLINE, faultFp) != NULL)
    {
        sscanf(buf, "%d %d", &fNum, &fVal);
        for (i=0; i<Nnodes; i++)
        {
            if (Node[i].num == fNum)
            {
                printf("%d %d %d\n", i, fNum, fVal);
                if (faultList == NULL)
                {
                    faultList = (FLIST *) malloc (sizeof(FLIST));
                    faultList->indx = Node[i].indx;
                    faultList->saf = fVal;
                    faultList->next = NULL;
                }
                else
                {
                    temp = (FLIST *) malloc (sizeof(FLIST));
                    temp->indx = Node[i].indx;
                    temp->saf = fVal;
                    temp->next = faultList;
                    faultList = temp;
                }
            }
        }
    }


    temp = faultList;

    for (i=0; i<Nnodes; i++)
    {
        if((Node[i].type>1) && (Node[i].type<8) && (Node[i].type != NOT))
        {
            if (Node[i].fin == 1)
            {
                Node[i].fin = 2;
                tempIndx = Node[i].unodes[0]->indx;

                free(Node[i].unodes);
                Node[i].unodes = (NSTRUC **) malloc(2 * sizeof(NSTRUC *));
                Node[i].unodes[0] = &Node[tempIndx];
                Node[i].unodes[1] = &Node[tempIndx];
                printf("Made new input: Index %d\n", i);
            }
        }
    }

    while(temp != NULL)
    {
        printf("Starting ATPG....Fault Index=%d   SAF=%d\n", temp->indx, temp->saf);

        faultIndx = temp->indx;
        faultVal = temp->saf;


        initialize(temp->indx);
        printf("Initialize Done\n");

        //printCirc();
        //return;

        if (atpgRound(-2) == 0)
        {
            //printCirc();
            printf("TEST GENERATED SUCCESSFULLY\n");
            fprintf(vectFp, "%d %d\n", Node[faultIndx].num, faultVal);
            for (i=0; i<Npi; i++)
            {
                printf("%d = %c\n", Pinput[i]->num, conv16ValTo3Val(Pinput[i]->value));
                fprintf(vectFp, "%c", conv16ValTo3Val(Pinput[i]->value));
            }
            fprintf(vectFp, "\n");
        }
        else
        {
            //printCirc();
            fprintf(vectFp, "%d %d\n", Node[faultIndx].num, faultVal);
            fprintf(vectFp, "There is no test vector \n");
            printf("There is no test vector \n");
            printf("Press [ENTER] to Continue...\n");
            printf("Press [ENTER] to Continue...\n");
            scanf("%c",&ch);
        }

        temp = temp->next;
    }

    temp = faultList;
    while (temp != NULL)
    {
        faultList = temp->next;
        free(temp);
        temp = faultList;
    }
    printf("Test Generation Completed. See \"vector_file.txt\"\n");
    fclose(faultFp);
    fclose(vectFp);
}

int atpgRound(int indx)
{
    printf("Testing for sgementation fault Mark 1\n");
    lineVal assVal, tempVal;
    int i, faultBackup;
    int *inpNew;
    int *inpBackup;
    DLIST *dFront;
    int faultExciteVal;
     printf("Testing for sgementation fault Mark 2\n");
    inpBackup = (int *) malloc (Npi * sizeof(int));
     printf("Testing for sgementation fault Mark 3\n");
    if (indx != -2)
    {
        printf("Starting Implication on  Indx=%4d \tNum=%4d\n", indx, Node[indx].num);

        //printCirc();
        if (implication(indx) != 0)
        {
            printf("Implication on Indx=%4d \tNum=%4d failed...Bactracking...\n", indx, Node[indx].num);
            return 1;
        }
        //printCirc();
    }



    //printCirc();

    if (faultVal == 1)
        faultExciteVal = V_1000;
    else if (faultVal == 0)
        faultExciteVal = V_0100;
    else
        printf("Fault Val can only be a 0 or 1, Value = %d\n", faultVal);

    //is fault excited?
    //backup current value at fault line
    faultBackup = Node[faultIndx].value;

    if((Node[faultIndx].type > 1) && (Node[faultIndx].type < 8))
    {
       Node[faultIndx].value = V_1111;
       tempVal = getPosVal(faultIndx, "OUT", V_0000);
    }
    else if (Node[faultIndx].type == BRCH)
    {
        tempVal.indx = Node[faultIndx].indx;
        tempVal.val = Node[faultIndx].unodes[0]->value;
    }
    else if (Node[faultIndx].type == IPT)
    {
        tempVal.indx = Node[faultIndx].indx;
        tempVal.val = faultExciteVal;
    }
    else
    {
        printf("Unknown Gate\n");
        return 1;
    }
    Node[faultIndx].value = faultBackup;

    if (tempVal.val != faultExciteVal)
    {
        //tempVal = getPosVal(faultIndx, "IN", faultExciteVal);

        assVal = backtracing(faultIndx, faultExciteVal);
        //printf("Bactracing    Line:%d,  Value=%s\n", assVal.indx, get16ValStr(assVal.val));

        if ((assVal.val != V_1000) && (assVal.val != V_0100))
        {
            printf("bactracing should only ever return 0 or 1\n");
            return 1;
        }

        printf("FEE suggests indx=%5d \tnum =%5d  \tvalue %s\n", assVal.indx, Node[assVal.indx].num, get16ValStr(assVal.val));
        //printCirc();
        //printf("\n\n;");

        if (valAssign(assVal) != 0)
        {
            printf("FEE assignment on Index %d = %s  failed!...Bactracking...\n", assVal.indx, get16ValStr(assVal.val));
            //printCirc();
            return 1;
        }
        printf("FEE Assignment Successful!\n");
        //printCirc();
        return 0;
    }
    else
    {
        printf("FAULT IS EXCITED\n");

        //printCirc();
        dFront = genDFront();
        //printCirc();

        while(dFront != NULL)
        {

            for (i=0; i<Npo; i++)
            {
                if((Poutput[i]->value == V_0001) || (Poutput[i]->value == V_0010))
                {
                    return 0;
                }
            }

            inpBackup = getInVals();

            if (isPowOfTwo(dFront->posVal)) // Maybe change to specifically check if the gate is not an XOR
            {
                //Look int making a function for how this is done during fault excitation.
                assVal = backtracing(dFront->indx, dFront->posVal);

                //printf("Bactracing says   Line:%d,  Value=%s\n", assVal.indx, get16ValStr(assVal.val));

                printf("FEP suggests indx=%5d \tnum =%5d  \tvalue %s\n", assVal.indx, Node[assVal.indx].num, get16ValStr(assVal.val));
                if (valAssign(assVal) != 0)
                {
                    //printCirc();
                    //dFront = dFront->next;
                    printf("FEP assignment on Index %d = %s failed...Bactracking...\n", assVal.indx, get16ValStr(assVal.val));
                    dFront = dFront->next;
                    continue;
                    //return 1;
                    //return 1;
                }
                else
                {
                    return 0;
                }
            }
            else
            {
                if (Node[dFront->indx].type != XOR)
                    printf("Have D or DBAR for gate that is not an XOR\n");

                //printCirc();

                assVal = backtracing(dFront->indx, V_0001);
                printf("1st XOR FEP suggests indx=%5d \tnum =%5d  \tvalue %s\n", assVal.indx, Node[assVal.indx].num, get16ValStr(assVal.val));
                //if (isPowOfTwo(assVal.val)
                if (valAssign(assVal) != 0)
                {
                    //initialize(faultIndx);

                    printf("1st XOR FEP assignment on Index %d = %s failed...Bactracking...\n", assVal.indx, get16ValStr(assVal.val));
                    //printCirc();
                    assVal = backtracing(dFront->indx, V_0010);

                    //printCirc();
                    printf("2nd XOR FEP suggests indx=%5d \tnum =%5d  \tvalue %s\n", assVal.indx, Node[assVal.indx].num, get16ValStr(assVal.val));

                    if (valAssign(assVal) != 0)
                    {
                        //printCirc();
                        printf("2nd XOR FEP assignment on Index %d = %s failed...Bactracking...\n", assVal.indx, get16ValStr(assVal.val));
                        //return 1;
                    }
                    else
                    {
                        printf("FEP Assignment Succesful\n");
                        return 0;
                    }
                }
                else
                {
                    printf("FEP Assignment Succesful\n");
                    return 0;
                }

            }

            //dFront = genDFront();
            //printCirc();
            dFront = dFront->next;
            //assVal = dFo
        }

        //return 0;  //remove this later
        for (i=0; i<Npo; i++)
        {
            if((Poutput[i]->value == V_0010) || (Poutput[i]->value == V_0001))
            {
                return 0;
            }
        }
        printf("We are STUCK!!!...Backtracking...\n");
        //printCirc();
        //pc();
        //printf("Indx: %d\n", indx);
        return 1;  //CHANGE THIS TO RETURN 0
    }



}


int valAssign(lineVal assVal)
{
    int i;
    int *inpBackup;

    inpBackup = (int *) malloc (Npi * sizeof(int));

    inpBackup = getInVals();
    printf("Try Assignment %d=%s\n", Node[assVal.indx].num, get16ValStr(assVal.val));
    //printCirc();
    if ((Node[assVal.indx].value & assVal.val) == 0)
    {
        return 1;
    }

    if (assVal.val != -1)
    {

        if (assVal.indx != faultIndx)
            Node[assVal.indx].value = assVal.val;

        if (atpgRound(assVal.indx) != 0)
        {

            restoreInVals(inpBackup);
            if (assVal.indx != faultIndx)
                Node[assVal.indx].value = (assVal.val == V_1000) ? V_0100 : ((assVal.val == V_0100) ? V_1000 : V_0000);

            //printCirc();
            printf("Assignment failed. Trying Alternative: %d=%s\n",Node[assVal.indx].num, get16ValStr(Node[assVal.indx].value));
            if (atpgRound(assVal.indx) != 0)
            {
                restoreInVals(inpBackup);

                printf("2nd Assignment failed.\n");
                return 1;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
        //inpNew[Node[assVal.indx]
    }
    else{
        printf("There is a problem with assVal %d", assVal.val);
    }

    restoreInVals(inpBackup);
    return 1;
}



int *getInVals()
{
    int i;
    int *result;

    result = (int *) malloc(Npi * sizeof(int));
    for(i=0;i<Npi;i++)
    {
        result[i] = Pinput[i]->value;
    }
    return result;
}




void restoreInVals(int *inpBackup)
{
    int i;

    initialize(faultIndx);
    for(i=0; i<Npi; i++)
    {
        Pinput[i]->value = inpBackup[i];
        implication(Pinput[i]->indx);
    }
}
/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: main
description:
  The routine generates input vectors and simulates the input circuit
-----------------------------------------------------------------------*/
sls()
{
   int i, j, k, temp;
   char buf[MAXLINE];
   char *vect;
   FILE *vectFp;
   FILE *outFp;
   int faultSim;
   char *gate_in;
   NSTRUC *np;
   char **vect_array;
   char ch;
   int *output;
   int fNum, fVal, fIndx, testDet;
   int nVect=0;



    if ((vectFp = fopen("vector_file.txt", "r")) == NULL)
    {
        printf("Cannot open fault_list.txt for read\n");
        return -1;
    }

    if ((outFp = fopen("out_file.txt", "w")) == NULL)
    {
        printf("Cannot open out_list.txt for write\n");
        return -1;
    }

    output = (int *) malloc (Npo * sizeof(int));
    vect = (char *) malloc (Npi * sizeof(char));
    while(fgets(buf, MAXLINE, vectFp) != NULL)
    {
        sscanf(buf, "%d %d", &fNum, &fVal);
        fprintf(outFp, "%s", buf);
        for (i=0; i<Nnodes; i++)
        {
            if (Node[i].num == fNum)
            {
                printf("%d %d %d\n", i, fNum, fVal);
                fIndx = Node[i].indx;
                break;
            }
        }
        i=0;

        while ((ch = fgetc(vectFp)) != '\n')
        {
            fprintf(outFp, "%c", ch);
            if ((ch == '1') || (ch == '0') || (Lowcase(ch) == 'x'))
            {
                vect[i] = Lowcase(ch);
                i++;
            }
            else
            {
                vect[i] = 'x';
                i++;
            }
        }        
        fprintf(outFp, "\n");
        //vect_array = (char **) malloc (nPi * sizeof(char *));

        // The following loops just print a header for the input and output values
        printf("Line Numbers:   ");
        for(i=0; i<Npi; i++)
        {
            printf("%d ",Pinput[i]->num);
        }
        printf("      -> ");
        for (i=0; i<Npo; i++)
        {
            printf("%d ",Poutput[i]->num);
        }
        printf("\n");
        //---------------------------------------------

            // Initialize the primary inputs

        for (j=0; j<2; j++)
        {
            faultSim = j;
            for (i=0; i<Npi; i++)
            {
                if ((Pinput[i]->num == fNum) && (faultSim == 1))
                {
                    Pinput[i]->value = '0' + fVal;
                }
                else
                {
                    Pinput[i]->value = vect[i];
                }
            }

            //faultSim = 1;
            for(i=0; i<Npi; i++)
            {

                printf("%c ",Pinput[i]->value);

            }
            printf("      -> ");

            for (i=Npi; i<Nnodes; i++)
            {
                np = &Node[ready[i]];
                //printf("i = %d \t ready = %d \t num = %d \t idx = %d\n",i, ready[i], np->num, np->indx);

                //printf("my_num = %d, fin = %d unodes=%c in_num = %d\n",np->num, np->fin,\
                //np->unodes[0]->value, np->unodes[0]->num);

                // If the gate has just one fan in but is not a BRANCH or NOT gate
                //    Replicate the fan-in to form 2 input vectors to the gate
                if ((np->type != NOT) && (np->type != BRCH) && (np->fin == 1))
                {
                   gate_in = (char *) malloc(2 * sizeof(char));
                   gate_in[1] = np->unodes[0]->value;
                }
                else
                {
                   gate_in = (char *) malloc(np->fin * sizeof(char));
                }
                // Loop through the fan-ins and create an input vector character array.
                // Might need to change to handle parrallel simulation
                for (k=0; k<np->fin; k++)
                {
                   //printf("my_num = %d, fin = %d unodes=%c in_num = %d\n",np->num, np->fin, np->unodes[k]->value, np->unodes[k]->num);
                   gate_in[k] = np->unodes[k]->value;
                   //gate_in[k] = (char *) malloc(sizeof(char));
                }
                //The function below evaluates the gate given a certain character arra as the input
                if ((np->num == fNum) && (faultSim == 1))
                {
                    np->value = '0' + fVal;
                }
                else
                {
                    np->value = evaluateLogic(gate_in,np->type,np->fin);
                }
                //}
                free(gate_in);
            }

            printf("\t\t");
          // The following loops print the values at the PI and PO
            testDet = 0;
            for (i=0; i<Npo; i++)
            {
                if (faultSim == 0)
                {
                    output[i] = Poutput[i]->value;
                }
                else
                {
                    if ((output[i] != Poutput[i]->value) || (testDet == 1))
                        testDet = 1;
                    else
                        testDet = 0;
                }
                printf("%c ",Poutput[i]->value);
            }
            printf("\n");
        }
        if (testDet == 1)
        {
            fprintf(outFp, "yes\n");
            printf("Fault Detected\n");
        }
        else
        {
            fprintf(outFp, "no\n");
            printf("Fault is NOT Detected\n");
            printf("Press [ENTER] to Continue...\n");
            scanf("%c",&ch);
        }

    }
    fclose(vectFp);
    fclose(outFp);

}

//----------------------------------------------------------
//   input: character array to input vectors to gate
//          gate type
//          number of vectors in input vector array.
//   output: character '0', '1' or 'x'
//
// This function implements the expression corresponding to
// the gate type on the input vector character array
//
//----------------------------------------------------------
char evaluateLogic(char *input, enum e_gtype op, int nInput)
{
   int i, u1, u2, v1, v2, temp_u2, temp_v2;
   int val2;

   char *temp;
   char result;

   // The first few lines handle the NOT gate and BRANCH gate
   if (input[0] == 'x'){
      u1 = 0;
      v1 = 1;
   }
   else{
      u1 = v1 = input[0] - '0';
   }

   v2 = u1;
   u2 = v1;

   if (op == NOT)
   {
      v2 = ~u1;
      u2 = ~v1;

      u2 = u2 & 1;
      v2 = v2 & 1;
   }

   // The following loop goes through each input character vector
   // for the gate and evaluates the logic expression corresponding to the gate type
   //
   for(i = 1; i < nInput; i++)
   {
      if (input[i] == 'x'){
         u1 = 0;
         v1 = 1;
      }
      else{
         u1 = v1 = input[i] - '0';
      }
      temp_u2 = u2;
      temp_v2 = v2;

      switch (op)
      {
         case AND:
            u2 = temp_u2 & u1;
            v2 = temp_v2 & v1;
            break;
         case OR:
            u2 = temp_u2 | u1;
            v2 = temp_u2 | v1;
            break;
         case NAND:
            //printf("LOC1: %d %d %d %d\n", u1, v1, u2, v2);
            u2 = (i != nInput-1) ? (temp_v2 & v1) : ~(temp_v2 & v1);
            v2 = (i != nInput-1) ? (temp_u2 & u1) : ~(temp_u2 & u1);
            //printf("LOC2: %d %d %d %d\n", u1, v1, u2, v2);
            break;
         case NOR:
            u2 = (i != nInput-1) ? (temp_v2 | v1) : ~(temp_v2 | v1);
            v2 = (i != nInput-1) ? (temp_u2 | u1) : ~(temp_u2 | u1);
            break;
         case XOR:
            v2 = (~(temp_u2) & v1) | (~(u1) & temp_v2);
            u2 = (~(temp_v2) & u1) | (~(v1) & temp_u2);
            break;
         case BRCH:
            break;
         case NOT:
            break;
         default: printf("Invalid operand in function\n");
      }

      //printf("%d %d %d %d\n", u1, v1, u2, v2);
   }

    u2 = u2 & 1;
    v2 = v2 & 1;

    if ((u2 == 1) && (v2 == 0))
    {
     u2 = 0;
     v2 = 1;
    }
   //printf("%c %s %c = %d %d\n", input[0], gname(op), input[1], u2, v2);

   return getValChar(u2, v2);
}


char getValChar(int u, int v)
{
   if (u == v){
      return ('0' + u);
   }
   else {
      return 'x';
   }
}

gen_circ_faults()
{
    int i;

    NSTRUC *np;

    if (Gstate != CKTLVL)
    {
        lvl();
    }
    for (i=Nnodes-1;i>=0;i--)
    {
        np = &Node[ready[i]];

        if (np->type > 2 && np->type < 8)
        {
            printf("Dropping faults for gate: %d\n", np->num);

            fault_drop_prim_gate(np->indx);
        }
    }

    for (i=0; i<Nnodes; i++)
    {
        printf("Line Number: %d, Types of Faults: %s\n", Node[i].num, Node[i].fault);
    }
}

fault_drop_prim_gate(int line_idx)
{
    int j;
    int flag = 0;
    NSTRUC *np;

    Node[line_idx].fault[0] = '0';
    Node[line_idx].fault[1] = '0';

    switch(Node[line_idx].type)
    {
        case AND:
        case NAND:
            // NOTE: this is done for both AND and NAND since no "break" above
            //remove SA0 faults from all BUT 1 of the inputs
            for (j=0; j<Node[line_idx].fin; j++)
            {
                Node[line_idx].unodes[j]->fault[0] = '0';
                if (Node[line_idx].unodes[j]->type != BRCH && flag == 0)
                {
                    Node[line_idx].unodes[j]->fault[0] = '1';
                    flag = 1;
                }
            }
            if(Node[line_idx].unodes[j-1]->type == BRCH && flag == 0)
            {
                Node[line_idx].unodes[j-1]->fault[0] = '1';
            }
            break;
        case OR:
        case NOR:
            // NOTE: this is done for both OR and NOR since no "break" above
            //remove SA1 faults from all BUT 1 of the inputs
            for (j=0; j<Node[line_idx].fin; j++)
            {
                Node[line_idx].unodes[j]->fault[1] = '0';
                if (Node[line_idx].unodes[j]->type != BRCH && flag == 0)
                {
                    Node[line_idx].unodes[j]->fault[1] = '1';
                    flag = 1;
                }
            }
            if(Node[line_idx].unodes[j-1]->type == BRCH && flag == 0)
            {
                Node[line_idx].unodes[j-1]->fault[1] = '1';
            }
            break;
        case NOT:
            break;
        default:
            printf("YOU SHOULDN'T COME HERE SINCE YOU ARE NOT A PRIMITIVE GATE: line num = %d\n", Node[line_idx].num);
    }
}

gen_new_fault_list()
{
    int i;
    FLIST *temp;
    FILE *fp;
    //char buf[

    if ((fp = fopen("fault_list.txt", "w")) == NULL){
        printf("Trouble with writing fault list file\n");
        return;
    }

    //free current fault list content





    // Loop through the nodes to generate the fault list
    for (i=0;i<Nnodes;i++)
    {
        // create temp node
        //temp = (NLIST *) malloc (sizeof NLIST);
        if(Node[i].fault[0] == '1')
        {
            fprintf(fp, "%d %c\n", Node[i].num, '0');
        }
        if(Node[i].fault[1] == '1')
        {
            fprintf(fp, "%d %c\n", Node[i].num, '1');
        }
    }
    fclose(fp);
}

void generateCover(char* gateType, lineVal *result){
    FILE *fp;
    char buf[MAXLINE];
    char line_in[4];
    char *filename;
    int i;
    filename = gateType;
    strcat(filename, "_4valueTT_6cubecover.txt");
    
    if ((fp=fopen(filename, "r")) == NULL){
    printf("ERROR: Cant open %c for reading\n", filename);
    result->val = -1;
    result->indx = -1;
   // return result;
    }
    i = 0;
    printf("generating cover for %c\n", filename);
    
//    while (fgets(buf, MAXLINE, fp) != NULL)
  //  {
        while((fscanf(fp, "%c %c %c", &line_in[0], &line_in[1], &line_in[2])) != EOF)
        {
            cover[i][0] = convCharInt(line_in[0]);
            cover[i][1] = convCharInt(line_in[1]);
            cover[i][2] = convCharInt(line_in[2]);
            printf("cover[%d][0] = %d; cover[%d][0] = %d; cover[%d][0] = %d\n", i, convCharInt(line_in[0]), i, convCharInt(line_in[1]), i, convCharInt(line_in[2]));
           // xor3_cover[i][3] = conv16ValInt(line_in[3]);
            i++;
            
        }
        
  //  }
    fclose(fp);
    
}

lineVal getPosVal(int indx, char *dir, unsigned int outVal)
{
    lineVal result;
    int i, j, k, numVect;
    unsigned int inp3, temp;
  //  unsigned int (*cover)[4];
  /*  unsigned int or3_cover[16][4] =
            {{V_1000, V_1000, V_1000, V_1000},

             {V_0001, V_0010, V_1111, V_0100},
             {V_0010, V_0001, V_1111, V_0100},
             {V_0001, V_1111, V_0010, V_0100},
             {V_0010, V_1111, V_0001, V_0100},
             {V_1111, V_0010, V_0001, V_0100},
             {V_1111, V_0001, V_0010, V_0100},
             {V_0100, V_1111, V_1111, V_0100},
             {V_1111, V_0100, V_1111, V_0100},
             {V_1111, V_1111, V_0100, V_0100},

             {V_0010, V_1010, V_1010, V_0010},
             {V_1010, V_0010, V_1010, V_0010},
             {V_1010, V_1010, V_0010, V_0010},

             {V_0001, V_1001, V_1001, V_0001},
             {V_1001, V_0001, V_1001, V_0001},
             {V_1001, V_1001, V_0001, V_0001}};

    unsigned int nor3_cover[16][4] =
            {{V_1000, V_1000, V_1000, V_0100},

             {V_0001, V_0010, V_1111, V_1000},
             {V_0010, V_0001, V_1111, V_1000},
             {V_0001, V_1111, V_0010, V_1000},
             {V_0010, V_1111, V_0001, V_1000},
             {V_1111, V_0010, V_0001, V_1000},
             {V_1111, V_0001, V_0010, V_1000},
             {V_0100, V_1111, V_1111, V_1000},
             {V_1111, V_0100, V_1111, V_1000},
             {V_1111, V_1111, V_0100, V_1000},

             {V_0010, V_1010, V_1010, V_0001},
             {V_1010, V_0010, V_1010, V_0001},
             {V_1010, V_1010, V_0010, V_0001},

             {V_0001, V_1001, V_1001, V_0010},
             {V_1001, V_0001, V_1001, V_0010},
             {V_1001, V_1001, V_0001, V_0010}};

    unsigned int and3_cover[16][4] =
            {{V_0100, V_0100, V_0100, V_0100},

             {V_0001, V_0010, V_1111, V_1000},
             {V_0010, V_0001, V_1111, V_1000},
             {V_0001, V_1111, V_0010, V_1000},
             {V_0010, V_1111, V_0001, V_1000},
             {V_1111, V_0010, V_0001, V_1000},
             {V_1111, V_0001, V_0010, V_1000},
             {V_1000, V_1111, V_1111, V_1000},
             {V_1111, V_1000, V_1111, V_1000},
             {V_1111, V_1111, V_1000, V_1000},

             {V_0001, V_0101, V_0101, V_0001},
             {V_0101, V_0001, V_0101, V_0001},
             {V_0101, V_0101, V_0001, V_0001},

             {V_0010, V_0110, V_0110, V_0010},
             {V_0110, V_0010, V_0110, V_0010},
             {V_0110, V_0110, V_0010, V_0010}};

    unsigned int nand3_cover[16][4] =
             {{V_0100, V_0100, V_0100, V_1000},

              {V_0001, V_0010, V_1111, V_0100},
              {V_0010, V_0001, V_1111, V_0100},
              {V_0001, V_1111, V_0010, V_0100},
              {V_0010, V_1111, V_0001, V_0100},
              {V_1111, V_0010, V_0001, V_0100},
              {V_1111, V_0001, V_0010, V_0100},
              {V_1000, V_1111, V_1111, V_0100},
              {V_1111, V_1000, V_1111, V_0100},
              {V_1111, V_1111, V_1000, V_0100},

              {V_0001, V_0101, V_0101, V_0010},
              {V_0101, V_0001, V_0101, V_0010},
              {V_0101, V_0101, V_0001, V_0010},

              {V_0010, V_0110, V_0110, V_0001},
              {V_0110, V_0010, V_0110, V_0001},
              {V_0110, V_0110, V_0010, V_0001}};
*/
/*
    unsigned int nand2_cover[9][4] =
            {{V_0100, V_0100, V_1000, 0},

             {V_1000, V_1111, V_0100, 0},
             {V_1111, V_1000, V_0100, 0},
             {V_0010, V_0001, V_0100, 0},
             {V_0001, V_0010, V_0100, 0},

             {V_0001, V_0101, V_0010, 0},
             {V_0101, V_0001, V_0010, 0},

             {V_0010, V_0110, V_0001, 0},
             {V_0110, V_0010, V_0001, 0}};

    unsigned int and2_cover[9][4] =
            {{V_1000, V_1111, V_1000, 0},
             {V_1111, V_1000, V_1000, 0},
             {V_0010, V_0001, V_1000, 0},
             {V_0001, V_0010, V_1000, 0},

             {V_0100, V_0100, V_0100, 0},

             {V_0010, V_0110, V_0010, 0},
             {V_0110, V_0010, V_0010, 0},

             {V_0001, V_0101, V_0001, 0},
             {V_0101, V_0001, V_0001, 0}};

    unsigned int or2_cover[9][4] =
            {{V_1000, V_1000, V_1000, 0},

             {V_0100, V_1111, V_0100, 0},
             {V_1111, V_0100, V_0100, 0},
             {V_0010, V_0001, V_0100, 0},
             {V_0001, V_0010, V_0100, 0},

             {V_0010, V_1010, V_0010, 0},
             {V_1010, V_0010, V_0010, 0},

             {V_0001, V_1001, V_0001, 0},
             {V_1001, V_0001, V_0001, 0}};

    unsigned int nor2_cover[9][4] =
            {{V_1000, V_1000, V_0100, 0},

             {V_0100, V_1111, V_1000, 0},
             {V_1111, V_0100, V_1000, 0},
             {V_0010, V_0001, V_1000, 0},
             {V_0001, V_0010, V_1000, 0},

             {V_0010, V_1010, V_0001, 0},
             {V_1010, V_0010, V_0001, 0},

             {V_0001, V_1001, V_0010, 0},
             {V_1001, V_0001, V_0010, 0}};

	unsigned int xor2_cover[16][4] =
            {{V_1000, V_1000, V_1000, 0},
             {V_0100, V_0100, V_1000, 0},
             {V_0010, V_0010, V_1000, 0},
             {V_0001, V_0001, V_1000, 0},

             {V_0100, V_1000, V_0100, 0},
             {V_1000, V_0100, V_0100, 0},
             {V_0001, V_0010, V_0100, 0},
             {V_0010, V_0001, V_0100, 0},

             {V_1000, V_0010, V_0010, 0},
             {V_0010, V_1000, V_0010, 0},
             {V_0100, V_0001, V_0010, 0},
             {V_0001, V_0100, V_0010, 0},

             {V_1000, V_0001, V_0001, 0},
             {V_0001, V_1000, V_0001, 0},
             {V_0100, V_0010, V_0001, 0},
             {V_0010, V_0100, V_0001, 0}};*/

/*	unsigned int xor3_cover[64][4]   = //This is being read from the file xor3_covers.txt
           {{V_0001, V_0001, V_0100, V_0100},  {V_0001, V_0010, V_1000, V_0100},
            {V_0001, V_0100, V_0001, V_0100},  {V_0001, V_1000, V_0010, V_0100},
            {V_0010, V_0001, V_1000, V_0100},  {V_0010, V_0010, V_0100, V_0100},
            {V_0010, V_0100, V_0010, V_0100},  {V_0010, V_1000, V_0001, V_0100},
            {V_0100, V_0001, V_0001, V_0100},  {V_0100, V_0010, V_0010, V_0100},
            {V_0100, V_0100, V_0100, V_0100},  {V_0100, V_1000, V_1000, V_0100},
            {V_1000, V_0001, V_0010, V_0100},  {V_1000, V_0010, V_0001, V_0100},
            {V_1000, V_0100, V_1000, V_0100},  {V_1000, V_1000, V_0100, V_0100},

            {V_0001, V_0001, V_1000, V_1000},  {V_0001, V_0010, V_0100, V_1000},
            {V_0001, V_0100, V_0010, V_1000},  {V_0001, V_1000, V_0001, V_1000},
            {V_0010, V_0001, V_0100, V_1000},  {V_0010, V_0010, V_1000, V_1000},
            {V_0010, V_0100, V_0001, V_1000},  {V_0010, V_1000, V_0010, V_1000},
            {V_0100, V_0001, V_0010, V_1000},  {V_0100, V_0010, V_0001, V_1000},
            {V_0100, V_0100, V_1000, V_1000},  {V_0100, V_1000, V_0100, V_1000},
            {V_1000, V_0001, V_0001, V_1000},  {V_1000, V_0010, V_0010, V_1000},
            {V_1000, V_0100, V_0100, V_1000},  {V_1000, V_1000, V_1000, V_1000},

            {V_0001, V_0001, V_0010, V_0010},  {V_0001, V_0010, V_0001, V_0010},
            {V_0001, V_0100, V_1000, V_0010},  {V_0001, V_1000, V_0100, V_0010},
            {V_0010, V_0001, V_0001, V_0010},  {V_0010, V_0010, V_0010, V_0010},
            {V_0010, V_0100, V_0100, V_0010},  {V_0010, V_1000, V_1000, V_0010},
            {V_0100, V_0001, V_1000, V_0010},  {V_0100, V_0010, V_0100, V_0010},
            {V_0100, V_0100, V_0010, V_0010},  {V_0100, V_1000, V_0001, V_0010},
            {V_1000, V_0001, V_0100, V_0010},  {V_1000, V_0010, V_1000, V_0010},
            {V_1000, V_0100, V_0001, V_0010},  {V_1000, V_1000, V_0010, V_0010},

            {V_0001, V_0001, V_0001, V_0001},  {V_0001, V_0010, V_0010, V_0001},
            {V_0001, V_0100, V_0100, V_0001},  {V_0001, V_1000, V_1000, V_0001},
            {V_0010, V_0001, V_0010, V_0001},  {V_0010, V_0010, V_0001, V_0001},
            {V_0010, V_0100, V_1000, V_0001},  {V_0010, V_1000, V_0100, V_0001},
            {V_0100, V_0001, V_0100, V_0001},  {V_0100, V_0010, V_1000, V_0001},
            {V_0100, V_0100, V_0001, V_0001},  {V_0100, V_1000, V_0010, V_0001},
            {V_1000, V_0001, V_1000, V_0001},  {V_1000, V_0010, V_0100, V_0001},
            {V_1000, V_0100, V_0010, V_0001},  {V_1000, V_1000, V_0001, V_0001}};*/

	FILE *fp;
    char buf[MAXLINE];
    unsigned int line_in[4];

    /*
    if ((fp=fopen("xor3_covers.txt", "r")) == NULL){
        printf("ERROR: Cant open xor3_covers.txt for reading\n");
        result.val = -1;
        result.indx = -1;
        return result;
    }
    i = 0;
    while (fgets(buf, MAXLINE, fp) != NULL)
    {
        if(sscanf(buf, "%u %u %u %u", &line_in[0], &line_in[1], &line_in[2], &line_in[3]) == 4)
        {
            xor3_cover[i][0] = conv16ValInt(line_in[0]);
            xor3_cover[i][1] = conv16ValInt(line_in[1]);
            xor3_cover[i][2] = conv16ValInt(line_in[2]);
            xor3_cover[i][3] = conv16ValInt(line_in[3]);
            i++;
        }
        else{
            printf("ERROR: Invalid line format in xor3_covers.txt: Line %d\n", i);
            result.val = -1;
            result.indx = -1;
            return result;
        }
    }
    fclose(fp);
    */
	// READ XOR COVERS FROM FILES


	//char xor_file[20];
	result.val = V_0000;



	//cover = or3_cover;

	//printf("%d %d", or3_cover, cover);
        
        
        
        

    switch(Node[indx].type)
    {
        case IPT:
            if (dir == "IN")
            {
                result.val = outVal;
                result.indx = indx;
            }
            else if (dir == "OUT")
            {
                result.val = Node[indx].value;
                result.indx = indx;
            }
            else
            {
                printf("Invalid IN or OUT in getPosVal IPT case\n");
            }
            return result;
        case OR:
            
            numVect = (Node[indx].fin + 1) * (Node[indx].fin + 1);
            
            
            generateCover("OR", &result);
            
            //cover = (Node[indx].fin == 3) ? or3_cover : or2_cover;
            //printf("OR\n");
            break;
        case NOR:
            numVect = (Node[indx].fin + 1) * (Node[indx].fin + 1);
          // cover = (Node[indx].fin == 3) ? nor3_cover : nor2_cover;
            generateCover("NOR", &result);
            //printf("NOR\n");
            break;
        case NAND:
            numVect = (Node[indx].fin + 1) * (Node[indx].fin + 1);
          //  cover = (Node[indx].fin == 3) ? nand3_cover : nand2_cover;
            generateCover("NAND", &result);
            //printf("NAND\n");
            break;
        case AND:
            numVect = (Node[indx].fin + 1) * (Node[indx].fin + 1);
          //  cover = (Node[indx].fin == 3) ? and3_cover : and2_cover;
            generateCover("AND", &result);
            //printf("AND\n");
            break;
        case XOR:
			numVect = (Node[indx].fin == 3) ? 64 : 16;
          //  cover = (Node[indx].fin == 3) ? xor3_cover : xor2_cover;
                        generateCover("XOR", &result);
            //printf("OR\n");
            break;
        case NOT:
            result.indx = -1;
            result.val = -1;
            if (dir == "IN")
            {
                if ((((outVal & Node[indx].value) != 0) || (indx == faultIndx)) &&
                   ((Node[indx].unodes[0]->value & not_cover(outVal)) != 0))
                {
                    result.indx = Node[indx].unodes[0]->indx;
                    result.val = (not_cover(outVal) & Node[indx].unodes[0]->value);
                }
            }
            else if (dir ==  "OUT")
            {
                result.val = not_cover(Node[indx].unodes[0]->value) & Node[indx].value;
                result.indx =  Node[indx].indx;
            }
            else{
                printf("BAD COVER DIRECTION FOR NOT GATE!\n");
            }
            return result;
            //break;
        default:{ printf("WHY ARE YOU HERE!\n");}
    }

	//printf("This is a test %d %d %d\n", cover[2][0], cover[2][1], cover[2][2]);
    if (dir == "IN")
    {
        if (((outVal & Node[indx].value) == 0) && (indx != faultIndx))
        {
            result.indx = -1;
            result.val = -1;
            return result;
        }
        for(i=0; i<numVect; i++)
        {
			if(cover[i][Node[indx].fin] == outVal)
			{

				for (j=0; j<Node[indx].fin; j++)
				{
                    //printf("%s ", get16ValStr(Node[indx].unodes[j]->value));
					if ((cover[i][j] & Node[indx].unodes[j]->value) == 0)
					{
						break;
					}
					if((isPowOfTwo(cover[i][j] & Node[indx].unodes[j]->value)) && (!isPowOfTwo(Node[indx].unodes[j]->value)))
					{
						temp = 1;
						for(k=j+1; k<Node[indx].fin; k++)
						{
							//printf("HERE:%d %d %d %d\n", temp, (Node[indx].unodes[k]->value & cover[i][k]), i, k );
							temp = (temp != 0) ? (cover[i][k] & Node[indx].unodes[k]->value) : temp;
							//printf("HERE:%d %d %d %d\n", temp, (Node[indx].unodes[k]->value & cover[i][k]), i, k );
						}
						if (temp != 0)
						{
							result.indx = Node[indx].unodes[j]->indx;
							result.val = cover[i][j] & Node[indx].unodes[j]->value;

							//printf("Implication IN on Line Indx %d cur_val: %s  exp_val:  %s\n", Node[indx].indx, get16ValStr(Node[indx].value), get16ValStr(outVal));
							return result;
						}
					}
					else
					{
					    if ((j == (Node[indx].fin -1)) && (isPowOfTwo(cover[i][j] & Node[indx].unodes[j]->value)))
						{
						    if (Node[indx].unodes[j]->indx != faultIndx)
                            {
                                result.indx = Node[indx].unodes[j]->indx;
                                result.val = cover[i][j] & Node[indx].unodes[j]->value;
						    }
						    else
						    {
                                result.indx = Node[indx].unodes[j]->indx;
                                if (Node[indx].unodes[j]->indx == V_0001)
                                    result.val = V_1000;
                                else
                                    result.val = V_0100;

						    }
						    return result;
						}
					}
				}

			}
        }
        result.val = -1;
        result.indx = -1;
        return result;
    }
    else if(dir == "OUT")
    {
	    inp3 = (Node[indx].fin == 2) ? V_1111 : Node[indx].unodes[2]->value;
		for(i=0; i<numVect; i++)
		{
			//printf("%s %s %s %s %s %s\n", get16ValStr(cover[i][0]), get16ValStr(Node[indx].unodes[0]->value), get16ValStr(cover[i][1]), get16ValStr(Node[indx].unodes[1]->value), get16ValStr(cover[i][2]), get16ValStr(inp3) );
			if(((Node[indx].unodes[0]->value & cover[i][0]) != 0) &&
			   ((Node[indx].unodes[1]->value & cover[i][1]) != 0) &&
			   ((inp3 & cover[i][2]) != 0))
			{
                //printf("Here: %s \n",get16ValStr(cover[i][Node[indx].fin]));
				result.val |= cover[i][Node[indx].fin];
			}
		}
        result.val &= Node[indx].value;
		result.indx = indx;
		//printf("Implication OUT on Line Indx %d cur_val: %s  exp_val:  %s\n", Node[indx].indx, get16ValStr(Node[indx].value), get16ValStr(result.val));
		return result;
    }
    else
    {
        printf("Incorrect cube cover direction specified\n");
    }
}

int initFaultPath(int fault_indx)
{
    int i;

    if (Node[fault_indx].fout != 0)
    {
        for (i=0; i<Node[fault_indx].fout; i++)
        {
            initFaultPath(Node[fault_indx].dnodes[i]->indx);
        }
    }

    Node[fault_indx].value = V_1111;

    return 0;
}

int initialize(int faultIndx)
{
    int i;

    for (i=0; i<Nnodes; i++)
    {
        Node[i].value = V_1100;
    }


    initFaultPath(faultIndx);

    if (faultVal == 0)
    {
        Node[faultIndx].value = V_0010;
    }
    else
    {
        Node[faultIndx].value = V_0001;
    }

    if (implication(faultIndx) == 1)
    {
        printf("Fault can never be implied\n");
        return 1;
    }
    return 0;
}


int isPowOfTwo(unsigned int x)
{
    return ((x != 0) && !(x & (x - 1)));
}

char *get16ValStr(unsigned int num)
{
   switch(num)
   {
      case 0 : return "0000";
      case 1 : return "0001";
      case 2 : return "0010";
      case 3 : return "0011";
      case 4 : return "0100";
      case 5 : return "0101";
      case 6 : return "0110";
      case 7 : return "0111";
      case 8 : return "1000";
      case 9 : return "1001";
      case 10 : return "1010";
      case 11 : return "1011";
      case 12 : return "1100";
      case 13 : return "1101";
      case 14 : return "1110";
      case 15 : return "1111";
      default : printf("get16ValStr: WHY EXACTLY AM I HERE? val = %d \n", num);
   }
   return "BAD\0";
}

int conv16ValInt(unsigned int dec)
{
   switch(dec)
   {
      case 0    : return V_0000;
      case 1    : return V_0001;
      case 10   : return V_0010;
      case 11   : return V_0011;
      case 100  : return V_0100;
      case 101  : return V_0101;
      case 110  : return V_0110;
      case 111  : return V_0111;
      case 1000 : return V_1000;
      case 1001 : return V_1001;
      case 1010 : return V_1010;
      case 1011 : return V_1011;
      case 1100 : return V_1100;
      case 1101 : return V_1101;
      case 1110 : return V_1110;
      case 1111 : return V_1111;
      default : printf("conv16ValInt: WHY EXACTLY AM I HERE? dec = %d\n", dec);
   }
   return -1;
}


int convCharInt(XX)
char XX;
{
    
  
  // case 0    : return V_0000;
      if(XX== '0')    { return V_1000;}
      else if(XX== '1'){    return V_0100;}
      else if(XX== '2'){    return V_0010;}
      else if(XX== '3'){   return V_0001;}
      else if(XX== 'D'){   return V_0010;}
      else if(XX== 'B'){   return V_0001;}
      else if(XX=='X') {  return V_1111;}
     // case 1000 : return V_1000;
    //  case 1001 : return V_1001;
     // case 1010 : return V_1010;
    //  case 1011 : return V_1011;
    //  case 1100 : return V_1100;
    //  case 1101 : return V_1101;
    //  case 1110 : return V_1110;
    //  case 1111 : return V_1111;
      else printf("convCharInt: WHY EXACTLY AM I HERE? dec = %c\n", XX);
   
   return -1;
}



int implication(int index)
{
	lineVal result;
	NSTRUC *np;
	unsigned int outVal, j, temp;

    //printf("Implication Called at Index=%5d, \tNum=%5d\n", index, Node[index].num);
	np = &Node[index];
	if(np->dnodes)
	{
		if(np->dnodes[0]->type != BRCH)
		{
			temp = np->dnodes[0]->value;
			result = getPosVal (np->dnodes[0]->indx, "OUT", temp);

			//temp = np->value;
			if((result.val == V_0000) && (np->dnodes[0]->indx != faultIndx))
			{
                printf("Implication Failed\n");
			    return 1;
			}

			if (np->dnodes[0]->indx != faultIndx)
				np->dnodes[0]->value = result.val;

			if(temp != np->dnodes[0]->value)
				implication(np->dnodes[0]->indx);
		}

		else
		{
			for(j = 0; j < np->fout; j++)
			{
				temp = np->dnodes[j]->value;

                if(temp == V_0000)
                {
                    printf("Implication Failed\n");
                    return 1;
                }

                if (np->dnodes[j]->indx != faultIndx)
                    np->dnodes[j]->value = np->value;

				if(temp != np->dnodes[j]->value)
					implication(np->dnodes[j]->indx);
			}
		}
	}

    //printf("Implication Succesful\n");
	np = &Node[index];
	//printf("Implication called on line %d: ")
	return 0;
}

int not_cover(int value)
{
    int result = V_0000;

    if (value & V_1000)
    {
        result |= V_0100;
    }
    if (value & V_0100)
    {
        result |= V_1000;
    }
    if (value & V_0010)
    {
        result |= V_0001;
    }
    if (value & V_0001)
    {
        result |= V_0010;
    }
    return result;
}

lineVal backtracing(int indx, unsigned int value)
{
	lineVal back;

	printf("Bactracing Objective:  Index=%5d  \tNum=%5d  \tValue=%s\n", indx, Node[indx].num, get16ValStr(value));
	if(Node[indx].type == BRCH)
	{
		back.indx = Node[indx].unodes[0]->indx;
		back.val = value;
	}
	else
	{
	    if(Node[indx].type == IPT)
	    {
            back.val = value;
            back.indx = indx;
	    }
	    else
	    {
            back = getPosVal(indx, "IN", value);
            if (back.indx == -1)
            {
                printf("Bactracing Failed\n");
                back.val = -1;
                back.indx = -1;
                return back;
            }
	    }
	}

	if (Node[back.indx].type != IPT)
	{
		return backtracing(back.indx, back.val);
	}
	else
	{
	    if (((back.val & Node[back.indx].value) == 0) && (back.val != -1))
	    {
            printf("Bactracing Failed\n");
            back.val = -1;
            back.indx = -1;
	    }
		return back;
	}
}


void printCirc()
{
    int i,j;
    NSTRUC *np;
    char *gname();

    //for (i=0; i<Nnodes; i++)
    //{
    //    printf("Indx: %3d  \tNum: %3d \tValue: %s\n", Node[i].indx, Node[i].num, get16ValStr(Node[i].value));
    //}

   printf("\n\n");

   printf(" Node   Type \tIn     \t\t\tOut    \t\tValue\t\tIndex\n");
   printf("------ ------\t-------\t\t\t-------\t\t-------\t\t------\n");
   for(i = 0; i<Nnodes; i++) {
      np = &Node[i];
      printf("\t\t\t\t\t");
      for(j = 0; j<np->fout; j++) printf("%d ",np->dnodes[j]->num);
      printf("\t\t%s", get16ValStr(np->value));
      printf("\t\t%5d", np->indx);
      printf("\r%5d  %s\t", np->num, gname(np->type));
      for(j = 0; j<np->fin; j++) printf("%d ",np->unodes[j]->num);
      printf("\n");
   }
   printf("Primary inputs:  ");
   for(i = 0; i<Npi; i++) printf("%d ",Pinput[i]->num);
   printf("\n");
   printf("Primary outputs: ");
   for(i = 0; i<Npo; i++) printf("%d ",Poutput[i]->num);
   printf("\n\n");
   printf("Number of nodes = %d\n", Nnodes);
   printf("Number of primary inputs = %d\n", Npi);
   printf("Number of primary outputs = %d\n", Npo);
   printf("Fault Index:%d     Fault SAF %d\n\n\n", faultIndx, faultVal);
}


DLIST* genDFront()
{
    int i, j;
    int cnt = 0;
    DLIST *result;
    DLIST *temp;

    result = NULL;
    for (i=Nnodes-1; i>=0; i--)
    //for (i=0; i<Nnodes; i++)
    {
        if ((Node[i].type < 8) && (Node[i].type > 1) &&
            ((Node[i].value & V_0011) != V_0000) && !(isPowOfTwo(Node[i].value)))
        {
            for (j=0; j<Node[i].fin; j++)
            {
                if((Node[i].unodes[j]->value == V_0001) || (Node[i].unodes[j]->value == V_0010))
                {
                    if(result == NULL)
                    {
                        result = (DLIST *) malloc (sizeof(DLIST));
                        result->posVal = Node[i].value & V_0011;
                        result->indx = Node[i].indx;
                        result->next = NULL;
                        printf("Adding Line Num %d to D-Front with PosVal %s\n", Node[result->indx].num, get16ValStr(result->posVal));
                    }
                    else
                    {
                        temp = (DLIST *) malloc (sizeof(DLIST));

                        temp->posVal = Node[i].value & V_0011;
                        temp->indx = Node[i].indx;
                        temp->next = result;
                        result = temp;
                        printf("Adding Line Num %d to D-Front with PosVal %s\n", result->indx, get16ValStr(result->posVal));
                    }
                    j=Node[i].fin;
                    break;
                }
            }
        }
    }

    return result;
}

char conv16ValTo3Val(int num)
{
    char ch;

    if ((num == V_1000) || (num == V_0001))
        return '0';
    else if((num == V_0100) || (num == V_0010))
        return '1';
    else
        return 'x';
}
/*========================= End of program ============================*/

