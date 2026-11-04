// World.h
#ifndef A3_WORLD_H
#define A3_WORLD_H

#include <CSCI441/materials.hpp>
#include <glm/glm.hpp>
#include <vector>
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

/// \desc what a block is made of
enum class BlockType { SNOW, STONE, PEAK, ROAD, WOOD, ROOF, GRASS, TRUNK, LEAVES, WATER, COUNT };
constexpr int NUM_BLOCK_TYPES = static_cast<int>(BlockType::COUNT);

/// \desc one box in the world
struct Block {
    glm::vec3 position;   // center of the box
    glm::vec3 scale;      // (1, height, 1) for a column, bigger for houses
};

class World {
public:
    static constexpr int SIZE = 128;                    // grid cells per side
    static constexpr int HALF = SIZE / 2;               // world spans [-HALF, HALF) in x and z

    /// \desc generates Oxenfurt: heights, block types, houses, trees
    void build();

    const std::vector<Block>& blocks(BlockType t) const { return _blocks[static_cast<int>(t)]; }
    static const CSCI441::Materials::Material& material(BlockType t, bool textured = 0);
    /// \desc texture image for a block type, or nullptr if it is drawn with its material color only
    static const char* textureFile(BlockType t);

    /// \desc height of the ground under world (x, z); 0 = water or outside the grid
    GLfloat groundHeight(GLfloat x, GLfloat z) const;
    /// \desc can the hero stand at (x, z) when his feet are at currentHeight
    bool isWalkable(GLfloat x, GLfloat z, GLfloat currentHeight) const;
    // get a random valid position in the world
    // the position should be on the ground
    // not adjacent to any solid within R
    glm::vec3 getValidPosition(int R = 1);

private:
    int  _heights[SIZE][SIZE] = {};                 // 0 = water
    bool _solid[SIZE][SIZE]   = {};                 // houses and trees block movement
    BlockType _material[SIZE][SIZE] = {};           // material placed on the block
    std::vector<Block> _blocks[NUM_BLOCK_TYPES];    // grouped by type: bind each material once

    /// \desc world (x, z) -> grid cell (i, j); false if outside the grid
    static bool _toCell(GLfloat x, GLfloat z, int& i, int& j);

    void _addColumn(int i, int j, int height, BlockType type);
    void _addHouse(int i, int j);
    void _addTree(int i, int j);
};

struct Cloud {
    glm::vec3 position;
    glm::vec2 size;     // width and depth
};

#endif // A3_WORLD_H
