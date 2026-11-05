#include "A3Engine.h"

#include <CSCI441/FixedCam.hpp>
#include <CSCI441/objects.hpp>
#include <CSCI441/TextureUtils.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

//============================= Helper Functions =============================//

#ifndef M_PI
#define M_PI 3.14159265
#endif

//============================= Public Interface =============================//

A3Engine::A3Engine()
    : CSCI441::OpenGLEngine(4, 1, 512, 512, "A3: The Overworld"),
        _mousePosition( glm::vec2(MOUSE_UNINITIALIZED, MOUSE_UNINITIALIZED ) ),
        _leftMouseButtonState( GLFW_RELEASE ),
        _pArcballCam(nullptr),
        _pFirstCam(nullptr),
        _pFreeCam(nullptr),
        _pInspectCam(nullptr),
        _pActiveCamera(nullptr),
        _lightDir( glm::vec3(0.f, 0.f, 0.f) ),
        _gouraudShaderProgram(nullptr),
        _gouraudShaderProgramUniformLocations( {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1} ),
        _gouraudShaderProgramAttributeLocations( {-1, -1} ),
        _flatShaderProgram(nullptr),
        _flatShaderProgramUniformLocations( {-1, -1} )
{
    for(auto& _key : _keys) _key = GL_FALSE;
}

void A3Engine::handleKeyEvent(const GLint key, const GLint action) {
    if(key != GLFW_KEY_UNKNOWN)
        _keys[key] = ((action == GLFW_PRESS) || (action == GLFW_REPEAT));

    if(action == GLFW_PRESS) {
        switch( key ) {
            // camera view 
            case GLFW_KEY_1:
                cameraView = 1;
                _pActiveCamera = _pArcballCam;
                break;
            case GLFW_KEY_2:
                cameraView = 2;
                _pActiveCamera = _pFirstCam;
                break;
            case GLFW_KEY_3:
                cameraView = 3;
                // reset free camera
                _pFreeCam->setPosition(_pArcballCam->getPosition());
                _pFreeCam->setTheta(_pArcballCam->getTheta() + glm::pi<GLfloat>());
                _pFreeCam->setPhi(glm::pi<GLfloat>() - _pArcballCam->getPhi());
                _pFreeCam->recomputeOrientation();
                _pActiveCamera = _pFreeCam;
                break;
            case GLFW_KEY_4:
                cameraView = 4;
                _pActiveCamera = _pInspectCam;
                break;
            case GLFW_KEY_5:
                _showInset ^= 1;        // toggle secondary view 
                break;
            case GLFW_KEY_G:
                grid.showGrid ^= 1;     // toggle the switch
                break;
            case GLFW_KEY_C:
                _characterUnlit ^= 1;     // only has an effect in camera view 4
                break;
            case GLFW_KEY_L:
                _worldUnlit ^= 1;           // unlit world
                break;
            case GLFW_KEY_X:
                _xrayView ^= 1;                 // only has an effect in camera view 3
                break;
            case GLFW_KEY_T:
                toggleTextures ^= 1;         // toggle textures
                break;
            case GLFW_KEY_F11:
                _toggleFullscreen();
                break;
            // close window
            case GLFW_KEY_Q:
            case GLFW_KEY_ESCAPE:
                setWindowShouldClose();
                break;

            default: break; // suppress CLion warning
        }
    }
}

void A3Engine::handleMouseButtonEvent(const GLint button, const GLint action) {
    // if the event is for the left mouse button
    if( button == GLFW_MOUSE_BUTTON_LEFT ) {
        // update the left mouse button's state
        _leftMouseButtonState = action;
    }
}

void A3Engine::handleCursorPositionEvent(const glm::vec2 currMousePosition) {
    // if mouse hasn't moved in the window, prevent camera from flipping out
    if(fabs(_mousePosition.x - MOUSE_UNINITIALIZED) <= 0.000001f) {
        _mousePosition = currMousePosition;
    }

    // active motion - if the left mouse button is being held down while the mouse is moving
    if(_leftMouseButtonState == GLFW_PRESS) {
        const glm::vec2 delta = currMousePosition - _mousePosition;
        // if shift is held down
        if( (_keys[GLFW_KEY_LEFT_SHIFT] || _keys[GLFW_KEY_RIGHT_SHIFT]) && cameraView != 2 ) {
            // drag up = zoom in
            _pActiveCamera->moveForward(-delta.y * 0.05f);
        }
        // otherwise
        else {
            // mouse y grows downward: dragging down raises an arcball camera so it looks down at the target (game-style)
            // the free camera's phi aims the view itself, so dragging down has to lower phi to look down
            const GLfloat deltaPhi = (cameraView == 2 || cameraView == 3) ? -delta.y : delta.y;
            _pActiveCamera->rotate(delta.x * 0.005f, deltaPhi * 0.005f);
        }
    }
    // passive motion
    else {

    }

    // update the mouse position
    _mousePosition = currMousePosition;
}

void A3Engine::handleScrollEvent(const glm::vec2 offset) {
    if (cameraView == 2) return; // First-person view

    // update the camera radius in/out
    const GLfloat totChgSq = offset.y;
    _pActiveCamera->moveForward(totChgSq * 0.2f); 
}

