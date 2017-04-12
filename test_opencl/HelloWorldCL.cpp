#include "HelloWorldCL.h"
#include "Poco/FileStream.h"
#include "Poco/StreamCopier.h"
#include <sstream>
#include "Poco/LineEndingConverter.h"
#include "Poco/Stopwatch.h"
#include "ColorTable.h"
#include "libyuv.h"

#pragma comment(lib,"libyuv.lib")

HelloWorldCL::HelloWorldCL()
{
}

HelloWorldCL::~HelloWorldCL()
{
}

bool HelloWorldCL::Init(string file)
{
    // 查询可用的平台个数，并返回状态
    cl_uint ptf_num = 0;
    cl_int clRet = clGetPlatformIDs(0, NULL, &ptf_num);
    if (CL_SUCCESS != clRet)
    {
		g_log->error("Getting platforms error");
        return false;
    }

    // 获得平台地址
    if (ptf_num > 0)  // 如果有可用平台
    {
        Buffer<cl_platform_id> buf(ptf_num);
        clRet = clGetPlatformIDs(ptf_num, buf.begin(), NULL);
        cl_platform_id *ptfs = buf.begin();
        for (int i = 0; i < ptf_num; i++)
        {
            m_vec_ptf.push_back(ptfs[i]);
        }
    }

    //PrintfPlatform();

    for (int i = 0; i < m_vec_ptf.size(); i++)
    {
        TryGetCPUDevice(m_vec_ptf[i]);
        TryGetGPUDevice(m_vec_ptf[i]);
        //TryGetAllDevice(m_vec_ptf[i]);
    }

    //PrintfDevice();
    
    string source = ReadCodeFromFile(file);
    //创建设备
    for (int i = 0; i < m_vec_dev.size(); i++)
    {
        EapilDeviceInstance edev;
        edev.id = m_vec_dev[i];

        // 创建设备环境
        edev.ctx = clCreateContext(NULL, 1, &m_vec_dev[i], NULL, NULL, NULL);

        // -------------------4.创建命令队列--------------------------------------
        edev.cmd_queue = clCreateCommandQueue(edev.ctx, m_vec_dev[i], 0, NULL);
        if (NULL == edev.cmd_queue)
        {
			g_log->warning("(%?)error create command queue\n", m_vec_dev[i]);
            continue;
        }

        const char *source_ptr = source.c_str();
        const size_t source_len = source.length();
        cl_int clErr = CL_SUCCESS;
        // 创建程序对象
        edev.program = clCreateProgramWithSource(edev.ctx, 1, &source_ptr, &source_len, &clErr);
        if (clErr != CL_SUCCESS || NULL == edev.program)
        {
            printf("Create Program error(code=%d)\n", clErr);
            continue;
        }

        // 编译程序
        Buffer<char> build_log(1024 * 1024);
        clRet = clBuildProgram(edev.program, 1, &m_vec_dev[i], "-g", NULL, NULL);
        clGetProgramBuildInfo(edev.program, m_vec_dev[i], CL_PROGRAM_BUILD_LOG, build_log.capacity(), build_log.begin(), NULL);
        printf("build log*************beg***************\n");
        printf("%s", build_log.begin());
        printf("build log*************end***************\n");
        if (CL_SUCCESS != clRet)	// 编译错误
        {
            cout << "Error: Can not build program" << endl;
            clReleaseProgram(edev.program);
            continue;
        }

        m_vec_instance.push_back(edev);
        printf("success use device(%p)\n", m_vec_dev[i]);
    }

    //RunProgram();
    //TestMemcopy();
    //RunKernel_IDCheck();
	//DoImageCover();

    //RunKernel_DataCopy("DataCopy",32);
    //RunKernel_DataCopy("DataCopyVector",32);
    //RunKernel_DataCopy("DataCopyAsync",480);
    DoImageProcess();

    return true;
}

void HelloWorldCL::PrintfPlatform()
{
    for (int i = 0; i < m_vec_ptf.size(); i++)
    {
        g_log->information("***************************beg********************************");
		g_log->information("[version]:%s", GetPlatformInfo(m_vec_ptf[i], CL_PLATFORM_VERSION));
		g_log->information("[name]:%s", GetPlatformInfo(m_vec_ptf[i], CL_PLATFORM_NAME));
		g_log->information("[profile]:%s", GetPlatformInfo(m_vec_ptf[i], CL_PLATFORM_PROFILE));
		g_log->information("[vendor]:%s", GetPlatformInfo(m_vec_ptf[i], CL_PLATFORM_VENDOR));
		g_log->information("[extensions]:%s", GetPlatformInfo(m_vec_ptf[i], CL_PLATFORM_EXTENSIONS));
		g_log->information("[timer-resolution]:%s", GetPlatformInfo(m_vec_ptf[i], CL_PLATFORM_HOST_TIMER_RESOLUTION));
		g_log->information("***************************end********************************\n");
    }
}

std::string HelloWorldCL::GetPlatformInfo(cl_platform_id id, cl_platform_info info)
{
    string ret_info;
    size_t info_size = 0;
    cl_int clRet = clGetPlatformInfo(id, info, 0, NULL, &info_size);
    if (CL_SUCCESS == clRet)
    {
        Buffer<char> buf(info_size);
        clRet = clGetPlatformInfo(id, info, buf.capacity(), buf.begin(), NULL);
        if (CL_SUCCESS == clRet)
        {
            ret_info.assign(buf.begin(), buf.capacity());
        }
    }
    return ret_info;
}

