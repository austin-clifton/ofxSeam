#include "seam/nodes/hdrTonemapper.h"
#include "seam/shaderUtils.h"

using namespace seam::nodes;

HdrTonemapper::HdrTonemapper() : INode("HDR Tone Mapper") {
    flags = (NodeFlags)(flags | NodeFlags::IsVisual);
	gui_display_fbo = &tonemappedFbo;
}

void HdrTonemapper::Setup(SetupParams* params) {
    ReloadShaders();
    SetupBloomTextures();
}

HdrTonemapper::~HdrTonemapper() {

}

void HdrTonemapper::Update(UpdateParams* params) {
    // NO-OP?? delete me?
}

void HdrTonemapper::Draw(DrawParams* params) {
    // The brightness pass writes to the first FBO in the bloomFbos array.
    bloomFbos[0].begin();
    brightPassShader.begin();

    bloomFbos[0].clearColorBuffer(0.f);
    bloomFbos[0].draw(0,0);
    brightPassShader.end();
    bloomFbos[0].end();

    // Now use linear sampling to blit and downscale the first FBO to the smaller FBOs.
    // IMPROVEMENT: is there a nicer way to do this than explicitly casting to an opengl renderer?
    ofBaseGLRenderer* renderer = (ofBaseGLRenderer*)(ofGetCurrentRenderer().get());
    for (size_t i = 0; i < bloomFbos.size() - 1; i++) {
        ofFbo& srcFbo = bloomFbos[i];
        ofFbo& dstFbo = bloomFbos[i + 1];

        renderer->bindForBlitting(srcFbo, dstFbo, srcFbo.getDefaultTextureIndex());
        glBlitFramebuffer(0, 0, srcFbo.getWidth(), srcFbo.getHeight(),
            0, 0, dstFbo.getWidth(), dstFbo.getHeight(), GL_COLOR_BUFFER_BIT, GL_LINEAR);
        renderer->unbind(srcFbo);
    }

    // Now run a vertical and horizontal blur pass over each resolution,
    // using the back buffers to ping-pong between.
    const uint16_t blurPasses = 1;
    blurShader.begin();
    for (size_t i = 0; i < bloomFbos.size(); i++) {
        ofFbo* readFbo = &bloomFbos[i];
        ofFbo* writeFbo = &bloomFbosBack[i];
        bool horizontal = true;
        blurShader.setUniform2i("resolution", readFbo->getWidth(), readFbo->getHeight());

        for (uint16_t j = 0; j < blurPasses * 2; j++) {
            blurShader.setUniform1i("horizontal", horizontal);
            writeFbo->begin();

            readFbo->draw(0,0);

            writeFbo->end();
            std::swap(readFbo, writeFbo);
            horizontal = !horizontal;
        }
    }
    blurShader.end();

    // Now, run the tonemap shader that adds all the blur passes together 
    // with the original pass into a final image.
    tonemappedFbo.begin();
    tonemapShader.begin();

    tonemapShader.setUniform1f("gamma", gamma);

    tonemappedFbo.clearColorBuffer(0.f);
    tonemappedFbo.draw(0,0);

    tonemapShader.end();
    tonemappedFbo.end();
}

PinInput* HdrTonemapper::PinInputs(size_t& size) {
    size = pinInputs.size();
    return pinInputs.data();
}

PinOutput* HdrTonemapper::PinOutputs(size_t& size) {
    size = 1;
    return &pinOutFbo;
}

bool HdrTonemapper::GuiDrawPropertiesList(UpdateParams* params) {
    bool changed = false;
    if (ImGui::Button("Reload Shaders")) {
        changed = ReloadShaders();
    }
    
    return changed;
}