//=============================== Engine Setup ===============================//

void A3Engine::mSetupGLFW() {
    CSCI441::OpenGLEngine::mSetupGLFW();

    // hide the black window until run(), when loading is done
    glfwHideWindow(mpWindow);

    // set our callbacks
    glfwSetKeyCallback(mpWindow, a3_keyboard_callback);
    glfwSetMouseButtonCallback(mpWindow, a3_mouse_button_callback);
    glfwSetCursorPosCallback(mpWindow, a3_cursor_callback);
    glfwSetScrollCallback(mpWindow, a3_scroll_callback);
}

void A3Engine::mSetupOpenGL() {
    glEnable( GL_DEPTH_TEST );					                    // enable depth testing
    glDepthFunc( GL_LESS );							                // use less than depth test

    glEnable(GL_BLEND);									            // enable blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	            // use one minus blending equation

    glClearColor(0.48f, 0.70f, 0.94f, 1.0f);	                    // clear the frame buffer to gray

    glFrontFace(GL_CCW);                                            // the front faces are CCW
    glCullFace(GL_BACK);                                            // cull the back faces
}

void A3Engine::mSetupShaders() {
    //***************************************************************************
    // Setup Gouraud Shader Program

    _gouraudShaderProgram = new CSCI441::ShaderProgram( "shaders/gouraudShader.v.glsl", "shaders/gouraudShader.f.glsl" );
    // get uniform locations
    _gouraudShaderProgramUniformLocations.mvpMatrix             = _gouraudShaderProgram->getUniformLocation("mvpMatrix");
    _gouraudShaderProgramUniformLocations.normalMatrix          = _gouraudShaderProgram->getUniformLocation("normalMtx");
    _gouraudShaderProgramUniformLocations.modelMatrix           = _gouraudShaderProgram->getUniformLocation("modelMtx");
    _gouraudShaderProgramUniformLocations.lightDir              = _gouraudShaderProgram->getUniformLocation("lightDir");
    _gouraudShaderProgramUniformLocations.lightColor            = _gouraudShaderProgram->getUniformLocation("lightColor");
    _gouraudShaderProgramUniformLocations.cameraPosition        = _gouraudShaderProgram->getUniformLocation("camPos");
    _gouraudShaderProgramUniformLocations.materialDiffColor     = _gouraudShaderProgram->getUniformLocation("materialDiffColor");
    _gouraudShaderProgramUniformLocations.materialSpecColor     = _gouraudShaderProgram->getUniformLocation("materialSpecColor");
    _gouraudShaderProgramUniformLocations.materialAmbColor      = _gouraudShaderProgram->getUniformLocation("materialAmbColor");
    _gouraudShaderProgramUniformLocations.materialShininess     = _gouraudShaderProgram->getUniformLocation("materialShininess");
    _gouraudShaderProgramUniformLocations.useTexture            = _gouraudShaderProgram->getUniformLocation("useTexture");
    _gouraudShaderProgramUniformLocations.textureMap            = _gouraudShaderProgram->getUniformLocation("tex0");
    _gouraudShaderProgramUniformLocations.unlit                 = _gouraudShaderProgram->getUniformLocation("unlit");
    _gouraudShaderProgramUniformLocations.isWater               = _gouraudShaderProgram->getUniformLocation("isWater");
    _gouraudShaderProgramUniformLocations.time                  = _gouraudShaderProgram->getUniformLocation("time");
    // the sampler reads from texture unit 0, which is where block textures get bound
    _gouraudShaderProgram->setProgramUniform(_gouraudShaderProgramUniformLocations.textureMap, 0);
    // get attribute locations
    _gouraudShaderProgramAttributeLocations.vPos                = _gouraudShaderProgram->getAttributeLocation("vPos");
    _gouraudShaderProgramAttributeLocations.vNormal             = _gouraudShaderProgram->getAttributeLocation("vNormal");

    //***************************************************************************
    // Setup Flat Shader Program

    _flatShaderProgram = new CSCI441::ShaderProgram( "shaders/flatShader.v.glsl", "shaders/flatShader.f.glsl" );
    // get uniform locations
    _flatShaderProgramUniformLocations.mvpMatrix             = _flatShaderProgram->getUniformLocation("mvpMatrix");
    _flatShaderProgramUniformLocations.color                 = _flatShaderProgram->getUniformLocation("color");
    // NOTE: we do not query an attribute locations because in our shader we have set the locations to be the same as
    // the Gouraud Shader attribute locations

    // hook up the CSCI441 object library to our shader program - MUST be done after the shader is used and before the objects are drawn
    // if we have multiple shaders the flow would be:
    //      1) shader->useProgram()
    //      2) CSCI441::setVertexAttributeLocations()
    //      3) CSCI441::draw*()
    // OR the alternative is to ensure that all of your shader programs use the
    // same attribute locations for the vertex position, normal, and texture coordinate
    // as this lab is doing
    CSCI441::setVertexAttributeLocations(_gouraudShaderProgramAttributeLocations.vPos,
                                         _gouraudShaderProgramAttributeLocations.vNormal,
                                         -1 );
}

void A3Engine::mSetupBuffers() {

}

