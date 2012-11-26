//Akash Jain
//Shehzad Ismail

#include<stdio.h>
#include<stdlib.h>
#include<strings.h>

FILE *f_tt1;
FILE *ftt_check;
FILE *f_output;

	int GBVar; // Global
	int outarray[4][4];
	
	int OutputZero[16] = {};
	int OutputOne[16] = {};
	int OutputD[16] = {};
	int OutputDBar[16] = {};
	
	int OutputZeroCount = 0;
	int OutputOneCount = 0;
	int	OutputDCount = 0;
	int	OutputDBarCount = 0;
	
void array_gen(char *tt_check, int var)
   {
		//int [4][4] outarray = {};
		int cnt = 0;
		char rd[4];
		ftt_check = fopen(tt_check,"r");
		int gene;
		if(var == 4)
		{
		while((fscanf(ftt_check,"%c %c %c %c\n", &rd[0], &rd[1], &rd[2], &rd[3]))!= EOF)
			{
					for(gene = 0; gene < var ; gene ++)
					{
					if(rd[gene] == '0')
											outarray[cnt][gene] = 0;
					else if(rd[gene] == '1')
											outarray[cnt][gene] = 1;
					if(rd[gene] == 'D')
											outarray[cnt][gene] = 2;
					else if(rd[gene] == 'B')
											outarray[cnt][gene] = 3;
					}
					cnt++
			}
		}
		else		
		{
			while((fscanf(ftt_check,"%c %c\n", &rd[0], &rd[1]))!= EOF)
			{
					for(gene = 0; gene < var ; gene ++)
					{
					if(rd[gene] == '0')
											outarray[cnt][gene] = 0;
					else if(rd[gene] == '1')
											outarray[cnt][gene] = 1;
					else if(rd[gene] == 'D')
											outarray[cnt][gene] = 2;
					else if(rd[gene] == 'B')
											outarray[cnt][gene] = 3;
					}
					cnt ++;
			}
		}
		fclose(ftt_check);
		//return outarray;
   }
   
   
void SixValTT(int var)
	{
		int i, j;
		//int array_read[4][4];
		int read_val;
        for(i=0; i<var; i++)
		{
            for(j=0; j<var; j++)
			{
                //read_val = array_read[i][j];
				read_val = outarray[i][j];
                                        
					if(read_val == 0)
					{
						OutputZero[4*i+j] = 1;
						OutputZeroCount++;
                    }
					else if (read_val == 1)
					{
                        OutputOne[4*i+j] = 1;
						OutputOneCount++;
                    }
					else if (read_val == 2)
					{
                        OutputD[4*i+j] = 1;
						OutputDBarCount++;
                    }
					else if (read_val == 3)
					{
                        OutputDBar[4*i+j] = 1;
						OutputDBarCount++;
                    }
                }
            }
        }

    

int main()
{
	int x,y;
	int temp[2];
	int temp2[2];

	char arrayFree[30];
	char array_6tt[50];

	
	
	/*
	00	01	02	03	10	11	12	13	20	21	22	23	30	31	32	33
	00  01  --  --  10  11  --  --  --  --  --  --  --  --  --  --
	This is the arrangement order. for 4x4 and 2x2 respectively.
	*/
	
	printf("Please Enter the File Name: ");
	scanf("%s", &arrayFree);

	strcat(array_6tt,arrayFree);
	strcat(array_6tt, "_6cubecover.txt");
	strcat(arrayFree, ".txt");
	
	f_tt1 = fopen(arrayFree,"r"); // dont know if u need these

		 // Needed to check the number of lines in file. TO make sure its 4x4 or 2x2
		int cnt = 0;
		char rd1, rd2, rd3;
		while((fscanf(f_tt1,"%c %c %c", &rd1, &rd2, &rd3))!= EOF)
			{
				cnt ++;
			}	
		fclose(ftt_check);
		

	f_output = fopen(array_6tt,"w");	
	
	GBVar = cnt; // depends on 4x4 or 2x2 array. Use this to define the limit of the loop.
	array_gen(arrayFree, GBVar);
	//int myarray[4][4] = outarray;
	
	
//	SixValTT(outarray, GBVar);
	SixValTT(GBVar);
	
	printf("Printing Zero Array\n");
	for(x=0; x<16; x++){
		printf("%d -> %d\n",x,OutputZero[x]);
	}
	printf("\nPrinting One Array\n");
    for(x=0; x<16; x++){
		printf("%d -> %d\n",x,OutputOne[x]);
	}
	printf("\nPrinting D Array\n");
    for(x=0; x<16; x++){
		printf("%d -> %d\n",x,OutputD[x]);
	}
	printf("\nPrinting DBar Array\n");
    for(x=0; x<16; x++){
		printf("%d -> %d\n",x,OutputDBar[x]);
	}
	
} // End of Main Function

