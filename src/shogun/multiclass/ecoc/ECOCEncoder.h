/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 2012 Chiyuan Zhang
 * Copyright (C) 2012 Chiyuan Zhang
 */

#ifndef ECOCENCODER_H__
#define ECOCENCODER_H__

#include <shogun/base/SGObject.h>
#include <shogun/lib/DataType.h>

namespace shogun
{

class CECOCEncoder: public CSGObject
{
public:
    /** constructor */
    CECOCEncoder() {}

    /** destructor */
    virtual ~CECOCEncoder() {}

    /** get name */
    virtual const char* get_name() const
    {
        return "ECOCEncoder";
    }

    /** init codebook.
     * @param num_classes number of classes in this problem
     */
    virtual SGMatrix<int32_t> create_codebook(int32_t num_classes)=0;
};

}

#endif /* end of include guard: ECOCENCODER_H__ */