void HelloWorldCL::TryGetGPUDevice(cl_platform_id id)
{
    cl_uint dev_num = 0;
    cl_int clRet = clGetDeviceIDs(id, CL_DEVICE_TYPE_GPU, 0, NULL, &dev_num);
    if (0 == dev_num)	// 如果没有GPU设备
    {
		g_log->warning("platform(%#?i) no device for GPU\n", (UIntPtr)id);
        return;
    }

    Buffer<cl_device_id> buf(dev_num);
    clRet = clGetDeviceIDs(id, CL_DEVICE_TYPE_GPU, dev_num, buf.begin(), NULL);
    if (clRet == CL_SUCCESS)
    {
        cl_device_id* devs = buf.begin();
        for (int i = 0; i < dev_num; i++)
        {
            m_vec_dev.push_back(devs[i]);
			g_log->information("platform(%#?i) find CPU device(%#?i)", (UIntPtr)id, (UIntPtr)devs[i]);
        }
    }
}

void HelloWorldCL::TryGetCPUDevice(cl_platform_id id)
{
    cl_uint dev_num = 0;
    cl_int clRet = clGetDeviceIDs(id, CL_DEVICE_TYPE_CPU, 0, NULL, &dev_num);
    if (0 == dev_num)
    {
		g_log->warning("platform(%#?i) no device for CPU", (UIntPtr)id);
        return;
    }

    Buffer<cl_device_id> buf(dev_num);
    clRet = clGetDeviceIDs(id, CL_DEVICE_TYPE_CPU, dev_num, buf.begin(), NULL);
    if (clRet == CL_SUCCESS)
    {
        cl_device_id* devs = buf.begin();
        for (int i = 0; i < dev_num; i++)
        {
            m_vec_dev.push_back(devs[i]);
			g_log->information("platform(%#?i) find CPU device(%#?i)", (UIntPtr)id, (UIntPtr)devs[i]);
        }
    }
}

void HelloWorldCL::TryGetAllDevice(cl_platform_id id)
{
    cl_uint dev_num = 0;
    cl_int clRet = clGetDeviceIDs(id, CL_DEVICE_TYPE_ALL, 0, NULL, &dev_num);
    if (0 == dev_num)
    {
		g_log->warning("platform(%#?i) no device for CPU/GPU", (UIntPtr)id);
        return;
    }

    Buffer<cl_device_id> buf(dev_num);
    clRet = clGetDeviceIDs(id, CL_DEVICE_TYPE_ALL, dev_num, buf.begin(), NULL);
    if (clRet == CL_SUCCESS)
    {
        cl_device_id* devs = buf.begin();
        for (int i = 0; i < dev_num; i++)
        {
            m_vec_dev.push_back(devs[i]);
			g_log->information("platform(%#?i) find CPU/GPU device(%#?i)", (UIntPtr)id, (UIntPtr)devs[i]);
        }
    }
}

