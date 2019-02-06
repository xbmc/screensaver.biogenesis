#version 130

#ifdef GL_ES
precision mediump float;
#endif

// Varyings
varying vec4 v_color;

void main ()
{
  gl_FragColor = v_color;
}