void A3Engine::_createPlatform(const GLuint vao, const GLuint vbo, const GLuint ibo, GLsizei &numVAOPoints) const {
    
    // 
    
    // container for our vertex data
    struct VertexNormalCoordinate {
        GLfloat x, y, z;        // x. y, z position
        GLfloat nx, ny, nz;     // x, y, z normal vector
    };

    // create our platform
    VertexNormalCoordinate platformVertices[4] = {
            { -0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f }, // 0 - BL
            {  0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f }, // 1 - BR
            { -0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f }, // 2 - TL
            {  0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f }  // 3 - TR
    };

    // order to connect the vertices in
    constexpr GLushort platformIndices[4] = { 0, 2, 1, 3 };
    numVAOPoints = 4;

    glBindVertexArray(vao);

    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( platformVertices ), platformVertices, GL_STATIC_DRAW );

    glEnableVertexAttribArray( _gouraudShaderProgramAttributeLocations.vPos );
    glVertexAttribPointer(_gouraudShaderProgramAttributeLocations.vPos, 3, GL_FLOAT, GL_FALSE, sizeof(VertexNormalCoordinate), nullptr );

    glEnableVertexAttribArray( _gouraudShaderProgramAttributeLocations.vNormal );
    glVertexAttribPointer(_gouraudShaderProgramAttributeLocations.vNormal, 3, GL_FLOAT, GL_FALSE, sizeof(VertexNormalCoordinate), (void*) (sizeof(GLfloat) * 3) );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( platformIndices ), platformIndices, GL_STATIC_DRAW );

    CSCI441::LogUtils::log("[INFO]: platform read in with VAO/VBO/IBO %d/%d/%d & %d points\n", vao, vbo, ibo, numVAOPoints );
}

void A3Engine::mSetupTextures() {
    // load every texture the world asks for; block types without one keep handle 0
    for (int t = 0; t < NUM_BLOCK_TYPES; t++) {
        if (const char* file = World::textureFile(static_cast<BlockType>(t))) {
            // mipmapped far away avoids shimmering
            _blockTextures[t] = CSCI441::TextureUtils::loadAndRegisterTexture(file, GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_REPEAT, GL_REPEAT);
        }
    }
}

void A3Engine::mSetupScene() {
    // if either shader program is null, do not continue any further to prevent run time errors
    if(!_gouraudShaderProgram || !_flatShaderProgram) {
        return;
    }

    // set up light info
    constexpr glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    _lightDir = glm::vec3(-1.0f, -5.0f, -1.0f);

    _gouraudShaderProgram->setProgramUniform( _gouraudShaderProgramUniformLocations.lightColor, lightColor );
    _gouraudShaderProgram->setProgramUniform( _gouraudShaderProgramUniformLocations.lightDir, _lightDir );

    // set flat shading color
    constexpr glm::vec3 flatColor(1.0f, 1.0f, 1.0f);
    _flatShaderProgram->setProgramUniform( _flatShaderProgramUniformLocations.color, flatColor );

    // build world platform
    world.build();

    constexpr GLfloat Y = 0.52f;                       // just above the water surface (0.5) to avoid z-fighting
    for (int k = -World::HALF; k <= World::HALF; k++) {
        grid.gridPoints.emplace_back(k, Y, -World::HALF);   // line along z
        grid.gridPoints.emplace_back(k, Y,  World::HALF);
        grid.gridPoints.emplace_back(-World::HALF, Y, k);   // line along x
        grid.gridPoints.emplace_back( World::HALF, Y, k);
    }

    // generate random clouds once
    for (auto& cloud : _clouds) {
        cloud.position = glm::vec3(RandomGenerator::getRand() * 180.0f - 90.0f, 25.0f + RandomGenerator::getRand() * 20.0f, RandomGenerator::getRand() * 180.0f - 90.0f);
        cloud.size = glm::vec2(6.0f + RandomGenerator::getRand() * 12.0f, 3.0f + RandomGenerator::getRand() * 7.0f);
    }

    glGenVertexArrays(1, &grid.vao);
    glBindVertexArray(grid.vao);                       // everything below gets recorded into this VAO

    glGenBuffers(1, &grid.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, grid.vbo);
    glBufferData(GL_ARRAY_BUFFER, grid.gridPoints.size() * sizeof(glm::vec3), grid.gridPoints.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);                      // slot 0 = vPos in both shaders
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    grid.numOfPoints = static_cast<GLsizei>(grid.gridPoints.size());

    ShaderInfo shaderInfo {
        _gouraudShaderProgram->getShaderProgramHandle(),

        _gouraudShaderProgramAttributeLocations.vPos,
        _gouraudShaderProgramAttributeLocations.vNormal,

        _gouraudShaderProgramUniformLocations.materialDiffColor,
        _gouraudShaderProgramUniformLocations.materialSpecColor,
        _gouraudShaderProgramUniformLocations.materialAmbColor,
        _gouraudShaderProgramUniformLocations.materialShininess
    };

    character.initialize(shaderInfo);
    // character.setScale(glm::vec3{0.4f});

    
    // set up cameras
    GLfloat aspectRatio = static_cast<GLfloat>(mFramebufferWidth) / static_cast<GLfloat>(mFramebufferHeight);

    _pArcballCam = new ArcballCam(aspectRatio);
    _pFirstCam = new CSCI441::FreeCam(aspectRatio, glm::quarter_pi<GLfloat>(), 0.1f, 500.0f);
    // can zoom in closer than the follow camera, starting with the whole character in view
    _pInspectCam = new ArcballCam(aspectRatio, 1.5f);
    _pInspectCam->setRadius(8.0f);
    _pFreeCam = new CSCI441::FreeCam(aspectRatio, glm::quarter_pi<GLfloat>(), 0.1f, 500.0f);

    // place character somewhere random in the world
    // exclude montain or top of any building
    // const glm::vec3 start(0.5f, world.groundHeight(0.5f, -25.5f), -25.5f);
    const glm::vec3 start = world.getValidPosition();
    character.setPosition(start);

    _pArcballCam->setTarget(start);

    // first camera should be a free camera, so copy data from arcball camera to it
    _pFirstCam->setTheta(_pArcballCam->getTheta() + glm::pi<GLfloat>());
    _pFirstCam->setPhi(glm::pi<GLfloat>() - _pArcballCam->getPhi());
    _pFirstCam->recomputeOrientation();

    glm::vec3 forward = _pFirstCam->getViewDirection();
    forward.y = 0.0f;
    forward = glm::normalize(forward);
    _pFirstCam->setPosition(start + glm::vec3(0.0f, 2.5f, 0.0f) + forward * 0.8f);

    _pFreeCam->setPosition(_pArcballCam->getPosition());
    // the arcball angles point from the target to the camera, and the 
    // free camera's point where it looks, so they are opposite
    _pFreeCam->setTheta(_pArcballCam->getTheta() + glm::pi<GLfloat>());
    _pFreeCam->setPhi(glm::pi<GLfloat>() - _pArcballCam->getPhi());
    _pFreeCam->recomputeOrientation();

    _pInspectCam->setTarget(start);

    // default view option 1
    _pActiveCamera = _pArcballCam;
}

