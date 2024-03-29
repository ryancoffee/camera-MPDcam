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
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <algorithm> // possibly for the std::accumulate and related
#include <functional> // Only if I'm using lambdas for the accumulate functions for centroiding row-wise distributions.

#include <chrono> // millisecond system_clock
#include <thread> // sleep_for sleep_until

#include <stdint.h>

#include "Utility.h"
#include "mpdHist_tofile.h"
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"


//***********************************//
//		          	     //
//		Main code	     //
//				     //
//***********************************//




int main(int argc, char *argv[])
{
//////////////////////////
// variables definition //
//////////////////////////
//
	std::vector<int> tiff_compression_params;
	tiff_compression_params.push_back(cv::IMWRITE_TIFF_XDPI);
	tiff_compression_params.push_back(32);
	tiff_compression_params.push_back(cv::IMWRITE_TIFF_YDPI);
	tiff_compression_params.push_back(32);

	std::vector<int> jpg_compression_params;
	jpg_compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
	jpg_compression_params.push_back(100);

	std::vector<int> png_compression_params;
    	png_compression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
    	png_compression_params.push_back(5); // 0 is fastest, lowest compression

	std::cout << "Remember, if writing ascii to file (options 'd,e'), give the filehead (and maybe nimages) in the command" << std::endl;
	SPC3_H spc3= NULL;
	UInt16* Img= NULL;
	UInt16 hist[65535];
	UInt16 AppliedDT=0;
	std::vector<uint64_t> hist256(256,0);

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
	uint8_t * bgarray = NULL;
	BUFFER_H buffer = NULL;
	int counter = 0, aggressor=0;
	SPC3Return error = OK;
	FILE* f= NULL;
	time_t start,stop;
	char *Test[] = { "Live mode: write on stdout 5 images", //0
					 "OpenCV monitor mode: update images in window", //1
					 "NULL", //2
					 "NULL", //3
					 "NULL", //4
					 "NULL", //5
					 "NULL", //6
					 "NULL", //7
					 "NULL", //8
					 "NULL", //9
					"Live mode: main <argv[1] is filehead> <argv[2] is nimages>", //a
					"Dump nimages: main <argv[1] is filehead> <argv[2] is nimages> <argv[3] is optional bgfile>", //b
					"Dump sparse nimages as ascii: main <argv[1] is filehead> <argv[2] is minutes of acquisition>", //c
					"row sum hist: main <argv[1] is filehead> <argv[2] is nbatches> <argv[3] is optional bgfile>.", //d
					"Acquire histogram: main <argv[1] is filehead> <argv[2] is nbatches> <argv[3] is optional bgfile>" //e
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
		{
		//SPC3 constructor and parameter setting
		SPC3_Constr(&spc3, Normal,"");
		SPC3_Set_Camera_Par(spc3, 1, 1,5,1,Disabled,Disabled,Disabled);		
		SPC3_Set_Trigger_Out_State(spc3,Frame);
		SPC3_Apply_settings(spc3); 
		SPC3_Set_Live_Mode_ON(spc3);
		//Acquistion of 10 live images
		for(size_t i=0;i<5;i++)
		{
			std::ostringstream ss;
			ss << "Image_" << i;
			printf("Image %d:\n",i);
			cv::Mat mat(64,32,CV_16UC1);// ,(void*)Img);//,64*sizeof(uint8_t));
			const uint8_t scale(16);
			//cv::Mat outmat(scale*64,scale*32,CV_8UC1);// ,(void*)Img);//,64*sizeof(uint8_t));
			//SPC3_Get_Live_Img(spc3, Img);
			SPC3_Get_Live_Img(spc3, (uint16_t*) mat.data); // livemode usees uint16_t it seems, so multiplying by 2**8 for imshow
			for(size_t j=0;j<64;j++)
			{
				for(size_t k=0;k<32;k++){
					/*
 * 					for(size_t m=0;m<scale;m++){
						for(size_t n=0;n<scale;n++){
							outmat.at<uint8_t>(scale*j+n,scale*k+m) = (uint8_t) mat.at<uint16_t>(j,k);
						}
					}
					*/
					printf("%d ",(uint8_t) mat.at<uint16_t>(j,k));
				}
				printf("\n");
			}		
			//cv::namedWindow(ss.str());
			//cv::imshow(ss.str(),outmat.t()); // mat.mul(256) so that it is visible when rendered via imshow, outmat is set to CV_8UC1 so it shouldn't need mul(256)
			//std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
		//Live mode off
		SPC3_Set_Live_Mode_OFF(spc3);						
		//cv::waitKey(0);
		//cv::destroyAllWindows();
		}
		break;

	case '1'://OpenCV monitor mode
		{
			BUFFER_H mybuff = NULL; 
			SPC3_Constr(&spc3, Normal,""); // set the mode to Advanced to get the eposure below 10.4 usec
			uint16_t exposure = 1; // If we are in normal mode, exposure is not used, instead it is just the span of time 
			uint16_t getnimages(16); // UINT16_MAX - 2; // giving one extra for size
			const uint16_t nframeinteg(1); // this seems to fail if I set this to 100
			if ( SPC3_Set_Camera_Par(spc3, exposure, getnimages,nframeinteg,1,Enabled,Disabled,Disabled) != OK) {
				free(mybuff);
				break;
			}		
			//SPC3 constructor and parameter setting
			SPC3_Set_Trigger_Out_State(spc3,Frame);
			SPC3_Apply_settings(spc3); 
			SPC3_Set_Live_Mode_OFF(spc3);
			const uint8_t scale(16);
			std::ostringstream ss;
			ss << "Image monitor";
			cv::namedWindow(ss.str());
			cv::Mat outmat(64,32,CV_8UC1);// ,(void*)Img);//,64*sizeof(uint8_t));
			for(size_t i=0;i<5;i++)
			{
				printf("Image %d:\n",i);
				const uint8_t scale(16);
				//SPC3_Get_Live_Img(spc3, Img);
				SPC3_Prepare_Snap(spc3);
				SPC3_Get_Snap(spc3);
				if (SPC3_Get_Image_Buffer ( spc3, &mybuff ) != OK)
				{
					free(mybuff);
				} else {
					unsigned bytesPpix = unsigned(*(mybuff))/8; // checking the first bit to see if vector is 8 bits or 16.
					if (bytesPpix > 1){ // only check on the first pass
						getnimages /= 2;
						getnimages -= 4;
					}
					outmat = 0;
					for (size_t f=0;f<getnimages;f++)
					{
						cv::Mat mat(64,32,CV_8UC1,(void*)mybuff + 1 + f*2048*bytesPpix );// ,(void*)Img);//,64*sizeof(uint8_t));
						for (size_t j=0;j<32;j++){
							for (size_t k=0;k<64;k++){
								outmat.at<uint8_t>(j,k) += mat.at<uint8_t>(j,k);
							}
						}
					}
					cv::imshow(ss.str(),outmat.t()); // mat.mul(256) so that it is visible when rendered via imshow, outmat is set to CV_8UC1 so it shouldn't need mul(256)
					cv::waitKey(0);
				}
			}
			cv::destroyAllWindows();
			if (mybuff != NULL)
			{
				//free(mybuff);
				mybuff = NULL;
			}
		}
		break;
		break;

	case '2': //NULL
		break;

	case '3'://NULL
		break;

	case '4': // NULL
		break;

	case '5': // NULL
		break;

	case '6': // NULL
		break;

	case '7': // NULL
		break;

	case '8': // NULL
		break;

	case '9': // NULL
		break;

	case 'a': // Live mode
		//SPC3 constructor and parameter setting
		fname = (char*) calloc(256,sizeof(char));
		if (argc < 3) {
			std::cout << "failed argc\n======\tsyntax is " << argv[0] << " <filehead> <nimages> =======\n" << std::endl;
			break;
		} else {
		//SPC3_Constr(&spc3, Normal,""); // set the mode to Advanced to get the eposure below 10.4 usec
		//SPC3_Set_Camera_Par(spc3, 100, 30000,300,1,Disabled,Disabled,Disabled);		
		SPC3_Constr(&spc3, Normal,""); // set the mode to Advanced to get the eposure below 10.4 usec
		uint16_t exposure=100; // this is not used in Normal mode
		//SPC3_Set_Camera_Par(handle , exposure (10ns units), nframes for snap, nintegration , ncounters, Force 8bit, half array, signed (needs 3 counters));		
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
			cv::Mat mat(64,32,CV_16UC1);// ,(void*)Img);//,64*sizeof(uint8_t));
			SPC3_Get_Live_Img(spc3, (uint16_t*) mat.data); // livemode usees uint16_t it seems, so multiplying by 2**8 for imshow
			for (int n=0;n<2048;n++){
				if (mat.data[n]>0)
					if (hist16[ mat.data[n] ] == UINT16_MAX){
						hist16[ mat.data[n] ] = 0;
						hist16_rollover[ mat.data[n] ]++;
					} else {
						hist16[ mat.data[n] ]++;
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
	case 'b': // Dump nimages
		{
			if (argc < 3) {
				std::cout << "failed argc\n======\tsyntax is " << argv[0] << " <filehead> <nimages> <optional background img> =======\n" << std::endl;
				break;
			} 
			if (argc > 3) { 
				std::cout << "background subtraction not yet implemented" << std::endl;
				break;
			}

			cv::Mat mat(64,32,CV_8UC1);//,32*sizeof(uint8_t));
			mat = uint8_t(0);
			BUFFER_H mybuff = NULL;
			fname = (char*) calloc(256,sizeof(char));
			sprintf(fname,"%s.imdump",argv[1]);

			SPC3_Constr(&spc3, Advanced,""); // set the mode to Advanced to get the eposure below 10.4 usec
			//SPC3_Constr(&spc3, Normal,""); // set the mode to Advanced to get the eposure below 10.4 usec
			uint16_t exposure = 16; // If we are in normal mode, exposure is not used, instead it is just the span of time 
			uint64_t totalimages = atoi(argv[2]);
			//uint16_t getnimages = UINT8_MAX - 2; // giving one extra for size
			uint16_t getnimages = UINT16_MAX - 2; // giving one extra for size
			if (totalimages < getnimages){
				getnimages = totalimages;
			}
			const uint16_t nframeinteg(1); // this seems to fail if I set this to 100
			double wall0 = get_wall_time<double>();
			double cpu0  = get_cpu_time<double>();
			SPC3_Set_Trigger_Out_State(spc3,Frame);
			SPC3_Set_Live_Mode_OFF(spc3);
			SPC3_Set_Sync_In_State ( spc3, Disabled, 0);
			SPC3_Apply_settings(spc3); 
			size_t last=0;
			//std::cout << "max channels = " << CV_CN_MAX << std::endl;
			//std::vector<int> ndshape = {64,32,getnimages};
			while (totalimages > getnimages){
				std::cout << "getting " << getnimages << " images" << std::endl;
				if ( SPC3_Set_Camera_Par(spc3, exposure, getnimages,nframeinteg,1,Enabled,Disabled,Disabled) != OK) {
					free(fname);
				} else {

					SPC3_Prepare_Snap(spc3);
					SPC3_Get_Snap(spc3);
					if (SPC3_Get_Image_Buffer ( spc3, &mybuff ) != OK)
					{
						free(mybuff);
					} else {
						unsigned bytesPpix = unsigned(*(mybuff))/8; // checking the first bit to see if vector is 8 bits or 16.
						uint8_t v(0);
						if (bytesPpix > 1){ // only check on the first pass
							getnimages /= 2;
							getnimages -= 4;
						}
						for (size_t ptr=0;ptr < getnimages*2048;ptr += 2048){
							void* imPtr = (void*)(mybuff+1+ptr*bytesPpix);
							cv::Mat img(64,32,CV_8UC1,imPtr);
							sprintf(fname,"%s.imdump.%05i.jpg",argv[1],ptr/2048+last);
							cv::normalize(img, img, 0, 255,cv::NORM_MINMAX,CV_8UC1);
							cv::imwrite(fname,img,jpg_compression_params);
						}
						totalimages -= getnimages;
						last += getnimages;
					}
				}
			}
			if (totalimages > 0){
				getnimages = totalimages;
				std::cout << "getting " << getnimages << " images" << std::endl;
				if ( SPC3_Set_Camera_Par(spc3, exposure, getnimages,nframeinteg,1,Enabled,Disabled,Disabled) != OK) {
					std::cout << "didn't set the camera pars" << std::endl;
					free(fname);
				} else {
					SPC3_Prepare_Snap(spc3);
					SPC3_Get_Snap(spc3);
					if (SPC3_Get_Image_Buffer ( spc3, &mybuff ) != OK)
					{
						std::cout << "didn't get snap and get image buffer" << std::endl;
						free(mybuff);
					} else {
						unsigned bytesPpix = unsigned(*(mybuff))/8; // checking the first bit to see if vector is 8 bits or 16.
						if (bytesPpix > 1){ // only check on the first pass
							getnimages /= 2;
							getnimages -= 4;
						}
						//cv::Mat img(std::vector<int>())
						for (size_t ptr=0;ptr < getnimages*2048;ptr += 2048){
							//std::cout << "Image " << int(ptr/2048) << std::endl;
							void* imPtr = (void*)(mybuff+1+ptr*bytesPpix);
							cv::Mat img(64,32,CV_8UC1,imPtr);
							//std::cout << "img now set to imPtr" << std::endl;
							sprintf(fname,"%s.imdump.%05i.jpg",argv[1],ptr/2048+last);
							cv::normalize(img, img, 0, 255,cv::NORM_MINMAX,CV_8UC1);
							cv::imwrite(fname,img,jpg_compression_params);
						}
					}
				}
			}
			free(mybuff);
			mybuff = NULL;
		}
		break;
	case 'c': // dump sparse nimages
		{
			if (argc < 3) {
				std::cout << "failed argc\n======\tsyntax is " << argv[0] << " <filehead> <minutes of acquisition> =======\n" << std::endl;
				break;
			} 
			if (argc > 3) { 
				std::cout << "background subtraction not yet implemented" << std::endl;
				break;
			}
			BUFFER_H mybuff = NULL;
			fname = (char*) calloc(256,sizeof(char));
			sprintf(fname,"%s.batch%i.sparse",argv[1],0);
			std::cout << "sparse images fname = " << fname << std::endl;
			SPC3_Constr(&spc3, Advanced,""); // set the mode to Advanced to get the eposure below 10.4 usec
			//SPC3_Constr(&spc3, Normal,""); // set the mode to Advanced to get the eposure below 10.4 usec
			uint16_t exposure = 5; // If we are in normal mode, exposure is not used, instead it is just the span of time 
					// choosing 5 clocks for exposure to match 50ns MIN_DEAD_TIME
			uint16_t getnimages = UINT16_MAX/2; // giving extra for safety size
			const uint16_t nframeinteg(1); // this seems to fail if I set this to 100
			std::vector< std::vector< size_t > > ircv; // ids,rows,cols,vals
			if ( SPC3_Set_Camera_Par(spc3, exposure, getnimages,nframeinteg,1,Enabled,Disabled,Disabled) != OK) {
				free(fname);
			} else {
				double wall0 = get_wall_time<double>();
				double cpu0  = get_cpu_time<double>();
				SPC3_Set_Trigger_Out_State(spc3,Frame);
				SPC3_Set_Live_Mode_OFF(spc3);
				SPC3_Set_Sync_In_State ( spc3, Disabled, 0);
				SPC3_Set_DeadTime( spc3, MIN_DEAD_TIME); //uint16_t(100) ) //between MIN_DEAD_TIME and MAX_DEAD_TIME
				SPC3_Set_DeadTime_Correction ( spc3, Disabled );
				//
				// Applying the settings //
				//
				SPC3_Apply_settings(spc3); 
				double elapsed = (get_wall_time<double>() - wall0)/60.;
				uint16_t batchnum(0);
				std::ofstream output;
				while (elapsed < unsigned(atoi(argv[2])) ){
					sprintf(fname,"%s.batch%i.sparse.ascii",argv[1],batchnum);
					//std::cout << "getting " << getnimages << " images for file " << fname << std::endl;
					if ( SPC3_Set_Camera_Par(spc3, exposure, getnimages,nframeinteg,1,Enabled,Disabled,Disabled) != OK) {
						free(fname);
					} else {
						SPC3_Prepare_Snap(spc3);
						SPC3_Get_Snap(spc3);
						if (SPC3_Get_Image_Buffer ( spc3, &mybuff ) != OK)
						{
							std::cout << "didn't get snap and get image buffer" << std::endl;
							free(mybuff);
						} else {
							unsigned bytesPpix = unsigned(*(mybuff))/8; // checking the first bit to see if vector is 8 bits or 16.
							if (bytesPpix > 1){ // only check on the first pass
								getnimages /= 2;
								getnimages -= 4;
							}
							//cv::Mat img(std::vector<int>())
							for (size_t o=0; o < getnimages*2048; o++ ){
								uint8_t v = *(mybuff+1+o*bytesPpix);
								if (v>0){
									uint16_t p;
									p = o%2048;
									/*
									std::vector<size_t> vec; 
									vec[0] = o/2048;
									vec[1] = p/32;
									vec[2] = p%32;
									vec[3] = v;
									*/
									ircv.push_back( std::vector<size_t>( {o/2048,p/32,p%32,v}) ); // Not sure, but I think o is image number, p/32 is row number, p%32 is col number, v is pix value
								}
							}
							//sprintf(fname,"%s.batch%i.sparseascii",argv[1],batchnum);
							if (batchnum%10==0){
								output.open(fname,std::ios::out);
								output << "#shotID\trow\tcol\tval\n";
								while (ircv.size()>1){ // Sorry, I don't understand how I'm writing this file... we need to switch ot hdf5
									output << ircv.back();
									ircv.pop_back();
									output << "\n";
								}
								//output << ircv;
								output.close();
								std::cout << "wrote " << fname << "\n"; 
								elapsed = (get_wall_time<double>() - wall0)/60.;
								std::cout << int((batchnum+1)*getnimages) << " captures in elapsed time = " << elapsed << " minutes\n" << std::flush;
								ircv.resize(0);
							}
							batchnum++;
						}
					}
				}
				free(fname);
			}
		}
		break;
	case 'd': // row sum hist
		{
		if (argc < 3) {
			std::cout << "failed argc\n======\tsyntax is " << argv[0] << " <filehead> <nbatches> <optional background img> =======\n" << std::endl;
			break;
		} 
		if (argc > 3) { 
			std::cout << "background subtraction not yet implemented" << std::endl;
			break;
		}
		size_t nbatches = std::atoi(argv[2]);
		cv::Mat mat(64,32,CV_8UC1);//,32*sizeof(uint8_t));
		mat = uint8_t(0);

		BUFFER_H mybuff = NULL;
		fname = (char*) calloc(256,sizeof(char));
		sprintf(fname,"%s.rowsumhist",argv[1]);
		std::cout << "histogram fname = " << fname << std::endl;

		std::vector<uint64_t> SumImg(2048,0); // initializing to 0 eventhough log2(1) = 0 so we won't distinguish single counts and no counts.

		SPC3_Constr(&spc3, Advanced,""); // set the mode to Advanced to get the eposure below 10.4 usec
		//SPC3_Constr(&spc3, Normal,""); // set the mode to Advanced to get the eposure below 10.4 usec
		uint16_t exposure = 16; // If we are in normal mode, exposure is not used, instead it is just the span of time 
		uint16_t getnimages = UINT16_MAX - 2; // giving one extra for size
		const uint16_t nframeinteg(1); // this seems to fail if I set this to 100
		if ( SPC3_Set_Camera_Par(spc3, exposure, getnimages,nframeinteg,1,Enabled,Disabled,Disabled) != OK) {
			free(fname);
		} else {
			double wall0 = get_wall_time<double>();
			double cpu0  = get_cpu_time<double>();
			std::vector<size_t> hist16((size_t) UINT16_MAX+1 , 0);
			SPC3_Set_Trigger_Out_State(spc3,Frame);
			SPC3_Set_Live_Mode_OFF(spc3);
			SPC3_Set_Sync_In_State ( spc3, Disabled, 0);
			SPC3_Apply_settings(spc3); 
			size_t nbatches = std::atoi(argv[2]);
			uint16_t rowsum(0);
			for (size_t b=0;b<nbatches;b++){
				std::cout << "Working batch " << b << std::endl;
				SPC3_Prepare_Snap(spc3);
				SPC3_Get_Snap(spc3);
				if (SPC3_Get_Image_Buffer ( spc3, &mybuff ) != OK)
				{
					free(mybuff);
				} else {
					unsigned bytesPpix = unsigned(*(mybuff))/8; // checking the first bit to see if vector is 8 bits or 16.
					uint8_t v(0);
					if (b == 0 && bytesPpix > 1){ // only check on the first pass
						getnimages /= 2;
						getnimages -= 4;
					}
					for (size_t r = 0;r<2048*getnimages;r += 32){
						rowsum = std::accumulate(mybuff+1+r*bytesPpix , mybuff+1+(r+32)*bytesPpix , 0);
						hist16[rowsum] ++;
						if (b%10==0 && r<(4096+2048)){
							std::cout << rowsum << ' ';
							if ((r+1)%2048==0){
								std::cout << "\n";
							}
						}
					}
				}
			}
			//  Stop timers
			double wall1 = get_wall_time();
			double cpu1  = get_cpu_time();
			double runtime = (wall1 - wall0);
			double cputime = (cpu1 - cpu0);

			histstream.open(fname,std::ios::out);
			histstream << "#\tvalue\thist256\tlog2(hist256)\n#\tfrom \t" << (nbatches * getnimages) << "\tframes\n";
			histstream << "#image capture and process time was " << runtime << "s for " << (nbatches * getnimages) << " frames\n#\t" << cputime << " cpu time" << std::endl;
			histstream << "#actual captured laser pulses is pulse spacing ~10ns * exposure (in units of 10 ns) " << runtime << "s for " << (nbatches * getnimages * exposure) << " pulses\n#\t" << cputime << " cpu time" << std::endl;
			for (size_t j=0;j<hist256.size();j++){
				histstream << j << "\t" << hist16[j] << "\t" << log2(hist16[j]) << "\n";
				if (j<8)
					std::cout << j << "\t" << hist16[j] << "\t" << log2(hist16[j]) << "\n";
			}
			histstream << std::endl;
			histstream.close();
		}
		free(fname);
		fname = NULL;
		}
		break;

	case 'e': 
		/* 
		 * this option forces 8 bit images and processes a nearly full buffer, 
		 * if we were to do background subtract, then we would suffer 1/2 the number of frames
		 * because of the fixed buffer size but promotion of images to 16bit when using 
		 * background subtraction (though this technically only needs 9 bits to handle 
		 * signed the result of a subtraction of 2 8 bit ints.
		 */
		if (argc < 3) {
			std::cout << "failed argc\n======\tsyntax is " << argv[0] << " <filehead> <nbatches> <optional background img> =======\n" << std::endl;
			break;
		} 
		{
			//std::string samplewindow("sample image");
			//cv::namedWindow(samplewindow.c_str());

			size_t sz(2048);
			bool removeBGimg = false;
			bgfname = (char*) calloc(256,sizeof(char));
			bgarray = (uint8_t*) calloc(2048,sizeof(uint8_t));
			if (argc > 3) { 
				removeBGimg = true; 
			sprintf(bgfname,"%s",argv[3]);
			Utility::readBinaryFile(bgfname,sz,bgarray);
			std::cout << "bgarray = \n" << std::endl;
			for (size_t m=0;m<sz;m++){
				std::cout << (int)bgarray[m] << " ";
				if ((m+1)%32 == 0)
					std::cout << "\n";
			}
			std::cout << std::endl;
		}

		//std::cerr << "removeBGimg = " << removeBGimg << "\t and false = " << (false) << std::endl;

		fname = (char*) calloc(256,sizeof(char));
		sprintf(fname,"%s.hist",argv[1]);
		std::cout << "histogram fname = " << fname << std::endl;

		std::vector<uint64_t> SumImg(2048,0); // initializing to 0 eventhough log2(1) = 0 so we won't distinguish single counts and no counts.

		SPC3_Constr(&spc3, Normal,""); // set the mode to Advanced to get the eposure below 10.4 usec
		uint16_t exposure = 1; // If we are in normal mode, exposure is not used, instead it is just the span of time 
		uint16_t getnimages = UINT16_MAX - 2; // giving one extra for size
		const uint16_t nframeinteg(1); // this seems to fail if I set this to 100
		if ( SPC3_Set_Camera_Par(spc3, exposure, getnimages,nframeinteg,1,Enabled,Disabled,Disabled) != OK) {
			free(bgfname);
			free(fname);
			break;
		}		

		SPC3_Set_Trigger_Out_State(spc3,Frame);
		SPC3_Set_Live_Mode_OFF(spc3);
		SPC3_Set_Sync_In_State ( spc3, Disabled, 0);
		SPC3_Apply_settings(spc3); 
		size_t nbatches = std::atoi(argv[2]);

		BUFFER_H buff = NULL;
		const UInt16 counter(0);
		// Start timers
		double wall0 = get_wall_time<double>();
		double cpu0  = get_cpu_time<double>();
		for (size_t b = 0; b<nbatches; b++)
		{
			std::cout << "Working batch " << b << std::endl;
			SPC3_Prepare_Snap(spc3);
			SPC3_Get_Snap(spc3);
			if (SPC3_Get_Image_Buffer ( spc3, &buff ) != OK)
			{
				free(buff);
			} else {
				/*
				   Get the pointer to the image buffer in which snap acquisition is stored.
				   The first byte indicates if data is 8 or 16 bit. WARNING User must pay attention not to exceed the dimension of the
				   buffer (2 ∗ 1024 ∗ 65534 + 1 bytes) when accessing it.
				   */
				//std::cerr << "OK, got captured snap for batch: " << b << "\n" << std::flush;
				unsigned bytesPpix = unsigned(*(buff))/8; // checking the first bit to see if vector is 8 bits or 16.
				if (b == 0 && bytesPpix > 1){ // only check on the first pass
					getnimages /= 2;
					getnimages -= 4;
				}
				//std::cerr << "bytes per pixel = " << bytesPpix << "\twas the resoponse of int(*(buff))\n" << std::flush;
				for (size_t o=0;o<2048*getnimages-1;o++){
					uint8_t v;
					uint8_t bg;
					v = (uint8_t)(*(buff+1+o*bytesPpix));
					//v = (*(buff+(o+1)*bytesPpix));
					if (removeBGimg){
						bg = bgarray[o%2048];
						if (bg<v){
							v -= bg; 
						} else {
							v = uint8_t(0);
						}
					} 
					SumImg[o%2048] += v;
					hist256[v]++;
					if ((b%10==0) and (o<2048)){
						std::cout << (int)v << " ";
						if ((o+1)%32 == 0)
							std::cout << "\n";
					}	
				}
/*
				if (b%100==0)  // sample image to term
				{
					cv::Mat mat(64,32,CV_8UC1,(void*)buff+1);//,32*sizeof(uint8_t));

					for(size_t j=0;j<64;j++)
					{
						for(size_t k=0;k<32;k++){
							std::cout << mat.at<utint8_t>(j,k) << ' ';
						}
						std::cout << '\n' << std::flush;
					}
					const uint8_t scale(16);
					cv::Mat outmat(scale*64,scale*32,CV_8UC1);// ,(void*)Img);//,64*sizeof(uint8_t));
					for(size_t j=0;j<64;j++)
					{
						for(size_t k=0;k<32;k++){
							for(size_t m=0;m<scale;m++){
								for(size_t n=0;n<scale;n++){
									outmat.at<uint8_t>(scale*j+n,scale*k+m) = (uint8_t) mat.at<uint8_t>(j,k);
								}
							}
						}
					}		
					cv::imshow(samplewindow.c_str(),outmat.t()); // mat.mul(256) so that it is visible when rendered via imshow, outmat is set to CV_8UC1 so it shouldn't need mul(256)
					cv::waitKey(0);
				}
*/
			}
			histstream.open(fname,std::ios::out);
			histstream << "#\tvalue\thist256\tlog2(hist256)\n#\tfrom \t" << (nbatches * getnimages * nframeinteg) << "\tframes\n";
			for (size_t j=0;j<hist256.size();j++)
				histstream << j << "\t" << hist256[j] << "\t" << log2(hist256[j]) << "\n";
			histstream << std::endl;
			histstream.close();
			double wall = get_wall_time();
			std::cout << "runtime at batch " << b << " = " << (wall - wall0) << std::endl;
		}

		//  Stop timers
		double wall1 = get_wall_time();
		double cpu1  = get_cpu_time();
		double runtime = (wall1 - wall0);
		double cputime = (cpu1 - cpu0);

		histstream.open(fname,std::ios::out);
		histstream << "#\tvalue\thist256\tlog2(hist256)\n#\tfrom \t" << (nbatches * getnimages) << "\tframes\n";
		histstream << "#image capture and process time was " << runtime << "s for " << (nbatches * getnimages) << " frames\n#\t" << cputime << " cpu time" << std::endl;
		histstream << "#actual captured laser pulses is pulse spacing ~10ns * exposure (in units of 10 ns) " << runtime << "s for " << (nbatches * getnimages * exposure) << " pulses\n#\t" << cputime << " cpu time" << std::endl;
		for (size_t j=0;j<hist256.size();j++){
			histstream << j << "\t" << hist256[j] << "\t" << log2(hist256[j]) << "\n";
			if (j<8)
				std::cout << j << "\t" << hist256[j] << "\t" << log2(hist256[j]) << "\n";
		}
		histstream << std::endl;
		histstream.close();

		sprintf(fname,"%s.img",argv[1]);
		std::cout << "log2() of integrated image fname = " << fname << std::endl;
		std::cout << "image capture and process time was " << runtime << "s for " << (nbatches * getnimages) << " frames\t" << (nbatches * getnimages * exposure) << " pulses\n" << std::endl;
		imagestream.open(fname,std::ios::out);
		imagestream << "#\ttotal integrated image, log2() representation from \t" << (nbatches * getnimages) << "\tframes\n";
		imagestream << "#image capture and process time was " << runtime << "s for " << (nbatches * getnimages) << " frames\n";
		for(size_t j=0;j<64;j++)
		{
			for(k=0;k<32;k++){
				imagestream << SumImg[32*j+k] << " ";
			}
			imagestream << "\n";
		}		
		imagestream << "\n";
		imagestream.close();

		if (!removeBGimg){
			for (size_t i=0;i<sz;i++){
				bgarray[i] = (uint8_t)( (double)SumImg[i]/double(nbatches * getnimages) );
				std::cerr << (int) bgarray[i] << " ";
				if ((i+1)%32==0)
					std::cerr << "\n";
			}
			sprintf(fname,"%s.asbackground",argv[1]);
			Utility::writeBinaryFile(bgarray,sz, fname);
		}

		//cv::destroyAllWindows();
		free(fname);
		free(bgfname);
		}
		break;


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
	if (bgarray)
		free(bgarray);
	bgarray = NULL;
	free(Imgd);
	free(data);
	free(y);
	free(x);
	free(z);
	printf("Press ENTER to continue\n");
	getchar();
	return 0;
}
