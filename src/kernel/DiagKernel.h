/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 1999-2008 Soeren Sonnenburg
 * Copyright (C) 1999-2008 Fraunhofer Institute FIRST and Max-Planck-Society
 */

#ifndef _DIAGKERNEL_H___
#define _DIAGKERNEL_H___

#include "lib/common.h"
#include "kernel/Kernel.h"

/** Diagonal Kernel
 *
 * A kernel that returns zero for all non-diagonal elements and a single
 * constant otherwise, i.e.\f$k({\bf x_i}, {\bf x_j})= \delta_{ij} c\f$
 *
 */
class CDiagKernel: public CKernel
{
	public:
		/** constructor
		 *
		 * @param size cache size
		 * @param diag diagonal
		 */
		CDiagKernel(int32_t size, float64_t diag=1.0);

		/** constructor
		 *
		 * @param l features of left-hand side
		 * @param r features of right-hand side
		 * @param diag diagonal
		 */
		CDiagKernel(CFeatures* l, CFeatures* r, float64_t diag=1.0);

		virtual ~CDiagKernel();

		/** load kernel init_data
		 *
		 * @param src file to load from
		 * @return if loading was successful
		 */
		virtual bool load_init(FILE* src);

		/** save kernel init_data
		 *
		 * @param dest file to save to
		 * @return if saving was successful
		 */
		virtual bool save_init(FILE* dest);

		/** return feature type the kernel can deal with
		 *
		 * @return feature type ANY
		 */
		inline virtual EFeatureType get_feature_type()
		{
			return F_ANY;
		}

		/** return feature class the kernel can deal with
		 *
		 * @return feature class ANY
		 */
		inline virtual EFeatureClass get_feature_class()
		{
			return C_ANY;
		}

		/** return what type of kernel we are
		 *
		 * @return kernel type CUSTOM
		 */
		virtual EKernelType get_kernel_type() { return K_DIAG; }

		/** return the kernel's name
		 *
		 * @return name Custom
		 */
		virtual const char* get_name() { return "Diagonal"; }

	protected:
		/** compute kernel function for features a and b
		 * idx_{a,b} denote the index of the feature vectors
		 * in the corresponding feature object
		 *
		 * @param idx_a index a
		 * @param idx_b index b
		 * @return computed kernel function at indices a,b
		 */
		inline virtual float64_t compute(int32_t idx_a, int32_t idx_b)
		{
			if (idx_a==idx_b)
				return diag;
			else
				return 0;
		}

	protected:
		/** diagonal */
		float64_t diag;
};

#endif /* _DIAGKERNEL_H__ */