void A3Engine::mSetupFonts() {
    if (!CSCI441::FontUtils::loadFont("assets/fonts/DroidSansMono.ttf")) {
        CSCI441::LogUtils::logError("[ERROR]: Could not load font\n");
    }
    CSCI441::FontUtils::setWindowSize(mFramebufferWidth, mFramebufferHeight);
    CSCI441::FontUtils::setFontSize(2.0f / static_cast<GLfloat>(mFramebufferWidth), 2.0f / static_cast<GLfloat>(mFramebufferHeight) );
    CSCI441::OpenGLUtils::checkOpenGLErrors("mSetupFonts() done");
}

//============================== Engine Cleanup ==============================//

void A3Engine::mCleanupShaders() {
    CSCI441::LogUtils::log("[INFO]: ...deleting Shaders.\n" );
    delete _gouraudShaderProgram;
    _gouraudShaderProgram = nullptr;
    delete _flatShaderProgram;
    _flatShaderProgram = nullptr;
}

void A3Engine::mCleanupBuffers() {
    CSCI441::LogUtils::log("[INFO]: ...deleting VAOs....\n" );
    CSCI441::deleteObjectVAOs();
    // TODO: Free VAOs

    CSCI441::LogUtils::log("[INFO]: ...deleting VBOs....\n" );
    CSCI441::deleteObjectVBOs();
    // TODO: Free VBOs

    CSCI441::LogUtils::log("[INFO]: ...deleting IBOs....\n" );
    // TODO: Free IBOs

    CSCI441::LogUtils::log("[INFO]: ...deleting models..\n" );
    // TODO: Free model 
}

void A3Engine::mCleanupTextures() {
    CSCI441::LogUtils::log("[INFO]: ...deleting Textures\n" );
    glDeleteTextures(NUM_BLOCK_TYPES, _blockTextures);     // handle 0 is silently ignored
}

void A3Engine::mCleanupScene() {
    CSCI441::LogUtils::log("[INFO]: ...deleting scene...\n" );
    delete _pArcballCam;
    delete _pFirstCam;
    delete _pFreeCam;
    delete _pInspectCam;
    _pArcballCam = nullptr;
    _pFirstCam = nullptr;
    _pFreeCam = nullptr;
    _pInspectCam = nullptr;
    _pActiveCamera = nullptr;
}

void A3Engine::mCleanupFonts() {
    CSCI441::FontUtils::releaseFont();

    CSCI441::OpenGLUtils::checkOpenGLErrors("mCleanupFonts() done");
}

//====================== Rendering / Drawing Functions =======================//

