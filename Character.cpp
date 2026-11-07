#include "Character.h"

#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

//=============================== Animation tuning ===============================//
// angles are in radians, distances in model units (before setScale)
namespace {
    constexpr float WALK_BLEND_SPEED = 10.0f;   // how fast walk blends in/out
    constexpr float IDLE_BLEND_SPEED = 2.0f;    // how fast idle motion fades in/out
    constexpr float IDLE_DELAY = 2.0f;          // seconds standing still before idling

    // rest pose
    constexpr float LEFT_ARM_DOWN = 1.22f;      // ~70 deg down from the T-pose
    constexpr float LEFT_ELBOW_REST = 0.20f;
    constexpr float RIGHT_ARM_DOWN = 1.15f;     // staff arm
    constexpr float RIGHT_ELBOW_REST = 0.95f;   // forearm forward, holding the staff

    // walk cycle
    constexpr float STRIDE_RATE = 0.72f;        // phase per model unit travelled
    constexpr float LEG_SWING = 0.50f;
    constexpr float KNEE_BEND = 0.85f;
    constexpr float FOOT_COUNTER = 0.6f;        // feet stay roughly level with the ground
    constexpr float ARM_SWING = 0.45f;
    constexpr float STAFF_ARM_SWING = 0.20f;
    constexpr float ELBOW_SWING = 0.35f;
    constexpr float TORSO_TWIST = 0.10f;
    constexpr float FORWARD_LEAN = 0.08f;
    constexpr float WALK_BOB = 0.06f;

    // idle
    constexpr float BREATH_RATE = 2.2f;
    constexpr float BREATH_LIFT = 0.015f;
    constexpr float IDLE_ARM_SWAY = 0.05f;
    constexpr float LOOK_RATE = 0.35f;
    constexpr float LOOK_YAW = 0.30f;
    constexpr float HEAD_TILT = 0.06f;

    // staff: kept upright with its tip on the ground
    constexpr float STAFF_PIVOT_ABOVE_TIP = 2.29f;  // staff.obj: distance from its pivot down to the tip
    constexpr float GROUND_Y = 0.03f;               // height of the soles above ROOT

    glm::quat aroundX(const float a) { return glm::angleAxis(a, glm::vec3(1.0f, 0.0f, 0.0f)); }
    glm::quat aroundY(const float a) { return glm::angleAxis(a, glm::vec3(0.0f, 1.0f, 0.0f)); }
    glm::quat aroundZ(const float a) { return glm::angleAxis(a, glm::vec3(0.0f, 0.0f, 1.0f)); }

    // move current toward target smoothly, independent of frame rate
    float approach(const float current, const float target, const float speed, const float dt) {
        return current + (target - current) * (1.0f - std::exp(-speed * dt));
    }
}

void PumpkinCharacter::initialize(const ShaderInfo& shaderInfo) {
    // one default Part per PartID, so the enum values index directly into parts
    parts.resize(PART_COUNT);

    // load the mesh of each body part (ROOT is a joint only, so it has no mesh)
    // torso and head
    parts[TORSO].model.emplace("assets/character/obj_parts/torso.obj", shaderInfo);
    parts[HEAD].model.emplace("assets/character/obj_parts/head.obj", shaderInfo);

    // left arm
    parts[ARM_L].model.emplace("assets/character/obj_parts/arm_l.obj", shaderInfo);
    parts[ELBOW_L].model.emplace("assets/character/obj_parts/elbow_l.obj", shaderInfo);
    parts[HAND_L].model.emplace("assets/character/obj_parts/hand_l.obj", shaderInfo);

    // right arm
    parts[ARM_R].model.emplace("assets/character/obj_parts/arm_r.obj", shaderInfo);
    parts[ELBOW_R].model.emplace("assets/character/obj_parts/elbow_r.obj", shaderInfo);
    parts[HAND_R].model.emplace("assets/character/obj_parts/hand_r.obj", shaderInfo);

    // left leg
    parts[LEG_L].model.emplace("assets/character/obj_parts/leg_l.obj", shaderInfo);
    parts[KNEE_L].model.emplace("assets/character/obj_parts/knee_l.obj", shaderInfo);
    parts[FOOT_L].model.emplace("assets/character/obj_parts/foot_l.obj", shaderInfo);

    // right leg
    parts[LEG_R].model.emplace("assets/character/obj_parts/leg_r.obj", shaderInfo);
    parts[KNEE_R].model.emplace("assets/character/obj_parts/knee_r.obj", shaderInfo);
    parts[FOOT_R].model.emplace("assets/character/obj_parts/foot_r.obj", shaderInfo);

    // staff held in the right hand
    parts[STAFF].model.emplace("assets/character/obj_parts/staff.obj", shaderInfo);

    // place each joint relative to its parent
    loadOffsets("assets/character/obj_parts/parts.csv");

    // build the hierarchy: a part's rotation also moves all of its children
    // ROOT moves the whole character
    parts[ROOT].children = {
        TORSO, LEG_L, LEG_R
    };

    parts[TORSO].children = {
        HEAD, ARM_L, ARM_R
    };

    // left arm: shoulder -> elbow -> hand
    parts[ARM_L].children = {ELBOW_L};
    parts[ELBOW_L].children = {HAND_L};

    // right arm: shoulder -> elbow -> hand -> staff
    parts[ARM_R].children = {ELBOW_R};
    parts[ELBOW_R].children = {HAND_R};
    parts[HAND_R].children = {STAFF};

    // left leg: hip -> knee -> foot
    parts[LEG_L].children = {KNEE_L};
    parts[KNEE_L].children = {FOOT_L};

    // right leg: hip -> knee -> foot
    parts[LEG_R].children = {KNEE_R};
    parts[KNEE_R].children = {FOOT_R};

    // start in the rest pose instead of the T-pose
    applyPose();
}

