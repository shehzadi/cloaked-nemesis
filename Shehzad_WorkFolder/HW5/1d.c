//Akash Jain
//Shehzad Ismail

#include<stdio.h>
#include<stdlib.h>
#include<strings.h>

FILE *f_tt1;
FILE *ftt_check;
FILE *f_output;
	
	int OutputZero[8] = {};
	int OutputOne[8] = {};
    int OutputKai[8] = {};

    int OutputZeroCount = 0;
	int OutputOneCount = 0;
	int	OutputKaiCount = 0;

	int outarray[4];
   //Generate an Array
   void array_gen(char *tt_check)
   {
		//int [4] outarray = {};
		int cnt = 0;
		char rd1, rd2, rd3;
		ftt_check = fopen(tt_check,"r");
		while((fscanf(ftt_check,"%c %c %c\n", &rd1, &rd2, &rd3))!= EOF)
			{
					//outarray[cnt] = rd3;
					if(rd3 == '1')
						outarray[cnt] = 1;
					else
						outarray[cnt] = 0;
					cnt ++;
			}	
		fclose(ftt_check);
	//	return outarray;
   }
   
   int calc(int tt_check[], int i1, int i2)
   {
		//int temp = tt_check[2*i1+i2];
		int temp = outarray[2*i1+i2];
		return temp;
   }
   
   //function to calculate output
    int OutputCalc(int a, int b)
	{
		int x,y;
        if(a!=2 && b!=2)
            return(calc(outarray,a,b)); //calc is the same function used last time
        else
		{
            if(a==2)
			{
                x = calc(outarray, 0, b);
                y = calc(outarray, 1, b);
                if(x==y)
				{
                    return x;
                }
				else
				{
                    return 2;
                }
            }
			else if (b==2)
			{
                x = calc(outarray, a, 0);
                y = calc(outarray, a, 1);
                if(x==y)
				{
                    return x;
                }
				else
				{
                    return 2;
                }
            }
        }
           

    }

    //function to create 3 valued TT
void ThreeValTT()
	{
		int i, j, k, x;
        for(i=0; i<3; i++)
		{
            for(j=0; j<3; j++)
			{
                if(!(i==2 && j==2)) // for XX
				{
                    x = OutputCalc(i,j);
                    
					if(x == 0)
					{
						OutputZero[3*i+j] = 1;
						OutputZeroCount++;
                    }
					else if (x == 1)
					{
                        OutputOne[3*i+j] = 1;
						OutputOneCount++;
                    }
					else if (x == 2)
					{
                        OutputKai[3*i+j] = 1;
						OutputKaiCount++;

                    }
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
	char array_3tt[50];


	printf("Please Enter the First File Name");
	scanf("%s", &arrayFree);
	
	strcat(array_3tt,arrayFree);
	strcat(array_3tt, "_3cubecover.txt");
	strcat(arrayFree, ".txt");

	f_tt1 = fopen(arrayFree,"r"); // dont know if u need these

	f_output = fopen(array_3tt,"w");	

	array_gen(arrayFree);
	
	//int myarray[4];
	//myarray	= outarray;
	
	
	//So part d is only for fault free circuits
    //use previous parts code to read only one file this time and
	//convert the truth table into array like last time (myArray)
    //create three arrays
	
	/*  00		01		0x		10		11		1x		x0		x1		xx
		so if you index into array OutputZero[1] and if the value is 1
		I know that the output of input combinations 01	is 0
		if you index into array OutputOne[7] and if the value is 1
		That means the output of input combo x1 is 1. XX we never care about
		*/
	/*	
	int [8] OutputZero = {};
	int [8] OutputOne = {};
    int [8] OutputKai = {};

    int OutputZeroCount = 0;
	int OutputOneCount = 0;
	int	OutputKaiCount = 0;
	*/
	ThreeValTT();
	//printing needed here
	printf("Printing Zero Array\n");
	for(x=0; x<8; x++){
		printf("%d -> %d\n",x,OutputZero[x]);
	}
	printf("\nPrinting One Array\n");
    for(x=0; x<8; x++){
		printf("%d -> %d\n",x,OutputOne[x]);
	}
	printf("\nPrinting Kai Array\n");
    for(x=0; x<8; x++){
		printf("%d -> %d\n",x,OutputKai[x]);
	}
	if(OutputZero[2]==1){
		OutputZero[0] = 0;
		OutputZero[1] = 0;
	}
	if(OutputZero[5]==1){
		OutputZero[3] = 0;
		OutputZero[4] = 0;
	}
	if(OutputZero[6]==1){
		OutputZero[0] = 0;
		OutputZero[3] = 0;
	}
	if(OutputZero[7]==1){
		OutputZero[4] = 0;
		OutputZero[1] = 0;
	}
	
	if(OutputOne[2]==1){
		OutputOne[0] = 0;
		OutputOne[1] = 0;
	}
	if(OutputOne[5]==1){
		OutputOne[3] = 0;
		OutputOne[4] = 0;
	}
	if(OutputOne[6]==1){
		OutputOne[0] = 0;
		OutputOne[3] = 0;
	}
	if(OutputOne[7]==1){
		OutputOne[4] = 0;
		OutputOne[1] = 0;
	}
	
	
	for(x=0; x<8; x++){
		if(OutputZero[x]==1){
			if(x<3)
				fprintf(f_output,"%d ",0);
			else if (x<6)
				fprintf(f_output,"%d ",1);
			else if (x<8)
				fprintf(f_output,"%c ",'x');
			
			if((x+1)%3==0)
				fprintf(f_output,"%c ",'x');
			else if((x+2)%3==0) 
				fprintf(f_output,"%d ",1);
			else if((x+3)%3==0)
				fprintf(f_output,"%d ",0);
				
			fprintf(f_output,"%d",0);
			if(x!=7)
				fprintf(f_output,"\n");	
		}
	}
	for(x=0; x<8; x++){
		if(OutputOne[x]==1){
			if(x<3)
				fprintf(f_output,"%d ",0);
			else if (x<6)
				fprintf(f_output,"%d ",1);
			else if (x<8)
				fprintf(f_output,"%c ",'x');
			
			if((x+1)%3==0)
				fprintf(f_output,"%c ",'x');
			else if((x+2)%3==0) 
				fprintf(f_output,"%d ",1);
			else if((x+3)%3==0)
				fprintf(f_output,"%d ",0);
				
			fprintf(f_output,"%d",1);
			if(x!=7)
				fprintf(f_output,"\n");	
		}
	}
	for(x=0; x<8; x++){	
		if(OutputKai[x]==1){
			if(x<3)
				fprintf(f_output,"%d ",0);
			else if (x<6)
				fprintf(f_output,"%d ",1);
			else if (x<8)
				fprintf(f_output,"%c ",'x');
			
			if((x+1)%3==0)
				fprintf(f_output,"%c ",'x');
			else if((x+2)%3==0) 
				fprintf(f_output,"%d ",1);
			else if((x+3)%3==0)
				fprintf(f_output,"%d ",0);
				
			fprintf(f_output,"%c",'x');
			if(x!=7)
				fprintf(f_output,"\n");	
		}
		
	}
} // End of Main
   