void A3Engine::_basicRenderScene(const glm::mat4& viewMtx, const glm::mat4& projMtx, const glm::vec3& camPos) const {
    _gouraudShaderProgram->useProgram();
    _gouraudShaderProgram->setProgramUniform(_gouraudShaderProgramUniformLocations.cameraPosition, camPos);
    
    // gold teapot, ruby sphere, jade cube: different materials should look differently
    struct TestObjects { glm::vec3 pos; CSCI441::Materials::Material mat; int shape; };
    const TestObjects objects[] = {
        { {-3.0f, 1.0f, 0.0f}, CSCI441::Materials::GOLD, 0 },
        { { 0.0f, 1.0f, 0.0f}, CSCI441::Materials::RUBY, 1 },
        { { 3.0f, 1.0f, 0.0f}, CSCI441::Materials::JADE, 2 },
    };
    for (const auto& o : objects) {
        const glm::mat4 modelMtx = glm::translate(glm::mat4(1.0f), o.pos);
        _computeAndSendTransformationMatrices(_gouraudShaderProgram, modelMtx, viewMtx, projMtx,
            _gouraudShaderProgramUniformLocations.mvpMatrix,
            _gouraudShaderProgramUniformLocations.normalMatrix,
            _gouraudShaderProgramUniformLocations.modelMatrix);
        _setMaterialProperties(o.mat);
        if      (o.shape == 0) CSCI441::drawSolidTeapot();
        else if (o.shape == 1) CSCI441::drawSolidSphere(1.0f, 32, 32);
        else                   CSCI441::drawSolidCube(1.5f);
    }
}

void A3Engine::_renderScene(const glm::mat4& viewMtx, const glm::mat4& projMtx, const glm::vec3& camPos, const bool firstPerson) const {
    _gouraudShaderProgram->useProgram();
    _gouraudShaderProgram->setProgramUniform(_gouraudShaderProgramUniformLocations.cameraPosition, camPos);

    // world is unlit if _worldUnlit is true
    _gouraudShaderProgram->setProgramUniform(_gouraudShaderProgramUniformLocations.unlit, _worldUnlit);

    //---------------------- draw the ground ----------------------//
    // wireframe blocks let the free camera see through the world
    const bool wireframe = cameraView == 3 && _xrayView;

    for (int t = 0; t < NUM_BLOCK_TYPES; t++) {
        const auto blockType = static_cast<BlockType>(t);

        // textured types sample their image
        const GLboolean textured = _blockTextures[t] != 0 && toggleTextures;
        _setMaterialProperties(World::material(blockType, textured));
        
        const GLboolean isWater = blockType == BlockType::WATER;
        _gouraudShaderProgram->setProgramUniform(_gouraudShaderProgramUniformLocations.useTexture, textured ? 1 : 0);
        _gouraudShaderProgram->setProgramUniform(_gouraudShaderProgramUniformLocations.isWater, isWater);
        
        if (textured) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, _blockTextures[t]);
        }

        if (isWater) {
            _gouraudShaderProgram->setProgramUniform(_gouraudShaderProgramUniformLocations.time, (GLfloat)currentTime);
        }

        for (const Block& block : world.blocks(blockType)) {
            // move the unit cube to the block's center, then stretch it
            const glm::mat4 modelMtx = glm::scale(glm::translate(glm::mat4(1.0f), block.position), block.scale);
            _computeAndSendTransformationMatrices(_gouraudShaderProgram, modelMtx, viewMtx, projMtx,
                _gouraudShaderProgramUniformLocations.mvpMatrix,
                _gouraudShaderProgramUniformLocations.normalMatrix,
                _gouraudShaderProgramUniformLocations.modelMatrix
            );

            if (wireframe)              CSCI441::drawWireCube(1.0);
            else if (textured)          CSCI441::drawSolidCubeFlat(1.0);
            else                        CSCI441::drawSolidCube(1.0);
        }
    }

    // printf("Phi = %f\n", _pArcballCam->getPhi());
    // printf("Theta = %f\n", _pArcballCam->getTheta());


    //---------------------- draw the sun ----------------------//
    // sun: a flat-colored sphere far along the light direction
    _flatShaderProgram->useProgram();
    const glm::mat4 sunModelMtx = glm::scale(glm::translate(glm::mat4(1.0f), -normalize(_lightDir) * 80.0f), glm::vec3(4.0f));
    _computeAndSendTransformationMatrices(_flatShaderProgram, sunModelMtx, viewMtx, projMtx, 
        _flatShaderProgramUniformLocations.mvpMatrix
    );
    _flatShaderProgram->setProgramUniform(_flatShaderProgramUniformLocations.color, glm::vec3(1.0f, 0.92f, 0.55f));
    CSCI441::drawSolidSphere(1.0, 24, 24);

    // if grid needs to be drawn
    if (grid.showGrid) {
        _flatShaderProgram->useProgram();
        _computeAndSendTransformationMatrices(_flatShaderProgram, glm::mat4(1.0f), viewMtx, projMtx, 
            _flatShaderProgramUniformLocations.mvpMatrix
        );
        glBindVertexArray(grid.vao);

        // Pass 1
        _flatShaderProgram->setProgramUniform(_flatShaderProgramUniformLocations.color, glm::vec3(1.0f));
        for (GLsizei first = 0; first < grid.numOfPoints; first += 2) glDrawArrays(GL_LINE_STRIP, first, 2);    // each line is its own 2-point strip

        // Pass 2
        // depth writes off so these ghost lines don't block anything drawn later
        glDepthFunc(GL_GREATER);
        glDepthMask(GL_FALSE);
        _flatShaderProgram->setProgramUniform(_flatShaderProgramUniformLocations.color, glm::vec3(0.35f, 0.45f, 0.6f));
        for (GLsizei first = 0; first < grid.numOfPoints; first += 2) glDrawArrays(GL_LINE_STRIP, first, 2);

        // restore normal depth state for everything after (text, later the corner viewport)
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
    }

    //---------------------- draw the clouds ----------------------//
    _flatShaderProgram->useProgram();
    _flatShaderProgram->setProgramUniform(_flatShaderProgramUniformLocations.color, glm::vec3(0.95f, 0.95f, 1.0f));

    constexpr float CLOUD_SPEED = 2.0f;
    constexpr float CLOUD_WIDTH = 180.0f;

    for (const auto& cloud : _clouds) {
        // move clouds cyclically along X
        float x = fmod(cloud.position.x + CLOUD_WIDTH / 2.0f + glfwGetTime() * CLOUD_SPEED, CLOUD_WIDTH) - CLOUD_WIDTH / 2.0f;
        glm::vec3 pos(x, cloud.position.y, cloud.position.z);
        glm::mat4 modelMtx = glm::scale(glm::translate(glm::mat4(1.0f), pos), glm::vec3(cloud.size.x, 0.4f, cloud.size.y));

        _computeAndSendTransformationMatrices(_flatShaderProgram, modelMtx, viewMtx, projMtx, _flatShaderProgramUniformLocations.mvpMatrix);
        CSCI441::drawSolidCubeFlat(1.0f);
    }

    //---------------------- draw the character ----------------------//
    // drawn after the grid, so that the grid's ghost lines don't cover the character
    _gouraudShaderProgram->useProgram();
    _gouraudShaderProgram->setProgramUniform(_gouraudShaderProgramUniformLocations.useTexture, 0);
    // only the character can be unlit, the blocks stay lit
    _gouraudShaderProgram->setProgramUniform(_gouraudShaderProgramUniformLocations.unlit, (cameraView == 4 && _characterUnlit) ? 1 : _worldUnlit);
    if (firstPerson != 1) {
        character.draw([&](const glm::mat4& modelMtx) {
            _computeAndSendTransformationMatrices(_gouraudShaderProgram, modelMtx, viewMtx, projMtx,
                _gouraudShaderProgramUniformLocations.mvpMatrix,
                _gouraudShaderProgramUniformLocations.normalMatrix,
                _gouraudShaderProgramUniformLocations.modelMatrix
            );
        });
    }
    _gouraudShaderProgram->setProgramUniform(_gouraudShaderProgramUniformLocations.unlit, 0);
}

