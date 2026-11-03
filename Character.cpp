#include "Character.h"

#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <glm/gtc/matrix_transform.hpp>

constexpr static GLfloat PI = 3.14159265359;

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

    // Lower both arms from the initial T-pose
    parts[ARM_L].rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);
    parts[ARM_R].rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);

    parts[ARM_L].angle = glm::radians(65.0f);
    parts[ARM_R].angle = glm::radians(-65.0f);

    parts[STAFF].rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);
    parts[STAFF].angle = glm::radians(65.0f);
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

void PumpkinCharacter::update(float t) {
    float swing = sin(t / animationTime * 2.0f * PI);
    parts[LEG_L].angle = 0.45f * swing;
    parts[LEG_R].angle = -0.45f * swing;

    parts[ARM_L].angle = glm::radians(65.0f) - 0.30f * swing;
    parts[ARM_R].angle = glm::radians(-65.0f) + 0.30f * swing;

    parts[KNEE_L].angle =
        0.35f * std::max(0.0f, -swing);

    parts[KNEE_R].angle =
        0.35f * std::max(0.0f, swing);

    parts[STAFF].angle = -parts[ARM_R].angle;
}

void PumpkinCharacter::updateIdle(float t) {
    float slow = sin(1.2f * t);
    float slower = sin(0.7f * t);
    float tiny = sin(2.0f * t);

    // torso: tiny sway
    parts[TORSO].rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);
    parts[TORSO].angle = glm::radians(2.0f) * slow;

    // head: slight left-right motion
    parts[HEAD].rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    parts[HEAD].angle = glm::radians(5.0f) * slower;

    // keep arms dropped, but move a tiny bit
    parts[ARM_L].rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);
    parts[ARM_R].rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);

    parts[ARM_L].angle = glm::radians(65.0f) + glm::radians(4.0f) * slow;
    parts[ARM_R].angle = glm::radians(-65.0f) - glm::radians(4.0f) * slow;

    // slight elbow motion
    parts[ELBOW_L].rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    parts[ELBOW_R].rotationAxis = glm::vec3(1.0f, 0.0f, 0.0f);

    parts[ELBOW_L].angle = glm::radians(6.0f) * tiny;
    parts[ELBOW_R].angle = glm::radians(-6.0f) * tiny;

    // staff: mostly fixed, very tiny wobble
    parts[STAFF].rotationAxis = glm::vec3(0.0f, 0.0f, 1.0f);
    parts[STAFF].angle = -parts[ARM_R].angle + glm::radians(3.0f) * tiny;
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
    glm::mat4 modelMtx = glm::translate(parentMtx, part.offset);
    modelMtx = glm::rotate(modelMtx, part.angle, part.rotationAxis);

    if (part.model.has_value()) {   // ROOT has no mesh
        sendModelMtx(modelMtx);
        part.model->draw();
    }

    // children inherit this part's matrix
    for (const int child : part.children) {
        drawPart(child, modelMtx, sendModelMtx);
    }
}
