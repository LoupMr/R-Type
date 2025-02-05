#ifndef GRAPHICAL_LIBRARY_TEXTURE_HPP
#define GRAPHICAL_LIBRARY_TEXTURE_HPP

#include "Core/Core.hpp"

namespace GraphicalLibrary {
    class Texture {
    public:
        static Texture2D LoadTextureFromFile(const std::string& filePath);
        static void ReleaseTexture(Texture2D texture);
        static void RenderTexture(Texture2D texture, float x, float y);
    };
}

#endif // GRAPHICAL_LIBRARY_TEXTURE_HPP