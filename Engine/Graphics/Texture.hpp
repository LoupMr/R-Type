#ifndef GRAPHICAL_LIBRARY_TEXTURE_HPP
#define GRAPHICAL_LIBRARY_TEXTURE_HPP

#include "Core/Core.hpp"

namespace GraphicalLibrary {
    class Texture {
    public:
        static Texture2D LoadTextureFromFile(const std::string& filePath); // Renamed from LoadFromFile
        static void ReleaseTexture(Texture2D texture); // Renamed from Unload
        static void RenderTexture(Texture2D texture, float x, float y); // Renamed from Draw
    };
}

#endif // GRAPHICAL_LIBRARY_TEXTURE_HPP