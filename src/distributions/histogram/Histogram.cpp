/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 1999-2008 Soeren Sonnenburg
 * Written (W) 1999-2008 Gunnar Raetsch
 * Copyright (C) 1999-2008 Fraunhofer Institute FIRST and Max-Planck-Society
 */

#include "distributions/histogram/Histogram.h"
#include "lib/common.h"
#include "features/StringFeatures.h"
#include "lib/io.h"
#include "lib/Mathematics.h"


CHistogram::CHistogram()
: CDistribution()
{
	hist=new float64_t[1<<16];
}

CHistogram::CHistogram(CStringFeatures<uint16_t> *f)
: CDistribution()
{
	hist=new float64_t[1<<16];
	features=f;
}

CHistogram::~CHistogram()
{
	delete[] hist;
}

bool CHistogram::train()
{
	int32_t vec;
	int32_t feat;
	int32_t i;

	ASSERT(features);
	ASSERT(features->get_feature_class()==C_STRING);
	ASSERT(features->get_feature_type()==F_WORD);

	for (i=0; i< (int32_t) (1<<16); i++)
		hist[i]=0;

	for (vec=0; vec<features->get_num_vectors(); vec++)
	{
		int32_t len;

		uint16_t* vector=((CStringFeatures<uint16_t>*) features)->
			get_feature_vector(vec, len);

		for (feat=0; feat<len ; feat++)
			hist[vector[feat]]++;
	}

	for (i=0; i< (int32_t) (1<<16); i++)
		hist[i]=log(hist[i]);

	return true;
}

float64_t CHistogram::get_log_likelihood_example(int32_t num_example)
{
	ASSERT(features);
	ASSERT(features->get_feature_class()==C_STRING);
	ASSERT(features->get_feature_type()==F_WORD);

	int32_t len;
	float64_t loglik=0;

	uint16_t* vector=((CStringFeatures<uint16_t>*) features)->
		get_feature_vector(num_example, len);

	for (int32_t i=0; i<len; i++)
		loglik+=hist[vector[i]];

	return loglik;
}

float64_t CHistogram::get_log_derivative(int32_t num_param, int32_t num_example)
{
	if (hist[num_param] < CMath::ALMOST_NEG_INFTY)
		return -CMath::INFTY;
	else
	{
		ASSERT(features);
		ASSERT(features->get_feature_class()==C_STRING);
		ASSERT(features->get_feature_type()==F_WORD);

		int32_t len;
		float64_t deriv=0;

		uint16_t* vector=((CStringFeatures<uint16_t>*) features)->
			get_feature_vector(num_example, len);

		int32_t num_occurences=0;

		for (int32_t i=0; i<len; i++)
		{
			deriv+=hist[vector[i]];

			if (vector[i]==num_param)
				num_occurences++;
		}

		if (num_occurences>0)
			deriv+=log(num_occurences)-hist[num_param];
		else
			deriv=-CMath::INFTY;

		return deriv;
	}
}

float64_t CHistogram::get_log_model_parameter(int32_t num_param)
{
	return hist[num_param];
}

bool CHistogram::set_histogram(float64_t* src, int32_t num)
{
	ASSERT(num==get_num_model_parameters());

	delete[] hist;
	hist=new float64_t[num];
	for (int32_t i=0; i<num; i++) {
		hist[i]=src[i];
	}

	return true;
}

void CHistogram::get_histogram(float64_t** dst, int32_t* num)
{
	*num=get_num_model_parameters();
	size_t sz=sizeof(*hist)*(*num);
	*dst=(float64_t*) malloc(sz);
	ASSERT(dst);

	memcpy(*dst, hist, sz);
}

