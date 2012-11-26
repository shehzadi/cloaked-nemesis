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

#define MAXLINE 81               /* Input buffer size */
#define MAXNAME 31               /* File name size */

#define Upcase(x) ((isalpha(x) && islower(x))? toupper(x) : (x))
#define Lowcase(x) ((isalpha(x) && isupper(x))? tolower(x) : (x))

enum e_com {READ, PC, HELP, QUIT, LEV, LOGIC, IMP};
enum e_state {EXEC, CKTLD};         /* Gstate values */
enum e_ntype {GATE, PI, FB, PO};    /* column 1 of circuit format */
enum e_gtype {IPT, BRCH, XOR, OR, NOR, NOT, NAND, AND};  /* gate types */

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
   int level;                 /* level of the gate output */
   int numInpsReady;
   int val;
   int valueTracker[3];
   int numValues; 
} NSTRUC;                     

/*----------------- Command definitions ----------------------------------*/
#define NUMFUNCS 7
int cread(), pc(), help(), quit(), lev(), SimpleLogicSimulation(), implication();
struct cmdstruc command[NUMFUNCS] = {
   {"READ", cread, EXEC},
   {"PC", pc, CKTLD},
   {"HELP", help, EXEC},
   {"QUIT", quit, EXEC},
   {"LEV", lev, CKTLD},
   {"LOGIC", SimpleLogicSimulation, CKTLD},
   {"IMP", implication, CKTLD}
};

/*------------------------------------------------------------------------*/
enum e_state Gstate = EXEC;     /* global exectution sequence */
NSTRUC *Node;                   /* dynamic array of nodes */
NSTRUC **Pinput;                /* pointer to array of primary inputs */
NSTRUC **Poutput;               /* pointer to array of primary outputs */
int Nnodes;                     /* number of nodes */
int Npi;                        /* number of primary inputs */
int Npo;                        /* number of primary outputs */
int Done = 0;                   /* status bit to terminate program */
/*------------------------------------------------------------------------*/
NSTRUC *sortedNodes;
int input_vector[5];
int input_vector_pointer;
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
FILE *ftt_check;

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
         else printf("Execution out of sequence!\n");
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

lev(cp)
char *cp;
{
    sortedNodes = (NSTRUC*) malloc(Nnodes*sizeof(NSTRUC));
    printf("levelizing\n\n");
    int i,j,k, while_cond = 1, max_level, a, swapped=1;
    NSTRUC *np, *tmp_np;
    
  //  sortedNodes = (NSTRUC *) malloc(Nnodes * sizeof(NSTRUC));
    NSTRUC tempNode;
    
    for(i=0; i<Nnodes; i++){
        
        np = &Node[i];
        
        np->level = -1; //value of -1 denotes that the level is undefined
        np->numInpsReady = 0;
    
    }
    
    for(i=0; i<Nnodes; i++){
        
        np = &Node[i];
        if(np->type==0){
            np->level = 0;
            
            levelize_depth(np);
        }
    
    }
    printf("Sorting nodes based on levels...\n\n");
    //sort into an array of nodes
    
    for(a=0; a<Nnodes; a++){
        sortedNodes[a] = Node[a];
    }
   
    
    while(swapped==1){     
     swapped = 0;
     for(a = 1; a<Nnodes; a++){
       /* if this pair is out of order */
         if((sortedNodes[a-1].level)>(sortedNodes[a].level)){
             tempNode = sortedNodes[a-1];
             sortedNodes[a-1] = sortedNodes[a];
             sortedNodes[a] = tempNode;
             swapped = 1;
         }
         
         /* if A[i-1] > A[i] then
          swap them and remember something changed 
         swap( A[i-1], A[i] )
         swapped = true
       end if */
     }
    }
    printf("Nodes have been sorted.\n\n");
    printf("Printing out sorted nodes\n\n");
    for(a=0; a<Nnodes; a++){
        printf("Node %d level %d\n", sortedNodes[a].num, sortedNodes[a].level);
    }
    
    
    for(i = 0; i < Nnodes; i++) {
      for(j = 0; j < sortedNodes[i].fin; j++) {
         np = sortedNodes[i].unodes[j];
         k = 0;
         while(np->dnodes[k] != NULL) k++;
         np->dnodes[k] = &sortedNodes[i];
         }
      }
    
}

