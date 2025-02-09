#include "Texture.hpp"

namespace Engine {
    Texture2D Texture::LoadTextureFromFile(const std::string& filePath) {
        return LoadTexture(filePath.c_str());
    }

    void Texture::ReleaseTexture(Texture2D texture) {
        UnloadTexture(texture);
    }

    void Texture::RenderTexture(Texture2D texture, float x, float y) {
        Vector2 position = {x, y};
        DrawTextureV(texture, position, WHITE);
    }
}
