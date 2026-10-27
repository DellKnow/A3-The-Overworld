#include "World.h"
#include <cmath>

namespace {
    // {diffuse}, {specular}, shininess, {ambient}
    constexpr CSCI441::Materials::Material MATERIALS[NUM_BLOCK_TYPES] = {
        /* SNOW   */ {{0.90f, 0.92f, 0.96f, 1}, {0.30f, 0.30f, 0.35f, 1},  8.0f, {0.22f, 0.24f, 0.30f, 1}},
        /* STONE  */ {{0.50f, 0.50f, 0.52f, 1}, {0.15f, 0.15f, 0.15f, 1}, 10.0f, {0.12f, 0.12f, 0.14f, 1}},
        /* PEAK   */ {{0.95f, 0.96f, 1.00f, 1}, {0.40f, 0.40f, 0.45f, 1}, 12.0f, {0.24f, 0.26f, 0.32f, 1}},
        /* ROAD   */ {{0.78f, 0.76f, 0.72f, 1}, {0.10f, 0.10f, 0.10f, 1},  4.0f, {0.18f, 0.18f, 0.18f, 1}},
        /* WOOD   */ {{0.43f, 0.28f, 0.20f, 1}, {0.10f, 0.08f, 0.06f, 1},  6.0f, {0.11f, 0.07f, 0.05f, 1}},
        /* ROOF   */ {{0.53f, 0.00f, 0.08f, 1}, {0.30f, 0.20f, 0.20f, 1}, 20.0f, {0.13f, 0.00f, 0.02f, 1}},
        /* GRASS  */ {{0.25f, 0.55f, 0.20f, 1}, {0.05f, 0.08f, 0.05f, 1},  4.0f, {0.06f, 0.14f, 0.05f, 1}},
        /* TRUNK  */ {{0.35f, 0.22f, 0.12f, 1}, {0.05f, 0.04f, 0.03f, 1},  4.0f, {0.09f, 0.06f, 0.03f, 1}},
        /* LEAVES */ {{0.15f, 0.42f, 0.15f, 1}, {0.10f, 0.15f, 0.10f, 1}, 10.0f, {0.04f, 0.10f, 0.04f, 1}},
        // the texture supplies the water's color, so diffuse/ambient stay near white; strong specular = sun glint
        /* WATER  */ {{0.85f, 0.90f, 1.00f, 1}, {0.80f, 0.80f, 0.80f, 1}, 64.0f, {0.30f, 0.32f, 0.38f, 1}},
    };

    // texture per block type (nullptr = material color only); indexed like MATERIALS
    constexpr const char* TEXTURE_FILES[NUM_BLOCK_TYPES] = {
        /* SNOW   */ nullptr,
        /* STONE  */ nullptr,
        /* PEAK   */ nullptr,
        /* ROAD   */ nullptr,
        /* WOOD   */ nullptr,
        /* ROOF   */ nullptr,
        /* GRASS  */ nullptr,
        /* TRUNK  */ nullptr,
        /* LEAVES */ nullptr,
        /* WATER  */ "assets/textures/classic/water.png",
    };

    constexpr GLfloat ISLAND_RX = 48.0f;   // ellipse radius along x
    constexpr GLfloat ISLAND_RZ = 32.0f;   // ellipse radius along z
}

const CSCI441::Materials::Material& World::material(const BlockType t) {
    return MATERIALS[static_cast<int>(t)];
}

const char* World::textureFile(const BlockType t) {
    return TEXTURE_FILES[static_cast<int>(t)];
}

bool World::_toCell(const GLfloat x, const GLfloat z, int& i, int& j) {
    i = static_cast<int>(std::floor(x + HALF));
    j = static_cast<int>(std::floor(z + HALF));
    return i >= 0 && i < SIZE && j >= 0 && j < SIZE;
}

