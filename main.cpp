#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "CameraManager.h"

#define MAX_CAMERA  6
#define MAX_CAM_PROPERTY    8
typedef struct {
  uint8_t* start;
  size_t length;
} buffer_t;

int main()
{
    printf ("Hello camera\n");
    int nMaxCam = GetCameraManager()->MaxCamera();
    printf("Found %d capture devices\n", nMaxCam);

    for (int i=0; i< nMaxCam; i++) {
        Camera* pCam = GetCameraManager()->GetCameraBySeq(i);
        if (pCam) {
            int n = 0;
            printf("Camea /dev/video/%d property-----\n", pCam->GetId());
            printf("    width - height - format - fps \n");
            CamProperty* pCp = pCam->GetProperty(n);
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
    cp.width = 1920;
    cp.height = 1080;
    cp.fps = 30;
    cp.format = 'YVYU';  //UYVY
    if (! pCam->Open(&cp))
    {   //try my PC
        cp.width = 640;
        cp.height = 480;
        cp.fps = 30;
        cp.format = 'VYUY'; //YUYV
        pCam->Open(&cp);
    }

    if (!pCam->IsOpened()){
        printf("Open camera /dev/video%d failed!\n", pCam->GetId());
    }
    return 0;
}
