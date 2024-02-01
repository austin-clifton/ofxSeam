#pragma once

#include "ofMain.h"
#include "seam/pins/pin.h"

namespace seam {
    class TextureLocationResolver {
    public:
        TextureLocationResolver(pins::PushPatterns* pushPatterns, uint32_t availableLocations);

        /// @brief Retrieve a binding location for the given texture.
        /// After Bind() is called, you should call Release() when you're done using the texture.
        uint32_t Bind(ofTexture* tex);

        /// @brief Release a binding for the given texture.
        /// If all bindings are undone, the texture location will be available for use again.
        /// @return true if the texture location has been freed.
        bool Release(ofTexture* tex);

        /// @brief Release ALL bindings for a texture,
        /// and optionally pass in an output pin to unbind consumers of the FBO.
        /// Called automatically on exposed FBO output pins when deleting a Node.
        void ReleaseAll(ofTexture* tex, pins::PinOutput* pinOutFbo = nullptr);

    private:
        struct TextureLocation {
            ofTexture* texture = nullptr;
            uint32_t bindings = 0;
            uint32_t index;
        };

        std::vector<TextureLocation>::iterator FindLocation(ofTexture* tex);
        void MakeAvailable(uint32_t index);

        // Next available index in textureLocations; in-use locations
        // are kept in front of this index.
        size_t availableIndex = 0;
        std::vector<TextureLocation> textureLocations;
        pins::PushPatterns* pushPatterns;
    };
}