bool HdrTonemapper::ReloadShaders() {
    const std::string vertName = "screen-rect.vert";
    const std::string hdrTonemapName = "hdrTonemap.frag";
    const std::string brightPassName = "hdrBrightClamp.frag";
    const std::string blurName = "gaussianBlur.frag";
    
    bool brightPassLoaded = ShaderUtils::LoadShader(brightPassShader, vertName, brightPassName);
    bool tonemapLoaded = ShaderUtils::LoadShader(tonemapShader, vertName, hdrTonemapName);
    bool blurLoaded = ShaderUtils::LoadShader(blurShader, vertName, blurName);

    // Shaders were reloaded, update resolution params
    bool allLoaded = brightPassLoaded && tonemapLoaded && blurLoaded;
    if (allLoaded) {
        RebindTexture();
    }

    return allLoaded;
}

void HdrTonemapper::RebindTexture() {
	if (hdrFbo != nullptr) {
        // Resolution is set via the input FBO
        resolution = glm::vec2(hdrFbo->getWidth(), hdrFbo->getHeight());

        // Output resolution should match input resolution
        Seam().texLocResolver->ReleaseAll(&tonemappedFbo.getTexture());
        tonemappedFbo.clear();
        tonemappedFbo.allocate(resolution.x, resolution.y, GL_RGBA);

        SetupBloomTextures();

        uint32_t texLoc = Seam().texLocResolver->Bind(&hdrFbo->getTexture());

        // The brightness shader writes to the first downscaled FBO for bloom blur,
        // so make sure the right resolution is used!
        glm::ivec2 dsRes = DownscaledRes();

        brightPassShader.begin();
		brightPassShader.setUniformTexture("hdrBuffer", hdrFbo->getTexture(), texLoc);
        brightPassShader.setUniform2i("resolution", dsRes.x, dsRes.y);
        brightPassShader.end();

		tonemapShader.begin();
		tonemapShader.setUniformTexture("hdrBuffer", hdrFbo->getTexture(), texLoc);
        tonemapShader.setUniform2i("resolution", resolution.x, resolution.y);
		tonemapShader.end();

        // Tonemapped FBO changed, force reconnect
        pinOutFbo.Reconnect(Seam().pushPatterns);
	}
}

void HdrTonemapper::SetupBloomTextures() {

    // Don't set up bloom textures unless they can be re-bound after setup.
    if (!tonemapShader.isLoaded()) {
        return;
    }

    // The largest texture will be downscaled from the original FBO size to a power of 2.
    glm::ivec2 dsRes = DownscaledRes();

    // If textures are already allocated and the right size, there's nothing to do.
    if (bloomFbos[0].isAllocated() && 
        bloomFbos[0].getWidth() == dsRes.x
        && bloomFbos[0].getHeight() == dsRes.y
    ) {
        return;
    }

    for (size_t i = 0; i < bloomDownScales; i++) {
        Seam().texLocResolver->ReleaseAll(&bloomFbos[i].getTexture());

        bloomFbos[i].clear();
        bloomFbosBack[i].clear();

        bloomFbos[i].allocate(dsRes.x, dsRes.y, GL_RGBA16F);
        bloomFbosBack[i].allocate(dsRes.x, dsRes.y, GL_RGBA16F);

        dsRes = dsRes / 2;
    }

    RebindBloomTextures();
}

void HdrTonemapper::RebindBloomTextures() {
    tonemapShader.begin();

    std::string uniformName = "blurTexN";
    for (size_t i = 0; i < bloomDownScales; i++) {
        // Set the last char of the uniformName (N) to the actual index using offset from char '0'
        uniformName[uniformName.size() - 1] = '0' + i;

        uint32_t texLoc = Seam().texLocResolver->Bind(&bloomFbos[i].getTexture(0));
        tonemapShader.setUniformTexture(uniformName, bloomFbos[i].getTexture(0), texLoc);
    }
     
    tonemapShader.end();
}

glm::ivec2 HdrTonemapper::DownscaledRes() {
    return glm::ivec2(
        pow(2, floor(log2(resolution.x))), 
        pow(2, floor(log2(resolution.y)))
    );
}