void A3Engine::_updateScene() {
    // seconds since the last update, so that walking speed doesn't depend on fps
    currentTime = glfwGetTime();
    // capped so that a long frame can't step over a block
    const GLfloat dt = glm::min(static_cast<GLfloat>(currentTime - _lastUpdateTime), 0.1f);
    _lastUpdateTime = currentTime;

    // printf("%f\n", characterAnimationTimer);
    if (characterAnimationTimer > 0) {
        characterAnimationTimer = std::max(characterAnimationTimer - dt, 0.0f);
        character.update(characterAnimationTimer);
    }

    characterIdleTimer += dt;

    if (characterIdleTimer >= 2.0) {
        character.updateIdle(characterIdleTimer - 2.0);
    }

    // walk while W/S/A/D are held
    const GLfloat inputMain = (_keys[GLFW_KEY_W] ? 1.0f : 0.0f) - (_keys[GLFW_KEY_S] ? 1.0f : 0.0f);
    const GLfloat inputSide = (_keys[GLFW_KEY_A] ? 1.0f : 0.0f) - (_keys[GLFW_KEY_D] ? 1.0f : 0.0f);
   
    switch( cameraView ) {
        case 1:
        case 2: {
            constexpr GLfloat WALK_SPEED = 10.0f;    // units per second
            constexpr GLfloat TURN_SPEED = 2.0f;     // radians per second

            if (inputSide != 0.0f) {
                const GLfloat turn = inputSide * TURN_SPEED * dt;
                _characterHeading += turn;
                _pFirstCam->setTheta(_pFirstCam->getTheta() - turn);
                _pFirstCam->recomputeOrientation();
                characterIdleTimer = 0.0f;
            }

            // W/S move along the character's heading
            if (inputMain != 0.0f) {
                const glm::vec3 forward(glm::sin(_characterHeading), 0.0f, glm::cos(_characterHeading));
                glm::vec3 next = _pArcballCam->getLookAtPoint() + forward * inputMain * WALK_SPEED * dt;

                // stay on land, out of houses and trees, and climb at most one block
                if (world.isWalkable(next.x, next.z, next.y)) {
                    next.y = world.groundHeight(next.x, next.z);
                    _pArcballCam->setTarget(next);
                }

                if (characterAnimationTimer == 0) {
                    characterAnimationTimer = character.getAnimationTime();
                }
                characterIdleTimer = 0.0f;
            }

            character.setPosition(_pArcballCam->getLookAtPoint());
            character.setRotation(_characterHeading);

            // keep first-person camera attached to the character's front
            const glm::vec3 firstForward(glm::sin(_characterHeading), 0.0f, glm::cos(_characterHeading));
            _pFirstCam->setPosition(_pArcballCam->getLookAtPoint() + glm::vec3(0.0f, 2.5f, 0.0f) + firstForward * 1.2f);
            _pFirstCam->recomputeOrientation();
            break;
        }
        case 3: {
            constexpr GLfloat FLY_SPEED = 20.0f;    // units per second

            _pFreeCam->moveForward(inputMain * FLY_SPEED * dt);

            // normalized because the view direction can tilt up or down, which shortens the cross product
            const glm::vec3 left = glm::normalize(glm::cross(_pFreeCam->getUpVector(), _pFreeCam->getViewDirection()));
            _pFreeCam->setPosition(_pFreeCam->getPosition() + left * inputSide * FLY_SPEED * dt);
            _pFreeCam->recomputeOrientation();
            break;
        }
        case 4: {
            constexpr GLfloat FOCUS_SPEED = 2.0f;       // units per second
            constexpr GLfloat MAX_FOCUS_HEIGHT = 5.0f;  // just above the top of the head

            // W/S slide the orbit point up and down the body, so that zooming in works on any part of it
            _inspectHeight = glm::clamp(_inspectHeight + inputMain * FOCUS_SPEED * dt, 0.0f, MAX_FOCUS_HEIGHT);
            _pInspectCam->setTarget(_pArcballCam->getLookAtPoint() + glm::vec3(0.0f, _inspectHeight, 0.0f));
            break;
        }

        default: break; // suppress CLion warning
    }
    
}


