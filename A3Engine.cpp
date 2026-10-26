#include "A3Engine.h"


#include <CSCI441/FixedCam.hpp>
#include <CSCI441/objects.hpp>

#include <cmath>

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
        _lightDir( glm::vec3(0.f, 0.f, 0.f) ),
        _gouraudShaderProgram(nullptr),
        _gouraudShaderProgramUniformLocations( {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1} ),
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
            // quit!
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
        if( _keys[GLFW_KEY_LEFT_SHIFT] || _keys[GLFW_KEY_RIGHT_SHIFT] ) {
            _pArcballCam->moveForward(-delta.y * 0.05f);                // drag up = zoom in
        }
        // otherwise
        else {
            // mouse y grows downward: dragging down raises the camera so it looks down at the target (game-style)
            _pArcballCam->rotate(delta.x * 0.005f, delta.y * 0.005f);
        }
    }
    // passive motion
    else {

    }

    // update the mouse position
    _mousePosition = currMousePosition;
}

void A3Engine::handleScrollEvent(const glm::vec2 offset) {
    // update the camera radius in/out
    const GLfloat totChgSq = offset.y;
    _pArcballCam->moveForward(totChgSq * 0.2f );
}

//=============================== Engine Setup ===============================//

void A3Engine::mSetupGLFW() {
    CSCI441::OpenGLEngine::mSetupGLFW();

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

    glClearColor( 0.4f, 0.4f, 0.4f, 1.0f );	// clear the frame buffer to gray

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
    // unused in this lab
}

void A3Engine::mSetupScene() {
    // if either shader program is null, do not continue any further to prevent run time errors
    if(!_gouraudShaderProgram || !_flatShaderProgram) {
        return;
    }

    // set up camera
    _pArcballCam = new ArcballCam(static_cast<GLfloat>(mFramebufferWidth) / static_cast<GLfloat>(mFramebufferHeight));

    // set up light info
    constexpr glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    _lightDir = glm::vec3(-1.0f, -5.0f, -1.0f);

    _gouraudShaderProgram->setProgramUniform( _gouraudShaderProgramUniformLocations.lightColor, lightColor );
    _gouraudShaderProgram->setProgramUniform( _gouraudShaderProgramUniformLocations.lightDir, _lightDir );

    // set flat shading color
    constexpr glm::vec3 flatColor(1.0f, 1.0f, 1.0f);
    _flatShaderProgram->setProgramUniform( _flatShaderProgramUniformLocations.color, flatColor );
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
}

void A3Engine::mCleanupScene() {
    CSCI441::LogUtils::log("[INFO]: ...deleting scene...\n" );
    delete _pArcballCam;
    _pArcballCam = nullptr;
}

void A3Engine::mCleanupFonts() {
    CSCI441::FontUtils::releaseFont();

    CSCI441::OpenGLUtils::checkOpenGLErrors("mCleanupFonts() done");
}

//====================== Rendering / Drawing Functions =======================//

void A3Engine::_renderScene(const glm::mat4& viewMtx, const glm::mat4& projMtx, const glm::vec3& camPos) const {
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

void A3Engine::_updateScene() {

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

        // update the viewport - tell OpenGL we want to render to the whole window
        glViewport( 0, 0, framebufferWidth, framebufferHeight );

        // draw everything to the window
        _renderScene(_pArcballCam->getViewMatrix(), _pArcballCam->getProjectionMatrix(), _pArcballCam->getPosition());
        _renderTextOverlay(this->clockFrame(), this->fpsAverage()); // display FPS information on screen

        // animate the scene
        _updateScene();

        glfwSwapBuffers(mpWindow);                       // flush the OpenGL commands and make sure they get rendered!
        glfwPollEvents();				                // check for any events and signal to redraw screen
    }
}

//========================= Private Helper Functions =========================//


void A3Engine::_setMaterialProperties(const CSCI441::Materials::Material& material) const {
    // ensure our shader program is not null
    if(_gouraudShaderProgram) {
        // set the D, S, A, & shininess components of the material to our Gouraud Shader
        glProgramUniform3fv( _gouraudShaderProgram->getShaderProgramHandle(), _gouraudShaderProgramUniformLocations.materialDiffColor, 1, material.diffuse   );
        glProgramUniform3fv( _gouraudShaderProgram->getShaderProgramHandle(), _gouraudShaderProgramUniformLocations.materialSpecColor, 1, material.specular  );
        glProgramUniform3fv( _gouraudShaderProgram->getShaderProgramHandle(), _gouraudShaderProgramUniformLocations.materialAmbColor,  1, material.ambient   );
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
