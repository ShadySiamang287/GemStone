#include "TestScene.h"

void TestScene::Update(GEM::Input& input, GEM::Renderer& renderer, float dt) {
    mCamera.Update(input, dt);
    renderer.SetCameraMatrix(mCamera.getMatrix());
}