void World::_addColumn(const int i, const int j, const int height, const BlockType type) {
    _heights[i][j] = height;
    // the unit cube is centered, so lift it by half its height to rest on y = 0
    const glm::vec3 center(i - HALF + 0.5f, height * 0.5f, j - HALF + 0.5f);
    _blocks[static_cast<int>(type)].push_back({ center, glm::vec3(1.0f, static_cast<GLfloat>(height), 1.0f) });
}

void World::build() {
    // sea: one slab covering the whole grid
    _blocks[static_cast<int>(BlockType::WATER)].push_back({ glm::vec3(0.0f), glm::vec3(SIZE, 1.0f, SIZE) });

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            // normalized ellipse distance: 0 at the center, 1 at the shore
            const GLfloat x = i - HALF + 0.5f, z = j - HALF + 0.5f;
            const GLfloat d = std::sqrt((x / ISLAND_RX) * (x / ISLAND_RX) + (z / ISLAND_RZ) * (z / ISLAND_RZ));
            if (d >= 1.0f) continue;                              // water: height stays 0

            if      (d < 0.25f) _addColumn(i, j, 9, BlockType::PEAK);
            else if (d < 0.40f) _addColumn(i, j, 6, BlockType::STONE);
            else if (d < 0.55f) _addColumn(i, j, 3, BlockType::STONE);
            else if (std::fabs(d - 0.65f) < 0.03f || std::fabs(d - 0.85f) < 0.03f)
                                _addColumn(i, j, 1, BlockType::ROAD);
            else                _addColumn(i, j, 1, BlockType::SNOW);
        }
    }

    // 4 houses on the ring between the two roads, at the diagonals
    for (int k = 0; k < 4; k++) {
        const GLfloat angle = glm::quarter_pi<GLfloat>() + k * glm::half_pi<GLfloat>();
        int i, j;
        if (_toCell(0.75f * ISLAND_RX * std::cos(angle), 0.75f * ISLAND_RZ * std::sin(angle), i, j))
            _addHouse(i, j);
    }

    // TODO: grass corner + trees, bridge from the shore to the corner
}

GLfloat World::groundHeight(const GLfloat x, const GLfloat z) const {
    int i, j;
    return _toCell(x, z, i, j) ? static_cast<GLfloat>(_heights[i][j]) : 0.0f;
}

bool World::isWalkable(const GLfloat x, const GLfloat z, const GLfloat currentHeight) const {
    int i, j;
    if (!_toCell(x, z, i, j))   return false;                 // off the grid (rubric: stay on it)
    if (_heights[i][j] == 0)    return false;                 // water
    if (_solid[i][j])           return false;                 // house or tree
    return _heights[i][j] - currentHeight <= 1.0f;            // can climb at most one block
}

void World::_addHouse(const int i, const int j) {
    // cell center, standing on top of whatever ground is there (snow or road, height 1)
    const glm::vec3 base(i - HALF + 0.5f, static_cast<GLfloat>(_heights[i][j]), j - HALF + 0.5f);

    // walls: 3x3x3 box; the cube is centered, so lift by half its height
    _blocks[static_cast<int>(BlockType::WOOD)].push_back({ base + glm::vec3(0.0f, 1.5f, 0.0f), glm::vec3(3.0f, 3.0f, 3.0f) });

    // stepped roof: wide slab, then a small cap
    _blocks[static_cast<int>(BlockType::ROOF)].push_back({ base + glm::vec3(0.0f, 3.5f, 0.0f), glm::vec3(3.0f, 1.0f, 3.0f) });
    _blocks[static_cast<int>(BlockType::ROOF)].push_back({ base + glm::vec3(0.0f, 4.5f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f) });

    // the hero can't walk through the 3x3 footprint
    for (int di = -1; di <= 1; di++) {
        for (int dj = -1; dj <= 1; dj++) {
            const int ci = i + di, cj = j + dj;
            if (ci >= 0 && ci < SIZE && cj >= 0 && cj < SIZE) _solid[ci][cj] = true;
        }
    }
}
