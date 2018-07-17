#include <stdio.h>
#include <iostream>
#include <fstream>
using namespace std;
//#include <conio.h>
//#include <tchar.h>

#include <time.h>

#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>
//#include <cvcam.h>

#include "MontageImage.h"


#ifdef PRINT_DEBUG_MSG
fstream txtout;
#endif

void imageMix(IplImage* inImg1, IplImage* inImg2, float alpha1, IplImage* outImg)
{
	//ensure dimension of in,out image are equal.
	if ((cvGetSize(inImg1).height != cvGetSize(outImg).height) || (cvGetSize(inImg1).width != cvGetSize(outImg).width)) return;
	if ((cvGetSize(inImg2).height != cvGetSize(outImg).height) || (cvGetSize(inImg2).width != cvGetSize(outImg).width)) return;

	CvScalar i1, i2, out;

	for (int g=0; g<outImg->width; g++)
	{
		for (int n=0; n<outImg->height; n++)
		{
			i1 = cvGet2D(inImg1, n, g);
			i2 = cvGet2D(inImg2, n, g);
			
			for (int i=0; i<3; i++)
				out.val[i] = i1.val[i]*alpha1+i2.val[i]*(1.0-alpha1);
			out.val[3] = 0;
			
			cvSet2D(outImg, n, g, out);			
		}
	}
}

///*dirent*/
#if 0
void printBasicFileInfo(char fLink[]) // also can be "char* fLink"
{
	CFileFind finder;

	BOOL bResult = finder.FindFile(fLink);
	char* szFileToFind = fLink;
	//char szFileToFind[] = fLink; //<-- cannot convert from 'char[]' to 'char[]'

	if (bResult == 0) 
	{

		#ifdef PRINT_DEBUG_MSG
		printOut << "You have no " << szFileToFind << " file.\n";
		#endif

		system("pause");
		return;
	}

	bool hasMoreFile = 0;
	for (; (hasMoreFile = finder.FindNextFile())!=0; )
	{
		cout << "More files...\n";
		cout << "Root of " << szFileToFind;
		cout << " is " << (LPCTSTR) finder.GetRoot();
		cout << endl;
	
		cout << "Title of " << szFileToFind;
		cout << " is " << (LPCTSTR) finder.GetFileTitle();
		cout << endl;
  
		cout << "Path of " << szFileToFind;
		cout << " is " << (LPCTSTR) finder.GetFilePath();
		cout << endl;
	 
		cout << "URL of " << szFileToFind;
		cout << " is " << (LPCTSTR) finder.GetFileURL();
		cout << endl;
   
		cout << "Name of " << szFileToFind;
		cout << " is " << (LPCTSTR) finder.GetFileName();
		cout << endl;

		//finder.Close(); <-- cannot put this line here
	}

	if (hasMoreFile == 0) //actually, this line is dummy
	{
		cout << "Last file...\n";
		cout << "Root of " << szFileToFind;
		cout << " is " << (LPCTSTR) finder.GetRoot();
		cout << endl;
	
		cout << "Title of " << szFileToFind;
		cout << " is " << (LPCTSTR) finder.GetFileTitle();
		cout << endl;
  
		cout << "Path of " << szFileToFind;
		cout << " is " << (LPCTSTR) finder.GetFilePath();
		cout << endl;
	 
		cout << "URL of " << szFileToFind;
		cout << " is " << (LPCTSTR) finder.GetFileURL();
		cout << endl;
   
		cout << "Name of " << szFileToFind;
		cout << " is " << (LPCTSTR) finder.GetFileName();
		cout << endl;

		system("pause");

		//finder.Close(); <-- cannot put this line here
	}

	finder.Close();
}
#endif

