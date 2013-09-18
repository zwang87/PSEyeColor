#include "PSEyeColor.h"

///this is a test version
CLEyeCameraCapture::CLEyeCameraCapture(LPSTR windowName, GUID cameraGUID, CLEyeCameraColorMode mode, CLEyeCameraResolution resolution, float fps) :
_cameraGUID(cameraGUID), _cam(NULL), _mode(mode), _resolution(resolution), _fps(fps), _running(false)
{
	strcpy(_windowName, windowName);
	
	pCapBuffer = NULL;
	bool _isTracking = false;
	int length = strlen(_windowName);
	strcpy(trackingWindowName, _windowName);
	strcat(trackingWindowName, " test");
}
bool CLEyeCameraCapture::StartCapture()
{
	_running = true;
	namedWindow(_windowName, 0);
	namedWindow(trackingWindowName, 0);
	namedWindow("blue", 0);
	// Start CLEye image capture thread
	_hThread = CreateThread(NULL, 0, &CLEyeCameraCapture::CaptureThread, this, 0, 0);
	if(_hThread == NULL)
	{
		//MessageBox(NULL,"Could not create capture thread","CLEyeMulticamTest", MB_ICONEXCLAMATION);
		return false;
	}
	return true;
}
void CLEyeCameraCapture::StopCapture()
{
	if(!_running)	return;
	_running = false;
	WaitForSingleObject(_hThread, 1000);
	destroyAllWindows();
}
void CLEyeCameraCapture::IncrementCameraParameter(int param)
{
	if(!_cam)	return;
	cout << "CLEyeGetCameraParameter " << CLEyeGetCameraParameter(_cam, (CLEyeCameraParameter)param) << endl;
	CLEyeSetCameraParameter(_cam, (CLEyeCameraParameter)param, CLEyeGetCameraParameter(_cam, (CLEyeCameraParameter)param)+10);
}
void CLEyeCameraCapture::DecrementCameraParameter(int param)
{
	if(!_cam)	return;
	cout << "CLEyeGetCameraParameter " << CLEyeGetCameraParameter(_cam, (CLEyeCameraParameter)param) << endl;
	CLEyeSetCameraParameter(_cam, (CLEyeCameraParameter)param, CLEyeGetCameraParameter(_cam, (CLEyeCameraParameter)param)-10);
}
void CLEyeCameraCapture::Run()
{	
	// Create camera instance
	_cam = CLEyeCreateCamera(_cameraGUID, _mode, _resolution, _fps);
	if(_cam == NULL)		return;
	// Get camera frame dimensions
	CLEyeCameraGetFrameDimensions(_cam, w, h);
	// Depending on color mode chosen, create the appropriate OpenCV image
	if(_mode == CLEYE_COLOR_PROCESSED || _mode == CLEYE_COLOR_RAW)
		pCapImage = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 4);
	else
		pCapImage = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 1);

	// Set some camera parameters
	CLEyeSetCameraParameter(_cam, CLEYE_GAIN, 5);
	CLEyeSetCameraParameter(_cam, CLEYE_EXPOSURE, 200);
	//CLEyeSetCameraParameter(_cam, CLEYE_AUTO_GAIN, true);
	//CLEyeSetCameraParameter(_cam, CLEYE_AUTO_EXPOSURE, true);
	//CLEyeSetCameraParameter( _cam, CLEYE_HFLIP, true);

	// Start capturing
	CLEyeCameraStart(_cam);
	cvGetImageRawData(pCapImage, &pCapBuffer);
	pCapture = pCapImage;
	long frames = 0;
	long count = GetTickCount();
	long prevCount = 0;
	double fps = 0;
	// image capturing loop
	Mat src_blur, thresholdImage, src_hsv;

	//int radius = 0;
	//int counter = 0;
	char* fps_text = new char[5];
	//vector<Mat> chan;
	char* temp = new char[2];
	vector<vector<Point>> contours;

	while(_running)
	{
		CLEyeCameraGetFrame(_cam, pCapBuffer);
		
		frames++;
		inRange(pCapture, Scalar(0, 0, 240), Scalar(255, 255, 255), thresholdImage);
		findContours(thresholdImage, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

		int contourSize = contours.size();
		
		vector<vector<Point> > contours_poly( contourSize );
		vector<Point2f>center( contourSize);
		vector<float>radius( contourSize);
		
		int* sorted = new int[4];

		for( int i = 0; i < contourSize; i++ )
		{ 
			approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true );
			minEnclosingCircle( (Mat)contours_poly[i], center[i], radius[i] );
		}

		Mat drawing = Mat::zeros( thresholdImage.size(), CV_8UC3 );
		for( int i = 0; i< contourSize; i++ )
		{
			double a = contourArea(contours[i], false);
			if(a < 100){
				Scalar color( 0, 255, 0 );
				drawContours( drawing, contours_poly, i, color, 1, 8, vector<Vec4i>(), 0, Point() );
				circle( drawing, center[i], (int)radius[i], color, 2, 8, 0 );
				sprintf(temp, "%d", i);
				putText(pCapture, temp, center[i], CV_FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 0));
			}
		}
		
		if(contourSize == 4){
			double maxDis = 0;
			int flag = -1;
			for(int i = 0; i < contourSize; i++){
				double a1 = center[i].x - center[0].x;
				double a2 = center[i].y - center[0].y;
				double dis = a1*a1 + a2*a2;
				if(dis > maxDis){
					maxDis = dis;
					flag = i;
				}
			}
			for(int i = 0; i < contourSize; i++){
				if(i != flag){
					line(drawing, center[0], center[i], Scalar(0, 0, 255), 1, 8);
				}
			}
			for(int i = 0; i < contourSize; i++){
				if(i != 0){
					line(drawing, center[flag], center[i], Scalar(0, 0, 255), 1, 8);
				}
			}	
		}

		if((frames % 100) == 0){
			prevCount = count;
			count = GetTickCount();
			fps = 100000.0/(count - prevCount);
			//std::cout << "fps: " << fps << endl;
			sprintf(fps_text, "FPS: %f", fps);

		}
		if(frames > 200)
			putText(pCapture, fps_text, Point(5, 20), CV_FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 0));
		else
			putText(pCapture, "Calculating FPS", Point(5, 20), CV_FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 0));

		imshow("blue", drawing);
		imshow(trackingWindowName, thresholdImage);
		imshow(_windowName, pCapture);
	}

	// Stop camera capture
	CLEyeCameraStop(_cam);
	// Destroy camera object
	CLEyeDestroyCamera(_cam);
	// Destroy the allocated OpenCV image
	cvReleaseImage(&pCapImage);
	_cam = NULL;
}


