#include "MontageImage.h"

#include <cstdlib>
#include <numeric>
#include <assert.h>

template <class T>
inline std::string to_string (const T& t)
{
	std::stringstream ss;
    ss << t;
    return ss.str();
}
/* Usage Example:

//add for std::string operation
#include <sstream>

using namespace std;

				// variable types:
				// int imageSampling;
				// string image_name;

				image_name.clear();
				image_name = prefix1 + to_string(imageSamplingCtr) + img_type;
				cvSaveImage(image_name.c_str(), result);
*/

unsigned int GetRandomIndex(std::vector<double>& weights)
{
    assert(weights.size() > 0);
    // v: a random value in [0, 1]
    // rand() in [0, RAND_MAX]
    double v = static_cast<double>(rand()) / RAND_MAX;
    //printOut << "v: " << v << "\n";
    
    double totalWeights = std::accumulate(weights.begin(), weights.end(), 0.0);
    v *= totalWeights;  // scaled v
    //printOut << "v (scaled): " << v << "\n";

    // no randomization, just peak the idx with highest weight
    // this is to verify the weights and to compare with BestMatchNoRandom case
    // And this is also a good way to evaluate the quality of color matching before alpha blending
    // in general there are 2 steps to tune/optimize
    // 1. Color matching: before alpha blending, no randomization, see how close the image is
    // 2. Randomization: before alpha blending, with randomization, see how close the image is
    //    randomization is a trade-off between selectivity and randomness
    //    - we don't like the mosaic looks too random
    //      (can't see the profile shape/color before alpha blending)
    //    - we don't like the mosaic looks too selective
    //      (same images repeat several times in a similar or neighboring area)
    //return std::max_element(weights.begin(), weights.end()) - weights.begin();

    double currSum = 0;
    for (unsigned int i=0; i<weights.size(); i++)
    {
        currSum += weights[i];
        //printOut << "i=" << i << ", currSum=" << currSum << ", weights[i]=" << weights[i] << "\n";
        if (v <= currSum)
        {
            //printOut << "Hit! i=" << i << "\n";
            return i;
        }
    }
    return weights.size()-1; // shouldn't get here
}

MontageImage::MontageImage()
{
	mSubImgCounter = -1;
	mHasNoMoreFile = false;
	mLoadProfImg = mLoadSubImg = mOutImg = mProfOutImg = mGridOutImg = mTempSubImg = NULL;
	isInit_GeoColorMatch = false;

	for (int i=0; i<MAX_DATABASE_NUM; i++)	imgDatabase[i] = NULL;
	endIdx_imgDB = -1;
	imgDir_status = GCM_IMG_DIR_READ_STATUS::ON_GOING;

}
MontageImage::~MontageImage()
{
	mImageFileManager.Close();
	if (!mLoadProfImg)				cvReleaseImage(&mLoadProfImg);
	if (!mLoadSubImg)				cvReleaseImage(&mLoadSubImg);
	if (!mOutImg)					cvReleaseImage(&mOutImg);
	if (!mProfOutImg)				cvReleaseImage(&mProfOutImg);
	if (!mGridOutImg)				cvReleaseImage(&mGridOutImg);
	if (!mTempSubImg)				cvReleaseImage(&mTempSubImg);
	if (!mCurSubImgStruct.imgPtr)	cvReleaseImage(&mCurSubImgStruct.imgPtr);
	if (!GCM_profImg_segment)		cvReleaseImage(&GCM_profImg_segment);

	for (int k=0; k<=endIdx_imgDB; k++ ) //already including front decision {if (endIdx_imgDB > 0)}
		if (imgDatabase[k]) delete imgDatabase[k];
	//delete [] imgDatabase; //you cannot delete a array declared as imgDatabase[NUM]

	//////////release temp memory
	////////cvReleaseImage(&mask);
	////////cvReleaseImage(&hue);
	////////cvReleaseImage(&sat);
	////////cvReleaseImage(&val);
	////////cvReleaseImage(&hsv);
	////////cvReleaseImage(&rszSubImg);
	////////cvReleaseHist( &hist );
}

