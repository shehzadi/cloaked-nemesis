//Akash Jain
//Shehzad Ismail

//#include<conio.h>
#include<stdio.h>
#include<stdlib.h>
#include<strings.h>

FILE *f_tt1;
FILE *f_tt2;
FILE *ftt_check;
FILE *f_output;



int return_arr[2];
//int[] conv(int xyz)

int calc(char *tt_check,int a, int b)
	{
		int rd1, rd2, rd3;
		ftt_check = fopen(tt_check,"r");
		
		while((fscanf(ftt_check,"%d %d %d\n", &rd1, &rd2, &rd3))!= EOF)
			{
				printf("rd1 = %d; rd2=%d rd3=%d\n", rd1, rd2, rd3);
				
				if(a == rd1 && b == rd2)
					{
					fclose(ftt_check);
					return rd3;
					}
			}
			
		fclose(ftt_check);
		return (-1);
	}
	
int calc_fout1(char *tt_check,int a)
		{
			int rd1, rd2, rd3;
			ftt_check = fopen(tt_check,"r");
			while((fscanf(ftt_check,"%d %d %d\n", &rd1, &rd2, &rd3))!= EOF)
				{
					//printf("rd1 = %c; rd2=%c rd3=%c\n", rd1, rd2, rd3);
					if(a == rd1)
						{
						fclose(ftt_check);
						return rd2;
						}
				}

			fclose(ftt_check);
			return (-1);
		}
int calc_fout2(char *tt_check,int a)
				{
					int rd1, rd2, rd3;
					ftt_check = fopen(tt_check,"r");
					while((fscanf(ftt_check,"%d %d %d\n", &rd1, &rd2, &rd3))!= EOF)
						{
							//printf("rd1 = %c; rd2=%c rd3=%c\n", rd1, rd2, rd3);
							if(a == rd1)
								{
								fclose(ftt_check);
								return rd3;
								}
						}

					fclose(ftt_check);
					return (-1);
				}


void conv(int xyz)
	{
		//int return_arr[2];
		//take int 0, 1, 2, 3
		//output 2 bit binary
		if(xyz == 0)
		{
			return_arr[0] = 0;
			return_arr[1] = 0;
		}
		else if(xyz == 1)
		{
			return_arr[0] = 1;
			return_arr[1] = 1;
		}
		else if(xyz == 2)
		{
			return_arr[0] = 0;
			return_arr[1] = 1;
		}
		else if(xyz == 3)
		{
			return_arr[0] = 1;
			return_arr[1] = 0;
		}
	
	//	return return_arr;
	}

