#include<stdio.h>
#include<stdlib.h>
#include<strings.h>


read name of file
if file name starts with INV go to INV_processing function
if file name starts with FOUT go to FOUT_processing function
else go to gate processing function

INV_processing()
Generate stuck at fault files
Generate 4 valued truth tables for stuck at faults

FOUT processing()
if 2 output FOUT generate appropriate stuck at fault files
if 3 outpout generate appropriate stuck at fault files
based on whether it is 2 or 3 output generate 4 valued truth tables

Gate_processing()
Figure out if the gate is 2 input or 3 input
Regular processing for 2 input gate
create processing for 3 input gate
generate appropiate stuck at fault files