void A3Engine::_renderTextOverlay(const GLdouble fps, const GLdouble fpsAvg) const {
    CSCI441::FontUtils::bindFont();

    const GLint top = mFramebufferHeight;

    constexpr GLuint STR_LEN = 80;
    char fpsStr[STR_LEN];
    snprintf( fpsStr, STR_LEN, "%7.3f frames/sec (Avg: %7.3f)", fps, fpsAvg);
    CSCI441::FontUtils::renderText(fpsStr, 24.f,   static_cast<GLfloat>(top - 24*1));

    snprintf( fpsStr, STR_LEN, "%7.3f sec/frame  (Avg: %7.3f)", 1000.f / fps, 1000.f / fpsAvg);
    CSCI441::FontUtils::renderText(fpsStr, 24.f,   static_cast<GLfloat>(top - 24*2));
}

void A3Engine::run() {
    // everything is loaded, so the first frame comes right away
    glfwShowWindow(mpWindow);

    //  This is our draw loop - all rendering is done here.  We use a loop to keep the window open
    //	until the user decides to close the window and quit the program.  Without a loop, the
    //	window will display once and then the program exits.
    while( !glfwWindowShouldClose(mpWindow) ) {	        // check if the window was instructed to be closed
        glDrawBuffer( GL_BACK );				        // work with our back frame buffer
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );	// clear the current color contents and depth buffer in the window

        // Get the size of our framebuffer.  Ideally this should be the same dimensions as our window, but
        // when using a Retina display the actual window can be larger than the requested window.  Therefore,
        // query what the actual size of the window we are rendering to is.
        GLint framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize( mpWindow, &framebufferWidth, &framebufferHeight );

        // after a size change, update everything that depends on the size
        // (the size is 0 while minimized, which would break the aspect ratio)
        if (framebufferWidth > 0 && framebufferHeight > 0 &&
            (framebufferWidth != mFramebufferWidth || framebufferHeight != mFramebufferHeight)) {
            setCurrentFramebufferSize(framebufferWidth, framebufferHeight);
            // set up cameras
            GLfloat aspectRatio = static_cast<GLfloat>(mFramebufferWidth) / static_cast<GLfloat>(mFramebufferHeight);
            _pArcballCam->setAspectRatio(aspectRatio);
            _pFirstCam->setAspectRatio(aspectRatio);
            _pInspectCam->setAspectRatio(aspectRatio);
            _pFreeCam->setAspectRatio(aspectRatio);       
            CSCI441::FontUtils::setWindowSize(framebufferWidth, framebufferHeight);
            CSCI441::FontUtils::setFontSize(2.0f / static_cast<GLfloat>(framebufferWidth), 2.0f / static_cast<GLfloat>(framebufferHeight));
        }

        // update the viewport - tell OpenGL we want to render to the whole window
        glViewport( 0, 0, framebufferWidth, framebufferHeight );

        // draw everything to the window
        _renderScene(_pActiveCamera->getViewMatrix(), _pActiveCamera->getProjectionMatrix(), _pActiveCamera->getPosition(), cameraView == 2);
        
        // draw the opposite camera in the top-right quarter
        if (_showInset && (cameraView == 1 || cameraView == 2)) {
            const GLint x = 3 * framebufferWidth / 4, y = 3 * framebufferHeight / 4;
            const GLsizei w = framebufferWidth - x, h = framebufferHeight - y;

            // clear only the secondary viewport
            glEnable(GL_SCISSOR_TEST);
            glScissor(x, y, w, h);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_SCISSOR_TEST);

            glViewport(x, y, w, h);

            const glm::vec3 target = _pArcballCam->getLookAtPoint();
            const glm::vec3 forward(glm::sin(_characterHeading), 0.0f, glm::cos(_characterHeading));

            if (cameraView == 1) {
                // first-person inset, looking along the character's heading
                const glm::vec3 insetPos = target + glm::vec3(0.0f, 2.5f, 0.0f) + forward * 1.2f;
                const glm::mat4 insetView = glm::lookAt(insetPos, insetPos + forward, glm::vec3(0.0f, 1.0f, 0.0f));
                _renderScene(insetView, _pFirstCam->getProjectionMatrix(), insetPos, true);
            } else {
                // third-person inset, fixed behind the character
                const glm::vec3 insetPos = target - forward * 10.0f + glm::vec3(0.0f, 4.0f, 0.0f);
                const glm::mat4 insetView = glm::lookAt(insetPos, target + glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                _renderScene(insetView, _pArcballCam->getProjectionMatrix(), insetPos, false);
            }

            // restore full-screen viewport
            glViewport(0, 0, framebufferWidth, framebufferHeight);
        }
        
        _renderTextOverlay(this->clockFrame(), this->fpsAverage()); // display FPS information on screen

        // animate the scene
        _updateScene();

        glfwSwapBuffers(mpWindow);                       // flush the OpenGL commands and make sure they get rendered!
        glfwPollEvents();				                // check for any events and signal to redraw screen
    }
}

