/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 1999-2008 Gunnar Raetsch
 * Copyright (C) 1999-2008 Fraunhofer Institute FIRST and Max-Planck-Society
 */

#include "lib/common.h"
#include "lib/io.h"
#include "kernel/SalzbergWordStringKernel.h"
#include "features/Features.h"
#include "features/StringFeatures.h"
#include "features/Labels.h"
#include "classifier/PluginEstimate.h"

CSalzbergWordStringKernel::CSalzbergWordStringKernel(int32_t size, CPluginEstimate* pie, CLabels* labels)
: CStringKernel<uint16_t>(size), estimate(pie), mean(NULL), variance(NULL),
	sqrtdiag_lhs(NULL), sqrtdiag_rhs(NULL),
	ld_mean_lhs(NULL), ld_mean_rhs(NULL),
	num_params(0), num_symbols(0), sum_m2_s2(0), pos_prior(0.5),
	neg_prior(0.5), initialized(false)
{
	if (labels)
		set_prior_probs_from_labels(labels);
}

CSalzbergWordStringKernel::CSalzbergWordStringKernel(
	CStringFeatures<uint16_t>* l, CStringFeatures<uint16_t>* r,
	CPluginEstimate* pie, CLabels* labels)
: CStringKernel<uint16_t>(10),estimate(pie), mean(NULL), variance(NULL),
	sqrtdiag_lhs(NULL), sqrtdiag_rhs(NULL),
	ld_mean_lhs(NULL), ld_mean_rhs(NULL),
	num_params(0), num_symbols(0), sum_m2_s2(0), pos_prior(0.5),
	neg_prior(0.5), initialized(false)
{
	if (labels)
		set_prior_probs_from_labels(labels);

	init(l, r);
}

CSalzbergWordStringKernel::~CSalzbergWordStringKernel()
{
	cleanup();
}

