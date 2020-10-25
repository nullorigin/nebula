#pragma once
//------------------------------------------------------------------------------
/**
    ColumnDescription

    Describes a context state's type and default value.

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "util/fourcc.h"
#include "util/stringatom.h"
#include "util/hashtable.h"
#include "table.h"
#include "columndescriptor.h"

namespace MemDb
{

class ColumnDescription
{
public:
    template<typename T>
    explicit ColumnDescription(Util::StringAtom name, T const& defaultValue)
    {
        this->typeSize = sizeof(T);
        this->name = name;
        this->defVal = Memory::Alloc(Memory::HeapType::ObjectHeap, sizeof(T));
        Memory::Copy(&defaultValue, this->defVal, sizeof(T));
    }
    ColumnDescription()
    {
        // empty
    }
    ColumnDescription(ColumnDescription&& desc) : defVal(desc.defVal), typeSize(desc.typeSize), name(desc.name)
    {
        desc.defVal = nullptr;
        desc.name = nullptr;
        desc.typeSize = 0;
    }
    ~ColumnDescription()
    {
        if (this->defVal != nullptr)
        {
            Memory::Free(Memory::HeapType::ObjectHeap, this->defVal);
        }
    }
    void operator=(ColumnDescription&& rhs) noexcept
    {
        this->typeSize = rhs.typeSize;
        this->defVal = rhs.defVal;
        this->name = rhs.name;

        rhs.typeSize = 0;
        rhs.defVal = nullptr;
        rhs.name = nullptr;
    }
    void operator=(ColumnDescription& rhs)
    {
        this->typeSize = rhs.typeSize;
        this->defVal = rhs.defVal;
        this->name = rhs.name;
        
        rhs.typeSize = 0;
        rhs.defVal = nullptr;
        rhs.name = nullptr;
    }

    Util::StringAtom name;
    SizeT typeSize = 0;
    void* defVal = nullptr;
    using TableRegistry = Util::HashTable<TableId, void**, 32, 1>;
    // direct access to all buffers within tables
    TableRegistry tableRegistry;
};

} // namespace MemDb