levelize_depth(xp)
NSTRUC *xp;
{
    NSTRUC *tmp_np;
    int j,k, max_level;
    printf("Level of node %d is %d\n", xp->num, xp->level);
    for(j=0; j<xp->fout; j++){
                tmp_np = xp->dnodes[j];
                tmp_np->numInpsReady++;
                if(tmp_np->numInpsReady==tmp_np->fin){
                    max_level = 0;
                    for(k=0; k< tmp_np->fin; k++){
                        if(max_level<tmp_np->unodes[k]->level)
                            max_level = tmp_np->unodes[k]->level;
                    }
                    tmp_np ->level = max_level + 1;
                    levelize_depth(tmp_np);
                }
                
            } 

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
	Node[i].numValues = 3;
	Node[i].valueTracker[0] = 1;
	Node[i].valueTracker[1] = 1;
	Node[i].valueTracker[2] = 1;
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

implication(cp)
	char *cp;
	{
		int temp,x;
		printf("\nSelecting circuit element at Node 16 which is an AND gate and applying inputs  1 and 1...\n");
		temp = read3ValFile('NAND', 1, 1);
		printf("Node 16 type = %c\n", Node[15].type);
		Node[15].valueTracker[0] = 0;
		Node[15].valueTracker[1] = 0;
		Node[15].valueTracker[2] = 0;
		Node[15].valueTracker[temp] = 1;
		Node[15].numValues = 1;
		temp = Node[15].fout;
		for(x=0; x<temp; x++){
			if(Node[15].dnodes[x]->type=='BRANCH'){
				Node[15].dnodes[x]->valueTracker[0] = 0;
				Node[15].dnodes[x]->valueTracker[1] = 0;
				Node[15].dnodes[x]->valueTracker[2] = 0;
				Node[15].dnodes[x]->valueTracker[temp] = 1;
				Node[15].dnodes[x]->numValues = 1;
			}else{
				
			}
		}
		
	}


SimpleLogicSimulation(cp)
char *cp;
	{
    lev();
		int i,j;
            
                for(i=0; i<Nnodes; i++){
                    sortedNodes[i].val = 0;
                }
                
           for(j=0; j<5; j++){ 
                printf("\nGenerating Random Input Vector %d\n", j);
                for(i=0; i<Npi; i++){
                    input_vector[i] = rand()%2;
                    input_vector_pointer = 0;
                    printf("Value of input node %d = %d\n", sortedNodes[i].num, input_vector[i]);
                }
                
		for(i=0; i<Nnodes; i++){
                    node_eval(i);
                    
		}
                for(i=0; i<Nnodes; i++){
                    if(sortedNodes[i].fout==0){
                        printf("Output of node %d is %d\n", sortedNodes[i].num, sortedNodes[i].val);
                    }
                }
                
              //  for(i=0; i<Nnodes; i++){
                   
                //  printf("value of node %d is %d and fin is %d. Input node [0] is %d with value %d \n", sortedNodes[i].num, sortedNodes[i].val, sortedNodes[i].fin, sortedNodes[i].unodes[0]->num, getNodeVal(sortedNodes[i].unodes[0]->num) );
                    
             //   }
                printf("\n\n");
           }
		
	}
	
node_eval(xp)
int xp;
	{
    int i, temp1, temp2;
		if(gname(sortedNodes[xp].type)=="PI"){
                 //   printf("adding value &d to node %d\n", input_vector[sortedNodes[xp].num-1], sortedNodes[xp].num);
			sortedNodes[xp].val = input_vector[input_vector_pointer];
                        input_vector_pointer++;//input vector is a 5 location array that has values 1, 0 or -1(aka X)
		}else if (gname(sortedNodes[xp].type)=="BRANCH"){
			sortedNodes[xp].val = getNodeVal(sortedNodes[xp].unodes[0]->num);
                       
		}else if(gname(sortedNodes[xp].type)=="NOT"){
                    if(getNodeVal(sortedNodes[xp].unodes[0]->num)==0){
                        sortedNodes[xp].val = 1;
                    }else if(getNodeVal(sortedNodes[xp].unodes[0]->num)==1){
                        sortedNodes[xp].val = 0;
                    }else{
                        sortedNodes[xp].val = -1;
                    }
                }else if (gname(sortedNodes[xp].type)=="AND"){
			//create functions for AND, OR, NAND, NOR, and XOR that 
                    
                    if(sortedNodes[xp].fin>1){
                //        printf("AND with more than 1 input\n");
                        for(i=0; i<=sortedNodes[xp].fin-2; i++){
                            if(i==0){
                                temp1 = AND_gate(getNodeVal(sortedNodes[xp].unodes[0]->num), getNodeVal(sortedNodes[xp].unodes[1]->num));
                            }else{
                                
                                temp1 = AND_gate(temp1, getNodeVal(sortedNodes[xp].unodes[i+1]->num));
                            }
                            
                        }
                        sortedNodes[xp].val = temp1;
                    }else{
              //          printf("AND with 1 input\n");
                        sortedNodes[xp].val = getNodeVal(sortedNodes[xp].unodes[0]->num);
                       // printf("Assigned value %d to node %d because upnode[0] has value. Upnode node is %d out of %d nodes\n", sortedNodes[xp].val, sortedNodes[xp].num, sortedNodes[xp].unodes[xp]->num, sortedNodes[xp].fin);
                    }
		}else if (gname(sortedNodes[xp].type)=="OR"){
			//create functions for AND, OR, NAND, NOR, and XOR that 
                  //  printf("found OR gate for node %d with fin &d\n", sortedNodes[xp].num, sortedNodes[xp].fin);
                    if(sortedNodes[xp].fin>1){
                        for(i=0; i<=(sortedNodes[xp].fin-2); i++){
                            if(i==0){
                              //  printf("calling OR function for the first time for node %d with inputs %d and %d\n", sortedNodes[xp].num,sortedNodes[xp].unodes[0]->num), getNodeVal(sortedNodes[xp].unodes[1]->num);
                                temp1 = OR_gate(getNodeVal(sortedNodes[xp].unodes[0]->num), getNodeVal(sortedNodes[xp].unodes[1]->num));
                            }else{
                                
                                temp1 = OR_gate(temp1, getNodeVal(sortedNodes[xp].unodes[i+1]->num));
                            }
                            
                        }
                        sortedNodes[xp].val = temp1;
                    }else{
                        sortedNodes[xp].val = getNodeVal(sortedNodes[xp].unodes[0]->num);
                    }
		}else if (gname(sortedNodes[xp].type)=="NAND"){
			//create functions for AND, OR, NAND, NOR, and XOR that 
                    if(sortedNodes[xp].fin>1){
                        for(i=0; i<=sortedNodes[xp].fin-2; i++){
                            if(i==0){
                                temp1 = NAND_gate(getNodeVal(sortedNodes[xp].unodes[0]->num), getNodeVal(sortedNodes[xp].unodes[1]->num));
                            }else{
                                
                                temp1 = NAND_gate(temp1, getNodeVal(sortedNodes[xp].unodes[i+1]->num));
                            }
                            
                        }
                        sortedNodes[xp].val = temp1;
                    }else{
                        sortedNodes[xp].val = getNodeVal(sortedNodes[xp].unodes[0]->num);
                    }
		}else if (gname(sortedNodes[xp].type)=="NOR"){
			//create functions for AND, OR, NAND, NOR, and XOR that 
                    if(sortedNodes[xp].fin>1){
                        for(i=0; i<=sortedNodes[xp].fin-2; i++){
                            if(i==0){
                                temp1 = NOR_gate(getNodeVal(sortedNodes[xp].unodes[0]->num), getNodeVal(sortedNodes[xp].unodes[1]->num));
                            }else{
                                
                                temp1 = NOR_gate(temp1, getNodeVal(sortedNodes[xp].unodes[i+1]->num));
                            }
                            
                        }
                        sortedNodes[xp].val = temp1;
                    }else{
                        sortedNodes[xp].val = getNodeVal(sortedNodes[xp].unodes[0]->num);
                    }
		}else if (gname(sortedNodes[xp].type)=="XOR"){
			//create functions for AND, OR, NAND, NOR, and XOR that 
                    if(sortedNodes[xp].fin>1){
                        for(i=0; i<=sortedNodes[xp].fin-2; i++){
                            if(i==0){
                                temp1 = XOR_gate(getNodeVal(sortedNodes[xp].unodes[0]->num), getNodeVal(sortedNodes[xp].unodes[1]->num));
                            }else{
                                
                                temp1 = XOR_gate(temp1, getNodeVal(sortedNodes[xp].unodes[i+1]->num));
                            }
                            
                        }
                        sortedNodes[xp].val = temp1;
                    }else{
                        sortedNodes[xp].val = getNodeVal(sortedNodes[xp].unodes[0]->num);
                    }
		}
	}
	
int AND_gate(int A, int B){
	if((A==0) | (B==0)){
		return(0);
	}else if(A==1){
		return(B);
	}else if(B==1){
		return(A);
	}else{
		return(-1);
	}
}

int NAND_gate(int A, int B){
	if(AND_gate(A,B)==1){
		return(0);
	}else if(AND_gate(A,B)==0){
		return(1);
	}else{
		return(-1);
	}
}



int OR_gate(int A, int B){
	if((A==1) | (B==1)){
		return(1);
	}else if(A==0){
		return(B);
	}else if(B==0){
		return(A);
	}else{
		return(-1);
	}
}

int NOR_gate(int A, int B){
	if(OR_gate(A,B)==1){
		return(0);
	}else if(OR_gate(A,B)==0){
		return(1);
	}else{
		return(-1);
	}
}

int XOR_gate(int A, int B){
    if((A==1 && B==1)|(A==0 && B==0)){
        return(0);
    }else if (A==-1 | B==-1){
        return(-1);
    }else{
        return(1);
    }
}
/*========================= End of program ============================*/
int read3ValFile(char * gate, int a, int b){
	char * tt_check;
	char rd1, rd2, rd3, inpA, inpB;
	if(a==0){
		inpA = '0';
	}else if (a==1){
		inpA = '1';
	}else if (a==2){
		inpA = 'x';
	}
	if(b==0){
		inpB = '0';
	}else if (b==1){
		inpB = '1';
	}else if (b==2){
		inpB = 'x';
	}
	

		tt_check = &gate;
		strcat(tt_check, "_3cubecover.txt");
		ftt_check = fopen(tt_check, 'r');
		while((fscanf(ftt_check,"%c %c %c\n", &rd1, &rd2, &rd3))!= EOF)
			{
					//outarray[cnt] = rd3;
					if(rd1 == inpA || rd1 =='x'){
						if(rd2 == inpB || rd2=='x'){
							if(rd3=='0'){
								return(0);
							}else if (rd3=='1'){
								retrun(1);
							}else if (rd3=='x'){
								return(2);
							}
						}
					}
				
			}

	
}
int getNodeVal(num)
int num;
{
    int i; 
    for(i=0; i<Nnodes; i++){
        if(sortedNodes[i].num==num){
      //      printf("Returning node %d's value\n",num);
            return(sortedNodes[i].val);
        }
    }
}

/*setNodeVal(int num, int val)
{
    int i; 
    for(i=0; i<Nnodes; i++){
        if(sortedNodes[i].num==num){
            sortedNodes[i].val = val;
            printf("Setting value of node &d to %d", num, val);
        }
    }
}
*/

