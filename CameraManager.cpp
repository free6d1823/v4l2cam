#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include "CameraManager.h"

static vector <Camera*>    m_listCam;
static CameraManager theCameraManager;
CameraManager* GetCameraManager(){return &theCameraManager;}
/******************************************************************
 *  Global functions
 * ****************************************************************/
static int xioctl(int fd, int request, void* arg)
{
  for (int i = 0; i < 100; i++) {
    int r = ioctl(fd, request, arg);
    if (r != -1 || errno != EINTR) return r;
  }
  return -1;
}

/******************************************************************
 *  \class CameraManager
 * ****************************************************************/
 /*<! current max camera after previous reflesh */
int CameraManager::MaxCamera()
{
    return m_listCam.size();
}
void CameraManager::Clean()
{
    Camera* pCam = m_listCam.back();
    while (pCam){
        delete pCam;
        m_listCam.pop_back();
        pCam = m_listCam.back();
    }
    Reflesh();
}
CameraManager::CameraManager()
{
    m_listCam.clear();
    Reflesh();
}
CameraManager::~CameraManager()
{

}
int EnumFpsByFrameSize(int fd, v4l2_frmivalenum& fit, CamProperty* pCp, int& n)
{
    int ret = xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS,&fit);
    if ( 0 != ret)
        return n;
    if (fit.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
        do {
            pCp[n].width = fit.width;
            pCp[n].height = fit.height;
            pCp[n].format = fit.pixel_format;
            pCp[n].fps = fit.discrete.denominator /fit.discrete.numerator;

printf("EnumFpsByFrameSize %d %d %x %d/%d\n", pCp[n].width, pCp[n].height,fit.pixel_format,

            fit.discrete.numerator, fit.discrete.denominator);
            fit.index ++;
            ret = xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS,&fit);
            n++;
        }while (ret == 0);
    } else {
        //V4L2_FRMIVAL_TYPE_CONTINUOUS ||V4L2_FRMIVAL_TYPE_STEPWISE
        float min = (float) fit.stepwise.min.numerator/(float)fit.stepwise.min.denominator;
        float max = (float) fit.stepwise.max.numerator/(float)fit.stepwise.max.denominator;
        float dd =  (float) fit.stepwise.step.numerator/(float)fit.stepwise.step.denominator;
        printf(" step type(%d)-- size(%d/%d-%d/%d-%d/%d)\n",  fit.type,
               fit.stepwise.min.denominator, fit.stepwise.min.denominator,
               fit.stepwise.max.denominator,fit.stepwise.max.denominator,
               fit.stepwise.step.denominator,fit.stepwise.step.denominator
               );


        for (float ff=min; ff <=max; ff+= dd){
            pCp[n].width = fit.width;
            pCp[n].height = fit.height;
            pCp[n].format = fit.pixel_format;
            pCp[n].fps = 1.0/ff;
            n++;
        }
    }
    return n;
}

