#include "RealPlay.h"

void yv12toYUV(char *outYuv, char *inYu12, int width, int height, int widthStep)
{
    int col, row, tmp, idx;
    unsigned int Y, U, V;

    for (row = 0; row < height; row++)
    {
        idx = row * widthStep;
        int rowptr = row * width;
        for (col = 0; col < width; col++)
        {
            tmp = (row / 2) * (width / 2) + (col / 2);
            Y = (unsigned int)inYu12[row * width + col];
            U = (unsigned int)inYu12[width * height + width * height / 4 + tmp];
            V = (unsigned int)inYu12[width * height + tmp];
            outYuv[idx + col * 3] = Y;
            outYuv[idx + col * 3 + 1] = U;
            outYuv[idx + col * 3 + 2] = V;
        }
    }
}

//解码回调，视频数据为YUVSHUJU（YV12）
void CALLBACK DecCBFun(int lPort, char *pBuf, int nSize, FRAME_INFO *pFrameInfo, void *nReserved1, int nReserved2)
{
    long IFrameType = pFrameInfo->nType;
    if (IFrameType == T_YV12)
    {
#if USECOLOR
        //int start = clock();
        IplImage *pImgYCrCb = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);           //得到图像的Y分量
        yv12toYUV(pImgYCrCb->imageData, pBuf, pFrameInfo->nWidth, pFrameInfo->nHeight, pImgYCrCb->widthStep); //得到全部RGB图像
        IplImage *pImg = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);
        cvCvtColor(pImgYCrCb, pImg, CV_YCrCb2RGB);
        //int end = clock();
#else
        IplImage *pImg = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 1);
        memcpy(pImg->imageData, pBuf, pFrameInfo->nWidth * pFrameInfo->nHeight);
#endif
        Mat pic = cv::cvarrToMat(pImg);
        imwrite("./output/right_calibration1.jpg", pic);
        namedWindow("IPCamera",WINDOW_NORMAL);
        cvShowImage("IPCamera", pImg);
        waitKey(1);
        
#if USECOLOR
        cvReleaseImage(&pImgYCrCb);
        cvReleaseImage(&pImg);
#else
        cvReleaseImage(&pImg);
#endif
        //此时是YV12格式的视频数据，保存在pBuf中，可以fwrite(pBuf,nSize,1,Videofile);
        //fwrite(pBuf,nSize,1,fp);
    }
}

///实时流回调
void CALLBACK g_RealDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void *dwUser)
{
    DWORD dRet;
    switch (dwDataType)
    {
    case NET_DVR_SYSHEAD:            //系统头
        if (!PlayM4_GetPort(&lPort)) //获取播放库未使用的通道号
        {
            break;
        }
        if (dwBufSize > 0)
        {
            if (!PlayM4_OpenStream(lPort, pBuffer, dwBufSize, 1024 * 1024))
            {
                dRet = PlayM4_GetLastError(lPort);
                break;
            }
            //设置解码回调函数 只解码不显示
            if (!PlayM4_SetDecCallBack(lPort, DecCBFun))
            {
                dRet = PlayM4_GetLastError(lPort);
                break;
            }
            //打开视频解码
            if (!PlayM4_Play(lPort, hWnd))
            {
                dRet = PlayM4_GetLastError(lPort);
                break;
            }
            //打开音频解码, 需要码流是复合流
            if (!PlayM4_PlaySound(lPort))
            {
                dRet = PlayM4_GetLastError(lPort);
                break;
            }
        }
        break;

    case NET_DVR_STREAMDATA: //码流数据
        if (dwBufSize > 0 && lPort != -1)
        {
            BOOL inData = PlayM4_InputData(lPort, pBuffer, dwBufSize);
            while (!inData)
            {
                sleep(10);
                inData = PlayM4_InputData(lPort, pBuffer, dwBufSize);
                cout << "PlayM4_InputData failed \n";
            }
        }
        break;
    }
}

void CALLBACK g_ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser)
{
    char tempbuf[256] = {0};
    switch (dwType)
    {
    case EXCEPTION_RECONNECT: //预览时重连
        printf("----------reconnect--------%d\n", time(NULL));
        break;
    default:
        break;
    }
}

void realPlay()
{
    // 初始化
    NET_DVR_Init();
    //设置连接时间与重连时间
    NET_DVR_SetConnectTime(2000, 1);
    NET_DVR_SetReconnect(10000, true);
    // 注册设备
    LONG lUserID;
    NET_DVR_DEVICEINFO_V30 struDeviceInfo;
    lUserID = NET_DVR_Login_V30(DEV_IP1, 8000, USER_NAME, PASSWORD, &struDeviceInfo);
    if (lUserID < 0)
    {
        printf("Login error, %d\n", NET_DVR_GetLastError());
        NET_DVR_Cleanup();
        return;
    }

    //设置异常消息回调函数
    NET_DVR_SetExceptionCallBack_V30(0, NULL, g_ExceptionCallBack, NULL);

    //启动预览并设置回调数据流
    LONG lRealPlayHandle;
    NET_DVR_PREVIEWINFO struPlayInfo = {0};
    struPlayInfo.lChannel = 1;
    struPlayInfo.dwStreamType = 0;
    struPlayInfo.dwLinkMode = 0; //0- TCP 方式,1- UDP 方式,2- 多播方式,3- RTP 方式,4-RTP/RTSP,5-RSTP/HTTP
    struPlayInfo.bBlocked = 1;

    lRealPlayHandle = NET_DVR_RealPlay_V40(lUserID, &struPlayInfo, g_RealDataCallBack, NULL);
    if (lRealPlayHandle < 0)
    {
        printf("NET_DVR_RealPlay_V40 error\n");
        NET_DVR_Logout(lUserID);
        NET_DVR_Cleanup();
        return;
    }
    sleep(10000);

    //关闭预览
    NET_DVR_StopRealPlay(lRealPlayHandle);
    //注销用户
    NET_DVR_Logout(lUserID);
    //释放 SDK 资源
    NET_DVR_Cleanup();

    return;
}