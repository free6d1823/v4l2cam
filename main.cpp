#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <signal.h>
#include <time.h>
#include "CameraManager.h"


bool g_terminate = false;
void signalHandler(int act)
{
    switch(act)
    {
    case SIGINT:
        g_terminate = true;
        break;
    default:
        break;
    }
}
void onFrameCallback(void* pBuffer, int length, void* data)
{

static double curTime=0;
    (void)data;
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);    //wandboard works!
    double timeMs = (tv.tv_sec) * 1000 + (tv.tv_nsec) / 1000000 ;

#if 0
    struct timeval  tv;
    gettimeofday(&tv, NULL); //wandboard eturns zero
    double timeMs = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

#endif

    if (curTime > 0){
        printf("Diff=%8.3f sec -", (timeMs - curTime)/1000);
    } else {
        printf("current time %ld.%ldns - ", tv.tv_sec, tv.tv_nsec);
    }

    curTime = timeMs;
    printf("got mem(%p) %d bytes\n", pBuffer, length);
}

int main()
{
    printf ("Hello camera\n");
    struct sigaction action, old_action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = &signalHandler;
    sigaction(SIGINT, &action, &old_action);

    int nMaxCam = GetCameraManager()->MaxCamera();
    printf("Found %d capture devices\n", nMaxCam);

    for (int i=0; i< nMaxCam; i++) {
        Camera* pCam = GetCameraManager()->GetCameraBySeq(i);
        if (pCam) {
            int n = 0;
            printf("Camea /dev/video/%d property-----\n", pCam->GetId());
            printf("    width - height - format - fps \n");
            CamProperty* pCp = pCam->GetSupportedProperty(n);
            for(int i=0; i < n; i++) {
                printf("   %5d   %5d    [%c%c%c%c]   %5f\n",
                       pCp[i].width, pCp[i].height,
                       pCp[i].format&0xff,(pCp[i].format>>8 )&0xff,
                       (pCp[i].format>>16 )&0xff,(pCp[i].format>>24 )&0xff,
                       pCp[i].fps);
            }
        }
    }

    //open cam0
    printf("test open camera:\n");
    Camera* pCam = GetCameraManager()->GetCameraBySeq(0);
    if (!pCam)
        return -1;
    //wandboard:
    CamProperty cp;
    cp.width = 2592; //imx-ipuv3 2400000.ipu: IC output size(1944) cannot exceed 1024
    cp.height = 1944;
    cp.fps = 15;
    cp.format = v4l2_fourcc('U', 'Y', 'V','Y');  //UYVY
    if (! pCam->Open(&cp))
    {   //try my PC
        cp.width = 640;
        cp.height = 480;
        cp.fps = 30;
        cp.format = v4l2_fourcc('Y','U', 'Y', 'V'); //YUYV
        pCam->Open(&cp);
    }

    if (!pCam->IsOpened()){
        printf("Open camera /dev/video%d failed!\n", pCam->GetId());
        return -1;
    }
    pCam->GetCurrentProperty(&cp);
    printf ("Open camera %dx%d, format=%x\n", cp.width,cp.height, cp.format);

#ifdef USE_CALLBACK
//#if 1
    printf("Start with Callback method.\n");
    pCam->Start(onFrameCallback, NULL);
    while (!g_terminate){
        usleep(10000);
    }
#endif
//#ifdef USE_GET_FRAME
#if 1
    printf("Start with GetFrame method:\n");
    int nCount = 100;
    pCam->Start(NULL, NULL);
    void* pBuffer = malloc(cp.width*cp.height*2);
    int length = 0;
    for (int i=0; i<nCount; i++){
        if (g_terminate)
            break;
        length = pCam->GetFrame(pBuffer, cp.width*cp.height*2);
        {
            static double curTime=0;
        //use  timespec_get
            struct timespec tv;
            clock_gettime(CLOCK_MONOTONIC, &tv);
            double timeMs = (tv.tv_sec) * 1000 + (tv.tv_nsec) / 1000000 ;

        #if 0
            struct timeval  tv;
            gettimeofday(&tv, NULL); //wandboard eturns zero
            double timeMs = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;

        #endif
            if (curTime > 0){
                printf("Diff=%8.3f sec -", (timeMs - curTime)/1000);
            } else {
                printf("time %9.3f sec - ", timeMs/1000);
            }
            curTime = timeMs;
            printf("got mem(%p) %d bytes\n", pBuffer, length);
        }
    }

#endif //



    pCam->Stop();
    pCam->Close();

    return 0;
}
