#ifndef MY_UTILS_H
#define MY_UTILS_H


#include <CL/cl.h>

void show_all_platforms_and_devices();

bool get_gpu_platform_and_device(cl_platform_id& plat, cl_device_id& dev);

void show_plat_info(cl_platform_id plat);

void show_dev_info(cl_device_id dev, cl_uint& num_dim, size_t*& dim_sizes);

#endif // MY_UTILS_H
