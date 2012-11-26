#include<stdio.h>
#include<stdlib.h>
#include<strings.h>

FILE *f_tt;
FILE *f_output;

main()
{
char p,q,r;
int a,b,c;
char output;
f_output = fopen("Three_value_Truth_table_XOR.txt","w");

fprintf(f_output, "Three Valued Truth Table for XOR gate\n\n");

fprintf(f_output, "a\t  b\t  c\t  out\n\n"); 

for(a=0; a<3; a++){
    for(b=0; b<3; b++){
        for(c=0; c<3; c++){
            if(a==2)
                p = 'x';
            else 
                p = (char)(((int)'0')+a);
            
            if(b==2)
                q = 'x';
            else 
                q = (char)(((int)'0')+b);
            
            if(c==2)
                r = 'x';
            else 
                r = (char)(((int)'0')+c);
            

            output = ComputeHigherValuedTruthTable(p, q, r);
            
            fprintf(f_output, "\n %c\t %c\t  %c\t %c", p, q, r, output);
        }
    }
}


}

ComputeHigherValuedTruthTable(x, y, z)
char x,y,z;
{

    char op1, op2;				// op3, op4, op5, op6;
    char rd1, rd2, rd3, rd4;

    if(x == 'x')
        {
            op1 = ComputeHigherValuedTruthTable('0', y , z );
            op2 = ComputeHigherValuedTruthTable('1', y , z );

            if(op1 == op2)
                    return op1;
            else
                    return 'x';
        }

    else if( y == 'x')	
        {
            op1 = ComputeHigherValuedTruthTable( x, '0' , z );
            op2 = ComputeHigherValuedTruthTable( x, '1' , z );

            if(op1 == op2)
                    return op1;
            else
                    return 'x';
        }

    else if( z == 'x')	
        {
            op1 = ComputeHigherValuedTruthTable( x, y , '0' );
            op2 = ComputeHigherValuedTruthTable( x, y , '1' );

            if(op1 == op2)
                    return op1;
            else
                    return 'x';
        }
    else
        {

            f_tt=fopen("XOR3.txt", "r");

            while((fscanf(f_tt, "%c %c %c %c\n\t", &rd1, &rd2, &rd3, &rd4))!= EOF)
            {
                printf("rd1 = %c; rd2=%c rd3=%c rd4=%c\n", rd1, rd2, rd3, rd4);
                    if(x == rd1 && y == rd2 && z == rd3 )
                            return rd4;
            }
            fclose(f_tt);
        }
    printf("op1 = %c op2= %c \n", op1, op2);
}


