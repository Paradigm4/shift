/*
**
* BEGIN_COPYRIGHT
*
* Copyright (C) 2008-2016 SciDB, Inc.
* All Rights Reserved.
*
* shift is a plugin for SciDB, an Open Source Array DBMS maintained
* by Paradigm4. See http://www.paradigm4.com/
*
* shift is free software: you can redistribute it and/or modify
* it under the terms of the AFFERO GNU General Public License as published by
* the Free Software Foundation.
*
* shift is distributed "AS-IS" AND WITHOUT ANY WARRANTY OF ANY KIND,
* INCLUDING ANY IMPLIED WARRANTY OF MERCHANTABILITY,
* NON-INFRINGEMENT, OR FITNESS FOR A PARTICULAR PURPOSE. See
* the AFFERO GNU General Public License for the complete license terms.
*
* You should have received a copy of the AFFERO GNU General Public License
* along with shift.  If not, see <http://www.gnu.org/licenses/agpl-3.0.html>
*
* END_COPYRIGHT
*/

#include "log4cxx/logger.h"
#include <query/Operator.h>
#include <set>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
namespace scidb
{

class LogicalShift : public LogicalOperator
{
public:
    LogicalShift(const string& logicalName,const string& alias)
     : LogicalOperator(logicalName,alias)
    {
        ADD_PARAM_INPUT();
        ADD_PARAM_IN_DIMENSION_NAME();
        ADD_PARAM_CONSTANT("int64");
    }

    ArrayDesc inferSchema(vector< ArrayDesc> schemas,std::shared_ptr<Query> query)
    {
        std::shared_ptr<OperatorParamReference> const& reference = (std::shared_ptr<OperatorParamReference> const&) _parameters[0];
        string const& dimName = reference->getObjectName();
        string const& dimAlias = reference->getArrayName();
        Dimensions const& inputDims = schemas[0].getDimensions();
        size_t whichDim = 0;
        for (size_t j = 0, n = inputDims.size(); j < n; j++)
        {
            if (inputDims[j].hasNameAndAlias(dimName, dimAlias))
            {
                whichDim = j;
            }
        }
        DimensionDesc const& shiftedDim = inputDims[whichDim];
        int64_t shiftSize = evaluate(((std::shared_ptr<OperatorParamLogicalExpression>&)_parameters[1])->getExpression(), query, TID_INT64).getInt64();
        if(shiftSize % shiftedDim.getChunkInterval() != 0)
        {
          throw SYSTEM_EXCEPTION(SCIDB_SE_INTERNAL, SCIDB_LE_ILLEGAL_OPERATION) << "Shift size must be a multiple of chunk length";
        }
        return ArrayDesc("",
                         schemas[0].getAttributes(false),
                         schemas[0].getDimensions(),
                         createDistribution(psUndefined),
                         schemas[0].getResidency());
    }
};

REGISTER_LOGICAL_OPERATOR_FACTORY(LogicalShift, "shift");

} //namespace
