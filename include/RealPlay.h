#ifndef _REALPLAY_H_ 
#define _REALPLAY_H_

#include <cstdio>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <time.h>
#include "HCNetSDK.h"
#include "PlayM4.h"

#define USECOLOR 1
#define DEV_IP1 "192.168.1.64"
#define USER_NAME "admin"
#define PASSWORD "Zgoawayimgay0."

using namespace std;
using namespace cv;

static LONG lPort = -1;
static HWND hWnd = NULL;

void realPlay();

#endif _REALPLAY_H_