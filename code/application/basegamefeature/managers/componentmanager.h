#pragma once
//------------------------------------------------------------------------------
/**
	ComponentManager

	Holds components and acts as interface agains other systems.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/singleton.h"
#include "game/manager.h"
#include "util/delegate.h"
#include "util/bitfield.h"
#include "util/dictionary.h"

namespace Game
{

class BaseComponent;

class ComponentManager : public Game::Manager
{
	__DeclareClass(ComponentManager)
	__DeclareSingleton(ComponentManager)
public:
	ComponentManager();
	~ComponentManager();

	/// Register a component and setup all event delegates for it.
	void RegisterComponent(const Ptr<BaseComponent>& component);

	/// Deregister a component and remove all event delegates associated with it.
	void DeregisterComponent(const Ptr<BaseComponent>& component);

	/// Retrieve a component from the registry
	template<class T>
	const Ptr<T>& GetComponent();

	/// Execute all OnBeginFrame events
	void OnBeginFrame();

	/// Execute all OnFrame events
	void OnRender();

	/// Execute all OnEndFrame events
	void OnEndFrame();

	/// Execute all OnRenderDebug events
	void OnRenderDebug();

private:
	Util::Array<Ptr<BaseComponent>> components;

	Util::HashTable<Util::FourCC, Ptr<BaseComponent>> registry;

	/// Find the index of a delegate based on a component
	static IndexT FindDelegateIndex(const Util::Array<Util::Delegate<>>& delegateArray, const Ptr<BaseComponent>& component);

	/// All arrays containing the delegates for different events.
	/// Honestly, I don't remember why I designed it like this anymore.
	Util::Array<Util::Delegate<>> delegates_OnBeginFrame;
	Util::Array<Util::Delegate<>> delegates_OnRender;
	Util::Array<Util::Delegate<>> delegates_OnEndFrame;
	Util::Array<Util::Delegate<>> delegates_OnRenderDebug;
};

} // namespace Game