void MontageImage::subImages_SaveAs_norTempJPGImage_inCreatedTempDir(
        std::string subImgOrigDir,
        std::string tempDirName,
        int subImgLongLen,
        int subImgShortLen)
{
	std::string star_dot_format = "*.JPG"; 
	std::string subImgOrigDirLink;
	//subImgOrigDirLink.clear();
	subImgOrigDirLink = std::string(subImgOrigDir);// + star_dot_format;
	if (!mImageFileManager.FindFile(subImgOrigDirLink, {mFileExt.begin(), mFileExt.end()})) return;

	std::string loadSubImgName, saveSubImgName;
	std::string dash = "/";
	loadSubImgName.clear();
	saveSubImgName.clear();

	IplImage *loadSubImg;
	IplImage *tempSubImgW = cvCreateImage(cvSize(subImgLongLen, subImgShortLen), 8, 3);
	IplImage *tempSubImgH = cvCreateImage(cvSize(subImgShortLen, subImgLongLen), 8, 3);
	while (mImageFileManager.FindNextFile())
	{
		loadSubImgName = std::string(subImgOrigDir) + mImageFileManager.GetFileName();

		#ifdef PRINT_DEBUG_MSG
		printOut << loadSubImgName.c_str(); printOut << "\n";
		#endif

		loadSubImg = cvLoadImage(loadSubImgName.c_str());

		saveSubImgName = std::string(tempDirName) + dash + mImageFileManager.GetFileName();

		if (loadSubImg->width > loadSubImg->height) 
		{
			cvResize(loadSubImg, tempSubImgW);
			cvSaveImage(saveSubImgName.c_str(), tempSubImgW);
		}
		else 
		{
			cvResize(loadSubImg, tempSubImgH);
			cvSaveImage(saveSubImgName.c_str(), tempSubImgH);
		}
		cvReleaseImage(&loadSubImg);

	}

	// now, you got last file

	mImageFileManager.Close();
}

