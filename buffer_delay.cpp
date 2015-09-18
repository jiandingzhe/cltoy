#include "utils.h"

#include "htio2/OptionParser.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

typedef enum {
    Dummy = 0,
    HostOnly = 1,
    DeviceOnly = 2,
    DualMap = 3,
    Invalid = 255,
} BufferMode;

namespace htio2
{
template<>
bool from_string<BufferMode>(const std::string& input, BufferMode& result)
{
    if (input == "host_only") result = HostOnly;
    else if (input == "device_only") result = DeviceOnly;
    else if (input == "dual_map") result = DualMap;
    else if (input == "dummy") result = Dummy;
    else return false;
    return true;
}

template<>
std::string to_string<BufferMode>(BufferMode input)
{
    switch (input)
    {
    case Dummy: return "dummy";
    case HostOnly: return "host_only";
    case DeviceOnly: return "device_only";
    case DualMap: return "dual_map";
    case Invalid: return "invalid";
    default: abort();
    }
}

}

const char* src =
        "__kernel void hello(__global float* in,\n"
        "                    __global float* out,\n"
        "                    int num_sample,\n"
        "                    int chunk_size)\n"
        "{\n"
        "    int tid = get_global_id(0);\n"
        "    \n"
        "    for (int i = 0; i < chunk_size; i++)\n"
        "    {\n"
        "       int idx = tid * chunk_size + i;\n"
        "       if (idx >= num_sample) break;\n"
        "       out[idx] = in[idx] * in[idx];\n"
        "    }\n"
        "}\n";

BufferMode mode = Invalid;
int num_sample = 1024;
int num_iter = 0;
bool help;
bool do_validate;

htio2::Option opt_mode("mode", 'm', "General Parameters",
                       &mode, 0,
                       "dummy | host_only | device_only | dual_map", "MODE");

htio2::Option opt_num_sample("num-sample", 'n', "General Parameters",
                             &num_sample, 0,
                             "Number of samples to calculate.", "INT");

htio2::Option opt_num_iter("num-iter", 'i', "General Parameters",
                           &num_iter, 0,
                           "Number of times to run.", "INT");

htio2::Option opt_validate("validate", 'V', "General Parameters",
                           &do_validate, 0,
                           "Validate calculated results, which would cost extra time.");

htio2::Option opt_help("help", 'h', "General Parameters",
                       &help, 0,
                       "Show help and exit.");

float* data_input = nullptr;
float* data_result = nullptr;

void* pinned_input = nullptr;
void* pinned_result = nullptr;

size_t dim1_size = 0;

cl_platform_id plat = nullptr;
cl_device_id dev    = nullptr;
cl_context context  = nullptr;
cl_command_queue cmd_queue = nullptr;
cl_program prog = nullptr;
cl_kernel kern = nullptr;


cl_mem buf_input_host  = nullptr;
cl_mem buf_input_dev   = nullptr;
cl_mem buf_result_host = nullptr;
cl_mem buf_result_dev  = nullptr;

int str_to_int(const char* input)
{
    char* end = const_cast<char*>(input);
    int re = std::strtol(input, &end, 0);
    if (input == end)
    {
        std::fprintf(stderr, "failed to cast int using \"%s\"\n", input);
        std::exit(1);
    }
    return re;
}

void parse_arg(int argc, char** argv)
{
    htio2::OptionParser parser;
    parser.add_option(opt_mode);
    parser.add_option(opt_num_sample);
    parser.add_option(opt_num_iter);
    parser.add_option(opt_validate);
    parser.add_option(opt_help);

    parser.parse_options(argc, argv);

    if (help)
    {
        printf("%s\n", parser.format_document().c_str());
        exit(0);
    }

    if (num_sample <= 0)
    {
        fprintf(stderr, "invalid sample number: %d, must > 0\n", num_sample);
        exit(1);
    }

    if (num_iter <= 0)
    {
        fprintf(stderr, "invalid iteration time: %d, must > 0\n", num_iter);
        exit(1);
    }
}

void create_context()
{
    cl_context_properties context_props[] = {
        CL_CONTEXT_PLATFORM, cl_context_properties(plat),
        0, 0
    };
    cl_int err = 0;
    context = clCreateContext(context_props, 1, &dev, nullptr, nullptr, &err);
    if (err != CL_SUCCESS)
    {
        std::fprintf(stderr, "failed to create context: %d\n", err);
        std::exit(1);
    }
}