//========================= Private Helper Functions =========================//

void A3Engine::_toggleFullscreen() {
    // a monitor is only set while in fullscreen
    if (glfwGetWindowMonitor(mpWindow)) {
        glfwSetWindowMonitor(mpWindow, nullptr, _windowedX, _windowedY, _windowedWidth, _windowedHeight, GLFW_DONT_CARE);
    } else {
        // remember the window so the next F11 can restore it
        glfwGetWindowPos(mpWindow, &_windowedX, &_windowedY);
        glfwGetWindowSize(mpWindow, &_windowedWidth, &_windowedHeight);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(mpWindow, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
}

void A3Engine::_setMaterialProperties(const CSCI441::Materials::Material& material) const {
    // ensure our shader program is not null
    if(_gouraudShaderProgram) {
        // set the D, S, A, & shininess components of the material to our Gouraud Shader
        glProgramUniform4fv( _gouraudShaderProgram->getShaderProgramHandle(), _gouraudShaderProgramUniformLocations.materialDiffColor, 1, material.diffuse   );
        glProgramUniform4fv( _gouraudShaderProgram->getShaderProgramHandle(), _gouraudShaderProgramUniformLocations.materialSpecColor, 1, material.specular  );
        glProgramUniform4fv( _gouraudShaderProgram->getShaderProgramHandle(), _gouraudShaderProgramUniformLocations.materialAmbColor,  1, material.ambient   );
        glProgramUniform1f(  _gouraudShaderProgram->getShaderProgramHandle(), _gouraudShaderProgramUniformLocations.materialShininess,    material.shininess );
    }
}

void A3Engine::_computeAndSendTransformationMatrices(const CSCI441::ShaderProgram * const shaderProgram,
                                                        const glm::mat4& modelMatrix, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix,
                                                        const GLint mvpMtxLocation, const GLint normalMtxLocation, const GLint modelMatrixLocation) {
    // ensure our shader program is not null
    if( shaderProgram ) {
        // precompute the MVP matrix CPU side
        const glm::mat4 mvpMatrix = projectionMatrix * viewMatrix * modelMatrix;
        // precompute the Normal matrix CPU side
        const glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));

        // send the matrices to the shader
        shaderProgram->setProgramUniform(mvpMtxLocation, mvpMatrix);
        shaderProgram->setProgramUniform(normalMtxLocation, normalMatrix);
        shaderProgram->setProgramUniform(modelMatrixLocation, modelMatrix);
    }
}

//================================ Callbacks =================================//

void a3_keyboard_callback(GLFWwindow *window, const int key, const int scancode, const int action, const int mods ) {
    // ensure our engine is not null
    if(const auto engine = static_cast<A3Engine *>(glfwGetWindowUserPointer(window))) {
        // pass the key and action through to the engine
        engine->handleKeyEvent(key, action);
    }
}

void a3_cursor_callback(GLFWwindow *window, const double x, const double y ) {
    // ensure our engine is not null
    if(const auto engine = static_cast<A3Engine *>(glfwGetWindowUserPointer(window))) {
        // pass the cursor position through to the engine
        engine->handleCursorPositionEvent(glm::vec2(x, y));
    }
}

void a3_mouse_button_callback(GLFWwindow *window, const int button, const int action, const int mods ) {
    // ensure our engine is not null
    if(const auto engine = static_cast<A3Engine *>(glfwGetWindowUserPointer(window))) {
        // pass the mouse button and action through to the engine
        engine->handleMouseButtonEvent(button, action);
    }
}

void a3_scroll_callback(GLFWwindow *window,const double xOffset, const double yOffset) {
    // ensure our engine is not null
    if(const auto engine = static_cast<A3Engine *>(glfwGetWindowUserPointer(window))) {
        // pass the scroll offset through to the engine
        engine->handleScrollEvent(glm::vec2(xOffset, yOffset));
    }
}