// Main program entry point
int _tmain(int argc, _TCHAR* argv[])
{
	CLEyeCameraCapture *cam[2] = { NULL };
	srand(GetTickCount());
	// Query for number of connected cameras
	int numCams = CLEyeGetCameraCount();
	if(numCams == 0)
	{
		cout << "No PS3Eye cameras detected" << endl;
		return -1;
	}
	cout << "Found " << numCams << " cameras" << endl;
	for(int i = 0; i < numCams; i++)
	{
		char windowName[64];
		// Query unique camera uuid
		GUID guid = CLEyeGetCameraUUID(i);
		printf("Camera %d GUID: [%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x]\n", 
			i+1, guid.Data1, guid.Data2, guid.Data3,
			guid.Data4[0], guid.Data4[1], guid.Data4[2],
			guid.Data4[3], guid.Data4[4], guid.Data4[5],
			guid.Data4[6], guid.Data4[7]);
		sprintf(windowName, "Camera Window %d", i+1);
		// Create camera capture object
		cam[i] = new CLEyeCameraCapture(windowName, guid,  CLEYE_COLOR_PROCESSED, CLEYE_QVGA, 180);
		cout << "Starting capture on camera " << i+1 << endl;
		cam[i]->StartCapture();
	}
	cout << "Use the following keys to change camera parameters:\n"
		"\t'1' - select camera 1\n"
		"\t'2' - select camera 2\n"
		"\t'g' - select gain parameter\n"
		"\t'e' - select exposure parameter\n"
		"\t'z' - select zoom parameter\n"
		"\t'r' - select rotation parameter\n"
		"\t'+' - increment selected parameter\n"
		"\t'-' - decrement selected parameter\n" << endl;
	// The <ESC> key will exit the program
	CLEyeCameraCapture *pCam = NULL;
	int param = -1, key;
	while((key = cvWaitKey(0)) != 0x1b)
	{
		switch(key)
		{
		case 'g':	case 'G':	cout << "Parameter Gain" << endl;		param = CLEYE_GAIN;		break;
		case 'e':	case 'E':	cout << "Parameter Exposure" << endl;	param = CLEYE_EXPOSURE;	break;
		case 'z':	case 'Z':	cout << "Parameter Zoom" << endl;		param = CLEYE_ZOOM;		break;
		case 'r':	case 'R':	cout << "Parameter Rotation" << endl;	param = CLEYE_ROTATION;	break;
		case '1':				cout << "Selected camera 1" << endl;	pCam = cam[0];			break;
		case '2':				cout << "Selected camera 2" << endl;	pCam = cam[1];			break;
		case '+':	if(numCams == 1){
			pCam = cam[0];
					}
					if(pCam)	pCam->IncrementCameraParameter(param);		break;
		case '-':	if(numCams == 1){
			pCam = cam[0];
					}
					if(pCam)	pCam->DecrementCameraParameter(param);		break;
		}
	}
	//delete cams
	for(int i = 0; i < numCams; i++)
	{
		cout << "Stopping capture on camera " << i+1 << endl;
		cam[i]->StopCapture();
		delete cam[i];
	}
	return 0;
}