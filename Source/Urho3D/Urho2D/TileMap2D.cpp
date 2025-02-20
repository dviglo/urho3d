// Copyright (c) 2008-2022 the Urho3D project
// License: MIT

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Graphics/DebugRenderer.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Node.h"
#include "../Scene/Scene.h"
#include "../Urho2D/TileMap2D.h"
#include "../Urho2D/TileMapLayer2D.h"
#include "../Urho2D/TmxFile2D.h"

#include "../DebugNew.h"

namespace Urho3D
{

extern const char* URHO2D_CATEGORY;

TileMap2D::TileMap2D(Context* context) :
    Component(context)
{
}

TileMap2D::~TileMap2D() = default;

void TileMap2D::RegisterObject(Context* context)
{
    context->RegisterFactory<TileMap2D>(URHO2D_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Tmx File", GetTmxFileAttr, SetTmxFileAttr, ResourceRef(TmxFile2D::GetTypeStatic()),
        AM_DEFAULT);
}

// Transform vector from node-local space to global space
static Vector2 TransformNode2D(const Matrix3x4& transform, Vector2 local)
{
    Vector3 transformed = transform * Vector4(local.x_, local.y_, 0.f, 1.f);
    return Vector2(transformed.x_, transformed.y_);
}

void TileMap2D::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    const Color& color = Color::RED;
    float mapW = info_.GetMapWidth();
    float mapH = info_.GetMapHeight();
    const Matrix3x4 transform = GetNode()->GetTransform();

    switch (info_.orientation_)
    {
    case O_ORTHOGONAL:
    case O_STAGGERED:
    case O_HEXAGONAL:
        debug->AddLine(Vector3(TransformNode2D(transform, Vector2(0.0f, 0.0f))),
            Vector3(TransformNode2D(transform, Vector2(mapW, 0.0f))), color);
        debug->AddLine(Vector3(TransformNode2D(transform, Vector2(mapW, 0.0f))),
            Vector3(TransformNode2D(transform, Vector2(mapW, mapH))), color);
        debug->AddLine(Vector3(TransformNode2D(transform, Vector2(mapW, mapH))),
            Vector3(TransformNode2D(transform, Vector2(0.0f, mapH))), color);
        debug->AddLine(Vector3(TransformNode2D(transform, Vector2(0.0f, mapH))),
            Vector3(TransformNode2D(transform, Vector2(0.0f, 0.0f))), color);
        break;

    case O_ISOMETRIC:
        debug->AddLine(Vector3(TransformNode2D(transform, Vector2(0.0f, mapH * 0.5f))),
            Vector3(TransformNode2D(transform, Vector2(mapW * 0.5f, 0.0f))), color);
        debug->AddLine(Vector3(TransformNode2D(transform, Vector2(mapW * 0.5f, 0.0f))),
            Vector3(TransformNode2D(transform, Vector2(mapW, mapH * 0.5f))), color);
        debug->AddLine(Vector3(TransformNode2D(transform, Vector2(mapW, mapH * 0.5f))),
            Vector3(TransformNode2D(transform, Vector2(mapW * 0.5f, mapH))), color);
        debug->AddLine(Vector3(TransformNode2D(transform, Vector2(mapW * 0.5f, mapH))),
            Vector3(TransformNode2D(transform, Vector2(0.0f, mapH * 0.5f))), color);
        break;
    }

    for (const WeakPtr<TileMapLayer2D>& layer : layers_)
        layer->DrawDebugGeometry(debug, depthTest);
}

void TileMap2D::DrawDebugGeometry()
{
    Scene* scene = GetScene();
    if (!scene)
        return;

    auto* debug = scene->GetComponent<DebugRenderer>();
    if (!debug)
        return;

    DrawDebugGeometry(debug, false);
}

void TileMap2D::SetTmxFile(TmxFile2D* tmxFile)
{
    if (tmxFile == tmxFile_)
        return;

    if (rootNode_)
        rootNode_->RemoveAllChildren();

    layers_.Clear();

    tmxFile_ = tmxFile;
    if (!tmxFile_)
        return;

    info_ = tmxFile_->GetInfo();

    if (!rootNode_)
    {
        rootNode_ = GetNode()->CreateTemporaryChild("_root_", LOCAL);
    }

    unsigned numLayers = tmxFile_->GetNumLayers();
    layers_.Resize(numLayers);

    for (unsigned i = 0; i < numLayers; ++i)
    {
        const TmxLayer2D* tmxLayer = tmxFile_->GetLayer(i);

        Node* layerNode(rootNode_->CreateTemporaryChild(tmxLayer->GetName(), LOCAL));

        auto* layer = layerNode->CreateComponent<TileMapLayer2D>();
        layer->Initialize(this, tmxLayer);
        layer->SetDrawOrder(i * 10);

        layers_[i] = layer;
    }
}

TmxFile2D* TileMap2D::GetTmxFile() const
{
    return tmxFile_;
}

TileMapLayer2D* TileMap2D::GetLayer(unsigned index) const
{
    if (index >= layers_.Size())
        return nullptr;

    return layers_[index];
}

Vector2 TileMap2D::TileIndexToPosition(int x, int y) const
{
    return info_.TileIndexToPosition(x, y);
}

bool TileMap2D::PositionToTileIndex(int& x, int& y, const Vector2& position) const
{
    return info_.PositionToTileIndex(x, y, position);
}

void TileMap2D::SetTmxFileAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    SetTmxFile(cache->GetResource<TmxFile2D>(value.name_));
}

ResourceRef TileMap2D::GetTmxFileAttr() const
{
    return GetResourceRef(tmxFile_, TmxFile2D::GetTypeStatic());
}

Vector<SharedPtr<TileMapObject2D>> TileMap2D::GetTileCollisionShapes(unsigned gid) const
{
    Vector<SharedPtr<TileMapObject2D>> shapes;
    return tmxFile_ ? tmxFile_->GetTileCollisionShapes(gid) : shapes;
}

}
