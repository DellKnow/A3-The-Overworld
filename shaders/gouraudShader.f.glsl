#version 410 core

// ========================= uniform inputs ===============================//
uniform vec3 lightColor;                    // light color
uniform vec4 materialAmbColor;              // the material ambient color
uniform int useTexture;                     // 1 = this draw is textured, 0 = plain material color
uniform sampler2D tex0;                     // block texture
uniform int unlit;                          // 1 = show the material color with no lighting

// ========================= varying inputs ===============================//
layout(location = 0) in vec4 litColor;      // ambient + diffuse applied to the vertex
layout(location = 1) in vec4 specColor;     // specular color
layout(location = 2) in vec2 st;            // world-space texture coordinate

// ============================= outputs ==================================//
out vec4 fragColorOut;                  // color to apply to this fragment

void main() {
    // if we are looking at the back face of the fragment
    if (!gl_FrontFacing && unlit == 0) {
        fragColorOut = vec4( lightColor * materialAmbColor.rgb, 1.0f );
        return;
    } 
    // if we are looking at the front face of the fragment

    // white (1) when untextured, so the material color passes through unchanged
    vec4 tex = (useTexture == 1) ? texture(tex0, st) : vec4(1.0);

    if (useTexture == 1 && tex.a < 0.5)
        discard;

    // texture tints the lit color, the highlight is added on top in the light's color
    fragColorOut = vec4(tex.rgb * litColor.rgb + specColor.rgb, tex.a);
}