MontageImage::INIT_IO_RETURN MontageImage::initIOBasis(
        IMG_CONF_MODE conf_mode,
        CvSize subImgSize,
        CvSize subGridSize,
        CvSize outImgSize,
        std::string subImgFileDir,
        std::string profileImgFileName,
        std::string outImgFileName)
{
	mConf_mode = conf_mode;
	mSubImgSize = subImgSize;
	mSubGridSize = subGridSize;
	mOutImgSize = outImgSize;
	mSubImgFileDir = subImgFileDir;
	mProfImgFileName = profileImgFileName;
	mOutImgFileName = outImgFileName;


	if ( (mSubImgSize.width < IMG_BASE_WIDTH) || (mSubImgSize.height < IMG_BASE_HEIGHT) ||
		 (mOutImgSize.width < IMG_BASE_WIDTH) || (mOutImgSize.height < IMG_BASE_HEIGHT) ||
		 (mSubImgSize.width > OUT_MAX_WIDTH) || (mSubImgSize.height > OUT_MAX_HEIGHT) ||
		 (mOutImgSize.width > OUT_MAX_WIDTH) || (mOutImgSize.height > OUT_MAX_HEIGHT) ||
		 (mSubImgSize.width*mSubGridSize.width > OUT_MAX_WIDTH) || (mSubImgSize.height*mSubGridSize.height > OUT_MAX_HEIGHT) )
		 return INIT_IO_RETURN::ERROR_IMG_SIZE;

	mOutImgSize.width = mSubImgSize.width*mSubGridSize.width;
	mOutImgSize.height = mSubImgSize.height*mSubGridSize.height;	



	std::string star_dot_JPG = "*.JPG"; 
	//mSubImgFileLink.clear();
	mSubImgFileLink = std::string(mSubImgFileDir);// + star_dot_JPG;
	if (!mImageFileManager.FindFile(/*mSubImgFileDir*/mSubImgFileLink, {mFileExt.begin(), mFileExt.end()}))
	{

		#ifdef PRINT_DEBUG_MSG
		printOut << "You have no " << mSubImgFileLink << " file.\n";
		#endif

		//system("pause");
		return INIT_IO_RETURN::ERROR_SUBIMG_FILE_DIR;
	}
	// now you have gaurantee that "at least one image"


	mImgNameMatrix = new std::string*[mSubGridSize.width];
	for (int i=0; i<mSubGridSize.width; i++)
	{
		mImgNameMatrix[i] = new std::string[mSubGridSize.height];
		for (int j=0; j<mSubGridSize.height; j++)
		{
			mImgNameMatrix[i][j].clear();
		}
	}


	if ((mLoadProfImg = cvLoadImage(mProfImgFileName.c_str(), 1)) == NULL)
	{
		#ifdef PRINT_DEBUG_MSG
		printOut << "cannot load " << mProfImgFileName << "\n";
		#endif

		return INIT_IO_RETURN::ERROR_IMG_FILE_NAME;
	}
    printOut << "loaded prof image successfully\n";
	mOutImg =			cvCreateImage(mOutImgSize, 8, 3);
	mProfOutImg =		cvCreateImage(mOutImgSize, 8, 3);
	mGridOutImg =		cvCreateImage(mOutImgSize, 8, 3);
	mTempSubImg =				cvCreateImage(mSubImgSize, 8, 3);
	mCurSubImgStruct.imgPtr =	cvCreateImage(mSubImgSize, 8, 3);
	GCM_profImg_segment =		cvCreateImage(mSubImgSize, 8, 3);
	
	cvZero(mOutImg);
	cvZero(mProfOutImg);
	cvZero(mGridOutImg);

	cvResize(mLoadProfImg, mProfOutImg);


	return INIT_IO_RETURN::RETURN_NO_ERROR;
}
IplImage* MontageImage::loadSubImg_base()
{
	if (imgDir_status == GCM_IMG_DIR_READ_STATUS::REACH_END) imgDir_status = GCM_IMG_DIR_READ_STATUS::ALREADY_WRAP_AROUND;

	bool hasMoreFile;
	if ( (hasMoreFile = mImageFileManager.FindNextFile()) )
	{
		//++mSubImgCounter;

		#ifdef PRINT_DEBUG_MSG
		printOut << "More files...\n";
		#endif


		mCurSubImgName = std::string(mSubImgFileDir) + mImageFileManager.GetFileName();

		#ifdef PRINT_DEBUG_MSG
		printOut << mCurSubImgName.c_str(); printOut << "\n";
		#endif

		mLoadSubImg = cvLoadImage(mCurSubImgName.c_str());
		
		
		if (mLoadSubImg->width <= mLoadSubImg->height) 
		{
			//free current sub temp memory
			cvReleaseImage(&mLoadSubImg);
			
			return loadSubImg_base();
		}
		else //only for IMG_CONF_MODE::VTL_PROF_HZL_SUB
		{ 
			++mSubImgCounter;
			return mLoadSubImg;
		}
	}
	else
	{
		/*
		// change to "wrap around" version
		if (mHasNoMoreFile) 
		{
			cout << "No more files...\n";
			return NULL;
		}
		*/

		// now, you got last file
		//++mSubImgCounter;

		#ifdef PRINT_DEBUG_MSG
		printOut << "No more files to read...\n";
		#endif

    #if 0
		mCurSubImgName = std::string(mSubImgFileDir) + mImageFileManager.GetFileName();

		#ifdef PRINT_DEBUG_MSG
		printOut <<  mCurSubImgName.c_str(); printOut << "\n";
		#endif

		mLoadSubImg = cvLoadImage(mCurSubImgName.c_str());
    #endif

		// change to "wrap around" version
		/*mHasNoMoreFile = true;*/
		imgDir_status = GCM_IMG_DIR_READ_STATUS::REACH_END;

		#ifdef PRINT_DEBUG_MSG
		printOut << "Repeat from the first loaded file (wrap around)...\n";
		#endif

		mImageFileManager.Close();
        //already checked the return value of FindFile() in initIOBasis()
		mImageFileManager.FindFile(mSubImgFileLink, {mFileExt.begin(), mFileExt.end()});


    #if 0		
		if (mLoadSubImg->width <= mLoadSubImg->height)
		{
			//free current sub temp memory
			cvReleaseImage(&mLoadSubImg);
			return loadSubImg_base();
		}
		else //only for IMG_CONF_MODE::VTL_PROF_HZL_SUB
		{ 
			++mSubImgCounter;
			return mLoadSubImg;
		}
    #endif
        return loadSubImg_base();
	}

}
IplImage* MontageImage::loadSubImage(SUB_IMG_ARRANGE_ORDER order)
{
	if (order == SUB_IMG_ARRANGE_ORDER::DEFAULT_SYSTEM_READING)
	{
		return loadSubImg_base();
	}
	else if (order == SUB_IMG_ARRANGE_ORDER::SEMI_RANDOM)
	{
		srand(time(NULL));
		int randNum = (rand() % 10)+1;

		int load_cnt = 1;
		while (load_cnt < randNum)
		{
			loadSubImg_base();
			cvReleaseImage(&mLoadSubImg);
			--mSubImgCounter;

			++load_cnt;
		}
		return loadSubImg_base();
		
	}
	else if (order == SUB_IMG_ARRANGE_ORDER::GEO_COLOR_MATCH)
	{
		init_geoColorMatch();
		mSubImgCounter++;

		int gridX = mSubImgCounter/(mSubGridSize.height);
		int gridY = mSubImgCounter%(mSubGridSize.height);
		if ( (gridX >= mSubGridSize.width) || (gridY >= mSubGridSize.height) ) 
		{

			#ifdef PRINT_DEBUG_MSG
			printOut << "GEO_COLOR_MATCH: reach grid limits \n";
			#endif

			return NULL;
		}

		cvZero(GCM_profImg_segment);
		int outImgBasePosY = gridY*mSubImgSize.height;
		int outImgBasePosX = gridX*mSubImgSize.width;
		for (int g=0; g<mSubImgSize.width; g++)
		{
			for (int n=0; n<mSubImgSize.height; n++)
			{
				cvSet2D(GCM_profImg_segment, n, g, cvGet2D(mProfOutImg, outImgBasePosY+n, outImgBasePosX+g));	
			}
		}

		// start to extract geo-color information

		#ifdef PRINT_DEBUG_MSG
		printOut << "start extract_GCM_subImgInfo of GCM_profImg_segment\n";
		#endif

		GCM_subImgInfo curGCM_ofProfImg;
		extract_GCM_subImgInfo(GCM_profImg_segment, &curGCM_ofProfImg);

		//double minDist = GCM_HIST_BIN_NUM*4; // a value which is impossible to reach
		//double curDist;
		int best_subImg_idx = MAX_DATABASE_NUM+1; // a index which is impossible to reach

		#ifdef PRINT_DEBUG_MSG
		printOut << "start compare_GCM_subImgInfo within all sub image in databases\n";
		printOut << "endIdx_imgDB = " << endIdx_imgDB << "\n";
		#endif

        std::vector<double> weights(endIdx_imgDB+1, 0.0);
		for (int i=0; i<=endIdx_imgDB; i++)
		{

			#ifdef PRINT_DEBUG_MSG
			//printOut << "i=" << i << " ";
			#endif

#if 0 // Use an weighted random method for best idx selection instead
			if ((curDist = compare_GCM_subImgInfo(&curGCM_ofProfImg, imgDatabase[i])) <= minDist)
			{
				minDist = curDist;
				best_subImg_idx = i;
			}
#endif
            double dist = compare_GCM_subImgInfo(&curGCM_ofProfImg, imgDatabase[i]);
            dist *= (30); // normalized/scaled by some proper number by tuning
            auto weight = exp(-dist);
            //printOut << "i=" << i << ", weight=" << weight << "\n";
            weights[i] = weight;
		}

        // draw a random index based on weights
        best_subImg_idx = GetRandomIndex(weights);

		#ifdef PRINT_DEBUG_MSG
		printOut << "weight = " << weights[best_subImg_idx] << ", best_subImg_idx = " << best_subImg_idx << "\n";
		#endif

		return mLoadSubImg = cvLoadImage(imgDatabase[best_subImg_idx]->imgFileName.c_str());
	}
}

