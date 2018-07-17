#ifndef MONTAGEIMAGE_H
#define MONTAGEIMAGE_H

// C++ lib
//#include "opencv2/ml.hpp"
//#include "opencv2/core.hpp"
//#include "opencv2/imgproc.hpp"
//#include "opencv2/highgui.hpp"

// C lib
#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>
//#include <opencv/cvcam.h>



#include "self_def.h"

//add for std::string operation
#include <sstream>

//add for sub image arrangement: semi-random order 
#include <stdlib.h>
#include <time.h>
#include "self_def.h"
#include "ImageFileManager.h"

#ifdef PRINT_DEBUG_MSG
#include <fstream>
#include <iostream> //to show debug info
#endif

using namespace std;
using namespace cv;

#ifdef PRINT_DEBUG_MSG
extern fstream txtout;
#endif

class MontageImage
{
public:
	//=================== declare member variables ========================

	std::string **mImgNameMatrix; //2D matrix with element std::string
	CvSize mOutImgSize, mSubImgSize, mSubGridSize;
	ImageFileManager mImageFileManager;
	IplImage *mLoadProfImg, *mLoadSubImg; //with loaded img size
	IplImage *mOutImg, *mProfOutImg, *mGridOutImg; //with size = mOutImgSize
	IplImage *mTempSubImg; //with size = mSubImgSize
    std::string mSubImgFileDir, mProfImgFileName, mOutImgFileName;
	int mSubImgCounter;
	bool mHasNoMoreFile;
	std::string mSubImgFileLink, mCurSubImgName;


	//internal struct type
	struct SubImgStruct
	{
		int gridX;
		int gridY;
		std::string subImgName;
		IplImage* imgPtr;
	};
	enum class INIT_IO_RETURN
	{
		RETURN_NO_ERROR,
		ERROR_IMG_SIZE,
		ERROR_SUBIMG_FILE_DIR,
		ERROR_IMG_FILE_NAME
	};
	enum class IMG_CONF_MODE //now, only implement IMG_CONF_MODE::VTL_PROF_HZL_SUB
	{
		VTL_PROF_HZL_SUB,
		VTL_PROF_VTL_SUB,
		HZL_PROF_HZL_SUB,
		HZL_PROF_VTL_SUB
	};
	enum class SUB_IMG_ARRANGE_ORDER
	{
		DEFAULT_SYSTEM_READING,
		SEMI_RANDOM,
		GEO_COLOR_MATCH
	};

	SubImgStruct mCurSubImgStruct;
	IMG_CONF_MODE mConf_mode;
	
	struct GCM_subImgInfo //only for IMG_CONF_MODE::VTL_PROF_HZL_SUB
	{
		std::string imgFileName; // for load access
		double hueValues[4];
	};

	bool isInit_GeoColorMatch;
	GCM_subImgInfo* imgDatabase[MAX_DATABASE_NUM]; //only for IMG_CONF_MODE::VTL_PROF_HZL_SUB
	int endIdx_imgDB;
	
	enum class GCM_IMG_DIR_READ_STATUS
	{
		ON_GOING,
		REACH_END,
		ALREADY_WRAP_AROUND
	};
	GCM_IMG_DIR_READ_STATUS imgDir_status;

	IplImage* GCM_profImg_segment;


	//=================== declare member functions ==========================
	
	MontageImage(); //default constructor
	~MontageImage(); //destructor
	//copy constructor
	//assign operator overloading

	/*this function can be indep used with given image dir*/
    void subImages_SaveAs_norTempJPGImage_inCreatedTempDir(
            std::string subImgOrigDir,
            std::string tempDirName,
            int subImgLongLen,
            int subImgShortLen);

	INIT_IO_RETURN initIOBasis(IMG_CONF_MODE conf_mode, 
                               CvSize subImgSize,
                               CvSize subGridSize,
                               CvSize outImgSize, 
					           std::string subImgFileDir,
                               std::string profileImgFileName,
                               std::string outImgFileName);


	
	//Usage of function call: loadSubImg() -> setCurSubImg() -> fillCurSubImg()
	IplImage* loadSubImg_base();
	IplImage* loadSubImage(SUB_IMG_ARRANGE_ORDER order);
	void init_geoColorMatch(void);
	void extract_GCM_subImgInfo(IplImage* inImg, GCM_subImgInfo* gcm_info);
	double compare_GCM_subImgInfo(GCM_subImgInfo* gcm_info1, GCM_subImgInfo* gcm_info2);

	/*this function can be indep used with given img*/
    void setCurSubImg(int gridX,
                      int gridY,
                      IplImage* loadImgPtr,
                      std::string loadImgName,
                      bool isManualMode = false); //will be resized to subImgSize
	bool fillCurSubImg();

	/*this function can be indep used with given img*/
    bool imageMixing(IplImage* inImg1,
                     IplImage* inImg2,
                     float alpha1,
                     IplImage* outImg,
                     bool isManualMode = false);

	void show_and_save_OutImg();

	/*this function can be indep used with given img*/
    void addText(std::string loadImgName_NoText,
                 std::string saveImgName_Text, 
			     int wordType,
                 std::string word2print,
                 float wordScaleNum,
                 CvScalar textColor,
                 float norTextPos_width,
                 float norTextPos_height);

private:
    std::vector<std::string> mFileExt = {{".JPG"}, {".jpg"}, {".jpeg"}};

};

#endif // MONTAGEIMAGE_H
