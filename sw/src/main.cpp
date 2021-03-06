
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <dirent.h>
#include "svmResults.hpp"
#include <iostream>
#include "xhog.h"
#include <stdint.h>
#include <stdexcept>
#include "xparameters.h"
#include "display_ctrl.h"
#include "linuxmmap.h"
#include "linuxmisc.h"
#include "xil_printf.h"
#include "omp.h"
#include <sys/time.h>
#include "consts.h"
#include "math.h"

using namespace cv;
using namespace std;

/*********************************************/
/*************		VGA				**********/


/*--------------------------*/
/*        Constants         */
/*--------------------------*/

#define DISPLAY_MAX_FRAME 		( 1920*1080 )
#define DISPLAY_STRIDE 			( 1920*sizeof( uint32_t ) )
#define DISPLAY_IS_HDMI			( false )
#define VDMA_DEVICE_ID 			XPAR_AXI_VDMA_0_DEVICE_ID
#define VDMA_FRAME_BUFF_ADDR	( 0x01000000 ) /* Physical Address */

#define IMAGEBUFFER0 0x06000000 
#define IMAGEBUFFER1 0x07000000 

/*--------------------------*/
/*        Type Defs         */
/*--------------------------*/

typedef uint32_t ( vdma_framebuff )[ DISPLAY_NUM_FRAMES ][ DISPLAY_MAX_FRAME ];

/*--------------------------*/
/*       Declarations       */
/*--------------------------*/

/* Object that represents the Digilent AXI Display Controller. */
DisplayCtrl display_obj;

/* The following virtual addresses need to be defined such that
They are mapped to the appropriate physical addresses! */
linuxmmap dispctrl_mmap_obj( XPAR_AXI_DISPCTRL_0_S_AXI_BASEADDR, ( 0x10000 ) );
linuxmmap vdma_mmap_obj( XPAR_AXI_VDMA_0_BASEADDR, ( 0x10000 ) );
linuxmmap framebuff_mmap_obj( VDMA_FRAME_BUFF_ADDR, sizeof( vdma_framebuff ) );

/* Reference the memory dedicated to the frame buffers. */
vdma_framebuff& framebuff_arr = *( reinterpret_cast<vdma_framebuff*>( framebuff_mmap_obj.getvaddr() ) );
uint32_t* vir_framebuff_ptr[ DISPLAY_NUM_FRAMES ];
uint32_t* phy_framebuff_ptr[ DISPLAY_NUM_FRAMES ];

/* OpenCV objects. */
Mat framebuff_mat[ DISPLAY_NUM_FRAMES ];
int nframes;

/*********************************************/

XHog hog0;
XHog hog1;
XHog hog2;
XHog hog3;
XHog hog4;
XHog hog5;
XHog hog6;
XHog hog7;

XHog *HOG0 = &hog0;
XHog *HOG1 = &hog1;
XHog *HOG2 = &hog2;
XHog *HOG3 = &hog3;
XHog *HOG4 = &hog4;
XHog *HOG5 = &hog5;
XHog *HOG6 = &hog6;
XHog *HOG7 = &hog7;

void print_accel_status(XHog *HOG,char *name)
{
	int isDone, isIdle, isReady;
	isDone = XHog_IsDone(HOG);
	isIdle = XHog_IsIdle(HOG);
	isReady = XHog_IsReady(HOG);
	printf("%s Status: isDone %d, isIdle %d, isReady%d\r\n",name, isDone, isIdle, isReady);
}

void hog_init(XHog *HOG,char *name){
 	//Kernel - Init 
    printf("Ready to init kernel\n");
    int status = XHog_Initialize(HOG,name);
    if(status != XST_SUCCESS){ 
        printf("XHog_Initialize failed\n");
    }   
    printf("Kernel initialized\n");
    XHog_InterruptGlobalDisable(HOG);
    XHog_InterruptDisable(HOG, 1); 
    printf("Interrupts Disabled\n");

    print_accel_status(HOG,name);
}