double MontageImage::compare_GCM_subImgInfo(GCM_subImgInfo* gcm_info1, GCM_subImgInfo* gcm_info2)
{
	double distSum = 0;
	double dist;
	for (int i=0; i<4; i++)
	{
#if 0
		dist = gcm_info1->hueValues[i] - gcm_info2->hueValues[i];
		dist = (dist>=0) ? dist : -dist;
		distSum += ((dist>GCM_HIST_BIN_NUM/2) ? GCM_HIST_BIN_NUM - dist : dist);
#endif
        // dist in range [0, 1]
        dist = cvCompareHist(gcm_info1->hists[i], gcm_info2->hists[i], CV_COMP_BHATTACHARYYA);
        distSum += dist;
    }
	return distSum/4;  // average dist in range [0, 1]
}

void MontageImage::init_geoColorMatch(void)
{
	if (!isInit_GeoColorMatch) //first time enter this function
	{

		#ifdef PRINT_DEBUG_MSG
		printOut << "start init_geoColorMatch\n";
		#endif

		//construct the database for geo color matching
		IplImage* temp_ptr = 0;
		
		endIdx_imgDB = -1;
		while(temp_ptr = loadSubImg_base())
		{
			if ((imgDir_status == GCM_IMG_DIR_READ_STATUS::REACH_END) || (imgDir_status == GCM_IMG_DIR_READ_STATUS::ON_GOING))
			{
				++endIdx_imgDB; // create index for this round, 
				// since you won't know whether there's next round or not!!!
				// the following is the reason
				// possible case#1: ON_GOING -> REACH_END -> ALREADY_WRAP_AROUND
				// possible case#2: ON_GOING -> ALREADY_WRAP_AROUND (if the last image file is not OK and didn't pass out!!)
				imgDatabase[endIdx_imgDB] = new GCM_subImgInfo;
				imgDatabase[endIdx_imgDB]->imgFileName = mCurSubImgName;
				// now, start to extract geo-color information
				extract_GCM_subImgInfo(temp_ptr, imgDatabase[endIdx_imgDB]);
				cvReleaseImage(&temp_ptr);
			}
			else // imgDir_status == GCM_IMG_DIR_READ_STATUS::ALREADY_WRAP_AROUND, run out of image in directory
			{
				cvReleaseImage(&temp_ptr);
				break; //release load image before leaving while loop
			}
			
			if (endIdx_imgDB >= MAX_DATABASE_NUM) 
			{
				break; //release load image before leaving while loop
			}
		}

		isInit_GeoColorMatch = true;
		mSubImgCounter = -1; // same as constructor (initial value)

	}
}