int main()
{
int x,y;
int temp[2];
int temp2[2];
int print_num;

char arrayFree[30];
char arrayFault[30];
char array_4tt[50];


printf("Please Enter the First File Name. Do not enter faulty file in this prompt:");
scanf("%s", &arrayFree);
strcat(arrayFree, ".txt");

printf("Please Enter the Second File Name:");
scanf("%s", &arrayFault);
strcat(array_4tt,arrayFault);
strcat(array_4tt, "_4valueTT.txt");
strcat(arrayFault, ".txt");

f_tt1 = fopen(arrayFree,"r"); // dont know if u need these
f_tt2 = fopen(arrayFault,"r"); // dont know if u need these




/* If strcat does not work
char * s = malloc(snprintf(NULL, 0, "%s %s", arrayFault, "_4valueTT") + 1);
sprintf(array_4tt, "%s %s", arrayFault, "_4valueTT");
*/

f_output = fopen(array_4tt,"w");
	
	
	//function to calculate 4 valued table
	//Takes one array as input to know which truth table to check
	//Takes values of 2 inputs that can be either 0 or 1. 
	//Looking up the arrays it will output appropriate simulated value of the gate.
int freeOutput, faultOutput, freeOutput2, faultOutput2;
	if(strcmp(arrayFree, "FOUT.txt") == 0)
	{ // If it is FOUT. Needs to be done. I am not confident. Sorry
		if(strcmp(arrayFree, arrayFault) == 0){
			for (x = 0; x < 4; x++)
					{
					//	temp = conv(x);
						conv(x);
						temp[0] = return_arr[0];
						temp[1] = return_arr[1];
					//	temp2 = conv(y);
						freeOutput = calc_fout1(arrayFree,temp[1]);
						
						faultOutput = calc_fout1(arrayFault, temp[0]);
					
						if(freeOutput == faultOutput)
						{
							if(freeOutput == 1)
								print_num = 1;
								// give output only 1
							else
								print_num = 0;
								// give output only 0
						}
					else
						{
							if(freeOutput == 1)
								print_num = 2;
								// give output only 2
							else
								print_num = 3;
								// give output only 3
						}
					 		if(print_num==0)
								fprintf(f_output, "%c", '0');
							else if(print_num==1)
								fprintf(f_output, "%c", '1');
							else if(print_num==2)
								fprintf(f_output, "%c", 'D');
							else if(print_num==3)
								fprintf(f_output, "%c", 'B');
							if(x<3)
								fprintf(f_output, "\n");
							
					}
		}else{
			for (x = 0; x < 4; x++)
					{
						
							//temp = conv(x);
							conv(x);
							//temp = return_arr;
							temp[0] = return_arr[0];
							temp[1] = return_arr[1];
						//	temp2 = conv(y);
							freeOutput = calc_fout1(arrayFree,temp[1]);

							faultOutput = calc_fout1(arrayFree, temp[0]);
							freeOutput2 = calc_fout2(arrayFault,temp[1]);

							faultOutput2 = calc_fout2(arrayFault, temp[0]);
							if(freeOutput == faultOutput)
							{
								if(freeOutput == 1)
									print_num = 1;
									// give output only 1
								else
									print_num = 0;
									// give output only 0
							}
						else
							{
								if(freeOutput == 1)
									print_num = 2;
									// give output only 2
								else
									print_num = 3;
									// give output only 3
							}
						 		if(print_num==0)
									fprintf(f_output, "%c ", '0');
								else if(print_num==1)
									fprintf(f_output, "%c ", '1');
								else if(print_num==2)
									fprintf(f_output, "%c ", 'D');
								else if(print_num==3)
									fprintf(f_output, "%c ", 'B');
						if(freeOutput2 == faultOutput2)
								{
									if(freeOutput2 == 1)
										print_num = 1;
										// give output only 1
									else
										print_num = 0;
										// give output only 0
								}
							else
								{
									if(freeOutput2 == 1)
										print_num = 2;
										// give output only 2
									else
										print_num = 3;
										// give output only 3
								}
							 		if(print_num==0)
										fprintf(f_output, "%c", '0');
									else if(print_num==1)
										fprintf(f_output, "%c", '1');
									else if(print_num==2)
										fprintf(f_output, "%c", 'D');
									else if(print_num==3)
										fprintf(f_output, "%c", 'B');
									
									
								if(x<3)
									fprintf(f_output, "\n");

					}
		}
					
	}
	else // not FOUT
	{
		if(strcmp(arrayFree, arrayFault) == 0) // if the files are same, their array will be same :P
			for (y = 0; y < 4; y++)
			{
				for (x = 0; x < 4; x++)
				{
					//temp = conv(x);
						conv(x);
						//temp = return_arr;
						temp[0] = return_arr[0];
						temp[1] = return_arr[1];
						
					//temp2 = conv(y);
						conv(y);
						//temp2 = return_arr;
						temp2[0] = return_arr[0];
						temp2[1] = return_arr[1];
					freeOutput = calc(arrayFree, temp2[1], temp[1]);
					faultOutput = calc(arrayFault, temp2[0], temp[0]);
					printf("\ninputs are %d %d to free array and %d %d to fault array. FreeOutput is %d and faultoutput is %d\n", temp[1], temp[0], temp2[1], temp2[0], freeOutput, faultOutput);
					
					if(freeOutput == faultOutput)
					{
						if(freeOutput == 1)
							print_num = 1;
							// give output only 1
						else
							print_num = 0;
							// give output only 0
						
					}
				else
					{
						if(freeOutput == 1)
							print_num = 2;
							// give output only 2
						else
							print_num = 3;
							// give output only 3
					}
					printf("print num = %d\n", print_num);
					if(print_num==0)
						fprintf(f_output, "%c", '0');
					else if(print_num==1)
						fprintf(f_output, "%c", '1');
					else if(print_num==2)
						fprintf(f_output, "%c", 'D');
					else if(print_num==3)
						fprintf(f_output, "%c", 'B');
					if(x<3)
						fprintf(f_output, " ");
					if(x==3 && y!=3){
						fprintf(f_output, "\n");
					}
					//if both values found are same then output 0 or 1
					//otherwise output 2 (D) or 3(D bar) as appropriate
				}
			}
		else
		for (y = 0; y < 2; y++)
		{
			for (x = 0; x < 2; x++)
			{
				//temp = conv(x);
						conv(x);
						//temp = return_arr;
						temp[0] = return_arr[0];
						temp[1] = return_arr[1];
					//temp2 = conv(y);
						conv(y);
						//temp2 = return_arr;
						temp2[0] = return_arr[0];
						temp2[1] = return_arr[1];
				freeOutput = calc(arrayFree, temp2[1], temp[1]);
				faultOutput = calc(arrayFault, temp2[0], temp[0]);
				
				if(freeOutput == faultOutput)
					{
						if(freeOutput == 1)
							print_num = 1; 
							// give output only 1
						else
							print_num = 0;
							// give output only 0
					}
				else
					{
						if(freeOutput == 1)
							print_num = 2;
							// give output only 2
						else
							print_num = 3;
							// give output only 3
					}
					if(print_num==0)
						fprintf(f_output, "%c", '0');
					else if(print_num==1)
						fprintf(f_output, "%c", '1');
					else if(print_num==2)
						fprintf(f_output, "%c", 'D');
					else if(print_num==3)
						fprintf(f_output, "%c", 'B');
					if(x<1)
						fprintf(f_output, " ");
					if(x==1 && y!=1){
						fprintf(f_output, "\n");
					}
				//if both values found are same then output 0 or 1
				//otherwise output 2 (D) or 3(D bar) as appropriate
				}
		}
	}
}//	 End of Main

//function to covert int to desired input values
	/*
	0 -> 0,0
	1 -> 1,1
	2 -> 1,0
	3 -> 0,1*/
	
	//the following if statement is divided into two parts. If both files provided are fault-free then you execute the first condition. 
	//If one file is faulty that means we need a 4 valued truth table for faulty and need to only simulate inputs 0 and 1 on both inputs of the gate.


	