void PumpkinCharacter::loadOffsets(const char* filename) {
    // names used in the "part" column of the csv
    static const std::map<std::string, PartID> PART_IDS = {
        {"ROOT", ROOT}, {"TORSO", TORSO}, {"HEAD", HEAD},
        {"ARM_L", ARM_L}, {"ELBOW_L", ELBOW_L}, {"HAND_L", HAND_L},
        {"ARM_R", ARM_R}, {"ELBOW_R", ELBOW_R}, {"HAND_R", HAND_R},
        {"LEG_L", LEG_L}, {"KNEE_L", KNEE_L}, {"FOOT_L", FOOT_L},
        {"LEG_R", LEG_R}, {"KNEE_R", KNEE_R}, {"FOOT_R", FOOT_R},
        {"STAFF", STAFF}
    };

    std::ifstream file(filename);
    if (!file) {
        fprintf(stderr, "[ERROR]: Could not open character offsets file \"%s\"\n", filename);
        return;
    }

    // skip the header row
    std::string line;
    std::getline(file, line);

    // each row is: part,parent,offset_x,offset_y,offset_z,obj_file,mesh_objects,triangles
    while (std::getline(file, line)) {
        std::stringstream row(line);
        std::string name, parent;
        std::getline(row, name, ',');
        std::getline(row, parent, ',');

        glm::vec3 offset;
        char comma;
        row >> offset.x >> comma >> offset.y >> comma >> offset.z;

        const auto id = PART_IDS.find(name);
        if (row.fail() || id == PART_IDS.end()) {
            fprintf(stderr, "[ERROR]: Skipping bad row in \"%s\": %s\n", filename, line.c_str());
            continue;
        }
        parts[id->second].offset = offset;
    }
}

void PumpkinCharacter::update(const float dt, const float distanceMoved) {
    time += dt;

    // walking blends in while the character moves, and back out when it stops
    const bool moving = distanceMoved > 0.0001f;
    walkStrength = approach(walkStrength, moving ? 1.0f : 0.0f, WALK_BLEND_SPEED, dt);

    // the stride follows the distance actually covered, so feet don't slide
    walkPhase += distanceMoved / scale.y * STRIDE_RATE;

    // idle motion fades in after standing still for a while
    standTime = moving ? 0.0f : standTime + dt;
    idleStrength = approach(idleStrength, standTime >= IDLE_DELAY ? 1.0f : 0.0f, IDLE_BLEND_SPEED, dt);

    applyPose();
}