void HelloWorldCL::PrintfDevice()
{
    for (int i = 0; i < m_vec_dev.size(); i++)
    {
        g_log->information("***************************device beg********************************");
		cl_device_type type = GetDeviceInfo<cl_device_type>(m_vec_dev[i], CL_DEVICE_TYPE);
		if(type == CL_DEVICE_TYPE_CPU)
			g_log->information("[type]:CPU");
		else if (type == CL_DEVICE_TYPE_GPU)
			g_log->information("[type]:GPU");
		else
			g_log->information("[type]:Default");
		g_log->information("[version]:%s", GetDeviceInfoStr(m_vec_dev[i],CL_DEVICE_VERSION));
		g_log->information("[vendor]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_VENDOR_ID));
		g_log->information("[max compute units]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_MAX_COMPUTE_UNITS));
		g_log->information("[max work item dimensions]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS));
        //CL_DEVICE_MAX_WORK_ITEM_SIZES
		g_log->information("[max work group size]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_MAX_WORK_GROUP_SIZE));
		g_log->information("[preferred vector width char]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR));
		g_log->information("[preferred vector width short]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT));
		g_log->information("[preferred vector width int]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT));
		g_log->information("[preferred vector width long]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG));
		g_log->information("[preferred vector width float]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT));
		g_log->information("[preferred vector width double]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE));
		g_log->information("[preferred vector width half]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF));
		g_log->information("[native vector width char]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR));
		g_log->information("[native vector width short]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT));
		g_log->information("[native vector width int]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_NATIVE_VECTOR_WIDTH_INT));
		g_log->information("[native vector width long]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG));
		g_log->information("[native vector width float]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT));
		g_log->information("[native vector width double]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE));
		g_log->information("[native vector width half]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF));
		g_log->information("[max clock frequency]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_MAX_CLOCK_FREQUENCY));
		g_log->information("[address bits]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_ADDRESS_BITS));
		g_log->information("[version]:%?i", GetDeviceInfo<cl_ulong>(m_vec_dev[i], CL_DEVICE_MAX_MEM_ALLOC_SIZE));
		g_log->information("[image support]:%u", GetDeviceInfo<cl_bool>(m_vec_dev[i], CL_DEVICE_IMAGE_SUPPORT));
		g_log->information("[max read image args]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_MAX_READ_IMAGE_ARGS));
		g_log->information("[max write image args]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_MAX_WRITE_IMAGE_ARGS));
		g_log->information("[max read write image args]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS));
		g_log->information("[version]:%s", GetDeviceInfoStr(m_vec_dev[i], CL_DEVICE_IL_VERSION));
		g_log->information("[image2d max width]:%?i", GetDeviceInfo<size_t>(m_vec_dev[i], CL_DEVICE_IMAGE2D_MAX_WIDTH));
		g_log->information("[image2d max height]:%?i", GetDeviceInfo<size_t>(m_vec_dev[i], CL_DEVICE_IMAGE2D_MAX_HEIGHT));
		g_log->information("[image3d max width]:%?i", GetDeviceInfo<size_t>(m_vec_dev[i], CL_DEVICE_IMAGE3D_MAX_WIDTH));
		g_log->information("[image3d max height]:%?i", GetDeviceInfo<size_t>(m_vec_dev[i], CL_DEVICE_IMAGE3D_MAX_HEIGHT));
		g_log->information("[image3d max depth]:%?i", GetDeviceInfo<size_t>(m_vec_dev[i], CL_DEVICE_IMAGE3D_MAX_DEPTH));
		g_log->information("[image max buffer size]:%?i", GetDeviceInfo<size_t>(m_vec_dev[i], CL_DEVICE_IMAGE_MAX_BUFFER_SIZE));
		g_log->information("[image max array size]:%?i", GetDeviceInfo<size_t>(m_vec_dev[i], CL_DEVICE_IMAGE_MAX_ARRAY_SIZE));
		g_log->information("[max samplers]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_MAX_SAMPLERS));
		g_log->information("[image pitch alignment]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_IMAGE_PITCH_ALIGNMENT));
		g_log->information("[image base address alignment]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT));
		g_log->information("[max pipe args]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_MAX_PIPE_ARGS));
		g_log->information("[pipe max active reservations]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS));
		g_log->information("[pipe max packet size]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_PIPE_MAX_PACKET_SIZE));
		g_log->information("[max parameter size]:%?i", GetDeviceInfo<size_t>(m_vec_dev[i], CL_DEVICE_MAX_PARAMETER_SIZE));

        //memory;
		g_log->information("*****************************memory*****************************");
		g_log->information("[mem base addr align]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_MEM_BASE_ADDR_ALIGN));
		g_log->information("[single fp config]:%?i", GetDeviceInfo<cl_device_fp_config>(m_vec_dev[i], CL_DEVICE_SINGLE_FP_CONFIG));
		g_log->information("[double fp config]:%?i", GetDeviceInfo<cl_device_fp_config>(m_vec_dev[i], CL_DEVICE_DOUBLE_FP_CONFIG));
		g_log->information("[global mem cache type]:%u", GetDeviceInfo<cl_device_mem_cache_type>(m_vec_dev[i], CL_DEVICE_GLOBAL_MEM_CACHE_TYPE));
		g_log->information("[global mem cacheline size]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE));
        g_log->information("[global mem cache size]:%?i", GetDeviceInfo<cl_ulong>(m_vec_dev[i], CL_DEVICE_GLOBAL_MEM_CACHE_SIZE));
        g_log->information("[global mem size]:%?i", GetDeviceInfo<cl_ulong>(m_vec_dev[i], CL_DEVICE_GLOBAL_MEM_SIZE));
        g_log->information("[max constant buffer size]:%?i", GetDeviceInfo<cl_ulong>(m_vec_dev[i], CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE));
        g_log->information("[max constant args]:%u", GetDeviceInfo<cl_uint>(m_vec_dev[i], CL_DEVICE_MAX_CONSTANT_ARGS));
        g_log->information("[max global variable size]:%?i", GetDeviceInfo<size_t>(m_vec_dev[i], CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE));
        g_log->information("[global variable preferred total size]:%?i", GetDeviceInfo<size_t>(m_vec_dev[i], CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE));
        g_log->information("[local mem type]:%u", GetDeviceInfo<cl_device_local_mem_type>(m_vec_dev[i], CL_DEVICE_LOCAL_MEM_TYPE));
        g_log->information("[local mem size]:%?i", GetDeviceInfo<cl_ulong>(m_vec_dev[i], CL_DEVICE_LOCAL_MEM_SIZE));
        g_log->information("[error correction support]:%u", GetDeviceInfo<cl_bool>(m_vec_dev[i], CL_DEVICE_ERROR_CORRECTION_SUPPORT));
        g_log->information("[profiling timer resolution]:%?i", GetDeviceInfo<size_t>(m_vec_dev[i], CL_DEVICE_PROFILING_TIMER_RESOLUTION));
        g_log->information("[endian little]:%u", GetDeviceInfo<cl_bool>(m_vec_dev[i], CL_DEVICE_ENDIAN_LITTLE));
        g_log->information("[available]:%u", GetDeviceInfo<cl_bool>(m_vec_dev[i], CL_DEVICE_AVAILABLE));
        g_log->information("[compiler available]:%u", GetDeviceInfo<cl_bool>(m_vec_dev[i], CL_DEVICE_COMPILER_AVAILABLE));
        g_log->information("[linker available]:%u", GetDeviceInfo<cl_bool>(m_vec_dev[i], CL_DEVICE_LINKER_AVAILABLE));
        g_log->information("[execution capabilities]:%?i", GetDeviceInfo<cl_device_exec_capabilities>(m_vec_dev[i], CL_DEVICE_EXECUTION_CAPABILITIES));
        g_log->information("[queue on host properties]:%?i", GetDeviceInfo<cl_command_queue_properties>(m_vec_dev[i], CL_DEVICE_QUEUE_ON_HOST_PROPERTIES));
        g_log->information("[queue on device properties]:%?i", GetDeviceInfo<cl_command_queue_properties>(m_vec_dev[i], CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES));           

        g_log->information("***************************device end********************************\n\n");
    }
}

std::string HelloWorldCL::GetDeviceInfoStr(cl_device_id id, cl_device_info info)
{
    string ret_info;
    size_t info_size = 0;
    cl_int clRet = clGetDeviceInfo(id, info, 0, NULL, &info_size);
    if (CL_SUCCESS == clRet)
    {
        Buffer<char> buf(info_size);
        clRet = clGetDeviceInfo(id, info, buf.capacity(), buf.begin(), NULL);
        if (CL_SUCCESS == clRet)
        {
            ret_info.assign(buf.begin(), buf.capacity());
        }
    }
    return ret_info;
}

template<typename ValueType>
ValueType HelloWorldCL::GetDeviceInfo(cl_device_id id, cl_device_info info)
{
    ValueType ret_info = 0;
    cl_int clRet = clGetDeviceInfo(id, info, sizeof(ret_info), &ret_info, NULL);
    return ret_info;
}

std::string HelloWorldCL::ReadCodeFromFile(string file)
{
    FileInputStream fs(file);
    ostringstream output;
    InputLineEndingConverter conv(fs, LineEnding::NEWLINE_LF);
    StreamCopier::copyStream(conv, output);
    return output.str();
}

size_t HelloWorldCL::shrRoundUp(size_t f, size_t s)
{
    return (s + f - 1) / f*f;
}

void HelloWorldCL::TestMemcopy()
{
    int size = 1920*1080*3;
    Buffer<int> input_data(size);
    Buffer<int> output_data(size);
    int *input_arry = input_data.begin();
    int *output_arry = output_data.begin();
    int *output_arry2 = NULL;
    for (int i = 0; i < size; i++)
    {
        input_arry[i] = i;
        output_arry[i] = 0;
    }

    cl_int clErr = CL_SUCCESS;
    cl_int clRet = CL_SUCCESS;
    for (int i = 0; i < m_vec_instance.size(); i++)
    {
        //
        WATCH_FUNC_BEGIN(alloc_host);
        cl_mem alloc_host = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, input_data.capacityBytes(), NULL, &clErr);
        clRet = clEnqueueWriteBuffer(m_vec_instance[i].cmd_queue
            , alloc_host
            , CL_TRUE
            , 0
            , input_data.capacityBytes()
            , input_data.begin()
            , 0, NULL, NULL);
        //clFlush(m_vec_instance[i].cmd_queue);
        clRet = clEnqueueReadBuffer(m_vec_instance[i].cmd_queue
            , alloc_host
            , CL_TRUE
            , 0
            , output_data.capacityBytes()
            , output_data.begin()
            , 0, NULL, NULL);
        clFlush(m_vec_instance[i].cmd_queue);
        WATCH_FUNC_BEGIN(alloc_host_map);
        output_arry2 = (int*)clEnqueueMapBuffer(m_vec_instance[i].cmd_queue,
            alloc_host,
            CL_TRUE,
            CL_MAP_READ,
            0,
            input_data.capacityBytes(),
            0, NULL, NULL, NULL);
        clFlush(m_vec_instance[i].cmd_queue);
        WATCH_FUNC_END(alloc_host_map);
        clReleaseMemObject(alloc_host);
        WATCH_FUNC_END(alloc_host);

        WATCH_FUNC_BEGIN(use_host);
        cl_mem use_host = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, input_data.capacityBytes(), input_data.begin(), &clErr);
        clRet = clEnqueueReadBuffer(m_vec_instance[i].cmd_queue
            , use_host
            , CL_TRUE
            , 0
            , output_data.capacityBytes()
            , output_data.begin()
            , 0, NULL, NULL);
        clFlush(m_vec_instance[i].cmd_queue);
        WATCH_FUNC_BEGIN(use_host_map);
        output_arry2 = (int*)clEnqueueMapBuffer(m_vec_instance[i].cmd_queue,
            use_host,
            CL_TRUE,
            CL_MAP_READ,
            0,
            input_data.capacityBytes(),
            0, NULL, NULL, NULL);
        clFlush(m_vec_instance[i].cmd_queue);
        WATCH_FUNC_END(use_host_map);
        clReleaseMemObject(use_host);
        WATCH_FUNC_END(use_host);

        WATCH_FUNC_BEGIN(copy_host);
        cl_mem copy_host = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, input_data.capacityBytes(), input_data.begin(), &clErr);
        clRet = clEnqueueReadBuffer(m_vec_instance[i].cmd_queue
            , copy_host
            , CL_TRUE
            , 0
            , output_data.capacityBytes()
            , output_data.begin()
            , 0, NULL, NULL);
        clFlush(m_vec_instance[i].cmd_queue);
        WATCH_FUNC_BEGIN(copy_host_map);
        output_arry2 = (int*)clEnqueueMapBuffer(m_vec_instance[i].cmd_queue,
            copy_host,
            CL_TRUE,
            CL_MAP_READ,
            0,
            input_data.capacityBytes(),
            0, NULL, NULL, NULL);
        clFlush(m_vec_instance[i].cmd_queue);
        WATCH_FUNC_END(copy_host_map);
        clReleaseMemObject(copy_host);
        WATCH_FUNC_END(copy_host);
    }
}

void HelloWorldCL::DoImageCover()
{
	Path p_root = Path::current();
	p_root.popDirectory();
	p_root.pushDirectory("helpsource");
	Path p_child(p_root);
	p_child.pushDirectory("output");

	FileInputStream fs(p_root.setFileName("test_1920x960_one.yuv").toString(), ios::binary);
	int width = 1920;
	int height = 960;
	Buffer<unsigned char> Y(width * height);
	Buffer<unsigned char> U(width * height / 4);
	Buffer<unsigned char> V(width * height / 4);
	Buffer<unsigned char> rgb(width*height * 3);
	fs.read((char*)Y.begin(), Y.capacityBytes());
	fs.read((char*)U.begin(), U.capacityBytes());
	fs.read((char*)V.begin(), V.capacityBytes());

	DoImageCoverBasic(width, height, p_child, Y, U, V, rgb);
	DoImageCoverBasicTable(width, height, p_child, Y, U, V, rgb);
	DoImageCoverOpenclNew(width, height, p_child, Y, U, V, rgb);
	DoImageCoverOpenclOld(width, height, p_child, Y, U, V, rgb);
	DoImageCoverLibyuv(width, height, p_child, Y, U, V, rgb);
}

void HelloWorldCL::DoImageCoverBasic(int width, int height, Path p, Buffer<unsigned char> &Y, Buffer<unsigned char> &U, Buffer<unsigned char> &V, Buffer<unsigned char> &rgb)
{
	rgb.clear();
	WATCH_FUNC_BEGIN(Basic_height_width);
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			unsigned char y = Y[i * width + j];
			unsigned char u = U[(i / 2) * (width / 2) + (j / 2)];
			unsigned char v = V[(i / 2) * (width / 2) + (j / 2)];
			unsigned char* rgbv = &rgb[(i*width + j) * 3];
			YuvToRgbPixel(y, u, v, rgbv);
		}
	}
	FileOutputStream ofs(p.setFileName("test_1920x960_basic_for(height_width).rgb").toString(), ios::binary);
	ofs.write((char*)rgb.begin(), rgb.capacityBytes());
	WATCH_FUNC_END(Basic_height_width);

	rgb.clear();
	WATCH_FUNC_BEGIN(Basic_width_height);
	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			unsigned char y = Y[j * width + i];
			unsigned char u = U[(j / 2) * (width / 2) + (i / 2)];
			unsigned char v = V[(j / 2) * (width / 2) + (i / 2)];
			unsigned char* rgbv = &rgb[(j*width + i) * 3];
			YuvToRgbPixel(y, u, v, rgbv);
		}
	}
	FileOutputStream ofs(p.setFileName("test_1920x960_basic_for(width_height).rgb").toString(), ios::binary);
	ofs.write((char*)rgb.begin(), rgb.capacityBytes());
	WATCH_FUNC_END(Basic_width_height);
}

void HelloWorldCL::DoImageCoverBasicTable(int width, int height, Path p, Buffer<unsigned char> &Y, Buffer<unsigned char> &U, Buffer<unsigned char> &V, Buffer<unsigned char> &rgb)
{
	rgb.clear();
	WATCH_FUNC_BEGIN(Table);
	int rdif, invgdif, bdif;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			unsigned char y = Y[i * width + j];
			unsigned char u = U[(i / 2) * (width / 2) + (j / 2)];
			unsigned char v = V[(i / 2) * (width / 2) + (j / 2)];
			unsigned char* rgbv = &rgb[(i*width + j) * 3];

			rdif = Table_fv1[u];
			invgdif = Table_fu1[v] + Table_fv2[u];
			bdif = Table_fu2[v];

			rgbv[0] = y + bdif;
			rgbv[1] = y - invgdif;
			rgbv[2] = y + rdif;
			rgbv[0] = (rgbv[0] < 0 ? 0 : rgbv[0]>255 ? 255 : rgbv[0]);
			rgbv[1] = (rgbv[1] < 0 ? 0 : rgbv[1]>255 ? 255 : rgbv[1]);
			rgbv[2] = (rgbv[2] < 0 ? 0 : rgbv[2]>255 ? 255 : rgbv[2]);
		}
	}
	FileOutputStream ofs(p.setFileName("test_1920x960_table.rgb").toString(), ios::binary);
	ofs.write((char*)rgb.begin(), rgb.capacityBytes());
	WATCH_FUNC_END(Table);
}

void HelloWorldCL::DoImageCoverOpenclNew(int width, int height, Path p, Buffer<unsigned char> &Y, Buffer<unsigned char> &U, Buffer<unsigned char> &V, Buffer<unsigned char> &rgb)
{
	for (int i = 0; i < m_vec_instance.size(); i++)
	{
		rgb.clear();
		cl_kernel kernel = clCreateKernel(m_vec_instance[i].program, "Yuv420ToRGB24", NULL);
		if (NULL == kernel)
		{
			g_log->warning("Can not create kernel");
			continue;
		}
		cl_int clErr = CL_SUCCESS;
		cl_mem cl_y = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, Y.capacityBytes(), Y.begin(), &clErr);
		cl_mem cl_u = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, U.capacityBytes(), U.begin(), &clErr);
		cl_mem cl_v = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, V.capacityBytes(), V.begin(), &clErr);
		cl_mem cl_rgb24 = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, rgb.capacityBytes(), NULL, &clErr);

		int index = 0;
		cl_int clRet = clSetKernelArg(kernel, index++, sizeof(cl_mem), (void *)&cl_y);
		clRet |= clSetKernelArg(kernel, index++, sizeof(cl_mem), (void *)&cl_u);
		clRet |= clSetKernelArg(kernel, index++, sizeof(cl_mem), (void *)&cl_v);
		clRet |= clSetKernelArg(kernel, index++, sizeof(cl_mem), (void *)&cl_rgb24);

		// Launching kernel  
		const size_t global_ws[] = { width / 2,height / 2 };
		const size_t local_ws[] = { 32,1 };
		char *point_out = NULL;
		int point_size = rgb.capacityBytes();
		WATCH_FUNC_BEGIN(New_opencl_in);
		clRet = clEnqueueNDRangeKernel(m_vec_instance[i].cmd_queue
			, kernel
			, sizeof(global_ws) / sizeof(size_t)
			, NULL
			, global_ws
			, local_ws
			, 0, NULL, NULL);
		clFinish(m_vec_instance[i].cmd_queue);
		point_out = (char *)clEnqueueMapBuffer(m_vec_instance[i].cmd_queue,
			cl_rgb24,
			CL_TRUE,
			CL_MAP_READ,
			0,
			point_size,
			0, NULL, NULL, NULL);
		clFinish(m_vec_instance[i].cmd_queue);
		g_log->information("type=%?i version=%s"
			, GetDeviceInfo<cl_device_type>(m_vec_instance[i].id, CL_DEVICE_TYPE)
			, GetDeviceInfoStr(m_vec_instance[i].id, CL_DEVICE_VERSION));
		WATCH_FUNC_END(New_opencl_in);
		FileOutputStream ofs(p.setFileName(format("new_test_1920x960_cl%d.rgb", i)).toString(), ios::binary);
		ofs.write((char*)point_out, rgb.capacityBytes());
		clReleaseKernel(kernel);
		clReleaseMemObject(cl_y);
		clReleaseMemObject(cl_u);
		clReleaseMemObject(cl_v);
		clReleaseMemObject(cl_rgb24);
	}
}

void HelloWorldCL::DoImageCoverOpenclOld(int width, int height, Path p, Buffer<unsigned char> &Y, Buffer<unsigned char> &U, Buffer<unsigned char> &V, Buffer<unsigned char> &rgb)
{
	for (int i = 0; i < m_vec_instance.size(); i++)
	{
		rgb.clear();
		cl_kernel kernel = clCreateKernel(m_vec_instance[i].program, "Yuv420ToRGB24_old", NULL);
		if (NULL == kernel)
		{
			g_log->warning("Can not create kernel");
			continue;
		}
		cl_int clErr = CL_SUCCESS;
		cl_mem cl_y = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, Y.capacityBytes(), Y.begin(), &clErr);
		cl_mem cl_u = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, U.capacityBytes(), U.begin(), &clErr);
		cl_mem cl_v = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, V.capacityBytes(), V.begin(), &clErr);
		cl_mem cl_rgb24 = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, rgb.capacityBytes(), NULL, &clErr);

		int index = 0;
		cl_int clRet = clSetKernelArg(kernel, index++, sizeof(cl_mem), (void *)&cl_y);
		clRet |= clSetKernelArg(kernel, index++, sizeof(cl_mem), (void *)&cl_u);
		clRet |= clSetKernelArg(kernel, index++, sizeof(cl_mem), (void *)&cl_v);
		clRet |= clSetKernelArg(kernel, index++, sizeof(cl_mem), (void *)&cl_rgb24);

		// Launching kernel  
		const size_t global_ws[] = { height,width };
		const size_t local_ws[] = { 1,128 };
		char *point_out = NULL;
		int point_size = rgb.capacityBytes();
		WATCH_FUNC_BEGIN(Old_opencl_in);
		clRet = clEnqueueNDRangeKernel(m_vec_instance[i].cmd_queue
			, kernel
			, sizeof(global_ws) / sizeof(size_t)
			, NULL
			, global_ws
			, local_ws
			, 0, NULL, NULL);
		clFinish(m_vec_instance[i].cmd_queue);
		point_out = (char *)clEnqueueMapBuffer(m_vec_instance[i].cmd_queue,
			cl_rgb24,
			CL_TRUE,
			CL_MAP_READ,
			0,
			point_size,
			0, NULL, NULL, NULL);
		clFinish(m_vec_instance[i].cmd_queue);
		g_log->information("type=%?i version=%s"
			, GetDeviceInfo<cl_device_type>(m_vec_instance[i].id, CL_DEVICE_TYPE)
			, GetDeviceInfoStr(m_vec_instance[i].id, CL_DEVICE_VERSION));
		WATCH_FUNC_END(Old_opencl_in);
		FileOutputStream ofs(p.setFileName(format("old_test_1920x960_cl%d.rgb", i)).toString(), ios::binary);
		ofs.write((char*)point_out, rgb.capacityBytes());
		clReleaseKernel(kernel);
		clReleaseMemObject(cl_y);
		clReleaseMemObject(cl_u);
		clReleaseMemObject(cl_v);
		clReleaseMemObject(cl_rgb24);
	}
}

void HelloWorldCL::DoImageCoverLibyuv(int width, int height, Path p, Buffer<unsigned char> &Y, Buffer<unsigned char> &U, Buffer<unsigned char> &V, Buffer<unsigned char> &rgb)
{
	rgb.clear();
	WATCH_FUNC_BEGIN(Libyuv);
	libyuv::I420ToRAW(Y.begin(), width, U.begin(), width / 2, V.begin(), width / 2, rgb.begin(), width * 3, width, height);
	WATCH_FUNC_END(Libyuv);
	FileOutputStream ofs(p.setFileName("test_1920x960_libyuv.rgb").toString(), ios::binary);
	ofs.write((char*)rgb.begin(), rgb.capacityBytes());
}

void HelloWorldCL::YuvToRgbPixel(unsigned char y, unsigned char u, unsigned char v, unsigned char* rgb)
{
	rgb[0] = (int)((y & 0xff) + 1.4075 * ((v & 0xff) - 128));
	rgb[1] = (int)((y & 0xff) - 0.3455 * ((u & 0xff) - 128) - 0.7169*((v & 0xff) - 128));
	rgb[2] = (int)((y & 0xff) + 1.779 * ((u & 0xff) - 128));
	rgb[0] = (rgb[0] < 0 ? 0 : rgb[0]>255 ? 255 : rgb[0]);
	rgb[1] = (rgb[1] < 0 ? 0 : rgb[1]>255 ? 255 : rgb[1]);
	rgb[2] = (rgb[2] < 0 ? 0 : rgb[2]>255 ? 255 : rgb[2]);
}

void HelloWorldCL::RunKernel_IDCheck()
{
    Buffer<cl_float> buf(50);
    buf.clear();
    for (int i = 0; i < m_vec_instance.size(); i++)
    {
        cl_kernel kernel = clCreateKernel(m_vec_instance[i].program, "IDCheck", NULL);
        if (NULL == kernel)
        {
            g_log->warning("Can not create kernel");
            continue;
        }
        cl_int clErr = CL_SUCCESS;
        
        cl_mem cl_buf = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, buf.capacityBytes(), buf.begin(), &clErr);

        int index = 0;
        cl_int clRet = clSetKernelArg(kernel, index++, sizeof(cl_mem), (void *)&cl_buf);

        // Launching kernel  
        const size_t global_ws[] = { 6,4 };
        const size_t local_ws[] = { 3,2 };
        const size_t offset[] = { 3,5 };
        char *point_out = NULL;
        int point_size = buf.capacityBytes();
        WATCH_FUNC_BEGIN(RunKernel_IDCheck);
        clRet = clEnqueueNDRangeKernel(m_vec_instance[i].cmd_queue
            , kernel
            , sizeof(global_ws) / sizeof(size_t)
            , offset
            , global_ws
            , local_ws
            , 0, NULL, NULL);
        clFinish(m_vec_instance[i].cmd_queue);
        point_out = (char *)clEnqueueMapBuffer(m_vec_instance[i].cmd_queue,
            cl_buf,
            CL_TRUE,
            CL_MAP_READ,
            0,
            point_size,
            0, NULL, NULL, NULL);
        clFinish(m_vec_instance[i].cmd_queue);
        g_log->information("type=%?i version=%s"
            , GetDeviceInfo<cl_device_type>(m_vec_instance[i].id, CL_DEVICE_TYPE)
            , GetDeviceInfoStr(m_vec_instance[i].id, CL_DEVICE_VERSION));
        WATCH_FUNC_END(RunKernel_IDCheck);
        for (int i = 0; i < buf.capacity(); i++)
        {
            printf("%f\n", buf[i]);
        }
        clReleaseKernel(kernel);
        clReleaseMemObject(cl_buf);
    }
}

void HelloWorldCL::RunKernel_DataCopy(string funcname, int localsize)
{
    Buffer<cl_char> bufin(1920 * 1080);
    Buffer<cl_char> bufout(1920 * 1080);
    for (int i = 0; i < m_vec_instance.size(); i++)
    {
        bufout[1920] = 0;
        bufin[1920] = 64;
        cl_kernel kernel = clCreateKernel(m_vec_instance[i].program, funcname.c_str(), NULL);
        if (NULL == kernel)
        {
            g_log->warning("Can not create kernel");
            continue;
        }
        cl_int clErr = CL_SUCCESS;

        cl_mem cl_buf_in = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, bufin.capacityBytes(), bufin.begin(), &clErr);
        cl_mem cl_buf_out = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, bufout.capacityBytes(), bufout.begin(), &clErr);

        int index = 0;
        cl_int clRet = clSetKernelArg(kernel, index++, sizeof(cl_mem), (void *)&cl_buf_in);
        clRet |= clSetKernelArg(kernel, index++, sizeof(cl_mem), (void *)&cl_buf_out);

        // Launching kernel  
        const size_t global_ws[] = { bufin.capacity() / 16 };
        const size_t local_ws[] = { localsize };
        const size_t offset[] = { 0 };
        char *point_out = NULL;
        int point_size = bufin.capacityBytes();
        WATCH_FUNC_BEGIN(RunKernel_DataCopy);
        clRet = clEnqueueNDRangeKernel(m_vec_instance[i].cmd_queue
            , kernel
            , sizeof(global_ws) / sizeof(size_t)
            , offset
            , global_ws
            , local_ws
            , 0, NULL, NULL);
        clFinish(m_vec_instance[i].cmd_queue);
        point_out = (char *)clEnqueueMapBuffer(m_vec_instance[i].cmd_queue,
            cl_buf_out,
            CL_TRUE,
            CL_MAP_READ,
            0,
            point_size,
            0, NULL, NULL, NULL);
        clFinish(m_vec_instance[i].cmd_queue);
        g_log->information("type=%?i version=%s"
            , GetDeviceInfo<cl_device_type>(m_vec_instance[i].id, CL_DEVICE_TYPE)
            , GetDeviceInfoStr(m_vec_instance[i].id, CL_DEVICE_VERSION));
        g_log->information("result = %d", (int)bufout[1920]);
        WATCH_FUNC_END(RunKernel_DataCopy);
        clReleaseKernel(kernel);
        clReleaseMemObject(cl_buf_in);
        clReleaseMemObject(cl_buf_out);
    }
}

void HelloWorldCL::DoImageProcess()
{
    Path p_root = Path::current();
    p_root.popDirectory();
    p_root.pushDirectory("helpsource");
    Path p_child(p_root);
    p_child.pushDirectory("output");

    FileInputStream fs(p_root.setFileName("test_1920x960_one.rgba").toString(), ios::binary);
    int width = 1920;
    int height = 960;
    Buffer<unsigned char> rgbInput(width * height * 4);
    fs.read((char*)rgbInput.begin(), rgbInput.capacityBytes());
    Buffer<unsigned char> rgbOutput(width * height * 4);
    for (int i = 0; i < m_vec_instance.size(); i++)
    {
        rgbOutput.clear();
        cl_kernel kernel = clCreateKernel(m_vec_instance[i].program, "ImageProcessCopy", NULL);
        if (NULL == kernel)
        {
            g_log->warning("Can not create kernel");
            continue;
        }
        cl_int clErr = CL_SUCCESS;       

        cl_image_format format_in = { CL_RGBA, CL_UNSIGNED_INT8 };
        cl_mem cl_buf_in = clCreateImage2D(m_vec_instance[i].ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, &format_in, width, height, width * 4, rgbInput.begin(), &clErr);
        cl_mem cl_buf_out = clCreateImage2D(m_vec_instance[i].ctx, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, &format_in, width, height, width * 4, rgbOutput.begin(), &clErr);

        int index = 0;
        cl_int clRet = clSetKernelArg(kernel, index++, sizeof(cl_mem), (void *)&cl_buf_in);
        clRet |= clSetKernelArg(kernel, index++, sizeof(cl_mem), (void *)&cl_buf_out);

        // Launching kernel  
        const size_t global_ws[] = { width,height};
        const size_t local_ws[] = { 32,1 };
        const size_t offset[] = { 0,0 };
        char *point_out = NULL;
        int point_size = rgbInput.capacityBytes();
        WATCH_FUNC_BEGIN(DoImageProcess);
        clRet = clEnqueueNDRangeKernel(m_vec_instance[i].cmd_queue
            , kernel
            , sizeof(global_ws) / sizeof(size_t)
            , NULL//offset
            , global_ws
            , local_ws
            , 0, NULL, NULL);
        clFinish(m_vec_instance[i].cmd_queue);
        point_out = (char *)clEnqueueMapBuffer(m_vec_instance[i].cmd_queue,
            cl_buf_out,
            CL_TRUE,
            CL_MAP_READ,
            0,
            point_size,
            0, NULL, NULL, NULL);
        clFinish(m_vec_instance[i].cmd_queue);
        g_log->information("type=%?i version=%s"
            , GetDeviceInfo<cl_device_type>(m_vec_instance[i].id, CL_DEVICE_TYPE)
            , GetDeviceInfoStr(m_vec_instance[i].id, CL_DEVICE_VERSION));
        WATCH_FUNC_END(DoImageProcess);
        FileOutputStream ofs(p_child.setFileName(format("Image_copy_test_1920x960_cl%d.rgb", i)).toString(), ios::binary);
        ofs.write((char*)point_out, rgbOutput.capacityBytes());
        clReleaseKernel(kernel);
        clReleaseMemObject(cl_buf_in);
        clReleaseMemObject(cl_buf_out);
    }
}

void vector_add_cpu(const float* src_a, const float* src_b,float*  res,const int num)
{
    for (int i = 0; i < num; i++)
        res[i] = src_a[i] + src_b[i];
}

void HelloWorldCL::RunProgram()
{
    const int size = 12345678;
    Buffer<float> src_a_h(size);
    Buffer<float> src_b_h(size);
    Buffer<float> res_h(size);
    // Initialize both vectors  

    for (int i = 0; i < size; i++) 
    {
        src_a_h.begin()[i] = src_b_h.begin()[i] = (float)i;
    }

    for (int i = 0; i < m_vec_instance.size(); i++)
    {
		WATCH_FUNC_BEGIN(opencl);
        cl_kernel kernel = clCreateKernel(m_vec_instance[i].program, "vector_add_gpu", NULL);
        if (NULL == kernel)
        {
			g_log->warning("Can not create kernel");
            continue;
        }

        cl_int clErr = CL_SUCCESS;
        cl_mem src_a_d = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, src_a_h.capacityBytes(), src_a_h.begin(), &clErr);
        cl_mem src_b_d = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, src_b_h.capacityBytes(), src_b_h.begin(), &clErr);
        cl_mem res_d = clCreateBuffer(m_vec_instance[i].ctx, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, res_h.capacityBytes(), res_h.begin(), &clErr);
        
        cl_int clRet = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&src_a_d);
        clRet |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&src_b_h);
        clRet |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&res_h);
        clRet |= clSetKernelArg(kernel, 2, sizeof(int), (void *)&size);

        // Launching kernel  
        const size_t local_ws = 512;    // Number of work-items per work-group  
        // shrRoundUp returns the smallest multiple of local_ws bigger than size  
        const size_t global_ws = shrRoundUp(local_ws, size);    // Total number of work-items 

        clRet = clEnqueueNDRangeKernel(m_vec_instance[i].cmd_queue
            , kernel
            , GetDeviceInfo<cl_uint>(m_vec_instance[i].id, CL_DEVICE_MAX_COMPUTE_UNITS)
            , NULL
            , &global_ws
            , &local_ws
            , 0, NULL, NULL);

        clRet = clEnqueueReadBuffer(m_vec_instance[i].cmd_queue
            , res_d
            , CL_TRUE
            , 0
            , res_h.capacityBytes()
            , res_h.begin()
            , 0, NULL, NULL);
        clReleaseKernel(kernel); 
        clReleaseMemObject(src_a_d);
        clReleaseMemObject(src_b_d);
        clReleaseMemObject(res_d);
        clFinish(m_vec_instance[i].cmd_queue);
		WATCH_FUNC_END(opencl);
    }

	WATCH_FUNC_BEGIN(for);
    vector_add_cpu(src_a_h.begin(), src_b_h.begin(), res_h.begin(), size);
	WATCH_FUNC_END(for);
}
