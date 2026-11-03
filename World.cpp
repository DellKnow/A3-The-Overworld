#include "World.h"
#include <cmath>
#include <random>

namespace {
    /// \brief Simple helper class to return a random number between 0.0f and 1.0f.
    class RandomGenerator {
    public:
        /// \brief generates a random value
        /// \returns random GLfloat within the inclusive range [0.0f, 1.0f]
        static GLfloat getRand() { return _dist( _rng ); }

        // do not allow an object to be copied nor moved
        RandomGenerator(RandomGenerator&) = delete;
        RandomGenerator& operator=(RandomGenerator&) = delete;
        RandomGenerator(RandomGenerator&&) = delete;
        RandomGenerator& operator=(RandomGenerator&&) = delete;
    private:
        // do not allow an object to be externally created
        RandomGenerator() {};
        static std::mt19937 _rng;
        static std::uniform_real_distribution<GLfloat> _dist;
    };

    std::mt19937 RandomGenerator::_rng = std::mt19937( std::random_device{}() );
    std::uniform_real_distribution<GLfloat> RandomGenerator::_dist = std::uniform_real_distribution<GLfloat>( 0.0f, std::nextafter( 1.0f, 2.0f ) );
}

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
        /* WATER  */ {{0.45f, 0.68f, 0.82f, 1}, {0.12f, 0.15f, 0.18f, 1}, 48.0f, {0.08f, 0.12f, 0.16f, 1}},
    };

    // texture per block type (nullptr = material color only); indexed like MATERIALS
    constexpr const char* TEXTURE_FILES[NUM_BLOCK_TYPES] = {
        /* SNOW   */ "assets/textures/classic/snow.png",
        /* STONE  */ "assets/textures/classic/stone.png",
        /* PEAK   */ "assets/textures/classic/peak.png",
        /* ROAD   */ "assets/textures/classic/road.png",
        /* WOOD   */ "assets/textures/classic/wood.png",
        /* ROOF   */ "assets/textures/classic/roof.png",
        /* GRASS  */ "assets/textures/classic/grass.png",
        /* TRUNK  */ "assets/textures/classic/trunk.png",
        /* LEAVES */ "assets/textures/classic/leaves.png",
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
    _material[i][j] = type;
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
            _material[i][j] = BlockType::WATER;
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

    // place trees randomly on the snow
    constexpr int NUM_TREES = 15;
    for (int placed = 0, attempts = 0; placed < NUM_TREES && attempts < 1000; attempts++) {
        // Generate random coordinate
        const int i = std::min(static_cast<int>(RandomGenerator::getRand() * SIZE), SIZE - 1);
        const int j = std::min(static_cast<int>(RandomGenerator::getRand() * SIZE), SIZE - 1);

        // the whole 3x3 canopy footprint must be open snow
        bool allSnow  = 1;
        for (int di = -1; di <= 1; di++) {
            for (int dj = -1; dj <= 1; dj++) {
                const int ci = i + di, cj = j + dj;
                if (ci >= 0 && ci < SIZE && cj >= 0 && cj < SIZE) {
                    allSnow  &= _material[ci][cj] == BlockType::SNOW;
                }
            }
        }
        // if nothing was found, place a tree there
        if (allSnow) {
            _addTree(i, j);
            placed++;
        }
    }
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
    // cell center
    const glm::vec3 base(i - HALF + 0.5f, static_cast<GLfloat>(_heights[i][j]), j - HALF + 0.5f);

    // walls: 5x3x5 box; the cube is centered, so lift by half its height
    _blocks[static_cast<int>(BlockType::WOOD)].push_back({ base + glm::vec3(0.0f, 1.5f, 0.0f), glm::vec3(5.0f, 3.0f, 5.0f) });

    // stepped roof: wide slab, then a small cap
    _blocks[static_cast<int>(BlockType::ROOF)].push_back({ base + glm::vec3(0.0f, 3.5f, 0.0f), glm::vec3(5.0f, 1.0f, 5.0f) });
    _blocks[static_cast<int>(BlockType::ROOF)].push_back({ base + glm::vec3(0.0f, 4.5f, 0.0f), glm::vec3(3.0f, 1.0f, 3.0f) });

    // the hero can't walk through the 5x5 footprint
    for (int di = -2; di <= 2; di++) {
        for (int dj = -2; dj <= 2; dj++) {
            const int ci = i + di, cj = j + dj;
            if (ci >= 0 && ci < SIZE && cj >= 0 && cj < SIZE) {
                _solid[ci][cj] = true;
                _material[ci][cj] = BlockType::ROOF;
            }
        }
    }
}

void World::_addTree(const int i, const int j) {
    // trunk height 3 or 4
    const GLfloat trunkHeight = 3.0f + std::min(static_cast<int>(RandomGenerator::getRand() * 2), 1);
    // cell center
    const glm::vec3 base(i - HALF + 0.5f, static_cast<GLfloat>(_heights[i][j]), j - HALF + 0.5f);

    // the hero can't walk through the tree
    _solid[i][j] = true;

    // trunk: 1 x h x 1 column resting on the ground
    _blocks[static_cast<int>(BlockType::TRUNK)].push_back({ base + glm::vec3(0.0f, trunkHeight * 0.5f, 0.0f),
                                                            glm::vec3(1.0f, trunkHeight, 1.0f) });
    // canopy: 3 x 2 x 3, overlapping the trunk top by half a block
    _blocks[static_cast<int>(BlockType::LEAVES)].push_back({ base + glm::vec3(0.0f, trunkHeight + 0.5f, 0.0f),
                                                             glm::vec3(3.0f, 2.0f, 3.0f) });
    // trunk blocks movement
    _solid[i][j] = true;
    for (int di = -1; di <= 1; di++)
        for (int dj = -1; dj <= 1; dj++)
            _material[i + di][j + dj] = BlockType::LEAVES; 
    _material[i][j] = BlockType::TRUNK;
}