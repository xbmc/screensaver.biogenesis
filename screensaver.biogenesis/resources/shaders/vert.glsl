#version 130

#ifdef GL_ES
precision mediump float;
#endif

// Attributes
attribute vec4 a_position;
attribute vec4 a_color;

// Varyings
varying vec4 v_color;

void main ()
{
  gl_Position = a_position;
  v_color = a_color;
}