int main_back (void)
{

	#ifdef PRINT_DEBUG_MSG
	printOut <<  "Test message\n";
	#endif

	IplImage* load_SubImg = cvLoadImage("C:\\Users\\idfs\\Pictures\\20120119 ¬K¸`«e®È¦æªZ³®+©yÄõ\\IMG_0296.JPG"/*"profilePhoto.JPG"*//*"rmp4_s.bmp"*/,1);
	IplImage* load_SubImg2 = cvLoadImage("rmp4_s.bmp",1);
	int size_factor = 3;
	IplImage* dnSz_SubImg = cvCreateImage(cvSize(320*size_factor, 240*size_factor), 8, 3);
	IplImage* dnSz_SubImg_ch1 = cvCreateImage(cvSize(320*size_factor, 240*size_factor), 8, 1);
	IplImage* dnSz_SubImg_ch2 = cvCreateImage(cvSize(320*size_factor, 240*size_factor), 8, 1);
	IplImage* dnSz_SubImg_ch3 = cvCreateImage(cvSize(320*size_factor, 240*size_factor), 8, 1);
	IplImage* dnSz_SubImg2 = cvCreateImage(cvSize(320*size_factor, 240*size_factor), 8, 3);
	IplImage* dnSz_SubImg2_ch1 = cvCreateImage(cvSize(320*size_factor, 240*size_factor), 8, 1);
	IplImage* dnSz_SubImg2_ch2 = cvCreateImage(cvSize(320*size_factor, 240*size_factor), 8, 1);
	IplImage* dnSz_SubImg2_ch3 = cvCreateImage(cvSize(320*size_factor, 240*size_factor), 8, 1);
	cvResize(load_SubImg, dnSz_SubImg, 1);
	cvResize(load_SubImg2, dnSz_SubImg2, 1);

	IplImage* tmpSubImg = cvCreateImage(cvGetSize(dnSz_SubImg), 8, 3);

	imageMix(dnSz_SubImg, dnSz_SubImg2, 0.2, tmpSubImg);


	cvSplit(dnSz_SubImg, dnSz_SubImg_ch1, dnSz_SubImg_ch2, dnSz_SubImg_ch3, 0);
	cvSplit(dnSz_SubImg2, dnSz_SubImg2_ch1, dnSz_SubImg2_ch2, dnSz_SubImg2_ch3, 0);

	const char* winName = "Loaded Sub Image";
	const char* winName2 = "Loaded Sub Image 2";
	const char* winNameTmp = "Tmp Sub Image";

	cvNamedWindow(winName,1);
	cvNamedWindow(winName2,1);
	cvNamedWindow(winNameTmp,1);
	while (cvWaitKey(30) != 27) 
	{
		cvShowImage(winName, dnSz_SubImg);
		cvShowImage(winName2, dnSz_SubImg2);
		cvShowImage(winNameTmp, tmpSubImg);
	}

	cvDestroyWindow(winName);
	cvDestroyWindow(winName2);
	cvDestroyWindow(winNameTmp);

	system("pause"); 
	
	///*dirent*/printBasicFileInfo("profilePhoto.JPG");	
	//printBasicFileInfo("C:\\Users\\idfs\\Pictures\\20120119 ¬K¸`«e®È¦æªZ³®+©yÄõ\\*.JPG");
		
	system("pause");
	//return 0;



	load_SubImg = cvLoadImage("crab.JPG"/*"rmp4_s.bmp"*/,1);
	size_factor = 32;
	IplImage* largeImg1 = cvCreateImage(cvSize(320*size_factor, 240*size_factor), 8, 3);
	IplImage* largeImg2 = cvCreateImage(cvSize(320*size_factor, 240*size_factor), 8, 3);
	IplImage* large_SaveImg = cvCreateImage(cvSize(320*size_factor, 240*size_factor), 8, 3);
	cvResize(load_SubImg, largeImg1, 1);
	cvSaveImage("largeImg1.JPG", largeImg1);

	//return 0;

	cvResize(load_SubImg2, largeImg2, 1);
	imageMix(largeImg1, largeImg2, 0.2, large_SaveImg);
	cvSaveImage("large_SaveImg.JPG", large_SaveImg);

	
	system("pause");
	return 0;
}


int main (void)
{

	#ifdef PRINT_DEBUG_MSG
	txtout.open("dbglog.txt", ios::out);
	#endif

	//To-do new features:
	//	4 modes: {prof = vtl, hzl} X {sub = vtl, hzl}
	//	sub image arrangement: default order of system reading, semi-random order (both with auto wrap-around mechanism)
	//	merge function: adding text


	MontageImage montage;

	MontageImage::IMG_CONF_MODE conf_mode = MontageImage::IMG_CONF_MODE::VTL_PROF_HZL_SUB;
	int subImgWidth = 320/2;
	int subImgHeight = 240/2;
	int gridX = /*6*//*9*//*12*//*18*/18*2;
	int gridY = /*10*//*16*//*20*//*32*/32*2;

	//system("md temp_dir_scaled_JPG");
	//system("dir");
	//system("pause");

	#ifdef CREATE_SUB_IMGS
	montage.subImages_SaveAs_norTempJPGImage_inCreatedTempDir(
            "/home/idfs/Documents/fatrmp4_images/",
            "small_test_image_lib/",
            subImgWidth,
            subImgHeight);
	#endif

#ifdef IMG_MIX

	if (montage.initIOBasis(conf_mode, 
                            cvSize(subImgWidth,subImgHeight),
                            cvSize(gridX,gridY),
                            cvSize(subImgWidth*gridX, subImgHeight*gridY),
							"small_test_image_lib/", 
							"/home/idfs/Documents/fatrmp4_images/profile.jpeg", 
							"result.JPG")
            != MontageImage::INIT_IO_RETURN::RETURN_NO_ERROR)
    {
        return 0;
    }

	#ifdef PRINT_DEBUG_MSG
	printOut << "initIOBasis done\n";
	#endif

	IplImage* imgPtr;
	while ((imgPtr = montage.loadSubImage(
                    MontageImage::SUB_IMG_ARRANGE_ORDER::GEO_COLOR_MATCH/*SEMI_RANDOM*//*DEFAULT_SYSTEM_READING*/))
                != NULL)
	{
		montage.setCurSubImg(0, 0, NULL, {});

		#ifdef PRINT_DEBUG_MSG
		printOut << "current setCurSubImg: gridX = "
                 << montage.mCurSubImgStruct.gridX
                 << ", gridY = "
                 << montage.mCurSubImgStruct.gridY
                 << "\n";
		#endif

		if (montage.fillCurSubImg() == false) 
		{

			#ifdef PRINT_DEBUG_MSG
			printOut << "fillCurSubImg done (1)\n";
			#endif

			break;
		}
	}

	#ifdef PRINT_DEBUG_MSG
	if (!imgPtr) printOut << "fillCurSubImg done (2)\n";
	#endif


	if (montage.imageMixing(NULL, NULL, 0.65/*0.7*/, NULL) == false) return 0;

	#ifdef PRINT_DEBUG_MSG
	printOut << "imageMixing done\n";
	#endif

	montage.show_and_save_OutImg();
#endif
	
	
	//------------------ this part of code can be executed independantly --------------------//

	#ifdef PRINT_DEBUG_MSG
	printOut << "start to add text to image\n";
	#endif

	montage.addText("mOutImg.JPG", "img_Text.JPG", CV_FONT_HERSHEY_TRIPLEX, 
		"*~To My Love~*", ((float)gridX)/9, CV_RGB(250,0,0), 0.23, 0.95);


	#ifdef PRINT_DEBUG_MSG
	printOut << "all done.\n";	
	txtout.close();
	#endif

	system("pause");
	return 0;
}
