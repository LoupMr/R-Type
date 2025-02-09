#ifndef ENGINE_TEXTURE_HPP
#define ENGINE_TEXTURE_HPP

#include <raylib.h>
#include <string>

namespace Engine {
    class Texture {
    public:
        static Texture2D LoadTextureFromFile(const std::string& filePath);
        static void ReleaseTexture(Texture2D texture);
        static void RenderTexture(Texture2D texture, float x, float y);
    };
}

#endif // ENGINE_TEXTURE_HPP
