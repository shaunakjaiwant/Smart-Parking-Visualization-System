#version 330 core
in vec4 vertexColor;
in vec2 TexCoords;

out vec4 FragColor;

uniform bool useTexture;
uniform sampler2D texture_diffuse1;
uniform float alphaMultiplier;

void main()
{
    vec4 col = vertexColor;
    if (useTexture) {
        col *= texture(texture_diffuse1, TexCoords);
    }
    FragColor = vec4(col.rgb, col.a * (alphaMultiplier > 0.0 ? alphaMultiplier : 1.0));
}
