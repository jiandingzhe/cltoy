#include <CL/cl.h>
#include <cstdio>

#include "utils.h"

using std::printf;

const char* src =
        "__kernel void hello(__global float* in1,"
        "                    __global float* in2,"
        "                    __global float* out)"
        "{\n"
        "    tid = get_global_id(0);\n"
        "    out[tid] = in1[tid] * in2[tid];\n"
        "}\n";

int main(int argc, char** argv)
{
    show_all_platforms_and_devices();

    cl_platform_id plat = nullptr;
    cl_device_id dev = nullptr;
    if (!get_gpu_platform_and_device(plat, dev))
    {
        printf("failed to get GPU platform and device!\n");
        exit(1);
    }

    // create context using GPU
    printf("create context\n");
    cl_int context_err = 0;

    cl_context_properties context_props[] = {
        CL_CONTEXT_PLATFORM, cl_context_properties(plat),
        0, 0
    };
    cl_context context = clCreateContext(context_props, 1, &dev, nullptr, nullptr, &context_err);

    if (context_err != CL_SUCCESS)
    {
        printf("failed to create context, error %d\n", context_err);
        exit(1);
    }

    // command queue
    cl_command_queue cmd_queue = nullptr;
    {
        cl_int err = 0;
        cmd_queue = clCreateCommandQueue(context, dev, 0, &err);
        if (err != CL_SUCCESS)
        {
            printf("failed to create command queue with error %d\n", err);
            exit(1);
        }
    }

    // create program
    cl_program prog = nullptr;
    cl_kernel kern = nullptr;
    {
        printf("create program\n");
        cl_int err = 0;
        prog = clCreateProgramWithSource(context, 1, &src, nullptr, &err);
        if (err != CL_SUCCESS)
        {
            printf("failed to create program with error: %d\n", err);
            exit(1);
        }

        printf("build program\n");

        printf("create kernel\n");
        err = 0;
        kern = clCreateKernel(prog, "hello", &err);
        if (err != CL_SUCCESS)
        {
            printf("failed to create kernel with error: %d\n", err);
            exit(1);
        }
    }


    // create buffer objects
    printf("create buffers\n");
    cl_mem in_a = nullptr;
    cl_mem in_b = nullptr;
    cl_mem out = nullptr;

    void* ptr_in_a = nullptr;
    void* ptr_in_b = nullptr;
    void* ptr_out = nullptr;

    {
        cl_int err = 0;
        in_a = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(float) * 16384, nullptr, &err);
        if (err != CL_SUCCESS)
        {
            printf("create buffer failed with error %d\n", err);
            exit(1);
        }
    }
    {
        cl_int err = 0;
        in_b = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(float) * 16384, nullptr, &err);
        if (err != CL_SUCCESS)
        {
            printf("create buffer failed with error %d\n", err);
            exit(1);
        }
    }
    {
        cl_int err = 0;
        out = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, sizeof(float) * 16384, nullptr, &err);
        if (err != CL_SUCCESS)
        {
            printf("create buffer failed with error %d\n", err);
            exit(1);
        }
    }


    // fill input

    // run

    // fetch output
}

