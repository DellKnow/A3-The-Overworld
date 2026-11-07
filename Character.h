// Character.h
#ifndef A3_CHARACTER_H
#define A3_CHARACTER_H

#include <CSCI441/materials.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <CSCI441/ModelLoader.hpp>
#include <vector>
#include <optional>
#include <functional>

// identifies each body part; values are indices into PumpkinCharacter::parts
// _L is the -X side and _R the +X side, i.e. left/right when looking at the
// character's front (it faces +Z)
enum PartID {
    // ROOT places the whole character and has no mesh
    ROOT, TORSO, HEAD,

    // left arm: shoulder, elbow, hand
    ARM_L, ELBOW_L, HAND_L,

    // right arm: shoulder, elbow, hand
    ARM_R, ELBOW_R, HAND_R,

    // left leg: hip, knee, foot
    LEG_L, KNEE_L, FOOT_L,

    // right leg: hip, knee, foot
    LEG_R, KNEE_R, FOOT_R,

    // staff held in the right hand
    STAFF,

    // number of parts, must stay last
    PART_COUNT
};

struct ShaderInfo {
    GLuint program;             // shader's program

    GLint position;             // position of the mesh
    GLint normal;               // normal vector of the mesh

    GLint diffuse;              // diffuse material color of the mesh
    GLint specular;             // specular material color of the mesh
    GLint ambient;              // ambient material color of the mesh
    GLint shininess;            // shininess for specular illumination of the mesh
};

// wrapper class for CSCI441::ModelLoader
class CharacterMesh {
private:
    // model of the body part stored in ModelLoader
    CSCI441::ModelLoader model;
    // shader information
    ShaderInfo shader;
public:
    // constructor that takes filename, and shader information of the part
    CharacterMesh(const char* filename, const ShaderInfo& shaderInfo) 
            : model(filename), shader(shaderInfo) {
        // set attributes for the model
        model.setAttributeLocations(
            shader.position,
            shader.normal
        );
    }
    // draw the body part using .draw() from ModelLoader
    void draw() const {
        model.draw(
            shader.program,
            shader.diffuse,
            shader.specular,
            shader.shininess,
            shader.ambient
        );
    }
};

struct Part {
    std::optional<CharacterMesh> model;         //  Empty for joints without geometry

    glm::vec3 offset{0.0f};                     // joint position relative to the parent (from parts.csv)
    glm::vec3 shift{0.0f};                      // extra animated translation (walk bob, breathing)
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f}; // animated rotation around the joint

    std::vector<int> children;                  // indices from std::vector<Part> in PumpkinCharacter

    // this part's matrix relative to its parent: move to the joint, then rotate
    glm::mat4 localMatrix() const {
        return glm::translate(glm::mat4(1.0f), offset + shift) * glm::mat4_cast(rotation);
    }
};

class PumpkinCharacter {
private:
    // store parts of the pumpkin character
    std::vector<Part> parts;

    glm::vec3 position{0.0f};   // position of the pumpkin character
    float rotation = 0.0f;      // rotation of the pumpkin character (axes defined by body parts)
    glm::vec3 scale{1.0f};      // scaled pumpkin character if needed

    // animation state
    float walkPhase = 0.0f;     // stride phase in radians, advances with distance travelled
    float walkStrength = 0.0f;  // 0 = standing, 1 = walking (blended over time)
    float idleStrength = 0.0f;  // 0 = rest pose, 1 = full idle motion (blended over time)
    float standTime = 0.0f;     // seconds since the character last moved
    float time = 0.0f;          // total animated time, drives breathing and looking around

    // pose every joint from the current animation state
    void applyPose();

    void loadOffsets(const char* filename);         // read each part's offset from parts.csv

    // draw a part and its children; parentMtx is the parent's model matrix
    void drawPart(int id, const glm::mat4& parentMtx,
                  const std::function<void(const glm::mat4&)>& sendModelMtx) const;
public:
    void initialize(const ShaderInfo& shaderInfo);  // initialize the pumpkin character
    void setPosition(const glm::vec3& newPosition) { position = newPosition; }  // place the character in the world
    void setRotation(const float& newRotation) { rotation = newRotation; } // rotate the character in the world
    void setScale(const glm::vec3& newScale) { scale = newScale; } // scale the character in the world
    void moveCharacter(const glm::vec3& dir) { position += dir; }   // move the character in the world in the given direction
    // animate the character every frame; distanceMoved is how far it walked this frame (world units)
    // walking and idling blend smoothly, idle motion starts after 2 seconds of standing still
    void update(float dt, float distanceMoved);
    void reset();                                   // reset the character to the initial position
    // draw the character; sendModelMtx is called with each part's model matrix before it is drawn
    void draw(const std::function<void(const glm::mat4&)>& sendModelMtx) const;
};

#endif // A3_CHARACTER_H