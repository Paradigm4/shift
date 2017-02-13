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

#include <array/Metadata.h>
#include <boost/array.hpp>
#include <system/SystemCatalog.h>
#include <query/Operator.h>
#include <log4cxx/logger.h>
#include <array/TransientCache.h>
#include <util/session/Session.h>
#include <array/DelegateArray.h>

using namespace std;


namespace scidb
{

//TODO: careful - there's already a ShiftArray/ family used by reshape
class PositionShiftArrayIterator : public DelegateArrayIterator
{
private:
    size_t  const    _whichDim;
    int64_t const    _shift;
    Array const*     _arr;
    ArrayDesc _schema;
    MemChunk _chunk;
    Coordinates _outPos;

public:
    PositionShiftArrayIterator(DelegateArray const& delegate, AttributeID attrID, std::shared_ptr<ConstArrayIterator> inputIterator,
                       size_t const& whichDim, int64_t const&  shift):
        DelegateArrayIterator(delegate, attrID, inputIterator),
        _whichDim(whichDim),
        _shift(shift),
        _arr(&delegate),
        _schema(_arr->getArrayDesc())
    {}

    ConstChunk const& getChunk() override
    {
        ConstChunk const& inputChunk = inputIterator->getChunk();
        ConstChunk* mc = inputChunk.materialize();
        _outPos = inputIterator->getPosition();
        _outPos[_whichDim] += _shift;
        Address addr(attr, _outPos);
        _chunk.initialize(_arr, &_schema, addr, 0);
        PinBuffer scope(_chunk);
        PinBuffer scope2(*mc);
        _chunk.allocate(mc->getSize());
        memcpy(_chunk.getData(), mc->getData(), mc->getSize());
        return _chunk;
    }

    Coordinates const& getPosition() override
    {
        _outPos = inputIterator->getPosition();
        _outPos[_whichDim] += _shift;
        return _outPos;
    }

    bool setPosition(Coordinates const& pos) override
    {
        _outPos = pos;
        _outPos[_whichDim] -= _shift;
        return(inputIterator->setPosition(_outPos));
    }
};

class PositionShiftArray : public DelegateArray
{
private:
    size_t  const    _whichDim;
    int64_t const    _shift;

public:
    PositionShiftArray(ArrayDesc const& desc, std::shared_ptr<Array> input, size_t const whichDim, int64_t const shift):
        DelegateArray(desc, input),
        _whichDim(whichDim),
        _shift(shift)
    {}

    virtual ~PositionShiftArray()
    {}

    virtual DelegateArrayIterator* createArrayIterator(AttributeID id) const
    {
        return new PositionShiftArrayIterator(*this, id, inputArray->getConstIterator(id), _whichDim, _shift);
    }
};



class PhysicalShift : public PhysicalOperator
{
public:
    PhysicalShift(const string& logicalName,
                                const string& physicalName,
                                const Parameters& parameters,
                                const ArrayDesc& schema)
     : PhysicalOperator(logicalName,physicalName,parameters,schema)
    {}

    virtual bool changesDistribution(std::vector<ArrayDesc> const& sourceSchemas) const
    {
        return true;
    }

    virtual RedistributeContext getOutputDistribution(std::vector<RedistributeContext> const&,
                                                        std::vector<ArrayDesc> const&) const
    {
      std::shared_ptr<Query> query(Query::getValidQueryPtr(_query));
      SCIDB_ASSERT(query);
      ArrayDesc* mySchema = const_cast<ArrayDesc*>(&_schema);
      mySchema->setResidency(query->getDefaultArrayResidency());
      SCIDB_ASSERT(_schema.getDistribution()->getPartitioningSchema() == psUndefined);
      return RedistributeContext(_schema.getDistribution(),
                                 _schema.getResidency());
    }

    virtual std::shared_ptr<Array> execute(vector<shared_ptr<Array> >& inputs, shared_ptr<Query> query)
    {
        shared_ptr<Array> & input = inputs[0];
        ArrayDesc const& inputSchema = input->getArrayDesc();
        Dimensions const& inputDims = inputSchema.getDimensions();
        std::shared_ptr<OperatorParamReference> const& reference = (std::shared_ptr<OperatorParamReference> const&) _parameters[0];
        string const& dimName = reference->getObjectName();
        string const& dimAlias = reference->getArrayName();
        size_t whichDim = 0;
        for (size_t j = 0; j < inputDims.size(); j++)
        {
            if (inputDims[j].hasNameAndAlias(dimName, dimAlias))
            {
                whichDim = j;
                break;
            }
        }
        int64_t shift = ((std::shared_ptr<OperatorParamPhysicalExpression>&)_parameters[1])->getExpression()->evaluate().getInt64();
        //return shared_ptr<Array>(new DelegateArray(_schema, input));
        return shared_ptr<Array>(new PositionShiftArray(_schema, input, whichDim, shift));
    }
};

REGISTER_PHYSICAL_OPERATOR_FACTORY(PhysicalShift, "shift", "PhysicalShift");

}


