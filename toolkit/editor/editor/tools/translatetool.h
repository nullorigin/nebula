#pragma once
//------------------------------------------------------------------------------
/**
	@file translatetool.h

	(C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolinterface.h"
#include "math/plane.h"

namespace Editor
{
class Camera;
}

namespace Tools
{

//------------------------------------------------------------------------------
/**
    @class  TranslateTool

    @brief  Used to translate entities using a gizmo.
*/
class TranslateTool : public ToolInterface
{
public:
    /// Call before Update
    void Render(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera) override;
    /// Call after render
    void Update(Math::vec2 const& viewPortPosition, Math::vec2 const& viewPortSize, Editor::Camera const* camera) override;
    
    bool IsModifying() const override;

    void Abort() override;

private:
    struct
    {
        /// used to check if any changes should be applied to the entities
        bool isDirty = false;
        ///
        bool gizmoTransforming = false;
    } translation;
};

} // namespace Tools