void MontageImage::extract_GCM_subImgInfo(IplImage* inImg, GCM_subImgInfo* gcm_info)
{
	static IplImage* mask = cvCreateImage(mSubImgSize, 8, 1 ); //only for IMG_CONF_MODE::VTL_PROF_HZL_SUB
	static IplImage* hue = cvCreateImage(mSubImgSize,8,1);
	static IplImage* sat = cvCreateImage(mSubImgSize,8,1);
	static IplImage* val = cvCreateImage(mSubImgSize,8,1);
	static IplImage* hsv = cvCreateImage(mSubImgSize,8,3);
	static IplImage* rszSubImg = cvCreateImage(mSubImgSize,8,3);

	int hdims[] = {GCM_HIST_H_BIN_NUM, GCM_HIST_S_BIN_NUM};
	float hranges_arr[] = {0, 180};
    float sranges_arr[] = {0, 256};
	float* ranges[] = {hranges_arr, sranges_arr};
	static CvHistogram *hist = cvCreateHist(2, hdims, CV_HIST_ARRAY, ranges, 1 );
	int max_idx = -1;
	float max_val = 0;

	cvResize(inImg, rszSubImg);

	cvCvtColor(rszSubImg, hsv, CV_BGR2HSV );
	cvSplit(hsv, hue, sat, val, 0);
    IplImage* hs_plane[] = {hue, sat};

	// 1st quarter (hueValues[0])
	cvZero(mask);
	cvRectangle(mask,
                cvPoint(mSubImgSize.width/2*1, mSubImgSize.height/2*1),
                cvPoint(mSubImgSize.width/2*2, mSubImgSize.height/2*0),
                cvScalar(255),
                -1); 
	cvCalcHist(hs_plane, hist, 0, mask);
    cvNormalizeHist(hist, 1.0);
	//cvGetMinMaxHistValue(hist, 0, &max_val, 0, &max_idx );
	//cvConvertScale( hist->bins, hist->bins, max_val ? 255. / max_val : 0., 0 );
	//gcm_info->hueValues[0] = max_idx;
    gcm_info->hists[0] = NULL;
    cvCopyHist(hist, &(gcm_info->hists[0]));

	// 2st quarter (hueValues[1])
	cvZero(mask);
	cvRectangle(mask,
                cvPoint(mSubImgSize.width/2*1, mSubImgSize.height/2*1),
                cvPoint(mSubImgSize.width/2*0, mSubImgSize.height/2*0),
                cvScalar(255),
                -1); 
	cvCalcHist(hs_plane, hist, 0, mask);
    cvNormalizeHist(hist, 1.0);
	//cvGetMinMaxHistValue( hist, 0, &max_val, 0, &max_idx );
	//cvConvertScale( hist->bins, hist->bins, max_val ? 255. / max_val : 0., 0 );
	//gcm_info->hueValues[1] = max_idx;
    gcm_info->hists[1] = NULL;
    cvCopyHist(hist, &(gcm_info->hists[1]));

	// 3st quarter (hueValues[2])
	cvZero(mask);
	cvRectangle(mask,
                cvPoint(mSubImgSize.width/2*1, mSubImgSize.height/2*1),
                cvPoint(mSubImgSize.width/2*0, mSubImgSize.height/2*2),
                cvScalar(255),
                -1); 
	cvCalcHist(hs_plane, hist, 0, mask);
    cvNormalizeHist(hist, 1.0);
	//cvGetMinMaxHistValue( hist, 0, &max_val, 0, &max_idx );
	//cvConvertScale( hist->bins, hist->bins, max_val ? 255. / max_val : 0., 0 );
	//gcm_info->hueValues[2] = max_idx;
    gcm_info->hists[2] = NULL;
    cvCopyHist(hist, &(gcm_info->hists[2]));

	// 4st quarter (hueValues[3])
	cvZero(mask);
	cvRectangle(mask,
                cvPoint(mSubImgSize.width/2*1, mSubImgSize.height/2*1),
                cvPoint(mSubImgSize.width/2*2, mSubImgSize.height/2*2),
                cvScalar(255),
                -1); 
	cvCalcHist(hs_plane, hist, 0, mask);
    cvNormalizeHist(hist, 1.0);
	//cvGetMinMaxHistValue( hist, 0, &max_val, 0, &max_idx );
	//cvConvertScale( hist->bins, hist->bins, max_val ? 255. / max_val : 0., 0 );
	//gcm_info->hueValues[3] = max_idx;
    gcm_info->hists[3] = NULL;
    cvCopyHist(hist, &(gcm_info->hists[3]));
}


