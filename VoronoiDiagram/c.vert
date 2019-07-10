in vec3 vPos;

uniform mat4 modelMatrix;
uniform mat4 projViewMatrix;

void main() {
	gl_Position = projViewMatrix * modelMatrix * vec4(vPos, 1.0);
}