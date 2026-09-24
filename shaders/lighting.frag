#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 VertexColor;
in vec4 FragPosLightSpace;

// Material
uniform vec3 materialColor;
uniform bool useMaterialColor;
uniform float shininess;
uniform float specularStrength;

// Sun/Directional Light
uniform vec3 dirLightDirection;
uniform vec3 dirLightColor;
uniform float dirLightIntensity;

// Ambient Light
uniform vec3 ambientColor;

// Camera Position
uniform vec3 viewPos;

// Emissive / Beacon
uniform vec3 emissiveColor;

// Shadow Map
uniform sampler2D shadowMap;
uniform bool enableShadows;
uniform float shadowBias;
uniform float shadowIntensity;

// Point Lights (for street lamps & night mode)
struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};
#define MAX_POINT_LIGHTS 6
uniform int numPointLights;
uniform PointLight pointLights[MAX_POINT_LIGHTS];

float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    if (!enableShadows) {
        return 0.0;
    }

    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to [0, 1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Fragment outside light projection frustum has no shadow
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    // Slope-scale dynamic bias to eliminate shadow acne
    float bias = max(shadowBias * (1.0 - dot(normal, lightDir)), shadowBias * 0.2);

    // 3x3 Percentage-Closer Filtering (PCF) for soft shadow edges
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow * shadowIntensity;
}

void main()
{
    vec3 baseColor = useMaterialColor ? materialColor : VertexColor.rgb;
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // 1. Ambient Lighting
    vec3 ambient = ambientColor * baseColor;

    // 2. Directional Sun/Moon Light with Blinn-Phong Shading
    vec3 lightDir = normalize(-dirLightDirection);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * dirLightColor * dirLightIntensity * baseColor;

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess > 0.0 ? shininess : 32.0);
    vec3 specular = dirLightColor * (spec * specularStrength);

    // 3. Shadow Factor
    float shadow = calculateShadow(FragPosLightSpace, norm, lightDir);
    vec3 dirLightResult = ambient + (1.0 - shadow) * (diffuse + specular);

    // 4. Local Point Lights (Street lamps in aisles)
    vec3 pointLightResult = vec3(0.0);
    for (int i = 0; i < numPointLights && i < MAX_POINT_LIGHTS; ++i) {
        vec3 pLightDir = normalize(pointLights[i].position - FragPos);
        float pDist = length(pointLights[i].position - FragPos);
        // Attenuation
        float attenuation = 1.0 / (1.0 + 0.08 * pDist + 0.018 * pDist * pDist);

        // Diffuse
        float pDiff = max(dot(norm, pLightDir), 0.0);
        vec3 pDiffuse = pDiff * pointLights[i].color * pointLights[i].intensity * baseColor;

        // Specular
        vec3 pHalfway = normalize(pLightDir + viewDir);
        float pSpec = pow(max(dot(norm, pHalfway), 0.0), 16.0);
        vec3 pSpecular = pointLights[i].color * (pSpec * 0.3);

        pointLightResult += (pDiffuse + pSpecular) * attenuation;
    }

    // 5. Total Combined Illumination
    vec3 totalColor = dirLightResult + pointLightResult + emissiveColor;

    FragColor = vec4(totalColor, VertexColor.a > 0.0 ? VertexColor.a : 1.0);
}