void hog(unsigned int width,u32 input_address,int specs0[4],int specs1[4],int specs2[4],int specs3[4],int specs4[4],int specs5[4],int specs6[4],int specs7[4]){

	// Set each accelerator's initial parameters
	XHog_Write_specs_Words(HOG0,0,specs0,3);
 	XHog_Set_image0(HOG0,(u32)input_address);
	XHog_Write_specs_Words(HOG1,0,specs1,3);
 	XHog_Set_image0(HOG1,(u32)(input_address+width*16));
	XHog_Write_specs_Words(HOG2,0,specs2,3);
 	XHog_Set_image0(HOG2,(u32)(input_address+width*33));
	XHog_Write_specs_Words(HOG3,0,specs3,3);
 	XHog_Set_image0(HOG3,(u32)(input_address+width*50));
	XHog_Write_specs_Words(HOG4,0,specs4,3);
 	XHog_Set_image0(HOG4,(u32)input_address+width*67);
	XHog_Write_specs_Words(HOG5,0,specs5,3);
 	XHog_Set_image0(HOG5,(u32)(input_address+width*84));
	XHog_Write_specs_Words(HOG6,0,specs6,3);
 	XHog_Set_image0(HOG6,(u32)(input_address+width*101));
	XHog_Write_specs_Words(HOG7,0,specs7,3);
 	XHog_Set_image0(HOG7,(u32)(input_address+width*118));

 	// Start all the accelerators and wait until they are done
	XHog_Start(HOG0);
	XHog_Start(HOG1);
	XHog_Start(HOG2);
	XHog_Start(HOG3);
	XHog_Start(HOG4);
	XHog_Start(HOG5);
	XHog_Start(HOG6);
	XHog_Start(HOG7);
	while(	!XHog_IsDone(HOG0) && !XHog_IsDone(HOG1) && !XHog_IsDone(HOG2) && !XHog_IsDone(HOG3)
		&& 	!XHog_IsDone(HOG4) && !XHog_IsDone(HOG5) && !XHog_IsDone(HOG6) && !XHog_IsDone(HOG7)){
	}
	// Read each accelerator's output
	XHog_Read_specs_Words(HOG0,3,&specs0[3],1);
	XHog_Read_specs_Words(HOG1,3,&specs1[3],1);
	XHog_Read_specs_Words(HOG2,3,&specs2[3],1);
	XHog_Read_specs_Words(HOG3,3,&specs3[3],1);
	XHog_Read_specs_Words(HOG4,3,&specs4[3],1);
	XHog_Read_specs_Words(HOG5,3,&specs5[3],1);
	XHog_Read_specs_Words(HOG6,3,&specs6[3],1);
	XHog_Read_specs_Words(HOG7,3,&specs7[3],1);
}

float *assignToPhysicalFloat(unsigned long address,unsigned int size){
	int devmem = open("/dev/mem", O_RDWR | O_SYNC);
	off_t PageOffset = (off_t) address % getpagesize();
	off_t PageAddress = (off_t) (address - PageOffset);
	return (float *) mmap(0, size*sizeof(float), PROT_READ|PROT_WRITE, MAP_SHARED, devmem, PageAddress);
}

unsigned char *assignToPhysicalUChar(unsigned long address,unsigned int size){
	int devmem = open("/dev/mem", O_RDWR | O_SYNC);
	off_t PageOffset = (off_t) address % getpagesize();
	off_t PageAddress = (off_t) (address - PageOffset);
	return (unsigned char *) mmap(0, size*sizeof(unsigned char), PROT_READ|PROT_WRITE, MAP_SHARED, devmem, PageAddress);
}

unsigned int *assignToPhysicalUInt(unsigned long address,unsigned int size){
	int devmem = open("/dev/mem", O_RDWR | O_SYNC);
	off_t PageOffset = (off_t) address % getpagesize();
	off_t PageAddress = (off_t) (address - PageOffset);
	return (unsigned int *) mmap(0, size*sizeof(unsigned int), PROT_READ|PROT_WRITE, MAP_SHARED, devmem, PageAddress);
}

int *assignToPhysicalInt(unsigned long address,unsigned int size){
	int devmem = open("/dev/mem", O_RDWR | O_SYNC);
	off_t PageOffset = (off_t) address % getpagesize();
	off_t PageAddress = (off_t) (address - PageOffset);
	return (int *) mmap(0, size*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, devmem, PageAddress);
}

