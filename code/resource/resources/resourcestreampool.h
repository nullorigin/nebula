#pragma once
//------------------------------------------------------------------------------
/**
	Loads resources as streams and is updated for every ResourceManager::Update().

	Contains the names for the placeholder and failed-to-load resource names.
	When inheriting from this class, make sure to provide proper resource ids for:
		1. Placeholder resource
		2. Error resource

	If no placeholder resource is provided, the loader cannot execute asynchronously.
	If no error resource is provided and the resource fails to load, then the ResourceManager
	will raise an assertion. 

	Each resource pool also keeps a list of the resources loaded by it. Therefore,
	the ResourceManager is not responsible for maintaining which resources are loaded.

	The pool associates a resource name (StringAtom) with an id, such that it can be quickly
	retrieved. 
	
	When creating an instance of a resource, an ID is returned, this ID contains the following:
	32 bits (resource instance id), 24 bits (resource id) and 8 bits (loader id). The instance id
	is a recyclable number which uniquely identifies a single allocation. The next 24 bits is the
	internal ID for the resource, which is only loaded once. The last 8 bits identifies which loader
	created the resource. 

	Resources created with tags must also be removed using the tag. A tagged resource can only
	be discarded by using that tag. If a resource is loaded with a tag, it will remain bound
	to that tag, no matter what consecutive loads say. 
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resourcepool.h"
#include "ids/id.h"
#include "util/stringatom.h"
#include "io/stream.h"
#include "util/set.h"
#include "resource.h"
#include <tuple>
#include <functional>

namespace Resources
{
class Resource;
class ResourceStreamPool : public ResourcePool
{
	__DeclareAbstractClass(ResourceStreamPool);

public:
	/// constructor
	ResourceStreamPool();
	/// destructor
	virtual ~ResourceStreamPool();

	/// setup resource loader, initiates the placeholder and error resources if valid
	virtual void Setup();
	/// discard resource loader
	virtual void Discard();

	/// create a container with a tag associated with it, if no tag is provided, the resource will be untagged
	Resources::ResourceId CreateResource(const ResourceName& res, const Util::StringAtom& tag, std::function<void(const Resources::ResourceId)> success, std::function<void(const Resources::ResourceId)> failed, bool immediate);
	/// discard container
	void DiscardResource(const Resources::ResourceId id);
	/// discard all resources associated with a tag
	void DiscardByTag(const Util::StringAtom& tag);

protected:
	friend class ResourceManager;

	/// struct for pending resources which are about to be loaded
	struct _PendingResourceLoad
	{
		Resources::ResourceId id;
		Ids::Id32 pid;
		Ids::Id24 res;
		Util::StringAtom tag;
		bool inflight;
		bool immediate;
		std::function<void()> loadFunc;

		_PendingResourceLoad() : pid(Ids::InvalidId32) {};
	};

	struct _PendingResourceUnload
	{
		Ids::Id24 resourceId;
	};

	/// callback functions to run when an associated resource is loaded (can be stacked)
	struct _Callbacks
	{
		Ids::Id32 id;
		std::function<void(const Resources::ResourceId)> success;
		std::function<void(const Resources::ResourceId)> failed;
	};

	/// perform actual load, override in subclass
	virtual LoadStatus LoadFromStream(const Ids::Id24 id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream) = 0;

	/// update the resource loader, this is done every frame
	void Update(IndexT frameIndex);

	/// start loading
	LoadStatus PrepareLoad(_PendingResourceLoad& res);
	/// run callbacks
	void RunCallbacks(LoadStatus status, const Resources::ResourceId id);

	/// these types need to be properly initiated in a subclass Setup function
	Util::StringAtom placeholderResourceId;
	Util::StringAtom errorResourceId;

	Ids::Id32 placeholderResource;
	Ids::Id32 errorResource;

	bool async;

	//Util::Dictionary<Util::StringAtom, _PendingResource> pending;
	//Util::FixedArray<Util::Array<_PendingResource>> 

	Util::Dictionary<Resources::ResourceName, Ids::Id32> pendingLoadMap;
	Util::FixedArray<_PendingResourceLoad> pendingLoads;
	Ids::IdPool pendingLoadPool;
	Util::Array<_PendingResourceUnload> pendingUnloads;
	Util::FixedArray<Util::Array<_Callbacks>> callbacks;

	/// async section to sync callbacks and pending list with thread
	Threading::CriticalSection asyncSection;
};


} // namespace Resources