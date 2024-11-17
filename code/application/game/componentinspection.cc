//------------------------------------------------------------------------------
//  componentinspection.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "componentinspection.h"
#include "memdb/attributeregistry.h"
#include "game/entity.h"
#include "memdb/attributeid.h"
#include "imgui.h"
#include "math/mat4.h"
#include "util/stringatom.h"

namespace Game
{

ComponentInspection* ComponentInspection::Singleton = nullptr;

//------------------------------------------------------------------------------
/**
    The registry's constructor is called by the Instance() method, and
    nobody else.
*/
ComponentInspection*
ComponentInspection::Instance()
{
    if (nullptr == Singleton)
    {
        Singleton = new ComponentInspection;
        n_assert(nullptr != Singleton);
    }
    return Singleton;
}

//------------------------------------------------------------------------------
/**
    This static method is used to destroy the registry object and should be
    called right before the main function exits. It will make sure that
    no accidential memory leaks are reported by the debug heap.
*/
void
ComponentInspection::Destroy()
{
    if (nullptr != Singleton)
    {
        delete Singleton;
        Singleton = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentInspection::Register(ComponentId component, DrawFunc func)
{
    ComponentInspection* reg = Instance();
    while (reg->inspectors.Size() <= component.id)
    {
        IndexT first = reg->inspectors.Size();
        reg->inspectors.Grow();
        reg->inspectors.Resize(reg->inspectors.Capacity());
        reg->inspectors.Fill(first, reg->inspectors.Size() - first, nullptr);
    }

    n_assert(reg->inspectors[component.id] == nullptr);
    reg->inspectors[component.id] = func;
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentInspection::DrawInspector(ComponentId component, void* data, bool* commit)
{
    ComponentInspection* reg = Instance();

    if (reg->inspectors.Size() > component.id)
    {
        if (reg->inspectors[component.id] != nullptr)
            reg->inspectors[component.id](component, data, commit);
    }
}

//------------------------------------------------------------------------------
/**
*/
ComponentInspection::ComponentInspection()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ComponentInspection::~ComponentInspection()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Game::Entity>(ComponentId component, void* data, bool* commit)
{
	MemDb::Attribute* desc = MemDb::AttributeRegistry::GetAttribute(component);

	Game::Entity* entity = (Game::Entity*)data;
    Ids::Id64 id = (Ids::Id64)*entity;
	ImGui::Text("%s: %u", desc->name.Value(), id);
	ImGui::SameLine();
	ImGui::TextDisabled("| gen: %i | index: %i", entity->generation, entity->index);
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<bool>(ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::Checkbox("##input_data", (bool*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<int>(ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::InputInt("##input_data", (int*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template <>
void
ComponentDrawFuncT<int64>(ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::InputInt("##input_data", (int*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<uint>(ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::InputInt("##input_data", (int*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template <>
void
ComponentDrawFuncT<uint64>(ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::InputInt("##input_data", (int*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<float>(ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::InputFloat("##float_input", (float*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Util::StringAtom>(ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    ImGui::Text(((Util::StringAtom*)data)->Value());
    if (ImGui::BeginDragDropTarget())
    {
        auto payload = ImGui::AcceptDragDropPayload("resource");
        if (payload)
        {
            Util::String resourceName = (const char*)payload->Data;
            *(Util::StringAtom*)data = resourceName;
			*commit = true;
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Math::mat4>(ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::InputFloat4("##row0", (float*)data))
        *commit = true;
    if (ImGui::InputFloat4("##row1", (float*)data + 4))
        *commit = true;
    if (ImGui::InputFloat4("##row2", (float*)data + 8))
        *commit = true;
    if (ImGui::InputFloat4("##row3", (float*)data + 12))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Math::vec3>(ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::InputFloat3("##vec3", (float*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template <>
void
ComponentDrawFuncT<Math::vec4>(ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::InputFloat4("##vec4", (float*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template <>
void
ComponentDrawFuncT<Math::quat>(ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::InputFloat4("##quat", (float*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Game::Position>(ComponentId component, void* data, bool* commit)
{
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Position");
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::InputFloat3("##pos", (float*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Game::Orientation>(ComponentId component, void* data, bool* commit)
{
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Orientation");
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::InputFloat4("##orient", (float*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Game::Scale>(ComponentId component, void* data, bool* commit)
{
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Scale");
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::InputFloat3("##scl", (float*)data))
        *commit = true;
    ImGui::PopID();
}

//------------------------------------------------------------------------------
/**
*/
template<>
void
ComponentDrawFuncT<Util::Color>(ComponentId component, void* data, bool* commit)
{
    ImGui::PushID(component.id + 0x125233 + reinterpret_cast<intptr_t>(data));
    if (ImGui::ColorEdit4("##color", (float*)data))
        *commit = true;
    ImGui::PopID();
}


} // namespace Game
