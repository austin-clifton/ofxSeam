#include "seam/textureLocationResolver.h"
#include "seam/pins/push.h"

using namespace seam;

TextureLocationResolver::TextureLocationResolver(pins::PushPatterns* _pushPatterns, uint32_t availableLocations) {
    pushPatterns = _pushPatterns;
    textureLocations.resize(availableLocations);

    for (uint32_t i = 0; i <= availableLocations; i++) {
        textureLocations[i].index = i + 1;
    }
}

uint32_t TextureLocationResolver::Bind(ofTexture* tex) {
    // I'm gonna assume this list is fairly short, and the fastest way to look up
    // an existing texture location binding is to just search the textureLocations vector.
    auto existing = FindLocation(tex);

    if (existing != textureLocations.end()) {
        existing->bindings += 1;

        printf("Incrementing ref bindings for existing texture binding %u to %u\n",
            existing->index, existing->bindings);

        return existing->index;
    } else {
        // No more locations available!
        if (availableIndex == textureLocations.size()) {
            // TODO whine loudly?
            return UINT32_MAX;
        }

        // Take the location at the next available index.
        TextureLocation& taken = textureLocations[availableIndex];
        assert(taken.bindings == 0);
        availableIndex += 1;
        taken.bindings = 1;

        taken.texture = tex;

        printf("New texture binding to index %u\n", taken.index);

        return taken.index;
    }
}

bool TextureLocationResolver::Release(ofTexture* tex) {
    auto existing = FindLocation(tex);

    if (existing != textureLocations.end()) {
        assert(existing->bindings > 0);
        existing->bindings -= 1;
        if (existing->bindings == 0) {
            MakeAvailable(std::distance(textureLocations.begin(), existing));
            return true;
        }
    }
    return false;
}

void TextureLocationResolver::ReleaseAll(ofTexture* tex, pins::PinOutput* pinOutFbo) {
    auto existing = FindLocation(tex);
    if (existing != textureLocations.end()) {
        assert(existing->bindings > 0);
        existing->bindings = 0;
        MakeAvailable(std::distance(textureLocations.begin(), existing));
    }

    if (pinOutFbo != nullptr) {
        // Push nullptr to FBO pin connections
        pushPatterns->PushSingle<ofFbo*>(*pinOutFbo, nullptr);
    }
}

std::vector<TextureLocationResolver::TextureLocation>::iterator 
    TextureLocationResolver::FindLocation(ofTexture* tex) 
{
    return std::find_if(textureLocations.begin(), textureLocations.end(), [tex](const TextureLocation& other) {
        return tex == other.texture;
    });
}

void TextureLocationResolver::MakeAvailable(uint32_t index) {
    assert(availableIndex > 0 && index < availableIndex && textureLocations[index].bindings == 0);

    printf("Making texture index %u available\n", index);

    textureLocations[index].texture = nullptr;

    // Swap the last unavailable texture location with the index that should be made available.
    std::swap(textureLocations[availableIndex - 1], textureLocations[index]);
    availableIndex -= 1;
}
