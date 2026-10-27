// World.h
#ifndef A3_WORLD_H
#define A3_WORLD_H

#include <CSCI441/materials.hpp>
#include <glm/glm.hpp>
#include <vector>

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
    static const CSCI441::Materials::Material& material(BlockType t);
    /// \desc texture image for a block type, or nullptr if it is drawn with its material color only
    static const char* textureFile(BlockType t);

    /// \desc height of the ground under world (x, z); 0 = water or outside the grid
    GLfloat groundHeight(GLfloat x, GLfloat z) const;
    /// \desc can the hero stand at (x, z) when his feet are at currentHeight
    bool isWalkable(GLfloat x, GLfloat z, GLfloat currentHeight) const;

private:
    int  _heights[SIZE][SIZE] = {};                 // 0 = water
    bool _solid[SIZE][SIZE]   = {};                 // houses and trees block movement
    std::vector<Block> _blocks[NUM_BLOCK_TYPES];    // grouped by type: bind each material once

    /// \desc world (x, z) -> grid cell (i, j); false if outside the grid
    static bool _toCell(GLfloat x, GLfloat z, int& i, int& j);

    void _addColumn(int i, int j, int height, BlockType type);
    void _addHouse(int i, int j);
    void _addTree(int i, int j);
};

#endif // A3_WORLD_H