void create_buffer_object()
{
    cl_int err = 0;
    printf("create buffers in context %p\n", context);

    if (mode == HostOnly)
    {
        err = 0;
        buf_input_host = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, num_sample * sizeof(float), nullptr, &err);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to create host-side input buffer: %d\n", err);
            std::exit(1);
        }

        err = 0;
        buf_result_host = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, num_sample * sizeof(float), nullptr, &err);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to create host-side result buffer: %d\n", err);
            std::exit(1);
        }
    }
    else if (mode == DeviceOnly)
    {
        err = 0;
        buf_input_dev = clCreateBuffer(context, CL_MEM_READ_ONLY, num_sample * sizeof(float), nullptr, &err);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to create device-side input buffer: %d\n", err);
            std::exit(1);
        }

        err = 0;
        buf_result_dev = clCreateBuffer(context, CL_MEM_WRITE_ONLY, num_sample * sizeof(float), nullptr, &err);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to create device-side result buffer: %d\n", err);
            std::exit(1);
        }
    }
    else if (mode == DualMap)
    {
        // input buffers
        err = 0;
        buf_input_host = clCreateBuffer(context, CL_MEM_ALLOC_HOST_PTR, num_sample * sizeof(float), nullptr, &err);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to create host-side input buffer: %d\n", err);
            std::exit(1);
        }

        err = 0;
        buf_input_dev = clCreateBuffer(context, CL_MEM_READ_ONLY, num_sample * sizeof(float), nullptr, &err);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to create device-side input buffer: %d\n", err);
            std::exit(1);
        }

        pinned_input = clEnqueueMapBuffer(cmd_queue, buf_input_host, true, CL_MAP_WRITE,
                                          0, sizeof(float) * num_sample,
                                          0, nullptr, nullptr,
                                          &err);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to map host-side input buffer: %d\n", err);
            std::exit(1);
        }

        // result buffers
        err = 0;
        buf_result_host = clCreateBuffer(context, CL_MEM_ALLOC_HOST_PTR, num_sample * sizeof(float), nullptr, &err);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to create host-side result buffer: %d\n", err);
            std::exit(1);
        }

        err = 0;
        buf_result_dev = clCreateBuffer(context, CL_MEM_WRITE_ONLY, num_sample * sizeof(float), nullptr, &err);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to create device-side buffer: %d\n", err);
            std::exit(1);
        }

        pinned_result = clEnqueueMapBuffer(cmd_queue, buf_result_host, true, CL_MAP_READ,
                                           0, sizeof(float) * num_sample,
                                           0, nullptr, nullptr,
                                           &err);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to map host-side input buffer: %d\n", err);
            std::exit(1);
        }
    }
}

char build_log[8192];
void create_program_kernel()
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
    err = clBuildProgram(prog, 1, &dev, "", nullptr, nullptr);
    if (err != CL_SUCCESS)
    {
        printf("failed to build program: %d\n", err);
        clGetProgramBuildInfo(prog, dev, CL_PROGRAM_BUILD_LOG, 8192, build_log, nullptr);
        printf("%s\n", build_log);
        exit(1);
    }

    printf("create kernel\n");
    err = 0;
    kern = clCreateKernel(prog, "hello", &err);
    if (err != CL_SUCCESS)
    {
        printf("failed to create kernel with error: %d\n", err);
        exit(1);
    }
}

void create_cmd_queue()
{
    cl_int err = 0;
    cmd_queue = clCreateCommandQueue(context, dev, 0, &err);
    if (err != CL_SUCCESS)
    {
        printf("failed to create command queue with error %d\n", err);
        exit(1);
    }
}

void send_input()
{
    cl_int err = 0;

    if (mode == HostOnly)
    {
        pinned_input = clEnqueueMapBuffer(cmd_queue, buf_input_host, true, CL_MAP_WRITE,
                                          0, sizeof(float) * num_sample,
                                          0, nullptr, nullptr,
                                          &err);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to map host-side input buffer: %d\n", err);
            std::exit(1);
        }

        memcpy(pinned_input, data_input, sizeof(float) * num_sample);

        clEnqueueUnmapMemObject(cmd_queue, buf_input_host, pinned_input,
                                0, nullptr, nullptr);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to unmap host-side input buffer: %d\n", err);
            std::exit(1);
        }
    }
    else if (mode == DeviceOnly)
    {
        pinned_input = clEnqueueMapBuffer(cmd_queue, buf_input_dev, true, CL_MAP_WRITE,
                                          0, sizeof(float) * num_sample,
                                          0, nullptr, nullptr,
                                          &err);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to map device-side input buffer: %d\n", err);
            std::exit(1);
        }

        memcpy(pinned_input, data_input, sizeof(float) * num_sample);

        err = clEnqueueUnmapMemObject(cmd_queue, buf_input_dev, pinned_input,
                                      0, nullptr, nullptr);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to unmap device-side input buffer: %d\n", err);
            std::exit(1);
        }
    }
    else if (mode == DualMap)
    {
        memcpy(pinned_input, data_input, sizeof(float) * num_sample);

        err = clEnqueueWriteBuffer(cmd_queue, buf_input_dev, true,
                                   0, sizeof(float) * num_sample, pinned_input,
                                   0, nullptr, nullptr);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to write device-side input buffer: %d\n", err);
            std::exit(1);
        }
    }
    else
    {
        abort();
    }
}

