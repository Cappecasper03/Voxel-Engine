#version 460 core

layout ( location = 0 ) in vec3 i_position;
layout ( location = 1 ) in vec3 i_color;
layout ( location = 2 ) in vec2 i_tex_coord;

out vec3 color;
out vec2 tex_coord;

void main( )
{
    gl_Position = vec4( i_position, 1 );
    color = i_color;
    tex_coord = i_tex_coord;
}