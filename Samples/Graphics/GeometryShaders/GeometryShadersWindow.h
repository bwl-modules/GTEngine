// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2019
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.1 (2019/04/16)

#pragma once

#include <Applications/GteWindow3.h>
using namespace gte;

#define USE_DRAW_DIRECT
//#define SAVE_RENDERING_TO_DISK

class GeometryShadersWindow : public Window3
{
public:
    GeometryShadersWindow(Parameters& parameters);

    virtual void OnIdle() override;

private:
    bool SetEnvironment();
    bool CreateScene();
    void UpdateConstants();

    std::shared_ptr<ConstantBuffer> mMatrices;
    std::shared_ptr<Visual> mMesh;

#if !defined(USE_DRAW_DIRECT)
    std::shared_ptr<StructuredBuffer> mParticles;
#endif

#if defined(SAVE_RENDERING_TO_DISK)
    std::shared_ptr<DrawTarget> mTarget;
#endif
};
