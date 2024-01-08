#pragma once
//------------------------------------------------------------------------------
/**
    A subpass is a subset of attachments declared by pass, and if depth should be used.
    
    Subpasses can be dependent on each other, and can declare which attachments in the pass
    should be passed between them. 

    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "frameop.h"
#include "math/rectangle.h"
namespace Frame
{
class FrameSubpass : public FrameOp
{
public:
    /// constructor
    FrameSubpass();
    /// destructor
    virtual ~FrameSubpass();

    /// discard operation
    void Discard() override;

    /// handle display resizing
    void OnWindowResized() override;

    /// add viewport
    void AddViewport(const Math::rectangle<int>& rect);
    /// add viewport
    void AddScissor(const Math::rectangle<int>& rect);

    struct CompiledImpl : public FrameOp::Compiled
    {
        void Run(const CoreGraphics::CmdBufferId cmdBuf, const IndexT frameIndex, const IndexT bufferIndex) override;
        void Discard() override;

        Util::Array<Frame::FrameOp::Compiled*> ops;
        Util::Array<Math::rectangle<int>> viewports;
        Util::Array<Math::rectangle<int>> scissors;

#if NEBULA_GRAPHICS_DEBUG
        Util::StringAtom name;
#endif
    };

    FrameOp::Compiled* AllocCompiled(Memory::ArenaAllocator<BIG_CHUNK>& allocator) override;

protected:
    friend class FramePass;
    void Build(const BuildContext& ctx) override;

private:
    friend class FrameScriptLoader;
};

} // namespace Frame2