void PumpkinCharacter::applyPose() {
    const float walk = walkStrength;
    const float idle = (1.0f - walkStrength) * idleStrength;

    // shared signals
    const float stride = std::sin(walkPhase);           // +1 = left leg back
    const float breath = std::sin(time * BREATH_RATE);
    const float look = std::sin(time * LOOK_RATE);

    //==================================== Legs ====================================//
    // knees bend while the leg swings through, so the foot lifts off the ground
    const float leftLeg = walk * LEG_SWING * stride;
    const float rightLeg = -walk * LEG_SWING * stride;
    const float leftKnee = walk * KNEE_BEND * std::max(0.0f, std::sin(walkPhase + 0.9f));
    const float rightKnee = walk * KNEE_BEND * std::max(0.0f, -std::sin(walkPhase + 0.9f));

    parts[LEG_L].rotation = aroundX(leftLeg);
    parts[LEG_R].rotation = aroundX(rightLeg);
    parts[KNEE_L].rotation = aroundX(leftKnee);
    parts[KNEE_R].rotation = aroundX(rightKnee);
    parts[FOOT_L].rotation = aroundX(-(leftLeg + leftKnee) * FOOT_COUNTER);
    parts[FOOT_R].rotation = aroundX(-(rightLeg + rightKnee) * FOOT_COUNTER);

    //========================= Body bob, lean, and breath =========================//
    // lowest when the legs are spread, highest when they pass each other
    const float bob = walk * WALK_BOB * (std::cos(2.0f * walkPhase) - 1.0f) * 0.5f;
    parts[ROOT].shift = glm::vec3(0.0f, bob, 0.0f);

    parts[TORSO].shift = glm::vec3(0.0f, idle * BREATH_LIFT * breath, 0.0f);
    parts[TORSO].rotation = aroundY(walk * TORSO_TWIST * stride) * aroundX(walk * FORWARD_LEAN);

    //==================================== Head ====================================//
    // looks around when idle, stays facing forward against the torso twist when walking
    parts[HEAD].rotation =
        aroundY(idle * LOOK_YAW * look - walk * TORSO_TWIST * stride) *
        aroundZ(idle * HEAD_TILT * std::sin(time * LOOK_RATE * 0.7f)) *
        aroundX(-walk * FORWARD_LEAN);

    //==================================== Arms ====================================//
    // arms swing opposite to the legs; the staff arm swings less
    const float leftArmDown = LEFT_ARM_DOWN + idle * IDLE_ARM_SWAY * breath;
    const float rightArmDown = RIGHT_ARM_DOWN + idle * IDLE_ARM_SWAY * breath;

    parts[ARM_L].rotation = aroundX(-walk * ARM_SWING * stride) * aroundZ(leftArmDown);
    parts[ARM_R].rotation = aroundX(walk * STAFF_ARM_SWING * stride) * aroundZ(-rightArmDown);
    parts[ELBOW_L].rotation = aroundY(LEFT_ELBOW_REST + walk * ELBOW_SWING * std::max(0.0f, -stride));
    parts[ELBOW_R].rotation = aroundY(-RIGHT_ELBOW_REST);
    parts[HAND_L].rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    parts[HAND_R].rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    //=================================== Staff ====================================//
    // the staff stays upright with its tip near the ground, wherever the hand is:
    // find the hand in ROOT space, place the staff there, then convert back to hand space
    const glm::mat4 handInRoot =
        parts[TORSO].localMatrix() *
        parts[ARM_R].localMatrix() *
        parts[ELBOW_R].localMatrix() *
        parts[HAND_R].localMatrix();
    const glm::vec3 grip = glm::vec3(handInRoot * glm::vec4(parts[STAFF].offset, 1.0f));

    // lift the tip a little as the staff arm swings forward
    const float tipLift = walk * 0.12f * std::max(0.0f, -stride);
    const float staffY = GROUND_Y + STAFF_PIVOT_ABOVE_TIP + tipLift - bob;

    const glm::mat4 staffInRoot =
        glm::translate(glm::mat4(1.0f), glm::vec3(grip.x, staffY, grip.z)) *
        glm::mat4_cast(aroundX(walk * 0.10f * stride) * aroundZ(-0.05f));
    const glm::mat4 staffInHand = glm::inverse(handInRoot) * staffInRoot;

    // shift moves the staff from its csv offset to the computed spot
    parts[STAFF].shift = glm::vec3(staffInHand[3]) - parts[STAFF].offset;
    parts[STAFF].rotation = glm::normalize(glm::quat_cast(glm::mat3(staffInHand)));
}

void PumpkinCharacter::draw(const std::function<void(const glm::mat4&)>& sendModelMtx) const {
    // place the whole character in the world (rotation is in radians, around +Y)
    glm::mat4 modelMtx = glm::translate(glm::mat4(1.0f), position);
    modelMtx = glm::rotate(modelMtx, rotation, glm::vec3(0.0f, 1.0f, 0.0f));
    modelMtx = glm::scale(modelMtx, scale);

    drawPart(ROOT, modelMtx, sendModelMtx);
}

void PumpkinCharacter::drawPart(int id, const glm::mat4& parentMtx,
                                const std::function<void(const glm::mat4&)>& sendModelMtx) const {
    const Part& part = parts[id];

    // move to this part's joint, then rotate around it
    const glm::mat4 modelMtx = parentMtx * part.localMatrix();

    if (part.model.has_value()) {   // ROOT has no mesh
        sendModelMtx(modelMtx);
        part.model->draw();
    }

    // children inherit this part's matrix
    for (const int child : part.children) {
        drawPart(child, modelMtx, sendModelMtx);
    }
}
