#include "utils.h"

#include <cstdio>
#include <cstdlib>

void show_all_platforms_and_devices()
{
    cl_platform_id plats[256];
    cl_uint num_plat = -1;
    clGetPlatformIDs(256, plats, &num_plat);

    printf("system has %u platforms\n", num_plat);

    char info[1024];
    size_t info_size = -1;

    for (int i_plat = 0; i_plat < num_plat; i_plat++)
    {

        clGetPlatformInfo(plats[i_plat], CL_PLATFORM_NAME, 1024, info, &info_size);
        printf("    platform %s\n", info);

        clGetPlatformInfo(plats[i_plat], CL_PLATFORM_VERSION, 1024, info, &info_size);
        printf("    version: %s\n", info);

        clGetPlatformInfo(plats[i_plat], CL_PLATFORM_VENDOR, 1024, info, &info_size);
        printf("    vendor: %s\n", info);

        printf("\n");

        cl_device_id devs[128];
        cl_uint num_dev = -1;
        {
            cl_int re = clGetDeviceIDs(plats[i_plat], CL_DEVICE_TYPE_ALL, 128, devs, &num_dev);
            if (re != CL_SUCCESS)
            {
                if (re == CL_DEVICE_NOT_FOUND)
                {
                    printf("        no device\n");
                    continue;
                }
                printf("failed to get devices: error %d\n", re);
                exit(1);
            }
        }


        for (int i_dev = 0; i_dev < num_dev; i_dev++)
        {
//            clGetDeviceInfo(devs[i_dev], CL_DEVICE_TYPE, 1024, info, &info_size);
//            printf("    %s", info);

            clGetDeviceInfo(devs[i_dev], CL_DEVICE_NAME, 1024, info, &info_size);
            printf("        device %s\n", info);

            clGetDeviceInfo(devs[i_dev], CL_DEVICE_VERSION, 1024, info, &info_size);
            printf("        version: %s\n", info);

            clGetDeviceInfo(devs[i_dev], CL_DEVICE_VENDOR, 1024, info, &info_size);
            printf("        vendor: %s\n", info);

            printf("\n");
        }


    }
}

bool get_gpu_platform_and_device(cl_platform_id& plat, cl_device_id& dev)
{
    cl_platform_id plats[256];
    cl_uint num_plat = -1;
    {
        cl_int re = clGetPlatformIDs(256, plats, &num_plat);
        if (re != CL_SUCCESS)
        {
            fprintf(stderr, "failed to get platforms: error %d\n", re);
            std::abort();
        }
    }

    for (int i_plat = 0; i_plat < num_plat; i_plat++)
    {
        cl_device_id devs[128];
        cl_uint num_dev = -1;

        {
            cl_int re = clGetDeviceIDs(plats[i_plat], CL_DEVICE_TYPE_GPU, 128, devs, &num_dev);
            if (re != CL_SUCCESS)
                continue;
        }

        plat = plats[i_plat];
        dev = devs[0];
        return true;
    }


    return false;
}


void show_plat_info(cl_platform_id plat)
{
    char plat_name[256];
    char plat_version[256];
    char plat_vendor[256];
    clGetPlatformInfo(plat, CL_PLATFORM_NAME, 256, plat_name, nullptr);
    clGetPlatformInfo(plat, CL_PLATFORM_VERSION, 256, plat_version, nullptr);
    clGetPlatformInfo(plat, CL_PLATFORM_VENDOR, 256, plat_vendor, nullptr);

    printf("platform %p\n"
           "  name   : %s\n"
           "  version: %s\n"
           "  vendor : %s\n",
           plat, plat_name, plat_version, plat_vendor);
}

void show_dev_info(cl_device_id dev, cl_uint& num_dim, size_t*& dim_sizes)
{
    char dev_name[256];
    char dev_version[256];
    char dev_vendor[256];

    clGetDeviceInfo(dev, CL_DEVICE_NAME, 256, dev_name, nullptr);
    clGetDeviceInfo(dev, CL_DEVICE_VERSION, 256, dev_version, nullptr);
    clGetDeviceInfo(dev, CL_DEVICE_VENDOR, 256, dev_vendor, nullptr);

    printf("device %p\n"
           "  name   : %s\n"
           "  version: %s\n"
           "  vendor : %s\n",
           dev, dev_name, dev_version, dev_vendor);


    clGetDeviceInfo(dev, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &num_dim, nullptr);
    printf("  allowed dimensions: %u\n", num_dim);

    dim_sizes = (size_t*) malloc(num_dim * sizeof(size_t));
    clGetDeviceInfo(dev, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t) * num_dim, dim_sizes, nullptr);
    for (int i = 0; i < num_dim; i++)
    {
        printf("    %d: %lu\n", i+1, dim_sizes[i]);
    }
}