bool CSalzbergWordStringKernel::init(CFeatures* p_l, CFeatures* p_r)
{
	CStringKernel<uint16_t>::init(p_l,p_r);
	CStringFeatures<uint16_t>* l=(CStringFeatures<uint16_t>*) p_l;
	ASSERT(l);
	CStringFeatures<uint16_t>* r=(CStringFeatures<uint16_t>*) p_r;
	ASSERT(r);

	int32_t i;
	initialized=false;

	if (sqrtdiag_lhs!=sqrtdiag_rhs)
		delete[] sqrtdiag_rhs;
	sqrtdiag_rhs=NULL;
	delete[] sqrtdiag_lhs;
	sqrtdiag_lhs=NULL;
	if (ld_mean_lhs!=ld_mean_rhs)
		delete[] ld_mean_rhs;
	ld_mean_rhs=NULL;
	delete[] ld_mean_lhs;
	ld_mean_lhs=NULL;

	sqrtdiag_lhs=new float64_t[l->get_num_vectors()];
	ld_mean_lhs=new float64_t[l->get_num_vectors()];

	for (i=0; i<l->get_num_vectors(); i++)
		sqrtdiag_lhs[i]=1;

	if (l==r)
	{
		sqrtdiag_rhs=sqrtdiag_lhs;
		ld_mean_rhs=ld_mean_lhs;
	}
	else
	{
		sqrtdiag_rhs=new float64_t[r->get_num_vectors()];
		for (i=0; i<r->get_num_vectors(); i++)
			sqrtdiag_rhs[i]=1;

		ld_mean_rhs=new float64_t[r->get_num_vectors()];
	}

	float64_t* l_ld_mean_lhs=ld_mean_lhs;
	float64_t* l_ld_mean_rhs=ld_mean_rhs;

	//from our knowledge first normalize variance to 1 and then norm=1 does the job
	if (!initialized)
	{
		int32_t num_vectors=l->get_num_vectors();
		num_symbols=(int32_t) l->get_num_symbols();
		int32_t llen=l->get_vector_length(0);
		int32_t rlen=r->get_vector_length(0);
		num_params=(int32_t) llen*l->get_num_symbols();
		int32_t num_params2=(int32_t) llen*l->get_num_symbols()+rlen*r->get_num_symbols();
		if ((!estimate) || (!estimate->check_models()))
		{
			SG_ERROR( "no estimate available\n");
			return false ;
		} ;
		if (num_params2!=estimate->get_num_params())
		{
			SG_ERROR( "number of parameters of estimate and feature representation do not match\n");
			return false ;
		} ;

		delete[] variance;
		delete[] mean;
		mean=new float64_t[num_params];
		ASSERT(mean);
		variance=new float64_t[num_params];
		ASSERT(variance);

		for (i=0; i<num_params; i++)
		{
			mean[i]=0;
			variance[i]=0;
		}


		// compute mean
		for (i=0; i<num_vectors; i++)
		{
			int32_t len;
			uint16_t* vec=l->get_feature_vector(i, len);

			for (int32_t j=0; j<len; j++)
			{
				int32_t idx=compute_index(j, vec[j]);
				float64_t theta_p = 1/estimate->log_derivative_pos_obsolete(vec[j], j) ;
				float64_t theta_n = 1/estimate->log_derivative_neg_obsolete(vec[j], j) ;
				float64_t value   = (theta_p/(pos_prior*theta_p+neg_prior*theta_n)) ;

				mean[idx]   += value/num_vectors ;
			}
		}

		// compute variance
		for (i=0; i<num_vectors; i++)
		{
			int32_t len;
			uint16_t* vec=l->get_feature_vector(i, len);

			for (int32_t j=0; j<len; j++)
			{
				for (int32_t k=0; k<4; k++)
				{
					int32_t idx=compute_index(j, k);
					if (k!=vec[j])
						variance[idx]+=mean[idx]*mean[idx]/num_vectors;
					else
					{
						float64_t theta_p = 1/estimate->log_derivative_pos_obsolete(vec[j], j) ;
						float64_t theta_n = 1/estimate->log_derivative_neg_obsolete(vec[j], j) ;
						float64_t value = (theta_p/(pos_prior*theta_p+neg_prior*theta_n)) ;

						variance[idx] += CMath::sq(value-mean[idx])/num_vectors;
					}
				}
			}
		}


		// compute sum_i m_i^2/s_i^2
		sum_m2_s2=0 ;
		for (i=0; i<num_params; i++)
		{
			if (variance[i]<1e-14) // then it is likely to be numerical inaccuracy
				variance[i]=1 ;

			//fprintf(stderr, "%i: mean=%1.2e  std=%1.2e\n", i, mean[i], std[i]) ;
			sum_m2_s2 += mean[i]*mean[i]/(variance[i]) ;
		} ;
	} 

	// compute sum of 
	//result -= feature*mean[a_idx]/variance[a_idx] ;

	for (i=0; i<l->get_num_vectors(); i++)
	{
		int32_t alen ;
		uint16_t* avec=l->get_feature_vector(i, alen);
		float64_t  result=0 ;
		for (int32_t j=0; j<alen; j++)
		{
			int32_t a_idx = compute_index(j, avec[j]) ;
			float64_t theta_p = 1/estimate->log_derivative_pos_obsolete(avec[j], j) ;
			float64_t theta_n = 1/estimate->log_derivative_neg_obsolete(avec[j], j) ;
			float64_t value = (theta_p/(pos_prior*theta_p+neg_prior*theta_n)) ;

			if (variance[a_idx]!=0)
				result-=value*mean[a_idx]/variance[a_idx];
		}
		ld_mean_lhs[i]=result ;
	}

	if (ld_mean_lhs!=ld_mean_rhs)
	{
		// compute sum of 
		//result -= feature*mean[b_idx]/variance[b_idx] ;
		for (i=0; i<r->get_num_vectors(); i++)
		{
			int32_t alen ;
			uint16_t* avec=r->get_feature_vector(i, alen);
			float64_t  result=0 ;
			for (int32_t j=0; j<alen; j++)
			{
				int32_t a_idx = compute_index(j, avec[j]) ;
				float64_t theta_p=1/estimate->log_derivative_pos_obsolete(
					avec[j], j) ;
				float64_t theta_n=1/estimate->log_derivative_neg_obsolete(
					avec[j], j) ;
				float64_t value=(theta_p/(pos_prior*theta_p+neg_prior*theta_n));

				result -= value*mean[a_idx]/variance[a_idx] ;
			}
			ld_mean_rhs[i]=result ;
		} ;
	} ;

	//warning hacky
	//
	this->lhs=l;
	this->rhs=l;
	ld_mean_lhs = l_ld_mean_lhs ;
	ld_mean_rhs = l_ld_mean_lhs ;

	//compute normalize to 1 values
	for (i=0; i<lhs->get_num_vectors(); i++)
	{
		sqrtdiag_lhs[i]=sqrt(compute(i,i));

		//trap divide by zero exception
		if (sqrtdiag_lhs[i]==0)
			sqrtdiag_lhs[i]=1e-16;
	}

	// if lhs is different from rhs (train/test data)
	// compute also the normalization for rhs
	if (sqrtdiag_lhs!=sqrtdiag_rhs)
	{
		this->lhs=r;
		this->rhs=r;
		ld_mean_lhs = l_ld_mean_rhs ;
		ld_mean_rhs = l_ld_mean_rhs ;

		//compute normalize to 1 values
		for (i=0; i<rhs->get_num_vectors(); i++)
		{
			sqrtdiag_rhs[i]=sqrt(compute(i,i));

			//trap divide by zero exception
			if (sqrtdiag_rhs[i]==0)
				sqrtdiag_rhs[i]=1e-16;
		}
	}

	this->lhs=l;
	this->rhs=r;
	ld_mean_lhs = l_ld_mean_lhs ;
	ld_mean_rhs = l_ld_mean_rhs ;

	initialized = true ;
	return init_normalizer();
}