void computeHog(int i,double scale,int width, int height,svmpoints *found,u32 input_address){

	int result;
 	unsigned int x, y;
	unsigned int  numWindowsX = ((width - 64) / WINDOWSTRIDEX) + 1;
    unsigned int  numWindowsY = ((height -128) / WINDOWSTRIDEY) + 1;
    int res[8];
    int threshold = 50*1024;

    int specs0[4],specs1[4],specs2[4],specs3[4],specs4[4],specs5[4],specs6[4],specs7[4];
	for (y = 1; y < numWindowsY-1; y++) {
		for (x = 0; x < numWindowsX; x++) {

			/* specs[0] = the upper left corner of the detection window
			 * specs[1] = image width
			 * specs[2] = accelerator ID
			 * specs[3] = 1/8 of the final classification score
			*/
			specs0[0] = y*WINDOWSTRIDEY*width+x*WINDOWSTRIDEX;	specs1[0] = y*WINDOWSTRIDEY*width+x*WINDOWSTRIDEX;	specs2[0] = y*WINDOWSTRIDEY*width+x*WINDOWSTRIDEX;	specs3[0] = y*WINDOWSTRIDEY*width+x*WINDOWSTRIDEX;
			specs0[1] = width;				specs1[1] = width;				specs2[1] = width;				specs3[1] = width;	
			specs0[2] = 0;					specs1[2] = 1;					specs2[2] = 2;					specs3[2] = 3;
			
			specs4[0] = y*WINDOWSTRIDEY*width+x*WINDOWSTRIDEX;	specs5[0] = y*WINDOWSTRIDEY*width+x*WINDOWSTRIDEX;	specs6[0] = y*WINDOWSTRIDEY*width+x*WINDOWSTRIDEX;	specs7[0] = y*WINDOWSTRIDEY*width+x*WINDOWSTRIDEX;
			specs4[1] = width;				specs5[1] = width;				specs6[1] = width;				specs7[1] = width;	
			specs4[2] = 4;					specs5[2] = 5;					specs6[2] = 6;					specs7[2] = 7;
		
			hog(width,input_address,specs0,specs1,specs2,specs3,specs4,specs5,specs6,specs7);
			result=specs0[3]+specs1[3]+specs2[3]+specs3[3]+specs4[3]+specs5[3]+specs6[3]+specs7[3];

			if(result>threshold){
				addNode(x,y,scale,result/1024,i,found);
			}

		}
	}
}

void HOG(Mat image, unsigned char *imageBuffer0,unsigned char *imageBuffer1,svmpoints *found){

	double scale = START_SCALE;
	double scale_rate = SCALE_RATE;
	
	memcpy(imageBuffer0,image.data,image.rows*image.cols);

	for (int i=0;i<ITERS;i++){
		 #pragma omp parallel
		{
	        #pragma omp sections
	        {   
	        	// Double buffering with OpenMP
	        	// One core prepares the next scale of the frame
	        	// The other controls and monitors the HOG accelerators
	            #pragma omp section
	            {
					if(i%2 == 0)
						computeHog(i,scale,image.cols,image.rows,found,IMAGEBUFFER0);
					else 
						computeHog(i,scale,image.cols,image.rows,found,IMAGEBUFFER1);
	        	}
				#pragma omp section
				{
					if(i%2 == 0){
						resize(image,image,Size((int)image.cols*SCALE_RATE,(int)image.rows*SCALE_RATE),0.0,0.0,INTER_LINEAR);
						memcpy(imageBuffer1,image.data,image.rows*image.cols);
					}
					else {
						resize(image,image,Size((int)image.cols*SCALE_RATE,(int)image.rows*SCALE_RATE),0.0,0.0,INTER_LINEAR);
						memcpy(imageBuffer0,image.data,image.rows*image.cols);
					}
	        	}
	        }
        }    
		
        scale = scale*scale_rate;
	}	
}

