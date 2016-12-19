/*
 * Copyright 1993-2014 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

/*
* Copyright 1993-2014 NVIDIA Corporation.  All rights reserved.
*
* Please refer to the NVIDIA end user license agreement (EULA) associated
* with this source code for terms and conditions that govern your use of
* this software. Any use, reproduction, disclosure, or distribution of
* this software and related documentation outside the terms of the EULA
* is strictly prohibited.
*
*/

/**
**************************************************************************
* \file Common.h
* \brief Common includes header.
*
* This file contains includes of all libraries used by the project.
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <cuda_runtime.h>
#include <helper_cuda.h>       // helper functions for CUDA timing and initialization
#include <helper_functions.h>  // helper functions for timing, string parsing

#include "defines.h"

/**
*  This macro allows using aligned memory management
*/
//#define __ALLOW_ALIGNED_MEMORY_MANAGEMENT
