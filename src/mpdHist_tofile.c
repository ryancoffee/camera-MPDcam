/*
 * This is now modified by R Coffee Oct 2020
 * coffee@slac.stanford.edu
 * updating to perform histogram with intermittent frame storage
 *
 */

/*
#######################################
                                                 
Copyright 2015-2017 Micro-Photon-Devices s.r.l.                                 
                                                 
SOFTWARE PRODUCT: SPC3_SDK                                     
                                                 
Micro-Photon-Devices (MPD) expressly disclaims any warranty for the SOFTWARE PRODUCT.          
The SOFTWARE PRODUCT is provided 'As Is' without any express or implied warranty of any kind,      
including but not limited to any warranties of merchantability, noninfringement, or          
fitness of a particular purpose. MPD does not warrant or assume responsibility for the          
accuracy or completeness of any information, text, graphics, links or other items contained      
within the SOFTWARE PRODUCT. MPD further expressly disclaims any warranty or representation      
to Authorized Users or to any third party.                             
In no event shall MPD be liable for any damages (including, without limitation, lost profits,    
business interruption, or lost information) rising out of 'Authorized Users' use of or inability 
to use the SOFTWARE PRODUCT, even if MPD has been advised of the possibility of such damages.    
In no event will MPD be liable for loss of data or for indirect, special, incidental,          
consequential (including lost profit), or other damages based in contract, tort          
or otherwise. MPD shall have no liability with respect to the content of the              
SOFTWARE PRODUCT or any part thereof, including but not limited to errors or omissions contained 
therein, libel, infringements of rights of publicity, privacy, trademark rights, business      
interruption, personal injury, loss of privacy, moral rights or the disclosure of confidential      
information.                                             

#######################################
*/

#include "SPC3_SDK.h"
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <cstdlib>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <unistd.h>

#include "Utility.h"
#include <stdint.h>

#if defined(__linux__)
#define SLEEP usleep
#define MILLIS 1000
#include <time.h>
#include <sys/time.h>
double get_wall_time(){
	struct timeval time;
	if (gettimeofday(&time,NULL)){
		//  Handle error
		return 0;
	}
	return (double)time.tv_sec + (double)time.tv_usec * .000001;
}
double get_cpu_time(){
	return (double)clock() / CLOCKS_PER_SEC;
}
#elif defined(__APPLE__)
#define SLEEP usleep
#define MILLIS 1000
#include <unistd.h>
#elif defined(_WIN32)
#include <Windows.h>
#define SLEEP Sleep
#define MILLIS 1
double get_wall_time(){
	LARGE_INTEGER time,freq;
	if (!QueryPerformanceFrequency(&freq)){
		//  Handle error
		return 0;
	}
	if (!QueryPerformanceCounter(&time)){
		//  Handle error
		return 0;
	}
	return (double)time.QuadPart / freq.QuadPart;
}
double get_cpu_time(){
	FILETIME a,b,c,d;
	if (GetProcessTimes(GetCurrentProcess(),&a,&b,&c,&d) != 0){
		//  Returns total user time.
		//  Can be tweaked to include kernel times as well.
		return
			(double)(d.dwLowDateTime |
					((unsigned long long)d.dwHighDateTime << 32)) * 0.0000001;
	}else{
		//  Handle error
		return 0;
	}
}
#endif

