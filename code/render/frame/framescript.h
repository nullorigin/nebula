#pragma once
//------------------------------------------------------------------------------
/**
	A FrameScript describes render operations being done to produce a single frame.

	Frame scripts are loaded once like a template, and then compiled to produce
	an optimized result. When a pass is disabled or re-enabled, the script
	is rebuilt, so refrain from doing this frequently. 

	On DX12 and Vulkan, the compile process serves to insert proper barriers, events
	and semaphore operations such that shader resources are not stomped or read prematurely.
	
	(C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/texture.h"
#include "coregraphics/shaderrwbuffer.h"
#include "coregraphics/event.h"
#include "coregraphics/shader.h"
#include "frame/plugins/frameplugin.h"
#include "frame/frameop.h"
#include "frame/framepass.h"
#include "memory/arenaallocator.h"
namespace Graphics
{
	class View;
}
namespace Frame
{
class FrameScript : public Core::RefCounted
{
	__DeclareClass(FrameScript);
public:
	/// constructor
	FrameScript();
	/// destructor
	virtual ~FrameScript();

	/// get allocator
	Memory::ArenaAllocator<BIG_CHUNK>& GetAllocator();

	/// set name
	void SetResourceName(const Resources::ResourceName& name);
	/// get name
	const Resources::ResourceName& GetResourceName() const;

	/// add frame operation
	void AddOp(Frame::FrameOp* op);
	/// add texture
	void AddTexture(const Util::StringAtom& name, const CoreGraphics::TextureId tex);
	/// get texture
	const CoreGraphics::TextureId GetTexture(const Util::StringAtom& name);
	/// get all textures
	const Util::Dictionary<Util::StringAtom, CoreGraphics::TextureId>& GetTextures() const;
	/// add read-write buffer
	void AddReadWriteBuffer(const Util::StringAtom& name, const CoreGraphics::ShaderRWBufferId buf);
	/// get read-write buffer
	const CoreGraphics::ShaderRWBufferId GetReadWriteBuffer(const Util::StringAtom& name);
	/// add algorithm
	void AddPlugin(const Util::StringAtom& name, Frame::FramePlugin* alg);
	/// get algorithm
	Frame::FramePlugin* GetPlugin(const Util::StringAtom& name);

	/// setup script
	void Setup();
	/// discard script
	void Discard();
	/// run through script and call resource updates
	void UpdateResources(const IndexT frameIndex);
	/// run script
	void Run(const IndexT frameIndex);

	/// build framescript, this will delete and replace the old frame used for Run()
	void Build();

private:
	friend class FrameScriptLoader;
	friend class FrameServer;
	friend class Graphics::View;

	/// cleanup script internally
	void Cleanup();
	/// handle resizing
	void OnWindowResized();

	CoreGraphics::WindowId window;
	Memory::ArenaAllocator<BIG_CHUNK> allocator;

	Util::Array<CoreGraphics::EventId> events;
	Util::Array<CoreGraphics::BarrierId> barriers;
	Memory::ArenaAllocator<BIG_CHUNK> buildAllocator;

	Resources::ResourceName resId;

	Util::Array<CoreGraphics::ShaderRWBufferId> readWriteBuffers;
	Util::Dictionary<Util::StringAtom, CoreGraphics::ShaderRWBufferId> readWriteBuffersByName;
	Util::Array<CoreGraphics::TextureId> textures;
	Util::Dictionary<Util::StringAtom, CoreGraphics::TextureId> texturesByName;

	Util::Array<Frame::FrameOp*> ops;
	Util::Array<Frame::FrameOp::Compiled*> compiled;
	Util::Array<CoreGraphics::BarrierId> resourceResetBarriers;
	IndexT frameOpCounter;
	Util::Array<Frame::FramePlugin*> plugins;
	Util::Dictionary<Util::StringAtom, Frame::FramePlugin*> algorithmsByName;

	bool subScript; // if subscript, it means it can only be ran from within another script
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameScript::SetResourceName(const Resources::ResourceName& name)
{
	this->resId = name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceName&
FrameScript::GetResourceName() const
{
	return this->resId;
}

//------------------------------------------------------------------------------
/**
*/
inline Memory::ArenaAllocator<BIG_CHUNK>&
FrameScript::GetAllocator()
{
	return this->allocator;
}

//------------------------------------------------------------------------------
/**
*/
inline Frame::FramePlugin*
FrameScript::GetPlugin(const Util::StringAtom& name)
{
	return this->algorithmsByName[name];
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::TextureId 
FrameScript::GetTexture(const Util::StringAtom& name)
{
	return this->texturesByName[name];
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Dictionary<Util::StringAtom, CoreGraphics::TextureId>&
FrameScript::GetTextures() const
{
	return this->texturesByName;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::ShaderRWBufferId
FrameScript::GetReadWriteBuffer(const Util::StringAtom& name)
{
	IndexT i = this->readWriteBuffersByName.FindIndex(name);
	return i == InvalidIndex ? CoreGraphics::ShaderRWBufferId::Invalid() : this->readWriteBuffersByName.ValueAtIndex(i);
}

} // namespace Frame2