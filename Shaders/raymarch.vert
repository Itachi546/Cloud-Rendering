#version 460
/*
vec2 position[6] = vec2[6](
   vec2(-1.0f, -1.0f),
   vec2( 1.0f,  1.0f),
   vec2(-1.0f,  1.0f),

   vec2(-1.0f, -1.0f),
   vec2( 1.0f,  1.0f),
   vec2( 1.0f, -1.0f)
);
*/
layout(location = 0) in vec2 position;

out vec2 uv;

void main() {
   gl_Position = vec4(position, 0.0f, 1.0f);
   uv = position;
}