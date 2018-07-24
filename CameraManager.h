#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H
#include <stdint.h>
#include <vector>
using namespace std;

typedef struct {
    uint32_t    width;
    uint32_t    height;
    uint32_t    format; //RGB, YUV
    float         fps;
}CamProperty;

class Camera {
    friend class CameraManager;
public:
    Camera(int id);
    ~Camera();
    int GetId(){return m_id;}
    bool Open(CamProperty* pCp);
    void Close();
    CamProperty* GetProperty(int& count);
    bool IsOpened() { return (m_fd >=0);}

private:
    int m_id; //dev/video##
    int m_nMaxProperties;
    CamProperty* m_pPropList;
protected:
    int m_fd; //g.e. zero if device is opened
};
class CameraManager
{
public:
    CameraManager();
    ~CameraManager();
    int Reflesh();  /*<! re-init the camera, return the maximumal camera numbers */
    int MaxCamera();/*<! current max camera after previous reflesh */
    Camera* GetCamera(int id);
    Camera* GetCameraBySeq(int seq);/*<! get camera object by sequence number */

private:
    void Clean();
};
CameraManager* GetCameraManager();
#endif // CAMERAMANAGER_H