//will be resized to subImgSize
void MontageImage::setCurSubImg(int gridX, int gridY, IplImage *loadImgPtr, std::string loadImgName, bool isManualMode)
{
	if (isManualMode)
	{	
		mCurSubImgStruct.gridX = gridX;
		mCurSubImgStruct.gridY = gridY;
		mCurSubImgStruct.subImgName = loadImgName;
		cvResize(loadImgPtr, mCurSubImgStruct.imgPtr);
	}
	else //only use internal variables
	{
		int curCnt = mSubImgCounter;
		mCurSubImgStruct.gridX = curCnt/(mSubGridSize.height);
		mCurSubImgStruct.gridY = curCnt%(mSubGridSize.height);
		mCurSubImgStruct.subImgName = mCurSubImgName;
		cvResize(mLoadSubImg, mCurSubImgStruct.imgPtr);
	}

}
bool MontageImage::fillCurSubImg() // with current mCurSubImgStruct, to mSubGridOutImg
{
	if ( (mCurSubImgStruct.gridX >= mSubGridSize.width) || (mCurSubImgStruct.gridY >= mSubGridSize.height) ) return false;

	mImgNameMatrix[mSubImgCounter/(mSubGridSize.height)][mSubImgCounter%(mSubGridSize.height)] = mCurSubImgStruct.subImgName; 

	//CvScalar subImgPixel, gridOutImgPixel;
	int outImgBasePosY = mCurSubImgStruct.gridY*mSubImgSize.height;
	int outImgBasePosX = mCurSubImgStruct.gridX*mSubImgSize.width;

	for (int g=0; g<mCurSubImgStruct.imgPtr->width; g++)
	{
		for (int n=0; n<mCurSubImgStruct.imgPtr->height; n++)
		{
			//subImgPixel = cvGet2D(mCurSubImgStruct.imgPtr, n, g);
			cvSet2D(mGridOutImg, outImgBasePosY+n, outImgBasePosX+g, cvGet2D(mCurSubImgStruct.imgPtr, n, g));	
		}
	}

	//free sub temp memory
	cvReleaseImage(&mLoadSubImg);

	
	return true;
}