int EnumFrameSizeByPixelFormat(int fd, v4l2_frmsizeenum& fse, CamProperty* pCp, int& n)
{
    v4l2_frmivalenum fit;

    int ret = xioctl(fd, VIDIOC_ENUM_FRAMESIZES,&fse);
    //get resolution
    if ( 0 != ret)
       return n;
    memset(&fit, 0, sizeof(fit));
    fit.pixel_format = fse.pixel_format;
    if (fse.type == V4L2_FRMSIZE_TYPE_DISCRETE || fse.type == 0){//
        do {

            printf("EnumFrameSizeByPixelFormat type(%d) %dx%d--\n",fse.type, fse.discrete.width, fse.discrete.height );

            fit.width = fse.discrete.width;
            fit.height = fse.discrete.height;
            fit.index = 0;
            EnumFpsByFrameSize(fd, fit, pCp, n);
            fse.index ++;
            ret = xioctl(fd, VIDIOC_ENUM_FRAMESIZES,&fse);
        }while (ret == 0);

    } else {//V4L2_FRMIVAL_TYPE_CONTINUOUS(2) |V4L2_FRMIVAL_TYPE_STEPWISE(3)
        printf(" step type(%d)-- size(%d-%d-%d, %d-%d-%d)\n",  fse.type,
               fse.stepwise.min_width, fse.stepwise.max_width,
               fse.stepwise.step_width,
               fse.stepwise.min_height, fse.stepwise.max_height,
               fse.stepwise.step_height);
        fit.width = fse.stepwise.min_width;
        fit.height = fse.stepwise.min_height;
        fit.index = 0;
        EnumFpsByFrameSize(fd, fit, pCp, n);
    }
    return n;
}
/*<! re-init the camera, return the maximumal camera numbers */
int CameraManager::Reflesh()
{
    char device[32];
    int i;

    for (i=0;i<20; i++){
        sprintf(device, "/dev/video%d", i);
        int fd = open(device, O_RDWR | O_NONBLOCK, 0);
        if (fd < 0)
            continue;
        struct v4l2_capability cap;
        if ( (xioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) || !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
             || !(cap.capabilities & V4L2_CAP_STREAMING)) {
            close(fd);
            continue;
        }
        CamProperty cp[100];
        int n = 0; //number of property set
        struct v4l2_fmtdesc fmt;
        memset(&fmt, 0, sizeof(fmt));
        fmt.index = 0;
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        //get pixel format
        //To enumerate image formats applications initialize the type and index field of struct v4l2_fmtdesc
        //and call the ioctl VIDIOC_ENUM_FMT ioctl with a pointer to this structure. Drivers fill the rest
        //of the structure or return an EINVAL error code. All formats are enumerable by beginning at
        //index zero and incrementing by one until EINVAL is returned.
        uint32_t pixelformat = 0;
        while (0 == xioctl(fd, VIDIOC_ENUM_FMT,&fmt)){
            //some driver never end, so we check the fmt, break if the same as before
            if (pixelformat == fmt.pixelformat)
                break;
            pixelformat = fmt.pixelformat;

            v4l2_frmsizeenum fse;
            memset(&fse, 0, sizeof(fse));
            fse.index = 0;
            fse.pixel_format = fmt.pixelformat;
            EnumFrameSizeByPixelFormat(fd, fse, cp, n);

            if (n>= 80) { //this is a rough check; n could overfloat within above functions.
                break;
            }
            fmt.index ++;
            fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        }
        Camera* pCam = new Camera(i);
        pCam->m_nMaxProperties = n;
        pCam->m_pPropList = (CamProperty*) malloc(sizeof(CamProperty)*n);
        for (int k=0; k<n; k++){
            pCam->m_pPropList[k] = cp[k];
        }
        m_listCam.push_back(pCam);
        close(fd);
        //try next dev nodes
    }

    return (int) m_listCam.size();
}
/*<! open a camera object */
Camera* CameraManager::GetCamera(int id)
{
    vector<Camera*>::iterator it;
    for(it=m_listCam.begin(); it!=m_listCam.end(); ++it) {
        if ( (*it)->m_id == id)
            return ((*it));
    }
    return NULL;
}
/*<! get camera object by sequence number */
Camera* CameraManager::GetCameraBySeq(int seq)
{
    if (seq < (int) m_listCam.size())
        return m_listCam[seq];
    return NULL;
}

/******************************************************************
 *  \class Camera
 * ****************************************************************/
CamProperty* Camera::GetProperty(int& count)
{
    count = m_nMaxProperties;
    return m_pPropList;
}
Camera::Camera(int id)
{
    m_id = id;
    m_pPropList = NULL;
    m_fd = -1;
}
Camera::~Camera()
{
    if (m_pPropList)
        free (m_pPropList);
    if (m_fd >= 0)
        close(m_fd);
}
bool Camera::Open(CamProperty* pCp)
{
    if (m_fd >=0) {
        printf("Camera has been opened before.\n");
        return true;
    }
    char device[32];
    sprintf(device, "/dev/video%d", m_id);
    int fd = open(device, O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
        fprintf(stderr, "Open device %s failed!!\n", device);
        return false;
    }
    //

    m_fd = fd;
    v4l2_format fmt;
    if ( 0 != xioctl(fd, VIDIOC_G_FMT,&fmt));
        return false;

    return true;
}
void Camera::Close()
{
    if (m_fd >= 0)
        close(m_fd);
    m_fd = -1;
}