//***********************************//
//									 //
//		Main code					 //
//									 //
//***********************************//
int main(int argc, char *argv[])
{
//////////////////////////
// variables definition //
//////////////////////////
	std::cout << "Remember, if writing ascii to file (options 'd,e'), give the filehead (and maybe nimages) in the command" << std::endl;
	SPC3_H spc3= NULL;
	UInt16* Img= NULL;
	UInt16* bgImg= NULL;
	UInt16 hist[65535];
	UInt16 AppliedDT=0;
	std::vector<uint64_t> hist256(256,0);

	uint8_t exposure(20);// this is in units of 10 ns exposure
	std::ofstream histstream;
	std::ofstream imagestream;

	size_t nimages = 16;
	char header[1024]="";
	int integFrames=1;
	double read_bytes=0,total_bytes=0;
	int i=0,j=0,k=0,out=0;
	short trig=0;
	double gateoff=0;
	double gateoff_array[3] = { 0 };
	double counts[3] = {0};
	double *x = NULL,*y= NULL, *z = NULL, *Imgd=NULL;
	double* data= NULL;
	char c=0;
	char *fname = NULL;
	char *bgfname = NULL;
	BUFFER_H buffer = NULL;
	int counter = 0, aggressor=0;
	SPC3Return error = OK;
	FILE* f= NULL;
	time_t start,stop;
	char *Test[] = { "Live mode: write on stdout 10 images", //0
					 "Holdoff: mean number of photons at different holdoff values", //1
					 "Dead-time corrector improves the image quality", //2
					 "Background subtraction: 2 output files, with and without BG", //3
					 "Gate: 1000 images normal, 1000 images with gate 3 ns shift 5 ns", //4
					 "Synchronization output", //5
					 "Background statistics", //6
					 "Gate: calibrate the length of the gate signal", //7
					 "Triple gate mode", //8
					 "Read and write test images", //9
					 "Calculate the correlation image", //a
					"Continuous acquisition on file", //b
					"Continuous acquisition in memory", //c
					"Live mode: main <argv[1] is filehead> <argv[2] is nimages>", //d
					"future place for hardware subtract version: main <argv[1] is filehead> <argv[2] is nimages>" //e
					};
	Imgd =(double*) calloc(1, 2048* sizeof(double));
	Img =(UInt16*) calloc(1, 2048* sizeof(UInt16));
	data =(double*) calloc(1, 2048* sizeof(double));
	x =(double*) calloc(1, 65535* sizeof(double));
	y =(double*) calloc(1, 65535* sizeof(double));
	z = (double*)calloc(1, 65535 * sizeof(double));

////////////////////////////////////
// Simple menu for test selection //
////////////////////////////////////

	printf("*******************************************************\n");
	printf("SPC3 Test program\n");
	printf("*******************************************************\n\n\n\n");
	for (i = 0; i<15; i++)
		printf("\t%x) %s\n",i, Test[i]);
	printf("\tq) Quit\n");
	do 
	{
		c=getchar();
	} while ((c<48 || c>58) && c != 'q' && c != 'a' && c != 'b' && c != 'c' && c != 'd' && c != 'e');
	getchar();
	if(c>47 && c<59)
	{
		printf("*******************************************************\n");
		printf("%s\n",Test[c-48]);
		printf("*******************************************************\n\n\n");
	}
	if(c=='a')
	{
		printf("*******************************************************\n");
		printf("%s\n",Test[10]);
		printf("*******************************************************\n\n\n");
	}
	if(c=='b')
	{
		printf("*******************************************************\n");
		printf("%s\n",Test[11]);
		printf("*******************************************************\n\n\n");
	}
	if (c == 'c')
	{
		printf("*******************************************************\n");
		printf("%s\n", Test[12]);
		printf("*******************************************************\n\n\n");
	}
	if (c == 'd')
	{
		printf("*******************************************************\n");
		printf("%s\n", Test[13]);
		printf("*******************************************************\n\n\n");
	}
	if (c == 'e')
	{
		printf("*******************************************************\n");
		printf("%s\n", Test[14]);
		printf("*******************************************************\n\n\n");
	}
	switch(c)
	{
	case '0'://Test live mode
		//SPC3 constructor and parameter setting
		SPC3_Constr(&spc3, Normal,"");
		SPC3_Set_Camera_Par(spc3, 100, 30000,300,1,Disabled,Disabled,Disabled);		
		SPC3_Set_Trigger_Out_State(spc3,Frame);
		SPC3_Apply_settings(spc3); 
		SPC3_Set_Live_Mode_ON(spc3);
		//Acquistion of 10 live images
		for(i=0;i<5;i++)
		{
			printf("Image %d:\n",i);
			SPC3_Get_Live_Img(spc3, Img);
			for(j=0;j<32;j++)
			{
				for(k=0;k<32;k++)
					printf("%d ",Img[32*j+k]);
				printf("\n");
			}		
		}
		//Live mode off
		SPC3_Set_Live_Mode_OFF(spc3);						
		break;

	case '1'://Test dead-time
		//SPC3 constructor and parameter setting
		out=(int)SPC3_Constr(&spc3, Advanced,"");
		SPC3_Set_Camera_Par(spc3, 1040, 10,1,1,Disabled,Disabled,Disabled);
		SPC3_Apply_settings(spc3);
		k=0;			
		printf("Acquiring:\n");
		//Open file
		if((f = fopen("DTValues.txt","w")) == NULL)
		{
			printf("Unable to open the output file.\n");
			break;
		}
		//set deadtime, acquire snap, calculate mean photon count value and save results
		for(i=MAX_DEAD_TIME;i>=MIN_DEAD_TIME;i-=30)
		{			
			data[k]=0.0;
			SPC3_Set_DeadTime(spc3, i);
			SPC3_Apply_settings(spc3);
			SPC3_Get_DeadTime(spc3, i, &AppliedDT);
			x[k]=(double)AppliedDT;
			SPC3_Prepare_Snap(spc3);
			SPC3_Get_Snap(spc3);
			SPC3_Average_Img(spc3,Imgd,1);
			data[k]=mean_double(Imgd,2048);			
			printf("%d ns, Applied %d ns, %f\n", i, AppliedDT, data[k]);
			fprintf(f,"%d %f\n",i,data[k]);
			k++;
		}
		//print summary
		printf("\nDead-time calibration\n");
		for(i=0;i<k;i++)
		{
			printf("%f %f\n",x[i],data[i]);
		}
		fclose(f);				
		break;

	case '2': //Dead-time corrector effect
		//SPC3 constructor and parameter setting
		out=(int)SPC3_Constr(&spc3, Normal,"");
		SPC3_Set_Camera_Par(spc3, 4096, 1000,100,1,Disabled,Disabled,Disabled);
		SPC3_Set_DeadTime(spc3, 100);
		//acquisition with DTC on
		printf("Acquire the image using the dead-time correction\n");
		SPC3_Set_DeadTime_Correction(spc3, Enabled);
		SPC3_Apply_settings(spc3);
		//Acquire the BG image first
		printf("\n\n\nClose the camera shutter and press ENTER...\n");		
		getchar();
		SPC3_Prepare_Snap(spc3);
		SPC3_Get_Snap(spc3);
		//Calculate the average image
		SPC3_Average_Img(spc3, data,1);
		for(i=0;i<2048;i++)
			if(data[i]<= 65535)
				Img[i] = (UInt16) floor(data[i]+0.5);
			else
				Img[i] = 65535; // Avoid overflow
		SPC3_Set_Background_Img(spc3, Img);
		SPC3_Set_Background_Subtraction(spc3, Enabled);
		SPC3_Apply_settings(spc3);
		//now acquire the image with shutter open
		printf("\n\n\nOpen the camera shutter and press ENTER...\n");		
		getchar();
		SPC3_Prepare_Snap(spc3);
		SPC3_Get_Snap(spc3);
        SPC3_Save_Img_Disk(spc3, 1, 100, "Im_DeadTimeCorrected", TIFF_NO_COMPRESSION);
		printf("The the dead-time corrected image was acquired and stored on the hard disk succesfully!\n");
		//acquisition with DTC off
		printf("\n\nAcquire the reference image without the dead-time correction\n");
		SPC3_Set_Background_Subtraction(spc3, Disabled);
		SPC3_Set_DeadTime_Correction(spc3, Disabled);
		SPC3_Apply_settings(spc3);
		//Acquire the BG image first
		printf("\n\n\nClose the camera shutter and press ENTER...\n");		
		getchar();
		SPC3_Prepare_Snap(spc3);
		SPC3_Get_Snap(spc3);
		//Calculate the average image
		SPC3_Average_Img(spc3, data,1);
		for(i=0;i<2048;i++)
			if(data[i]<= 65535)
				Img[i] = (UInt16) floor(data[i]+0.5);
			else
				Img[i] = 65535; // Avoid overflow
		SPC3_Set_Background_Img(spc3, Img);
		SPC3_Set_Background_Subtraction(spc3, Enabled);
		SPC3_Apply_settings(spc3);
		//now acquire the image with shutter open
		printf("\n\n\nOpen the camera shutter and press ENTER...\n");		
		getchar();
		SPC3_Prepare_Snap(spc3);
		SPC3_Get_Snap(spc3);
        SPC3_Save_Img_Disk(spc3, 1, 100, "Im_DeadTimeReference", TIFF_NO_COMPRESSION);
		printf("The the dead-time corrected image was acquired and stored on the hard disk succesfully!\n");
		break;

	case '3'://Test background subtraction
		//SPC3 constructor and parameter setting
		out=(int)SPC3_Constr(&spc3, Normal,"");
		SPC3_Set_Camera_Par(spc3, 4096, 1000,100,1,Disabled,Disabled,Disabled);
		SPC3_Apply_settings(spc3);
		//acquire background image
		printf("\n\n\nClose the camera shutter and press ENTER...\n");
		getchar();
		SPC3_Prepare_Snap(spc3);
		SPC3_Get_Snap(spc3);
        SPC3_Save_Img_Disk(spc3, 1, 1, "Bg", TIFF_NO_COMPRESSION);
		SPC3_Average_Img(spc3, data,1);
		for(i=0;i<2048;i++)
			if(data[i]<= 65535)
				Img[i] = (UInt16) floor(data[i]+0.5);
			else
				Img[i] = 65535; // Avoid overflow
		SPC3_Set_Background_Img(spc3, Img);
		//acquire image with background subtration off
		printf("Open the camera shutter and press ENTER ...\n");
		getchar();
		SPC3_Set_Background_Subtraction(spc3, Disabled);
		SPC3_Apply_settings(spc3);
		SPC3_Prepare_Snap(spc3);
		SPC3_Get_Snap(spc3);
        SPC3_Save_Img_Disk(spc3, 1, 1, "Normal", TIFF_NO_COMPRESSION);
		//acquire image with background subtration on
		SPC3_Set_Background_Subtraction(spc3, Enabled);
		SPC3_Prepare_Snap(spc3);
		SPC3_Get_Snap(spc3);
        SPC3_Save_Img_Disk(spc3, 1, 1, "BgSubt", TIFF_NO_COMPRESSION);
		break;

	case '4': // Test gate
		//SPC3 constructor and parameter setting
		out=(int)SPC3_Constr(&spc3, Advanced,"");
		SPC3_Set_Camera_Par(spc3, 4096, 1000,100,1,Disabled,Disabled,Disabled);
		SPC3_Set_DeadTime(spc3, 100);
		SPC3_Apply_settings(spc3);
		//Normal image
		printf("Acquiring the reference image ...\n");
		SPC3_Prepare_Snap(spc3);
		SPC3_Get_Snap(spc3);
		printf("Save the reference image ...\n");
        SPC3_Save_Img_Disk(spc3, 1, 1000, "GateNormal", TIFF_NO_COMPRESSION);
		//gated image
		SPC3_Set_Gate_Mode(spc3, 1, Pulsed);
		SPC3_Set_Gate_Values(spc3, 10, 15); //Shift -10% of 20 ns --> 120 ns, Length 15% of 20 ns --> 3ns
		SPC3_Apply_settings(spc3);
		printf("Acquiring the gated image ...\n");
		SPC3_Prepare_Snap(spc3);
		SPC3_Get_Snap(spc3);
		printf("Save the gated image ...\n");
        SPC3_Save_Img_Disk(spc3, 1, 1000, "GatePulsed",TIFF_NO_COMPRESSION);
		break;

	case '5': // Test synchronization output
		//SPC3 constructor and parameter setting
		out=(int)SPC3_Constr(&spc3, Advanced,"");
		SPC3_Set_Camera_Par(spc3, 8192, 0xFFFF,5,1,Disabled,Disabled,Disabled);
		SPC3_Set_DeadTime(spc3, 100);
		//no output
		SPC3_Set_Trigger_Out_State(spc3,None);
		SPC3_Apply_settings(spc3);
		SPC3_Set_Live_Mode_ON(spc3);
		printf("\n\nNo output ...\n");
		printf("Press ENTER to continue\n");		
		getchar();
		SPC3_Set_Live_Mode_OFF(spc3);
		//gate clock output
		SPC3_Set_Trigger_Out_State(spc3,Gate_Clk);		
		SPC3_Set_Camera_Par(spc3, 8192, 0xFFFF,5,1,Disabled,Disabled,Disabled);
		SPC3_Apply_settings(spc3);
		SPC3_Set_Live_Mode_ON(spc3);
		printf("\n\nGate synchronization signal ...\n");
		printf("Press ENTER to continue\n");
		getchar();
		SPC3_Set_Live_Mode_OFF(spc3);
		//frame sync output
		SPC3_Set_Trigger_Out_State(spc3,Frame);
		SPC3_Set_Camera_Par(spc3, 8192, 0xFFFF,5,1,Disabled,Disabled,Disabled);
		SPC3_Apply_settings(spc3);
		SPC3_Set_Live_Mode_ON(spc3);
		printf("\n\nFrame synchronization signal ...\n");
		printf("Press ENTER to continue\n");
		getchar();
		SPC3_Set_Live_Mode_OFF(spc3);
		printf("\n\nWait for the trigger input ...\n");
		//trigger in enabled
		SPC3_Set_Sync_In_State(spc3, Enabled, 0);
		SPC3_Set_Camera_Par(spc3, 4096, 10,2,1,Disabled,Disabled,Disabled);
		SPC3_Apply_settings(spc3);
		SPC3_Prepare_Snap(spc3);
		while (trig!=1)
			SPC3_IsTriggered(spc3, &trig);
		printf("Trigger signal received!\n");
		break;

	case '6': // Test background statistics
		//The output file "BgHist.txt" contains the number of pixels which had a total number of counts given by the column index for each row. 
		//For example, column 3 contains the /number of pixels which had 3 dark-counts. 
		//Several rows are present because the histogram is calculated for several dead-time values.
		//SPC3 constructor and parameter setting
		out=(int)SPC3_Constr(&spc3, Advanced,"");
		SPC3_Set_Camera_Par(spc3, 1040, 100,10000,1,Disabled,Disabled,Disabled);
		f=fopen("BgHist.txt","w");		
		printf("Close the camera shutter and press ENTER ...\n");
		getchar();
		printf("Acquiring: ");
		for(i=150;i>=50;i-=50) //different hold-off values
		{		
			printf("%d ns ",i);
			SPC3_Set_DeadTime(spc3, i);
			SPC3_Apply_settings(spc3);
			SPC3_Prepare_Snap(spc3);
			SPC3_Get_Snap(spc3);
			SPC3_Average_Img(spc3, data,1);
			for(k=0;k<2048;k++)
			if(data[k]<= 65535)
				Img[k] = (UInt16) floor(data[k]+0.5);
			else
				Img[k] = 65535; // Avoid overflow
			Hist(Img,hist);
			for(j=0;j<65535;j++)
				fprintf(f,"%hd ",hist[j]);
			fprintf(f,"\n");
			
			
		}
		printf("\n");
		fclose(f);
		SPC3_Save_Img_Disk(spc3,1,100,"DCR_50ns_movie",SPC3_FILEFORMAT);
		SPC3_Save_Averaged_Img_Disk(spc3,1,"DCR_50ns_averaged",SPC3_FILEFORMAT);

		break;

	case '7': // Calibrate gate
		//SPC3 constructor and parameter setting
		out=(int)SPC3_Constr(&spc3, Advanced,"");
		SPC3_Set_Camera_Par(spc3, 1040 * 3, 10000, 5, 3, Disabled, Disabled, Disabled);
		SPC3_Set_DeadTime(spc3, 100);
		SPC3_Apply_settings(spc3);
		printf("Which counter do you want to acquire?\n");
		scanf("%d", &counter);
		if ((counter < 1) || (counter > 3))
		{
			printf("Insert a value between 1 and 3!\n");
			break;
		}
		printf("Expose the SPC3 camera to a time-independent luminous signal\n(room light might oscillate at 50 or 60 Hz)\nPress ENTER to continue ...\n");
		getchar();
		getchar();
		if((f = fopen("GateValues.txt","w")) == NULL)
		{
			printf("Unable to open the output file.\n");
			break;
		}
		//photon counts without any gate
		SPC3_Set_Gate_Mode(spc3, 1, Continuous);
		SPC3_Prepare_Snap(spc3);
		SPC3_Get_Snap(spc3);		
		SPC3_Average_Img(spc3, data,1);			
		gateoff = mean_double(data,2048);						
		printf("Gate OFF counts: %.2f\n",gateoff);
		fprintf(f,"Gate OFF counts: %.2f\n",gateoff);
		SPC3_Set_Gate_Mode(spc3, 1, Pulsed);
		SPC3_Apply_settings(spc3);
		printf("Acquiring:\n\nGate\t\tMean\t\tActual Gate\t\t\n");
		//photon counts for gate witdh ranging from 0% to 100%
		for(i=0;i<=100;i+=1)
		{		
			SPC3_Set_Gate_Values(spc3,0, i);
			SPC3_Apply_settings(spc3);
			SPC3_Prepare_Snap(spc3);
			SPC3_Get_Snap(spc3);		
			SPC3_Average_Img(spc3, data,1);			
			y[i] = mean_double(data,2048);		
			y[i+101] = y[i]/gateoff*100; //actual gate width calculated from photon counts
			x[i]=(double) i;						
			printf("%3.0f\t\t%.2f\t\t%.2f\n",x[i],y[i],y[i+101]);
			fprintf(f,"%.0f %.2f %.2f\n",x[i],y[i],y[i+101]);
		}
		fclose(f);
		printf("\n");
		break;

	case '8': // Triple gate mode
		//SPC3 constructor and parameter setting
		fname = (char*) calloc(256,sizeof(char));
		out=(int)SPC3_Constr(&spc3, Advanced,"");
		SPC3_Set_Camera_Par(spc3, 1040 * 3, 1000, 20, 3, Disabled, Disabled, Disabled);
		SPC3_Set_DeadTime(spc3, 45);	
		
		printf("Expose the SPC3 camera to a luminuos signal (pulsed or constant)\nPress ENTER to continue ...\n");
		getchar();
		printf("Acquiring...\n");
		
		sprintf(fname, "Triple gate mode.txt");
		if ((f = fopen(fname, "w")) == NULL)
		{
			printf("Unable to open the file %s.\n", fname);
			break;
		}

		SPC3_Set_TripleGate(spc3, Disabled, -400, 10, 10, 10, 0, 100, NULL);
		SPC3_Set_Trigger_Out_State(spc3, Gate_Clk);
		SPC3_Apply_settings(spc3);
		SPC3_Prepare_Snap(spc3);
		SPC3_Get_Snap(spc3);
		for (counter = 1; counter <= 3; counter++)
		{
			SPC3_Average_Img(spc3, data, counter);
			gateoff_array[counter - 1] = mean_double(data, 2048);
			
			printf("\nGate OFF counts on counter %d: %.2f\n", counter, gateoff_array[counter - 1]);
			fprintf(f, "Gate OFF counts on counter %d: %.2f\n", counter, gateoff_array[counter - 1]);
			
		}

		for (j = -400; j < 400; j++)
		{
			error = SPC3_Set_TripleGate(spc3, Enabled, j, 10, 10, 10, 0, 100, NULL);
			if (error < 0) break;
			SPC3_Apply_settings(spc3);
			SPC3_Prepare_Snap(spc3);
			SPC3_Get_Snap(spc3);
			for (counter = 1; counter <= 3; counter++)
			{
				SPC3_Average_Img(spc3, data, counter);
				counts[counter - 1] = mean_double(data, 2048);
			}

			printf("%d\t\t%.4f\t\t%.4f\t\t%.4f\n", j, counts[0] / gateoff_array[0] * 100, counts[2] / gateoff_array[2] * 100, counts[1] / gateoff_array[1] * 100);
			fprintf(f, "%d\t\t%.4f\t\t%.4f\t\t%.4f\n", j, counts[0] / gateoff_array[0] * 100, counts[2] / gateoff_array[2] * 100, counts[1] / gateoff_array[1] * 100);
		}
		fclose(f);


		printf("\n");
		free(fname);
		break;

	case '9': // Save and Read images
		//SPC3 constructor and parameter setting
		out=(int)SPC3_Constr(&spc3, Advanced,"");
		SPC3_Set_Camera_Par(spc3, 1040, 20,2,1,Disabled,Disabled,Disabled);
		SPC3_Set_DeadTime(spc3, 100);
		SPC3_Apply_settings(spc3);
		//acquiring and saving images
		printf("Acquiring 20 images and save them on the hard drive in the spc3 file format.\n");
		SPC3_Prepare_Snap(spc3);
		SPC3_Get_Snap(spc3);
		SPC3_Save_Img_Disk(spc3, 1,20,"Test20_Im", SPC3_FILEFORMAT);
		//reading images from file
		printf("Read the images from the disk and print the value of the top-left-corner pixel for each frame.\n(Press ENTER to continue)\n");
		getchar();
		SPC3_ReadSPC3FileFormatImage("Test20_Im.spc3", 1,1,Img, header );
		printf("Rows: %d, Columns: %d\n",header[100], header[101]);
		for(i=1;i<=20;i++)
		{
			SPC3_ReadSPC3FileFormatImage("Test20_Im.spc3", i,1,Img, header );
			printf("Image %d, pixel value = %hu\n",i, Img[0]);
		}
		break;

	case 'a': //Correlation image
		//SPC3 constructor and parameter setting
		out=(int)SPC3_Constr(&spc3, Normal,"");
		SPC3_Set_Camera_Par(spc3, 10500, 1024*64, 1,1,Disabled,Disabled,Disabled);
		SPC3_Set_DeadTime(spc3, 100);
		SPC3_Apply_settings(spc3);
		//acquire images
		printf("Acquiring the images...\n");
		SPC3_Prepare_Snap(spc3);
		SPC3_Get_Snap(spc3);
		//computer correlation
		SPC3_Set_Correlation_Mode(spc3, MultiTau, 8, Enabled);		
		printf("Starting the multi-tau autocorrelation algorithm\n");		
		start = clock();
		SPC3_Correlation_Img(spc3,1);
		stop = clock();
		printf("The correlation has terminated in %.3f s\n", (stop-start)/(float)CLOCKS_PER_SEC);		
		SPC3_Save_Correlation_Img(spc3,"Correlation_MultiTau.spcc");
		break;

	case 'b': //Continuous acquisition, number of integration frames selection. 
		//SPC3 constructor and parameter setting
		std::cout << "option disabled" << std::endl;
		break;
		/*
		out=(int)SPC3_Constr(&spc3, Normal,"");
		printf("\n");
		printf("Input the number of frames of 10.40us to be integrated (suggested > 10 to avoid data loss):\n");
		scanf("%d",&integFrames);
		printf("Total integration time: %.2fus\n", (float)(10.40*integFrames));
		SPC3_Set_Camera_Par(spc3, 1050, 1, integFrames,1,Disabled,Disabled,Disabled);
		SPC3_Set_DeadTime(spc3, 100);
		SPC3_Apply_settings(spc3);
		//acquire images
		printf("Continuous acquisition will be started and 10 memory dumps performed.\n");
		printf("Press ENTER to start continuous acquisition...\n");
		getchar();
		getchar();
		SPC3_Start_ContAcq(spc3, "contacq.spc3");
		for(i=1; i<10; i++)
		{	
			if (SPC3_Get_Memory(spc3,&read_bytes)==OK)
			{	
				total_bytes=total_bytes+read_bytes;
				printf("Acquired %f bytes in %d readout operation\n",total_bytes, i);
				SLEEP(1*MILLIS);
			}
			else
				break;
		}	
		SPC3_Stop_ContAcq(spc3);
		printf("Acquisition saved to contacq.spc3.\n");
		break;
		*/
	case 'c': //Continuous acquisition in memory, number of integration frames selection. 
		//SPC3 constructor and parameter setting
		std::cout << "continuous option disabled" << std::endl;
		break;
		/*
		out = (int)SPC3_Constr(&spc3, Normal, "");
		printf("\n");
		printf("Input the number of frames of 10.40us to be integrated (suggested > 10 to avoid data loss):\n");
		scanf("%d", &integFrames);
		printf("Total integration time: %.2fus\n", (float)(10.40*integFrames));
		SPC3_Set_Camera_Par(spc3, 1050, 1, integFrames, 1, Disabled, Disabled, Disabled);
		SPC3_Set_DeadTime(spc3, 100);
		SPC3_Apply_settings(spc3);
		//acquire images
		printf("Continuous acquisition will be started and 10 memory dumps performed.\n");
		printf("Press ENTER to start continuous acquisition...\n");
		getchar();
		getchar();
		SPC3_Start_ContAcq_in_Memory(spc3);
		for (i = 1; i<11; i++)
		{
			if (SPC3_Get_Memory_Buffer(spc3, &read_bytes, &buffer) == OK)
			{
				total_bytes = total_bytes + read_bytes;
				printf("Acquired %f bytes in %d readout operation\n", total_bytes, i);
				SLEEP(1 * MILLIS);
			}
			else
				break;
		}
		SPC3_Stop_ContAcq_in_Memory(spc3);
		break;
		*/
	case 'd':
		//SPC3 constructor and parameter setting
		fname = (char*) calloc(256,sizeof(char));
		if (argc < 3) {
			std::cout << "failed argc\n======\tsyntax is " << argv[0] << " <filehead> <nimages> =======\n" << std::endl;
			break;
		} else {
		//SPC3_Constr(&spc3, Normal,""); // set the mode to Advanced to get the eposure below 10.4 usec
		//SPC3_Set_Camera_Par(spc3, 100, 30000,300,1,Disabled,Disabled,Disabled);		
		/*
		 *
		 * OK change this to use the SPC3_Get_Memory_Buffer() method with the 
		 * SPC3_Start_ContAcq_in_Memory() method for filling a buffer.
		 *
		 */
		SPC3_Constr(&spc3, Advanced,""); // set the mode to Advanced to get the eposure below 10.4 usec
		//SPC3_Set_Camera_Par(handle , exposure (10ns units), nframes for snap, nintegration , ncounters, Force 8bit, half array, signed (needs 3 counters));		

		exposure=16;
		SPC3_Set_Camera_Par(spc3, exposure, 1,1,1,Enabled,Disabled,Disabled);		
		SPC3_Set_Trigger_Out_State(spc3,Frame);
		SPC3_Apply_settings(spc3); 
		SPC3_Set_Live_Mode_ON(spc3);
		//Acquistion of 10 live images
		std::vector<uint16_t> hist16(256,0);
		std::vector<uint16_t> hist16_rollover(256,0);
		nimages = std::atoi(argv[2]);
		size_t fnum = 0;
		for(i=0;i<nimages;i++)
		{
			SPC3_Get_Live_Img(spc3, Img);
			for (int n=0;n<2048;n++){
				if (Img[n]>0)
					if (hist16[ Img[n] ] == UINT16_MAX){
						hist16[ Img[n] ] = 0;
						hist16_rollover[ Img[n] ]++;
					} else {
						hist16[ Img[n] ]++;
					}
			}
			if (i%10000==0){
				std::ofstream hstream;
				sprintf(fname,"%s.hist",argv[1]);
				hstream.open(fname,std::ios::out);
				hstream << "#\tvalue\thist16\trollover " << uint32_t(UINT16_MAX+1) << "\t log2(hist16 + rollover*units)\n#\tfrom \t" << nimages << "\tframes\n";
				for (size_t i=0;i<hist16.size();i++)
					hstream << i << "\t" << hist16[i] << "\t" << hist16_rollover[i] << "\t" << log2(hist16[i] + uint64_t(UINT16_MAX * hist16_rollover[i])) << "\n";
				hstream << std::endl;
				hstream.close();
			}
			if (i%1000==0){
				sprintf(fname,"%s.%04d",argv[1],fnum++);
				imagestream.open(fname,std::ios::out);
				printf("Image %d:\n",fnum);
				for(j=0;j<64;j++)
				{
					for(k=0;k<32;k++)
						imagestream << Img[32*j+k] << "\t";
					imagestream << "\n";
				}		
				imagestream << "\n";
				imagestream.close();
			}
		}
		//Live mode off
		SPC3_Set_Live_Mode_OFF(spc3);						
		sprintf(fname,"%s.hist",argv[1]);
		histstream.open(fname,std::ios::out);
		histstream << "#\tvalue\thist16\trollover " << UINT16_MAX+1 << "\t log2(hist16 + rollover*units)\n#\tfrom \t" << nimages << "\tframes\n";
		for (size_t i=0;i<hist16.size();i++)
			histstream << i << "\t" << hist16[i] << "\t" << hist16_rollover[i] << "\t" << log2(hist16[i] + uint64_t(UINT16_MAX * hist16_rollover[i])) << "\n";
		histstream << std::endl;
		histstream.close();
		free(fname);
		break;
		}
	case 'e': 
		/* this option forces 8 bit images and processes a nearly full buffer, 
		 * if we were to do background subtract, then we would suffer 1/2 the number of frames
		 * because of the fixed buffer size but promotion of images to 16bit when using 
		 * background subtraction (though this technically only needs 9 bits to handle 
		 * signed the result of a subtraction of 2 8 bit ints.
		 */
		if (argc < 3) {
                        std::cout << "failed argc\n======\tsyntax is " << argv[0] << " <filehead> <nimages> <optional background img> =======\n" << std::endl;
                        break;
                } else {
		bool removeBGimg = false;
		bgfname = (char*) calloc(256,sizeof(char));
		std::vector< std::vector<uint8_t> > bgMat;
		bgMat.reserve(2048);
		std::vector<uint8_t> bgImg(2048,0); 
		if (argc > 3) { 
			removeBGimg = true; 
			sprintf(bgfname,"%s",argv[3]);
			std::cout << "Importing " << bgfname << " as background image" << std::endl;
			std::ifstream bgfile(bgfname,std::ios::in);
			if (!bgfile.is_open()){
				std::cerr << "load BGimg failed " << std::endl;
				removeBGimg = false;
			}
			bgMat.resize(64);
			for (unsigned i=0;i<64;i++)
				bgMat[i].resize(32);
			bgfile >> bgMat;
			bgfile.close();
			std::cerr << "bgMat shape = ( " << bgMat.size() << " , " << bgMat[0].size() << " )" << std::endl;
			for (unsigned i = 0;i<bgMat.size();i++){
				for (unsigned j = 0;j<bgMat[i].size();j++){
					bgImg[i*bgMat[i].size() + j] = bgMat[i][j];
				}
			}
		}
		

		fname = (char*) calloc(256,sizeof(char));
		sprintf(fname,"%s.hist",argv[1]);
		std::cout << "histogram fname = " << fname << std::endl;

		std::vector<uint64_t> SumImg(2048,0); // initializing to 0 eventhough log2(1) = 0 so we won't distinguish single counts and no counts.

		SPC3_Constr(&spc3, Advanced,""); // set the mode to Advanced to get the eposure below 10.4 usec
		exposure = 128;
		uint16_t getnimages = UINT16_MAX - 2; // giving one extra for size
		getnimages /= 2;
		SPC3_Set_Camera_Par(spc3, exposure, getnimages,1,1,Enabled,Disabled,Disabled);		

		/*
		 * Not doing hardware background subtraction since instead we are fixing the images to 8 bit to handle more images per file transfer to memory
		 * OK, actually, this might need to come back, but we should make it another option in order to fix the Img buffer processing to 1/2
		 * Or even better, we can subtract right from here without need to use the on-camera, then we preserve the frame rate and all.
		 */
		//		   SPC3_Set_Background_Img(spc3, bgImg );
		//		   SPC3_Set_Background_Subtraction ( spc3, Enabled);
		/*
		 * file read in from ascii is also good, since these are all 8 bit integers
		 * The stored background should be a forced uint8_t but maybe you could do a positive and a negative image...
		 * just in case the phase wrap is negative, then you still catch it in the negative image/hist.
		 */

		SPC3_Set_Trigger_Out_State(spc3,Frame);
		SPC3_Set_Live_Mode_OFF(spc3);
		SPC3_Set_Sync_In_State ( spc3, Disabled, 0);
		SPC3_Apply_settings(spc3); 
		size_t nbatches = std::atoi(argv[2]);

		BUFFER_H buff = NULL;
		const UInt16 counter(0);
		// Start timers
    		double wall0 = get_wall_time();
		double cpu0  = get_cpu_time();
		for (size_t b = 0; b<nbatches; b++)
		{
			std::cout << "Working batch " << b << std::endl;
			SPC3_Prepare_Snap(spc3);
			SPC3_Get_Snap(spc3);
			if (SPC3_Get_Image_Buffer ( spc3, &buff ) == OK)
			{
				/*
				   Get the pointer to the image buffer in which snap acquisition is stored.
				   The first byte indicates if data is 8 or 16 bit. WARNING User must pay attention not to exceed the dimension of the
				   buffer (2 ∗ 1024 ∗ 65534 + 1 bytes) when accessing it.
				   */
				//std::cerr << "OK, got captured snap for batch: " << b << "\n" << std::flush;
				unsigned bytesPpix = unsigned(*(buff))/8;
				if (b == 0 && bytesPpix > 1){ // only check on the first pass
					getnimages /= 2;
					getnimages -= 4;
				}
				//std::cerr << "bytes per pixel = " << bytesPpix << "\twas the resoponse of int(*(buff))\n" << std::flush;
				for (size_t o=0;o<2048*(getnimages-1);o++){
					uint8_t v = uint8_t(*(buff+(o+1)*bytesPpix));
					//if (v>0 && v<256){ // HERE HERE HERE HERE you can put the background subtraction based on e.g.
					if (v>bgImg[o%2048] && v<256+bgImg[o%2048]) { 
						v -= bgImg[o%2048]; 
						SumImg[o%2048] += v;
						hist256[v]++;
					}
				}
			}
			histstream.open(fname,std::ios::out);
			histstream << "#\tvalue\thist256\tlog2(hist256)\n#\tfrom \t" << (nbatches * getnimages) << "\tframes\n";
			for (size_t j=0;j<hist256.size();j++)
				histstream << j << "\t" << hist256[j] << "\t" << log2(hist256[j]) << "\n";
			histstream << std::endl;
			histstream.close();
		}

		//  Stop timers
		double wall1 = get_wall_time();
		double cpu1  = get_cpu_time();
		double runtime = (wall1 - wall0);
		double cputime = (cpu1 - cpu0);

		histstream.open(fname,std::ios::out);
		histstream << "#\tvalue\thist256\tlog2(hist256)\n#\tfrom \t" << (nbatches * getnimages) << "\tframes\n";
		histstream << "#image capture and process time was " << runtime << "s for " << (nbatches * getnimages) << " frames\n" << cputime << " cpu time" << std::endl;
		histstream << "#actual captured laser pulses is pulse spacing ~10ns * exposure (in units of 10 ns) " << runtime << "s for " << (nbatches * getnimages * exposure) << " pulses\n#\t" << cputime << " cpu time" << std::endl;
		for (size_t j=0;j<hist256.size();j++){
			histstream << j << "\t" << hist256[j] << "\t" << log2(hist256[j]) << "\n";
			if (j<8)
				std::cout << j << "\t" << hist256[j] << "\t" << log2(hist256[j]) << "\n";
		}
		histstream << std::endl;
		histstream.close();

// printing the log2() of the total integrated image
		sprintf(fname,"%s.log2img",argv[1]);
		std::cout << "log2() of integrated image fname = " << fname << std::endl;
		std::cout << "image capture and process time was " << runtime << "s for " << (nbatches * getnimages) << " frames\t" << (nbatches * getnimages * exposure) << " pulses\n" << std::endl;
		imagestream.open(fname,std::ios::out);
		imagestream << "#\ttotal integrated image, log2() representation from \t" << (nbatches * getnimages) << "\tframes\n";
		imagestream << "#image capture and process time was " << runtime << "s for " << (nbatches * getnimages) << " frames\n";
		for(size_t j=0;j<64;j++)
		{
			for(k=0;k<32;k++)
				imagestream << log2(SumImg[32*j+k]) << "\t";
			imagestream << "\n";
		}		
		imagestream << "\n";
		imagestream.close();

		if (!removeBGimg){
			sprintf(fname,"%s.asbackground",argv[1]);
			imagestream.open(fname,std::ios::out);
			imagestream << "#image capture and process time was " << runtime << "s for " << (nbatches * getnimages) << " frames of exposure = "<< exposure << "s\n";
			for(size_t j=0;j<64;j++)
			{
				for(size_t k=0;k<32;k++)
					imagestream << (SumImg[32*j+k]/(nbatches * getnimages * exposure)) << "\t";
				imagestream << "\n";
			}		
			imagestream << "\n";
			imagestream.close();
		}

		free(fname);
		free(bgfname);
		break;

		}


	default:
		break;
	}

////////////////////////////////////
// Destructors					  //
////////////////////////////////////

	if(spc3)
		SPC3_Destr(spc3);
	spc3 = NULL;
	free(Img);
	free(bgImg);
	free(Imgd);
	free(data);
	free(y);
	free(x);
	free(z);
	printf("Press ENTER to continue\n");
	getchar();
	return 0;
}
