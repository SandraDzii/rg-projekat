#version 330 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture1;
uniform bool light;

void main()
{
    if(light)
        FragColor = 0.35 * texture(texture1, TexCoords);
    else
        FragColor = 0.2 * texture(texture1, TexCoords);
}