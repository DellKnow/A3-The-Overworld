#version 410 core

// ========================= uniform inputs ===============================//
uniform vec3 lightColor;                    // light color
uniform vec3 materialAmbColor;              // the material ambient color
uniform int useTexture;                     // 1 = this draw is textured, 0 = plain material color
uniform sampler2D tex0;                     // block texture

// ========================= varying inputs ===============================//
layout(location = 0) in vec4 litColor;      // ambient + diffuse applied to the vertex
layout(location = 1) in vec4 specColor;     // specular color 

// ============================= outputs ==================================//
out vec4 fragColorOut;                  // color to apply to this fragment

void main() {
    // if we are looking at the back face of the fragment
    if (!gl_FrontFacing) {
        fragColorOut = vec4( lightColor * materialAmbColor, 1.0f );
        return;
    } 
    // if we are looking at the front face of the fragment

    // white (1) when untextured, so the material color passes through unchanged
    vec3 tex = (useTexture == 1) ? vec3(1.0) : vec3(1.0);   // T6: replace the first vec3(1.0) with texture(tex0, uv).rgb

    // texture tints the lit color, the highlight is added on top in the light's color
    fragColorOut = vec4(tex * litColor.rgb + specColor.rgb, 1.0);
}