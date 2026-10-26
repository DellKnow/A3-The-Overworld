#version 410 core

// ========================= uniform inputs ===============================//
// general parameters
uniform mat4 mvpMatrix;                     // the precomputed Model-View-Projection Matrix
uniform mat3 normalMtx;                     // normal matrix
uniform mat4 modelMtx;                      // model matrix

uniform vec3 lightDir;                      // light direction in world space
uniform vec3 lightColor;                    // light color
uniform vec3 camPos;                        // camera position

// material
uniform vec3 materialDiffColor;             // the material diffuse color
uniform vec3 materialSpecColor;             // the material specular color
uniform vec3 materialAmbColor;              // the material ambient color
uniform float materialShininess;            // the material shininess

// ======================== attribute inputs ===============================//
layout(location = 0) in vec3 vPos;          // the position of this specific vertex in object space
layout(location = 1) in vec3 vNormal;       // the normal of this specific vertex in object space

// ========================= varying inputs =================================//
layout(location = 0) out vec4 litColor;     // ambient + diffuse applied to the vertex
layout(location = 1) out vec4 specColor;    // specular color 

// compute the diffuse color using lambertian diffuse reflectance
vec3 diffuseColor(vec3 N, vec3 L) {
    return lightColor * materialDiffColor * max( dot(N, L), 0.0 );
}

// compute the ambient color, light scattered uniformly throughout the scene
vec3 ambientColor() {
    return lightColor * materialAmbColor;
}

// compute the specular color, direct reflection off shiny surfaces
vec3 specularColor(vec3 N, vec3 L, vec3 worldPos) {
    // a surface facing away from the light cannot reflect it
    if (dot(N, L) <= 0) return vec3(0.0);

    vec3 R = 2.0 * dot(N, L) * N - L;           // mirror of L around N
    vec3 V = normalize(camPos - worldPos);      // direction from the vertex to the eye

    return lightColor * materialSpecColor * pow(max( dot(R, V), 0.0 ), materialShininess);
}

void main() {
    // transform & output the vertex in clip space
    gl_Position = mvpMatrix * vec4(vPos, 1.0);

    // parameters for lightining in world space
    vec3 N = normalize( normalMtx * vNormal );
    vec3 L = normalize(-lightDir);  // direction towards the light
    vec3 worldPos = (modelMtx * vec4(vPos, 1.0)).xyz;

    // compute each component of the Phong Illumination Model
    vec3 diffuse = diffuseColor(N, L);
    vec3 specular = specularColor(N, L, worldPos);
    vec3 ambient = ambientColor();

    // assign the final components to the vertex
    litColor = vec4(ambient + diffuse, 1.0f);
    specColor = vec4(specular, 1.0f);
}