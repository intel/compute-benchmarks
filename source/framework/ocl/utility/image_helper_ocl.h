/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/ocl/cl.h"
#include "framework/ocl/utility/error.h"
#include "framework/utility/image_helper.h"

struct ImageHelperOcl : ImageHelper {
    static inline cl_mem_object_type getOclImageTypeFromDimensions(const size_t *dimensions) {
        switch (ImageHelper::getImageTypeFromDimensions(dimensions)) {
        case ImageType::Image1D:
            return CL_MEM_OBJECT_IMAGE1D;
        case ImageType::Image2D:
            return CL_MEM_OBJECT_IMAGE2D;
        case ImageType::Image3D:
            return CL_MEM_OBJECT_IMAGE3D;
        case ImageType::Image1D_Array:
            return CL_MEM_OBJECT_IMAGE1D_ARRAY;
        case ImageType::Image2D_Array:
            return CL_MEM_OBJECT_IMAGE2D_ARRAY;
        case ImageType::Image_Buffer:
            return CL_MEM_OBJECT_IMAGE1D_BUFFER;
        default:
            FATAL_ERROR("Unknown image type");
        }
    }

    static inline cl_image_format getImageFormat(ChannelOrder order, ChannelFormat format) {
        cl_image_format imageFormat = {};

        switch (order) {
        case ChannelOrder::R:
            imageFormat.image_channel_order = CL_R;
            break;
        case ChannelOrder::RG:
            imageFormat.image_channel_order = CL_RG;
            break;
        case ChannelOrder::RGBA:
            imageFormat.image_channel_order = CL_RGBA;
            break;
        default:
            FATAL_ERROR("Unknown channel order");
        }

        switch (format) {
        case ChannelFormat::Float:
            imageFormat.image_channel_data_type = CL_FLOAT;
            break;
        default:
            FATAL_ERROR("Unknown channel format");
        }

        return imageFormat;
    }

    static inline bool validateImageDimensions(cl_device_id device, const size_t *dimensions) {
        size_t maxDimensions[3] = {};

        switch (getImageTypeFromDimensions(dimensions)) {
        case ImageType::Image1D:
        case ImageType::Image1D_Array:
            // There's no CL_DEVICE_IMAGE1D_MAX_WIDTH, so using 2D max width
            CL_SUCCESS_OR_RETURN_FALSE(clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_WIDTH, sizeof(size_t), &maxDimensions[0], nullptr));
            return dimensions[0] <= maxDimensions[0];

        case ImageType::Image_Buffer:
            CL_SUCCESS_OR_RETURN_FALSE(clGetDeviceInfo(device, CL_DEVICE_IMAGE_MAX_BUFFER_SIZE, sizeof(size_t), &maxDimensions[0], nullptr));
            return dimensions[0] <= maxDimensions[0];

        case ImageType::Image2D:
        case ImageType::Image2D_Array:
            CL_SUCCESS_OR_RETURN_FALSE(clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_WIDTH, sizeof(size_t), &maxDimensions[0], nullptr));
            CL_SUCCESS_OR_RETURN_FALSE(clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_HEIGHT, sizeof(size_t), &maxDimensions[1], nullptr));
            return dimensions[0] <= maxDimensions[0] &&
                   dimensions[1] <= maxDimensions[1];

        case ImageType::Image3D:
            CL_SUCCESS_OR_RETURN_FALSE(clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_WIDTH, sizeof(size_t), &maxDimensions[0], nullptr));
            CL_SUCCESS_OR_RETURN_FALSE(clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_HEIGHT, sizeof(size_t), &maxDimensions[1], nullptr));
            CL_SUCCESS_OR_RETURN_FALSE(clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_DEPTH, sizeof(size_t), &maxDimensions[2], nullptr));
            return dimensions[0] <= maxDimensions[0] &&
                   dimensions[1] <= maxDimensions[1] &&
                   dimensions[2] <= maxDimensions[2];

        default:
            FATAL_ERROR("Unknown image type");
        }
    }
};
