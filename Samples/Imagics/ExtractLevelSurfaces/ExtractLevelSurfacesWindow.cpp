// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2019
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.23.2 (2019/05/03)

#include "ExtractLevelSurfacesWindow.h"
#include <LowLevel/GteLogReporter.h>
#include <Imagics/GteSurfaceExtractorCubes.h>
#include <Imagics/GteSurfaceExtractorTetrahedra.h>

int main(int, char const*[])
{
#if defined(_DEBUG)
    LogReporter reporter(
        "LogReport.txt",
        Logger::Listener::LISTEN_FOR_ALL,
        Logger::Listener::LISTEN_FOR_ALL,
        Logger::Listener::LISTEN_FOR_ALL,
        Logger::Listener::LISTEN_FOR_ALL);
#endif

    Window::Parameters parameters(L"ExtractLevelSurfacesWindow", 0, 0, 768, 768);
    auto window = TheWindowSystem.Create<ExtractLevelSurfacesWindow>(parameters);
    TheWindowSystem.MessagePump(window, TheWindowSystem.DEFAULT_ACTION);
    TheWindowSystem.Destroy<ExtractLevelSurfacesWindow>(window);
    return 0;
}

ExtractLevelSurfacesWindow::ExtractLevelSurfacesWindow(Parameters& parameters)
    :
    Window3(parameters),
    mUseCubes(true)
{
    if (!SetEnvironment())
    {
        parameters.created = false;
        return;
    }

    mEngine->SetClearColor({ 0.4f, 0.5f, 0.6f, 1.0f });

    mWireState = std::make_shared<RasterizerState>();
    mWireState->fillMode = RasterizerState::FILL_WIREFRAME;

    CreateScene();
    Vector3<float> pos = -2.0f * mScene->worldBound.GetRadius() * Vector3<float>::Unit(2);
    InitializeCamera(60.0f, GetAspectRatio(), 1.0f, 1000.0f, 1.0f, 0.01f,
        { pos[0], pos[1], pos[2] }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f });

    mPVWMatrices.Update();
}

void ExtractLevelSurfacesWindow::OnIdle()
{
    mTimer.Measure();

    mCameraRig.Move();
    UpdateConstants();

    mEngine->ClearBuffers();

    if (mUseCubes)
    {
        mEngine->Draw(mMeshCubes);
        mEngine->Draw(8, 24, { 0.0f, 0.0f, 0.0f, 1.0f }, "surface cubes");
    }
    else
    {
        mEngine->Draw(mMeshTetrahedra);
        mEngine->Draw(8, 24, { 0.0f, 0.0f, 0.0f, 1.0f }, "surface tetrahedra");
    }

    mEngine->Draw(8, mYSize - 8, { 0.0f, 0.0f, 0.0f, 1.0f }, mTimer.GetFPS());
    mEngine->DisplayColorBuffer(0);

    mTimer.UpdateFrameCount();
}

bool ExtractLevelSurfacesWindow::OnCharPress(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 'w':
    case 'W':
        if (mEngine->GetRasterizerState() == mWireState)
        {
            mEngine->SetRasterizerState(mWireState);
        }
        else
        {
            mEngine->SetDefaultRasterizerState();
        }
        return true;

    case 'e':
    case 'E':
        mUseCubes = !mUseCubes;
        return true;
    }
    return Window3::OnCharPress(key, x, y);
}

bool ExtractLevelSurfacesWindow::SetEnvironment()
{
    std::string path = GetGTEPath();
    if (path == "")
    {
        return false;
    }

    mEnvironment.Insert(path + "/Samples/Data/");
    if (mEnvironment.GetPath("Molecule_U8_X100_Y100_Z120.binary") == "")
    {
        LogError("Cannot find file Molecule_U8_X100_Y100_Z120.binary");
        return false;
    }
    return true;
}

