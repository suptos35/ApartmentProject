#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec2 TexCoords;
in vec3 Color;
in vec3 FragPos;

uniform vec3 viewPos;
uniform float materialShininess;
uniform bool diffuseEnabled;
uniform bool specularEnabled;

// Texture uniforms
uniform vec3 objectColor;
uniform bool useVertexColor;
uniform sampler2D imageTexture;
uniform int textureMode; 
uniform int blendMode;   
uniform float uvScale;

// Lighting Structs
struct DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    
    // Attenuation
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

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453123);
}

// Directional Light Model
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 baseColor) {
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    
    vec3 ambient = light.ambient * baseColor;
    vec3 diffuse = light.diffuse * diff * baseColor * (diffuseEnabled ? 1.0 : 0.0);
    vec3 specular = light.specular * spec * vec3(0.5) * (specularEnabled ? 1.0 : 0.0);
    
    return (ambient + diffuse + specular);
}

// Point Light Model with Attenuation
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor) {
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    vec3 ambient = light.ambient * baseColor * attenuation;
    vec3 diffuse = light.diffuse * diff * baseColor * attenuation * (diffuseEnabled ? 1.0 : 0.0);
    vec3 specular = light.specular * spec * vec3(0.5) * attenuation * (specularEnabled ? 1.0 : 0.0);
    
    return (ambient + diffuse + specular);
}

// Spot Light Model with Soft Edges
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor) {
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    vec3 ambient = light.ambient * baseColor * attenuation;
    vec3 diffuse = light.diffuse * diff * baseColor * attenuation * intensity * (diffuseEnabled ? 1.0 : 0.0);
    vec3 specular = light.specular * spec * vec3(0.5) * attenuation * intensity * (specularEnabled ? 1.0 : 0.0);
    
    return (ambient + diffuse + specular);
}

void main() {
    if (textureMode == 4) {
        FragColor = vec4(useVertexColor ? Color : objectColor, 1.0);
        return;
    }

    // 1. Get Base Color from Texture setup
    vec2 scaledUV = TexCoords * uvScale;
    vec3 baseCol = vec3(1.0);

    if (textureMode == 0) {
        baseCol = useVertexColor ? Color : objectColor;
    } else if (textureMode == 1) {
        baseCol = texture(imageTexture, scaledUV).rgb;
    } else if (textureMode == 2) {
        float cx = floor(scaledUV.x);
        float cy = floor(scaledUV.y);
        float res = mod(cx + cy, 2.0);
        baseCol = mix(vec3(0.9), vec3(0.2), res);
    } else if (textureMode == 3) {
        float n = hash(scaledUV);
        baseCol = mix(vec3(0.8), vec3(0.6), n);
    }

    if (blendMode == 1) {
        vec3 vCol = useVertexColor ? Color : objectColor;
        baseCol *= vCol;
    } else if (blendMode == 2) {
        float wave = (sin(FragPos.x * 2.0) + cos(FragPos.z * 2.0) + 2.0) / 4.0;
        vec3 waveCol = mix(vec3(0.2, 0.4, 0.8), vec3(0.8, 0.3, 0.2), wave);
        baseCol *= waveCol;
    }

    // 2. Calculate Lighting
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    vec3 result = CalcDirLight(dirLight, norm, viewDir, baseCol);
    
    for(int i = 0; i < activePointLights; i++) {
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, baseCol);
    }
    
    if (useSpotLight) {
        result += CalcSpotLight(spotLight, norm, FragPos, viewDir, baseCol);
    }

    FragColor = vec4(result, 1.0);
}