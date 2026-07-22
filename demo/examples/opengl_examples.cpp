#include "../../blur.hpp"

void RenderOpenGLBlurExample(Blur* blurInstance, int screenWidth, int screenHeight) {
    if (!blurInstance) return;

    int blurX = screenWidth / 4;
    int blurY = screenHeight / 4;
    int blurW = screenWidth / 2;
    int blurH = screenHeight / 2;
    float radius = 20.0f;
    int downscale = 4;

    blurInstance->BlurType = Blur::Type::Kawase;
    blurInstance->Process(blurX, blurY, blurW, blurH, radius, downscale);

    // blurInstance->Texture now contains the blurred region texture
}
