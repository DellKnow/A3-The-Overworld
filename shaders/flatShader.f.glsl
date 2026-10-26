#version 410 core

// uniform inputs
uniform vec3 color;                     // color for all fragments

// varying inputs

// outputs
out vec4 fragColorOut;                  // color to apply to this fragment

void main() {
    // pass the uniform color through as output
    fragColorOut = vec4(color, 1.0);
}