void CSalzbergWordStringKernel::cleanup()
{
	delete[] variance;
	variance=NULL;

	delete[] mean;
	mean=NULL;

	if (sqrtdiag_lhs != sqrtdiag_rhs)
		delete[] sqrtdiag_rhs;
	sqrtdiag_rhs=NULL;

	delete[] sqrtdiag_lhs;
	sqrtdiag_lhs=NULL;

	if (ld_mean_lhs!=ld_mean_rhs)
		delete[] ld_mean_rhs ;
	ld_mean_rhs=NULL;

	delete[] ld_mean_lhs ;
	ld_mean_lhs=NULL;

	CKernel::cleanup();
}

bool CSalzbergWordStringKernel::load_init(FILE* src)
{
	return false;
}

bool CSalzbergWordStringKernel::save_init(FILE* dest)
{
	return false;
}



float64_t CSalzbergWordStringKernel::compute(int32_t idx_a, int32_t idx_b)
{
	int32_t alen, blen;
	uint16_t* avec=((CStringFeatures<uint16_t>*) lhs)->get_feature_vector(idx_a, alen);
	uint16_t* bvec=((CStringFeatures<uint16_t>*) rhs)->get_feature_vector(idx_b, blen);
	// can only deal with strings of same length
	ASSERT(alen==blen);

	float64_t result = sum_m2_s2 ; // does not contain 0-th element

	for (int32_t i=0; i<alen; i++)
	{
		if (avec[i]==bvec[i])
		{
			int32_t a_idx = compute_index(i, avec[i]) ;

			float64_t theta_p = 1/estimate->log_derivative_pos_obsolete(avec[i], i) ;
			float64_t theta_n = 1/estimate->log_derivative_neg_obsolete(avec[i], i) ;
			float64_t value = (theta_p/(pos_prior*theta_p+neg_prior*theta_n)) ;

			result   += value*value/variance[a_idx] ;
		}
	}
	result += ld_mean_lhs[idx_a] + ld_mean_rhs[idx_b] ;


	if (initialized)
		result /=  (sqrtdiag_lhs[idx_a]*sqrtdiag_rhs[idx_b]) ;

	return result;
}

void CSalzbergWordStringKernel::set_prior_probs_from_labels(CLabels* labels)
{
	ASSERT(labels);

	int32_t num_pos=0, num_neg=0;
	for (int32_t i=0; i<labels->get_num_labels(); i++)
	{
		if (labels->get_int_label(i)==1)
			num_pos++;
		if (labels->get_int_label(i)==-1)
			num_neg++;
	}

	SG_INFO("priors: pos=%1.3f (%i)  neg=%1.3f (%i)\n",
		(float64_t) num_pos/(num_pos+num_neg), num_pos,
		(float64_t) num_neg/(num_pos+num_neg), num_neg);

	set_prior_probs(
		(float64_t)num_pos/(num_pos+num_neg),
		(float64_t)num_neg/(num_pos+num_neg));
}
