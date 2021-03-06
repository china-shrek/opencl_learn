#pragma once

#include "stdafx.h"
#include <CL/cl.h>
#include <vector>
#include "Poco/Buffer.h"

#pragma comment(lib,"OpenCL.lib")

struct EapilDeviceInstance
{
    cl_device_id id;
    cl_context ctx;
    cl_command_queue cmd_queue;
    cl_program program;

    EapilDeviceInstance() :id(0), ctx(0), cmd_queue(0), program(0){}
};

class HelloWorldCL
{
public:
    HelloWorldCL();
    ~HelloWorldCL();
    bool Init(string file);

private:
    void PrintfPlatform();
    string GetPlatformInfo(cl_platform_id id, cl_platform_info info);
    void TryGetGPUDevice(cl_platform_id id);
    void TryGetCPUDevice(cl_platform_id id);
    void TryGetAllDevice(cl_platform_id id);
    void PrintfDevice();
    string GetDeviceInfoStr(cl_device_id id, cl_device_info info);

    template<typename ValueType>
    ValueType GetDeviceInfo(cl_device_id id, cl_device_info info);

    string ReadCodeFromFile(string file);
    
    void RunProgram();
    size_t shrRoundUp(size_t f, size_t s);
    void TestMemcopy();

	void DoImageCover();
	void DoImageCoverBasic(int width,int height,Path p,Buffer<unsigned char> &Y, Buffer<unsigned char> &U, Buffer<unsigned char> &V, Buffer<unsigned char> &rgb);
	void DoImageCoverBasicTable(int width, int height, Path p, Buffer<unsigned char> &Y, Buffer<unsigned char> &U, Buffer<unsigned char> &V, Buffer<unsigned char> &rgb);
	void DoImageCoverOpenclNew(int width, int height, Path p, Buffer<unsigned char> &Y, Buffer<unsigned char> &U, Buffer<unsigned char> &V, Buffer<unsigned char> &rgb);
	void DoImageCoverOpenclOld(int width, int height, Path p, Buffer<unsigned char> &Y, Buffer<unsigned char> &U, Buffer<unsigned char> &V, Buffer<unsigned char> &rgb);
	void DoImageCoverLibyuv(int width, int height, Path p, Buffer<unsigned char> &Y, Buffer<unsigned char> &U, Buffer<unsigned char> &V, Buffer<unsigned char> &rgb);
	void YuvToRgbPixel(unsigned char y, unsigned char u, unsigned char v,unsigned char* rgb);

    void RunKernel_IDCheck();
    void RunKernel_DataCopy(string funcname,int localsize);
    void DoImageProcess();
    void DoImageProcessRoate(int angle);

private:
    vector<cl_platform_id>      m_vec_ptf;
    vector<cl_device_id>        m_vec_dev;
    vector<EapilDeviceInstance> m_vec_instance;
};