//Usage:
//inImg1 = mProfOutImg
//inImg2 = mGridOutImg
//outImg = mOutImg
bool MontageImage::imageMixing(IplImage* inImg1, IplImage* inImg2, float alpha1, IplImage* outImg, bool isManualMode)
{
	if (!isManualMode) //only use internal variables
	{
		inImg1 = mProfOutImg;
		inImg2 = mGridOutImg;
		outImg = mOutImg;
	}

	//ensure dimension of in,out image are equal.
	if ((cvGetSize(inImg1).height != cvGetSize(outImg).height) || (cvGetSize(inImg1).width != cvGetSize(outImg).width)) return false;
	if ((cvGetSize(inImg2).height != cvGetSize(outImg).height) || (cvGetSize(inImg2).width != cvGetSize(outImg).width)) return false;

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
	return true;
}

void MontageImage::show_and_save_OutImg()
{
	int size_factor = 3;
	IplImage* mProfOutImg_dnSz =	cvCreateImage(cvSize(320*size_factor, 240*size_factor), 8, 3);
	IplImage* mGridOutImg_dnSz =	cvCreateImage(cvSize(320*size_factor, 240*size_factor), 8, 3);
	IplImage* mOutImg_dnSz =		cvCreateImage(cvSize(320*size_factor, 240*size_factor), 8, 3);
	
	cvResize(mProfOutImg, mProfOutImg_dnSz, 1);
	cvResize(mGridOutImg, mGridOutImg_dnSz, 1);
	cvResize(mOutImg, mOutImg_dnSz, 1);

	cvNamedWindow("mProfOutImg_dnSz",1);
	cvNamedWindow("mGridOutImg_dnSz",1);
	cvNamedWindow("mOutImg_dnSz",1);
	while (cvWaitKey(30) != 27) 
	{
		cvShowImage("mProfOutImg_dnSz", mProfOutImg_dnSz);
		cvShowImage("mGridOutImg_dnSz", mGridOutImg_dnSz);
		cvShowImage("mOutImg_dnSz", mOutImg_dnSz);

	}
	cvDestroyWindow("mProfOutImg_dnSz");
	cvDestroyWindow("mGridOutImg_dnSz");
	cvDestroyWindow("mOutImg_dnSz");

	cvReleaseImage(&mProfOutImg_dnSz);
	cvReleaseImage(&mGridOutImg_dnSz);
	cvReleaseImage(&mOutImg_dnSz);

	//save
	cvSaveImage("mProfOutImg.JPG", mProfOutImg);
	cvSaveImage("mGridOutImg.JPG", mGridOutImg);
	cvSaveImage("mOutImg.JPG", mOutImg);
}



void MontageImage::addText(std::string loadImgName_NoText,
                           std::string saveImgName_Text, 
						   int wordType,
                           std::string word2print,
                           float wordScaleNum,
                           CvScalar textColor,
                           float norTextPos_width,
                           float norTextPos_height)
{
	//------------------ this function can be executed independantly --------------------//

	#ifdef PRINT_DEBUG_MSG
	printOut << "start to add text to image\n";
	#endif


	IplImage *loadImg_NoText = cvLoadImage(loadImgName_NoText.c_str()/*"mOutImg.JPG"*/, 1);
	IplImage *img_Text = cvCreateImage(cvSize(loadImg_NoText->width, loadImg_NoText->height), 8, 3);
	cvCopy(loadImg_NoText, img_Text);
	CvFont font;

	//CV_FONT_HERSHEY_TRIPLEX
	//CV_FONT_HERSHEY_DUPLEX
	//CV_FONT_HERSHEY_PLAIN
	//cvInitFont( &font, CV_FONT_HERSHEY_TRIPLEX, 4, 4, 0, 12 );
	//cvPutText(img_Text, "*~To My Love~*", cvPoint(img_Text->width*0.23, img_Text->height*0.95), &font, CV_RGB(250,0,0));

	float testScaleNum = /*4*/wordScaleNum;
	cvInitFont(&font,
               wordType/*CV_FONT_HERSHEY_TRIPLEX*/,
               4*testScaleNum, 4*testScaleNum,
               0,
               12*testScaleNum );
	cvPutText(img_Text,
              word2print.c_str()/*"*~To My Love~*"*/,
              cvPoint(img_Text->width*norTextPos_width/*0.23*/, img_Text->height*norTextPos_height/*0.95*/),
              &font,
              textColor/*CV_RGB(250,0,0)*/);

	cvSaveImage(saveImgName_Text.c_str()/*"img_Text.JPG"*/, img_Text);

	cvReleaseImage(&loadImg_NoText);
	cvReleaseImage(&img_Text);
}