void run()
{
    cl_int err = 0;

    int chunk_size = -1;
    {
        int rem = num_sample % dim1_size;
        chunk_size = (num_sample - rem) / dim1_size;
        if (rem) chunk_size += 1;
    }
    //    printf("%d samples, each chunk %d\n", num_sample, chunk_size);

    if (mode == HostOnly)
    {
        err = clSetKernelArg(kern, 0, sizeof(cl_mem), &buf_input_host);
        if (err != CL_SUCCESS)
        {
            printf("failed to set arg0 using host-side input buffer %p: %d\n", buf_input_host, err);
            exit(1);
        }

        err = clSetKernelArg(kern, 1, sizeof(cl_mem), &buf_result_host);
        if (err != CL_SUCCESS)
        {
            printf("failed to set arg0 using host-side result buffer %p\n: %d", buf_result_host, err);
            exit(1);
        }
    }
    else if (mode == DeviceOnly || mode == DualMap)
    {
        err = clSetKernelArg(kern, 0, sizeof(cl_mem), &buf_input_dev);
        if (err != CL_SUCCESS)
        {
            printf("failed to set arg0 using device-side input buffer %p\n: %d", buf_input_dev, err);
            exit(1);
        }

        err = clSetKernelArg(kern, 1, sizeof(cl_mem), &buf_result_dev);
        if (err != CL_SUCCESS)
        {
            printf("failed to set arg0 using device-side result buffer %p\n: %d", buf_result_dev, err);
            exit(1);
        }
    }

    clSetKernelArg(kern, 2, sizeof(num_sample), &num_sample);
    clSetKernelArg(kern, 3, sizeof(chunk_size), &chunk_size);

    clEnqueueNDRangeKernel(cmd_queue, kern,
                           1,
                           nullptr, &dim1_size,
                           nullptr,
                           0, nullptr, nullptr);
    clFinish(cmd_queue);
}

void fetch_result()
{
    cl_int err = 0;

    if (mode == HostOnly)
    {
        pinned_result = clEnqueueMapBuffer(cmd_queue, buf_result_host, true, CL_MAP_READ,
                                           0, sizeof(float) * num_sample,
                                           0, nullptr, nullptr,
                                           &err);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to map host-side result buffer: %d\n", err);
            exit(1);
        }

        memcpy(data_result, pinned_result, num_sample * sizeof(float));

        err = clEnqueueUnmapMemObject(cmd_queue, buf_result_host, pinned_result,
                                      0, nullptr, nullptr);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to unmap host-side result buffer: %d\n", err);
            exit(1);
        }
    }
    else if (mode == DeviceOnly)
    {
        pinned_result = clEnqueueMapBuffer(cmd_queue, buf_result_dev, true, CL_MAP_READ,
                                           0, sizeof(float) * num_sample,
                                           0, nullptr, nullptr,
                                           &err);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to map device-side result buffer: %d\n", err);
            exit(1);
        }

        memcpy(data_result, pinned_result, num_sample * sizeof(float));

        err = clEnqueueUnmapMemObject(cmd_queue, buf_result_dev, pinned_result,
                                      0, nullptr, nullptr);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to unmap device-side result buffer: %d\n", err);
            exit(1);
        }
    }
    else if (mode == DualMap)
    {
        err = clEnqueueReadBuffer(cmd_queue, buf_result_dev, true,
                                  0, sizeof(float) * num_sample, pinned_result,
                                  0, nullptr, nullptr);
        if (err != CL_SUCCESS)
        {
            std::fprintf(stderr, "failed to read device-side result buffer: %d\n");
            exit(1);
        }

        memcpy(data_result, pinned_result, sizeof(float) * num_sample);
    }
    else
    {
        abort();
    }
}

int main(int argc, char** argv)
{
    parse_arg(argc, argv);

    if (!get_gpu_platform_and_device(plat, dev))
    {
        printf("failed to get GPU device\n");
        exit(1);
    }

    cl_uint num_dim = 0;
    size_t* dim_sizes = nullptr;
    show_plat_info(plat);
    show_dev_info(dev, num_dim, dim_sizes);
    dim1_size = dim_sizes[0];

    create_context();
    create_cmd_queue();
    create_buffer_object();
    create_program_kernel();

    // initialize input data
    data_input = (float*) malloc(num_sample * sizeof(float));
    data_result = (float*) malloc(num_sample * sizeof(float));
    for (int i = 0; i < num_sample; i++)
        data_input[i] = i;

    // run
    for (int cycle = 0; cycle < num_iter; cycle++)
    {
        //        printf("cycle %d\n", cycle);
        send_input();
        run();
        fetch_result();

        // validate result
        if (do_validate)
        {
            for (int i = 0; i < num_sample; i++)
            {
                float expect = float(i) * float(i);
                if (data_result[i] != expect)
                {
                    fprintf(stderr, "result data at %d is %f, which is not %f\n", i, data_result[i], expect);
                    abort();
                }
            }
        }

        // clear result store
        for (int i = 0; i < num_sample; i++)
            data_result[i] = 0.0f;
    }


    free(data_input);
    free(data_result);
}

