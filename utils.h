#ifndef MY_UTILS_H
#define MY_UTILS_H

#include <CL/cl.h>
#include <string>

#include "htio2/Cast.h"

typedef enum {
    BUFFER_MODE_DUMMY = 0,
    BUFFER_MODE_HOST_MAP = 1,
    BUFFER_MODE_DEVICE_MAP = 2,
    BUFFER_MODE_PINNED = 3,
    BUFFER_MODE_INVALID = 255,
} BufferMode;

typedef enum {
    JOB_TYPE_MIXED = 0,
    JOB_TYPE_SINE = 1,
    JOB_TYPE_TANGENT = 2,
    JOB_TYPE_INVALID = 255,
} JobType;

namespace htio2
{
template<>
bool from_string<BufferMode>(const std::string& input, BufferMode& result);

template<>
bool from_string<JobType>(const std::string& input, JobType& result);

template<>
std::string to_string<BufferMode>(BufferMode input);

template<>
std::string to_string<JobType>(JobType input);

} // namespace htio2


void show_all_platforms_and_devices();

bool get_gpu_platform_and_device(cl_platform_id& plat, cl_device_id& dev);

void show_plat_info(cl_platform_id plat);

void show_dev_info(cl_device_id dev, cl_uint& num_dim, size_t*& dim_sizes);

#endif // MY_UTILS_H
