#version 330 core
out vec4 FragColor;

in vec2 TexCoords;


uniform sampler2D texture_diffuse1;
uniform sampler2D texture_ambient1;
uniform vec4 color;

void main()
{   
    vec4 diffuse = texture(texture_diffuse1, TexCoords);
    vec4 ambient = texture(texture_ambient1, TexCoords);

    float alpha = color.a;
    FragColor = vec4(mix(diffuse, color, alpha));
}