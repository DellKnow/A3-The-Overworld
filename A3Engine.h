#ifndef A3_A3_ENGINE_H
#define A3_A3_ENGINE_H

#include "Cameras.h"

#include <CSCI441/Camera.hpp>
#include <CSCI441/materials.hpp>
#include <CSCI441/OpenGLEngine.hpp>
#include <CSCI441/ShaderProgram.hpp>

class A3Engine final : public CSCI441::OpenGLEngine {
public:
    //============================= Engine Interface =============================//

    A3Engine();

    void run() override;

    //============================== Event Handlers ==============================//

    /// \desc handle any key events inside the engine
    /// \param key key as represented by GLFW_KEY_ macros
    /// \param action key event action as represented by GLFW_ macros
    void handleKeyEvent(GLint key, GLint action);

    /// \desc handle any mouse button events inside the engine
    /// \param button mouse button as represented by GLFW_MOUSE_BUTTON_ macros
    /// \param action mouse event as represented by GLFW_ macros
    void handleMouseButtonEvent(GLint button, GLint action);

    /// \desc handle any cursor movement events inside the engine
    /// \param currMousePosition the current cursor position
    void handleCursorPositionEvent(glm::vec2 currMousePosition);

    /// \desc handle any scroll events inside the engine
    /// \param offset the current scroll offset
    void handleScrollEvent(glm::vec2 offset);

    /// \desc value off-screen to represent mouse has not begun interacting with window yet
    static constexpr GLfloat MOUSE_UNINITIALIZED = -9999.0f;

protected:
    //=============================== Engine Setup ===============================//

    void mSetupGLFW() override;
    void mSetupOpenGL() override;
    void mSetupShaders() override;
    void mSetupBuffers() override;
    void mSetupTextures() override;
    void mSetupScene() override;
    void mSetupFonts() override;

    //============================== Engine Cleanup ==============================//

    void mCleanupScene() override;
    void mCleanupTextures() override;
    void mCleanupBuffers() override;
    void mCleanupShaders() override;
    void mCleanupFonts() override;

private:
    //======================= Engine Rendering & Updating ========================//

    /// \desc draws everything to the scene from a particular point of view
    /// \param viewMtx the current view matrix for our camera
    /// \param projMtx the current projection matrix for our camera
    /// \param camPos  the current position of the camera
    void _renderScene(const glm::mat4& viewMtx, const glm::mat4& projMtx, const glm::vec3& camPos) const;
    /// \desc handles moving our FreeCam as determined by keyboard input
    void _updateScene();

    /// \brief draws FPS information to screen
    /// \param fps frames per second of last frame
    /// \param fpsAvg frames per second average over last 10 frames
    void _renderTextOverlay(GLdouble fps, GLdouble fpsAvg) const;

    //==================== Input Tracking (Keyboard & Mouse) =====================//

    /// \desc tracks the number of different keys that can be present as determined by GLFW
    static constexpr GLuint NUM_KEYS = GLFW_KEY_LAST;
    /// \desc boolean array tracking each key state.  if true, then the key is in a pressed or held
    /// down state.  if false, then the key is in a released state and not being interacted with
    GLboolean _keys[NUM_KEYS] = {0};

    /// \desc last location of the mouse in window coordinates
    glm::vec2 _mousePosition;
    /// \desc current state of the left mouse button
    GLint _leftMouseButtonState;

    //============================ Camera Information ============================//

    /// \desc the fixed camera in our world
    ArcballCam* _pArcballCam;

    //========================= Illumination Information =========================//

    /// \desc direction of the light for directional or spotlight
    glm::vec3 _lightDir;

    //========================= VAO & Object Information =========================//

    /// \desc creates the platform object
    /// \param [in] vao VAO descriptor to bind
    /// \param [in] vbo VBO descriptor to bind
    /// \param [in] ibo IBO descriptor to bind
    /// \param [out] numVAOPoints sets the number of vertices that make up the IBO array
    void _createPlatform(GLuint vao, GLuint vbo, GLuint ibo, GLsizei &numVAOPoints) const;

    //======================== Shader Program Information ========================//

    /// \desc shader program that performs Gouraud Shading with the Phong Illumination Model
    CSCI441::ShaderProgram* _gouraudShaderProgram;
    /// \desc stores the locations of all of our shader uniforms
    struct GouraudShaderProgramUniformLocations {
        /// \desc precomputed MVP matrix location
        GLint mvpMatrix;
        /// \desc normal matrix location
        GLint normalMatrix;
        /// \desc model matrix location
        GLint modelMatrix;
        /// \desc light direction location - used for directional/spot
        GLint lightDir;
        /// \desc color of the light location
        GLint lightColor;
        /// \desc position of the camera
        GLint cameraPosition;
        /// \desc material diffuse color location
        GLint materialDiffColor;
        /// \desc material specular color location
        GLint materialSpecColor;
        /// \desc material ambient color location
        GLint materialAmbColor;
        /// \desc material shininess
        GLint materialShininess;
        
    } _gouraudShaderProgramUniformLocations;
    /// \desc stores the locations of all of our shader attributes
    struct GouraudShaderProgramAttributeLocations {
        /// \desc vertex position location
        GLint vPos;
        /// \desc vertex normal location
        GLint vNormal;
    } _gouraudShaderProgramAttributeLocations;

    /// \desc shader program that performs flat shading of a single color
    CSCI441::ShaderProgram* _flatShaderProgram;
    struct FlatShaderProgramUniformLocations {
        /// \desc precomputed MVP matrix location
        GLint mvpMatrix;
        /// \desc the color to apply location
        GLint color;
    } _flatShaderProgramUniformLocations;

    
    //========================= Private Helper Functions =========================//

    /// \desc sends the material uniforms to the Gouraud Shader Program
    /// \param material material properties to set
    void _setMaterialProperties(const CSCI441::Materials::Material& material) const;

    /// \desc sends the matrix uniforms to a given shader program
    /// \param shaderProgram shader program to send matrices to
    /// \param modelMatrix model matrix
    /// \param viewMatrix view matrix
    /// \param projectionMatrix projection matrix
    /// \param mvpMtxLocation location of the MVP matrix within the shaderProgram
    /// \param normalMtxLocation location of the Normal matrix within the shaderProgram (-1 by default)
    static void _computeAndSendTransformationMatrices(const CSCI441::ShaderProgram* shaderProgram,
                                               const glm::mat4& modelMatrix, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix,
                                               GLint mvpMtxLocation, GLint normalMtxLocation = -1, GLint modelMatrixLocation = -1);
};

void a3_keyboard_callback(GLFWwindow *window, int key, int scancode, int action, int mods );
void a3_cursor_callback(GLFWwindow *window, double x, double y );
void a3_mouse_button_callback(GLFWwindow *window, int button, int action, int mods );
void a3_scroll_callback(GLFWwindow *window, double xOffset, double yOffset);

#endif// A3_A3_ENGINE_H