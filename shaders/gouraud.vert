#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 viewPos;
uniform float materialShininess;
uniform vec3 objectColor;
uniform bool useVertexColor;
uniform bool diffuseEnabled;
uniform bool specularEnabled;

struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#define NR_POINT_LIGHTS 22
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform int activePointLights;
uniform DirLight dirLight;
uniform SpotLight spotLight;
uniform bool useSpotLight;

out vec2 TexCoords;
out vec3 Color;
out vec3 VertexLightingColor;
out vec3 FragPos;

void main() {
    vec3 worldPos = vec3(model * vec4(aPos, 1.0));
    vec3 norm = normalize(mat3(transpose(inverse(model))) * aNormal);
    vec3 viewDir = normalize(viewPos - worldPos);

    vec3 baseColor = useVertexColor ? aColor : objectColor;

    vec3 result = vec3(0.0);

    vec3 dirLightDir = normalize(-dirLight.direction);
    float dirDiff = max(dot(norm, dirLightDir), 0.0);
    vec3 dirReflectDir = reflect(-dirLightDir, norm);
    float dirSpec = pow(max(dot(viewDir, dirReflectDir), 0.0), materialShininess);

    vec3 dirAmbient = dirLight.ambient * baseColor;
    vec3 dirDiffuse = dirLight.diffuse * dirDiff * baseColor * (diffuseEnabled ? 1.0 : 0.0);
    vec3 dirSpecular = dirLight.specular * dirSpec * vec3(0.5) * (specularEnabled ? 1.0 : 0.0);
    result += (dirAmbient + dirDiffuse + dirSpecular);

    for (int i = 0; i < activePointLights; i++) {
        vec3 pointLightDir = normalize(pointLights[i].position - worldPos);
        float pointDiff = max(dot(norm, pointLightDir), 0.0);
        vec3 pointReflectDir = reflect(-pointLightDir, norm);
        float pointSpec = pow(max(dot(viewDir, pointReflectDir), 0.0), materialShininess);

        float distance = length(pointLights[i].position - worldPos);
        float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].quadratic * (distance * distance));

        vec3 pointAmbient = pointLights[i].ambient * baseColor * attenuation;
        vec3 pointDiffuse = pointLights[i].diffuse * pointDiff * baseColor * attenuation * (diffuseEnabled ? 1.0 : 0.0);
        vec3 pointSpecular = pointLights[i].specular * pointSpec * vec3(0.5) * attenuation * (specularEnabled ? 1.0 : 0.0);
        result += (pointAmbient + pointDiffuse + pointSpecular);
    }

    if (useSpotLight) {
        vec3 spotLightDir = normalize(spotLight.position - worldPos);
        float spotDiff = max(dot(norm, spotLightDir), 0.0);
        vec3 spotReflectDir = reflect(-spotLightDir, norm);
        float spotSpec = pow(max(dot(viewDir, spotReflectDir), 0.0), materialShininess);

        float spotDistance = length(spotLight.position - worldPos);
        float spotAttenuation = 1.0 / (spotLight.constant + spotLight.linear * spotDistance + spotLight.quadratic * (spotDistance * spotDistance));

        float theta = dot(spotLightDir, normalize(-spotLight.direction));
        float epsilon = spotLight.cutOff - spotLight.outerCutOff;
        float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);

        vec3 spotAmbient = spotLight.ambient * baseColor * spotAttenuation;
        vec3 spotDiffuse = spotLight.diffuse * spotDiff * baseColor * spotAttenuation * intensity * (diffuseEnabled ? 1.0 : 0.0);
        vec3 spotSpecular = spotLight.specular * spotSpec * vec3(0.5) * spotAttenuation * intensity * (specularEnabled ? 1.0 : 0.0);
        result += (spotAmbient + spotDiffuse + spotSpecular);
    }

    FragPos = worldPos;
    TexCoords = aTexCoords;
    Color = aColor;
    VertexLightingColor = result;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