void ExtractLevelSurfacesWindow::CreateScene()
{
    mScene = std::make_shared<Node>();
    mTrackball.Attach(mScene);

    mMaterial = std::make_shared<Material>();
    mMaterial->emissive = { 0.0f, 0.0f, 0.0f, 1.0f };
    mMaterial->ambient = { 0.5f, 0.5f, 0.5f, 1.0f };
    mMaterial->diffuse = { 0.99607f, 0.83920f, 0.67059f, 1.0f };
    mMaterial->specular = { 0.8f, 0.8f, 0.8f, 4.0f };

    mLighting = std::make_shared<Lighting>();
    mLighting->ambient = { 0.25f, 0.25f, 0.25f, 1.0f };
    mLighting->diffuse = { 0.5f, 0.5f, 0.5f, 1.0f };
    mLighting->specular = { 0.1f, 0.1f, 0.1f, 1.0f };

    mLightWorldDirection = { 0.0f, 0.0f, 1.0f, 0.0f };

    mXBound = 100;
    mYBound = 100;
    mZBound = 120;
    mImage.resize(mXBound * mYBound * mZBound);
    std::string path = mEnvironment.GetPath("Molecule_U8_X100_Y100_Z120.binary");
    std::ifstream input(path, std::ios::binary);
    input.read((char*)mImage.data(), mImage.size() * sizeof(uint8_t));
    input.close();

    CreateMeshCubes();
    CreateMeshTetrahedra();

    mTrackball.Update();
    mScene->localTransform.SetTranslation(-mTrackball.GetRoot()->worldBound.GetCenter());
    mTrackball.Update();
}

void ExtractLevelSurfacesWindow::CreateMeshCubes()
{
    // Extract a level set from the image.
    SurfaceExtractorCubes<uint8_t, float> esc(mXBound, mYBound, mXBound, mImage.data());
    std::vector<SurfaceExtractorTetrahedra<uint8_t, float>::Vertex> esVertices;
    std::vector<SurfaceExtractor<uint8_t, float>::Triangle> esTriangles;
    esc.Extract(64, esVertices, esTriangles);

    // Remove duplicate vertices.
    esc.MakeUnique(esVertices, esTriangles);

    // Convert to floating-point vertices.
    std::vector<std::array<float, 3>> fvertices;
    esc.Convert(esVertices, fvertices);

    // Orient the triangles to have consistent winding order.
    esc.OrientTriangles(fvertices, esTriangles, false);

    // Compute normals for use in directional lighting.
    std::vector<std::array<float, 3>> fnormals;
    esc.ComputeNormals(fvertices, esTriangles, fnormals);

    // Create a triangle mesh for the surface.  The mesh uses directional
    // lighting for visualization.
    VertexFormat vformat;
    vformat.Bind(VA_POSITION, DF_R32G32B32_FLOAT, 0);
    vformat.Bind(VA_NORMAL, DF_R32G32B32_FLOAT, 0);

    unsigned int numVertices = static_cast<unsigned int>(esVertices.size());
    auto vbuffer = std::make_shared<VertexBuffer>(vformat, numVertices);
    auto* vertices = vbuffer->Get<Vector3<float>>();
    for (unsigned int i = 0; i < numVertices; ++i)
    {
        vertices[2 * i] = fvertices[i];
        vertices[2 * i + 1] = fnormals[i];
    }

    unsigned int numTriangles = static_cast<unsigned int>(esTriangles.size());
    auto ibuffer = std::make_shared<IndexBuffer>(IP_TRIMESH, numTriangles, sizeof(unsigned int));
    std::memcpy(ibuffer->GetData(), &esTriangles[0], ibuffer->GetNumBytes());

    auto geometry = std::make_shared<LightCameraGeometry>();
    mLightEffectCubes = std::make_shared<DirectionalLightEffect>(mProgramFactory,
        mUpdater, 1, mMaterial, mLighting, geometry);

    mMeshCubes = std::make_shared<Visual>(vbuffer, ibuffer, mLightEffectCubes);
    mMeshCubes->UpdateModelBound();
    mScene->AttachChild(mMeshCubes);
    mPVWMatrices.Subscribe(mMeshCubes->worldTransform, mLightEffectCubes->GetPVWMatrixConstant());
}

