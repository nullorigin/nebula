//------------------------------------------------------------------------------
//  api.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "api.h"
#include "gameserver.h"
#include "ids/idallocator.h"
#include "memdb/tablesignature.h"
#include "memdb/database.h"
#include "memory/arenaallocator.h"
#include "basegamefeature/managers/blueprintmanager.h"
#include "profiling/profiling.h"

namespace Game
{

//------------------------------------------------------------------------------
using InclusiveTableMask = MemDb::TableSignature;
using ExclusiveTableMask = MemDb::TableSignature;
using PropertyArray = Util::FixedArray<PropertyId>;
using AccessModeArray = Util::FixedArray<AccessMode>;

static Ids::IdAllocator<InclusiveTableMask, ExclusiveTableMask, PropertyArray, AccessModeArray> filterAllocator;
static Memory::ArenaAllocator<sizeof(Dataset::CategoryTableView) * 256> viewAllocator;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
using RegPidQueue = Util::Queue<Op::RegisterProperty>;
using DeregPidQueue = Util::Queue<Op::DeregisterProperty>;

static Ids::IdAllocator<RegPidQueue, DeregPidQueue> opBufferAllocator;
static Memory::ArenaAllocator<1024> opAllocator;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
*/
Ptr<MemDb::Database>
GetWorldDatabase()
{
    n_assert(GameServer::HasInstance());
    return GameServer::Singleton->state.world.db;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
CreateEntity(EntityCreateInfo const& info)
{
    n_assert(GameServer::HasInstance());
    GameServer::State* const state = &GameServer::Singleton->state;

    World& world = state->world;

    Entity entity;
    world.pool.Allocate(entity);
    world.numEntities++;

    // Make sure the entitymap can contain this entity
    if (world.entityMap.Size() <= entity.index)
    {
        world.entityMap.Grow();
        world.entityMap.Resize(world.entityMap.Capacity());
    }

    World::AllocateInstanceCommand cmd;
    cmd.entity = entity;
    if (info.templateId != TemplateId::Invalid())
    {
        cmd.tid = info.templateId;
    }
    else
    {
        cmd.tid.blueprintId = info.blueprint.id;
        cmd.tid.templateId = Ids::InvalidId16;
    }

    if (!info.immediate)
    {
        world.allocQueue.Enqueue(std::move(cmd));
    }
    else
    {
        if (cmd.tid.templateId != Ids::InvalidId16)
        {
            world.AllocateInstance(cmd.entity, cmd.tid);
        }
        else
        {
            world.AllocateInstance(cmd.entity, (BlueprintId)cmd.tid.blueprintId);
        }
    }

    return entity;
}

//------------------------------------------------------------------------------
/**
*/
void
DeleteEntity(Game::Entity entity)
{
    n_assert(IsValid(entity));
    n_assert(GameServer::HasInstance());
    GameServer::State* const state = &GameServer::Singleton->state;

    // make sure we're not trying to dealloc an instance that does not exist
    n_assert2(IsActive(entity), "Cannot delete and entity before it has been instantiated!\n");

    World& world = state->world;

    world.pool.Deallocate(entity);

    World::DeallocInstanceCommand cmd;
    cmd.entity = entity;

    world.deallocQueue.Enqueue(std::move(cmd));

    world.numEntities--;
}

//------------------------------------------------------------------------------
/**
*/
OpBuffer
CreateOpBuffer()
{
    return opBufferAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
    @todo   We can bundle all add and remove property for each entity into one
            migration.
            We can also batch them based on their new category, so we won't
            need to do as many column id lookups.
    @todo   This is not thread safe. Either, we keep it like it is and make sure
            this function is always called synchronously, or add a critical section?
*/
void
Dispatch(OpBuffer& buffer)
{
    RegPidQueue& registerPropertyQueue = opBufferAllocator.Get<0>(buffer);
    DeregPidQueue& deregisterPropertyQueue = opBufferAllocator.Get<1>(buffer);

    while (!registerPropertyQueue.IsEmpty())
    {
        auto op = registerPropertyQueue.Dequeue();
        Game::Execute(op);
    }

    while (!deregisterPropertyQueue.IsEmpty())
    {
        auto op = deregisterPropertyQueue.Dequeue();
        Game::Execute(op);
    }

    opBufferAllocator.Dealloc(buffer);
    buffer = InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    @todo   optimize
*/
void
AddOp(OpBuffer buffer, Op::RegisterProperty op)
{
    if (op.value != nullptr)
    {
        SizeT const typeSize = MemDb::TypeRegistry::TypeSize(op.pid);
        void* value = opAllocator.Alloc(typeSize);
        Memory::Copy(op.value, value, typeSize);
        op.value = value;
    }
    opBufferAllocator.Get<0>(buffer).Enqueue(op);
}

//------------------------------------------------------------------------------
/**
*/
void
AddOp(OpBuffer buffer, Op::DeregisterProperty const& op)
{
    opBufferAllocator.Get<1>(buffer).Enqueue(op);
}

//------------------------------------------------------------------------------
/**
*/
void
Execute(Op::RegisterProperty const& op)
{
    GameServer::State* const state = &GameServer::Singleton->state;
    EntityMapping mapping = GetEntityMapping(op.entity);
    World& world = GameServer::Singleton->state.world;
    Category const& cat = world.GetCategory(mapping.category);
    CategoryHash newHash = cat.hash;
    newHash.AddToHash(op.pid.id);
    CategoryId newCategoryId;
    if (world.catIndexMap.Contains(newHash))
    {
        newCategoryId = world.catIndexMap[newHash];
    }
    else
    {
        // Category with this hash does not exist. Create a new category.
        CategoryCreateInfo info;
        auto const& cols = world.db->GetTable(cat.instanceTable).properties;
        info.properties.SetSize(cols.Size() + 1);
        IndexT i;
        for (i = 0; i < cols.Size(); ++i)
        {
            info.properties[i] = cols[i];
        }
        info.properties[i] = op.pid;

#ifdef NEBULA_DEBUG
        info.name = cat.name + " + ";
        info.name += MemDb::TypeRegistry::GetDescription(op.pid)->name.AsString();
#endif
        newCategoryId = world.CreateCategory(info);
    }

    InstanceId newInstance = world.Migrate(op.entity, newCategoryId);

    if (op.value == nullptr)
        return; // default value should already be set

    Category const& newCat = world.GetCategory(newCategoryId);
    auto cid = world.db->GetColumnId(newCat.instanceTable, op.pid);
    void* ptr = world.db->GetValuePointer(newCat.instanceTable, cid, newInstance.id);
    Memory::Copy(op.value, ptr, MemDb::TypeRegistry::TypeSize(op.pid));
}

//------------------------------------------------------------------------------
/**
    @bug   If you deregister a managed property, the property will just disappear
           without letting the manager clean up any resources, leading to memleaks.
*/
void
Execute(Op::DeregisterProperty const& op)
{
#if NEBULA_DEBUG
    n_assert(Game::HasProperty(op.entity, op.pid));
#endif
    GameServer::State* const state = &GameServer::Singleton->state;
    EntityMapping mapping = GetEntityMapping(op.entity);
    World& world = GameServer::Singleton->state.world;
    Category const& cat = world.GetCategory(mapping.category);
    CategoryHash newHash = cat.hash;
    newHash.RemoveFromHash(op.pid.id);
    CategoryId newCategoryId;
    if (world.catIndexMap.Contains(newHash))
    {
        newCategoryId = world.catIndexMap[newHash];
    }
    else
    {
        CategoryCreateInfo info;
        auto const& cols = world.db->GetTable(cat.instanceTable).properties;
        SizeT const num = cols.Size();
        info.properties.SetSize(num - 1);
        int col = 0;
        for (int i = 0; i < num; ++i)
        {
            if (cols[i] == op.pid)
                continue;

            info.properties[col++] = cols[i];
        }

#ifdef NEBULA_DEBUG
        info.name = cat.name + " - ";
        info.name += MemDb::TypeRegistry::GetDescription(op.pid)->name.AsString();
#endif

        newCategoryId = world.CreateCategory(info);
    }

    world.Migrate(op.entity, newCategoryId);
}

//------------------------------------------------------------------------------
/**
*/
void
ReleaseAllOps()
{
    opAllocator.Release();
}

//------------------------------------------------------------------------------
/**
*/
Filter
CreateFilter(FilterCreateInfo const& info)
{
    n_assert(info.numInclusive > 0);
    uint32_t filter = filterAllocator.Alloc();

    PropertyArray inclusiveArray;
    inclusiveArray.Resize(info.numInclusive);
    for (uint8_t i = 0; i < info.numInclusive; i++)
    {
        inclusiveArray[i] = info.inclusive[i];
    }

    PropertyArray exclusiveArray;
    exclusiveArray.Resize(info.numExclusive);
    for (uint8_t i = 0; i < info.numExclusive; i++)
    {
        exclusiveArray[i] = info.exclusive[i];
    }

    AccessModeArray accessArray;
    accessArray.Resize(info.numInclusive);
    for (uint8_t i = 0; i < info.numInclusive; i++)
    {
        accessArray[i] = info.access[i];
    }

    filterAllocator.Set(filter,
        InclusiveTableMask(inclusiveArray),
        ExclusiveTableMask(exclusiveArray),
        inclusiveArray,
        accessArray
    );

    return filter;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyFilter(Filter filter)
{
    filterAllocator.Dealloc(filter);
}

//------------------------------------------------------------------------------
/**
*/
ProcessorHandle
CreateProcessor(ProcessorCreateInfo const& info)
{
    return Game::GameServer::Instance()->CreateProcessor(info);
}

//------------------------------------------------------------------------------
/**
*/
void
ReleaseDatasets()
{
    viewAllocator.Release();
}

//------------------------------------------------------------------------------
/**
    @returns    Dataset with category table views.

    @note       The category table view buffer can be NULL if the filter contains
                a non-typed/flag property.
*/
Dataset Query(Filter filter)
{
#if NEBULA_ENABLE_PROFILING
    //N_COUNTER_INCR("Calls to Game::Query", 1);
    N_SCOPE_ACCUM(QueryTime, EntitySystem);
#endif
    Ptr<MemDb::Database> db = Game::GetWorldDatabase();

    Util::Array<MemDb::TableId> tids = db->Query(filterAllocator.Get<0>(filter), filterAllocator.Get<1>(filter));

    Dataset data;
    data.numViews = tids.Size();

    if (tids.Size() == 0)
    {
        data.views = nullptr;
        return data;
    }

    data.views = (Dataset::CategoryTableView*)viewAllocator.Alloc(sizeof(Dataset::CategoryTableView) * data.numViews);

    PropertyArray const& properties = filterAllocator.Get<2>(filter);

    for (IndexT viewIndex = 0; viewIndex < data.numViews; viewIndex++)
    {
        Dataset::CategoryTableView* view = data.views + viewIndex;
        // FIXME
        view->cid = CategoryId::Invalid();

        MemDb::Table const& tbl = db->GetTable(tids[viewIndex]);

        IndexT i = 0;
        for (auto pid : properties)
        {
            MemDb::ColumnIndex colId = db->GetColumnId(tbl.tid, pid);
            // Check if the property is a flag, and return a nullptr in that case.
            if (colId != InvalidIndex)
                view->buffers[i] = db->GetBuffer(tbl.tid, colId);
            else
                view->buffers[i] = nullptr;
            i++;
        }

        view->numInstances = db->GetNumRows(tbl.tid);
    }

    return data;
}

//------------------------------------------------------------------------------
/**
*/
bool
IsValid(Entity e)
{
    n_assert(GameServer::HasInstance());
    return GameServer::Singleton->state.world.pool.IsValid(e);
}

//------------------------------------------------------------------------------
/**
*/
bool
IsActive(Entity e)
{
    n_assert(GameServer::HasInstance());
    n_assert(IsValid(e));
    return GameServer::Singleton->state.world.entityMap[e.index].instance != InstanceId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
uint
GetNumEntities()
{
    n_assert(GameServer::HasInstance());
    return GameServer::Singleton->state.world.numEntities;
}


//------------------------------------------------------------------------------
/**
*/
bool
CategoryExists(CategoryHash hash)
{
    n_assert(GameServer::HasInstance());
    return GameServer::Singleton->state.world.catIndexMap.Contains(hash);
}

//------------------------------------------------------------------------------
/**
*/
CategoryId const
GetCategoryId(CategoryHash hash)
{
    n_assert(GameServer::HasInstance());
    n_assert(GameServer::Singleton->state.world.catIndexMap.Contains(hash));
    return GameServer::Singleton->state.world.catIndexMap[hash];
}

//------------------------------------------------------------------------------
/**
*/
EntityMapping
GetEntityMapping(Game::Entity entity)
{
    n_assert(GameServer::HasInstance());
    n_assert(IsActive(entity));
    return GameServer::Singleton->state.world.entityMap[entity.index];
}

//------------------------------------------------------------------------------
/**
*/
PropertyId
CreateProperty(PropertyCreateInfo const& info)
{
    return MemDb::TypeRegistry::Register(info.name, info.byteSize, info.defaultValue, info.flags);
}

//------------------------------------------------------------------------------
/**
*/
PropertyId
GetPropertyId(Util::StringAtom name)
{
    return MemDb::TypeRegistry::GetPropertyId(name);
}

//------------------------------------------------------------------------------
/**
   TODO: This is not thread safe!
*/
bool
HasProperty(Game::Entity const entity, PropertyId const pid)
{
    GameServer::State& state = GameServer::Singleton->state;
    EntityMapping mapping = GetEntityMapping(entity);
    Category const& cat = GameServer::Singleton->state.world.GetCategory(mapping.category);
    return GameServer::Singleton->state.world.db->HasProperty(cat.instanceTable, pid);
}

//------------------------------------------------------------------------------
/**
*/
BlueprintId
GetBlueprintId(Util::StringAtom name)
{
    return BlueprintManager::GetBlueprintId(name);
}

//------------------------------------------------------------------------------
/**
*/
TemplateId
GetTemplateId(Util::StringAtom name)
{
    return BlueprintManager::GetTemplateId(name);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
GetNumInstances(CategoryId category)
{
    Ptr<MemDb::Database> db = GetWorldDatabase();
    MemDb::TableId tid = GameServer::Singleton->state.world.GetCategory(category).instanceTable;
    return db->GetNumRows(tid);
}

//------------------------------------------------------------------------------
/**
*/
void*
GetInstanceBuffer(CategoryId const category, PropertyId const pid)
{
    Category const& cat = Game::GameServer::Singleton->state.world.GetCategory(category);
    Ptr<MemDb::Database> db = Game::GameServer::Singleton->state.world.db;
    auto cid = db->GetColumnId(cat.instanceTable, pid);
#if NEBULA_DEBUG
    n_assert_fmt(cid != MemDb::ColumnIndex::Invalid(), "GetInstanceBuffer: Category does not have property with id '%i'!\n", pid.id);
#endif
    return db->GetBuffer(cat.instanceTable, cid);
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
GetInstanceId(Entity entity)
{
    return GetEntityMapping(entity).instance;
}

} // namespace Game