void initDisplay(){
	/* Configure display. */
	cout << "Configuring display..." << endl;
	{
		/* Modifications need to be done to the AXI VDMA's configuration table. */
		extern XAxiVdma_Config XAxiVdma_ConfigTable[];
		XAxiVdma_ConfigTable[ VDMA_DEVICE_ID ].BaseAddress = vdma_mmap_obj.getvaddr();

		/* There needs to be pointers that point to each of the frames. Since the display
		driver needs to configure the VDMA with physical pointers, both the virtual and
		physical addresses are needed. */
		for ( int each_frame = 0; each_frame < DISPLAY_NUM_FRAMES; each_frame++ )
		{
			vir_framebuff_ptr[ each_frame ] = framebuff_arr[ each_frame ];
			phy_framebuff_ptr[ each_frame ] = reinterpret_cast<uint32_t*>( VDMA_FRAME_BUFF_ADDR +
					( each_frame * DISPLAY_MAX_FRAME * sizeof( uint32_t ) ) );
		}

		/* Configure the display driver. */
		int Status;
		Status = DisplayInitialize( &display_obj,
				VDMA_DEVICE_ID,
				dispctrl_mmap_obj.getvaddr(),
				DISPLAY_IS_HDMI,
				vir_framebuff_ptr, phy_framebuff_ptr,
				DISPLAY_STRIDE );
		if ( Status!= XST_SUCCESS )
			throw runtime_error( "The display driver wasn't properly configured." );

		/* Set the resolution. */
		DisplaySetMode( &display_obj, &VMODE_1280x720 );

		/* Start display. */
		Status = DisplayStart( &display_obj );
		if ( Status!= XST_SUCCESS )
			throw runtime_error( "The display could not be started." );
	}

	/* Configure opencv frames with framebuffer. */
	cout << "Configuring framebuffer with opencv Mats..." << endl;
	{
		const int sizes[] =
		{
				static_cast<const int>( display_obj.vMode.height ),
				static_cast<const int>( display_obj.vMode.width )
		};
		const size_t steps[] = { DISPLAY_STRIDE };
		for ( int each_frame = 0; each_frame < DISPLAY_NUM_FRAMES; each_frame++ )
		{
			framebuff_mat[ each_frame ] = Mat(
					2, sizes,
					CV_8UC(4),
					reinterpret_cast<void*>( display_obj.vframePtr[ each_frame ] ),
					steps );
			framebuff_mat[ each_frame ] = Scalar( 0, 0, 0 );
		}
	}
}

int main(int argc, char* argv[]) {

    omp_set_num_threads(2);                                                 

    // Set as input source the webcam 
	VideoCapture capture(0);

	if( !capture.isOpened() )
	throw "Camera not found";
	
	//Initialize hardware
	initDisplay();
	hog_init(HOG0,"hog0");
	hog_init(HOG1,"hog1");
	hog_init(HOG2,"hog2");
	hog_init(HOG3,"hog3");
	hog_init(HOG4,"hog4");
	hog_init(HOG5,"hog5");
	hog_init(HOG6,"hog6");
	hog_init(HOG7,"hog7");

	Mat cameraImage;
	Mat gray;
	Mat grayWithBorder;
	Scalar value = Scalar(0,0,0);
	svmpoints *found = (svmpoints*)malloc(sizeof(svmpoints));
	unsigned char *imageBuffer0;
	unsigned char *imageBuffer1;

	// Map image buffers to physical memory
	imageBuffer0 = assignToPhysicalUChar(IMAGEBUFFER0,656*496);
	imageBuffer1 = assignToPhysicalUChar(IMAGEBUFFER1,656*496);

	struct timeval start, end;

	int from_to[] = { 0,0, 1,1, 2,2, };

	for(;;){
		gettimeofday(&start, NULL);

		//Frame grab
		capture >> cameraImage;

		// RGB -> Grayscale
		cvtColor(cameraImage, gray, CV_BGR2GRAY);
		// Pad image
		copyMakeBorder( gray, grayWithBorder, 2, 14, 8, 8, BORDER_CONSTANT, value );
		// Initial image scale 
		resize(grayWithBorder,grayWithBorder,Size((int)656*START_SCALE,(int)496*START_SCALE),0.0,0.0,INTER_LINEAR);

		found->next = NULL;
		found->prev = NULL;

		// Execute HOG
		HOG(grayWithBorder,imageBuffer0,imageBuffer1,found);
		printf("Hog done\n");
		if(found->next != NULL) {
			//NMS
			reduceOverlap(found,0.7);
			draw(found,cameraImage);
		}

		// Use 4 color channels for the display controller
		Mat mixed_image( cameraImage.size(), CV_MAKE_TYPE( cameraImage.type(), 4 ) );
		mixChannels( &cameraImage, 1, &mixed_image, 1, from_to, 3 );
		Mat roi = framebuff_mat[ 0 ]( Rect( 320, 120, 640, 480 ) );
		mixed_image.copyTo( roi );

 		gettimeofday(&end, NULL);
 		
		cout << "fps:\t" 	<< 1000/(((end.tv_sec  - start.tv_sec) * 1000000u + 
         							end.tv_usec - start.tv_usec) / 1.e3) << endl;
	}
		
	return 0;
}