void ExtractLevelSurfacesWindow::CreateMeshTetrahedra()
{
    // Extract a level set from the image.
    SurfaceExtractorTetrahedra<uint8_t, float> est(mXBound, mYBound, mXBound, mImage.data());
    std::vector<SurfaceExtractorTetrahedra<uint8_t, float>::Vertex> esVertices;
    std::vector<SurfaceExtractor<uint8_t, float>::Triangle> esTriangles;
    est.Extract(64, esVertices, esTriangles);

    // Remove duplicate vertices.
    est.MakeUnique(esVertices, esTriangles);

    // Convert to floating-point vertices.
    std::vector<std::array<float, 3>> fvertices;
    est.Convert(esVertices, fvertices);

    // Orient the triangles to have consistent winding order.
    est.OrientTriangles(fvertices, esTriangles, false);

    // Compute normals for use in directional lighting.
    std::vector<std::array<float, 3>> fnormals;
    est.ComputeNormals(fvertices, esTriangles, fnormals);

    // Create a triangle mesh for the surface.  The mesh uses directional
    // lighting for visualization.
    VertexFormat vformat;
    vformat.Bind(VA_POSITION, DF_R32G32B32_FLOAT, 0);
    vformat.Bind(VA_NORMAL, DF_R32G32B32_FLOAT, 0);

    unsigned int numVertices = static_cast<unsigned int>(esVertices.size());
    auto vbuffer = std::make_shared<VertexBuffer>(vformat, numVertices);
    auto* vertices = vbuffer->Get<Vector3<float>>();
    for (unsigned int i = 0; i < numVertices; ++i)
    {
        vertices[2 * i] = fvertices[i];
        vertices[2 * i + 1] = fnormals[i];
    }

    unsigned int numTriangles = static_cast<unsigned int>(esTriangles.size());
    auto ibuffer = std::make_shared<IndexBuffer>(IP_TRIMESH, numTriangles, sizeof(unsigned int));
    std::memcpy(ibuffer->GetData(), &esTriangles[0], ibuffer->GetNumBytes());

    auto geometry = std::make_shared<LightCameraGeometry>();
    mLightEffectTetrahedra = std::make_shared<DirectionalLightEffect>(mProgramFactory,
        mUpdater, 1, mMaterial, mLighting, geometry);

    mMeshTetrahedra = std::make_shared<Visual>(vbuffer, ibuffer, mLightEffectTetrahedra);
    mMeshTetrahedra->UpdateModelBound();
    mScene->AttachChild(mMeshTetrahedra);
    mPVWMatrices.Subscribe(mMeshTetrahedra->worldTransform, mLightEffectTetrahedra->GetPVWMatrixConstant());
}

void ExtractLevelSurfacesWindow::UpdateConstants()
{
    Vector4<float> cameraWorldPosition = mCamera->GetPosition();

    Matrix4x4<float> invWMatrix = mMeshCubes->worldTransform.GetHInverse();
    auto geometry = mLightEffectCubes->GetGeometry();
    geometry->cameraModelPosition = DoTransform(invWMatrix, cameraWorldPosition);
    geometry->lightModelDirection = DoTransform(invWMatrix, mLightWorldDirection);
    mLightEffectCubes->UpdateGeometryConstant();

    invWMatrix = mMeshTetrahedra->worldTransform.GetHInverse();
    geometry = mLightEffectTetrahedra->GetGeometry();
    geometry->cameraModelPosition = DoTransform(invWMatrix, cameraWorldPosition);
    geometry->lightModelDirection = DoTransform(invWMatrix, mLightWorldDirection);
    mLightEffectTetrahedra->UpdateGeometryConstant();

    mTrackball.Update();
    mPVWMatrices.Update();
}