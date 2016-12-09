#version 420 core

struct Material
{
 vec4 ambRefl;
 vec4 difRefl;
 vec4 specRefl;
 vec4 emitCols;
 float shininess;
};
uniform Material terrainFandB;
struct Light
{
 vec4 ambCols;
 vec4 difCols;
 vec4 specCols;
 vec4 coords;
};
uniform Light light0;

layout(location=0) in vec4 terrainCoords;
layout(location=1) in vec3 Normals;
uniform vec4 globAmb;
uniform mat4 projMat;
uniform mat4 modelViewMat;
uniform mat3 normalMat;

vec3 normal, lightDirection, eyeDirection, halfway;
vec4 fAndBEmit, fAndBGlobAmb, fAndBAmb, fAndBDif, fAndBSpec;

smooth out vec4 colorsExport;
void main(void)
{
   gl_Position = projMat * modelViewMat * terrainCoords;
   colorsExport = globAmb * terrainFandB.ambRefl;
   lightDirection = normalize(vec3(light0.coords));
   normal = normalize(normalMat * Normals);
   vec2 tst = normal.xy;
   halfway = (length(lightDirection + eyeDirection) == 0.0) ? 
             vec3(0.0) : (lightDirection + eyeDirection)/length(lightDirection + eyeDirection);
   
   fAndBEmit = terrainFandB.emitCols;
   fAndBGlobAmb = globAmb * terrainFandB.ambRefl;
   fAndBAmb = light0.ambCols * terrainFandB.ambRefl;
   fAndBDif = max(dot(normal, lightDirection), 0.0) * light0.difCols * terrainFandB.difRefl;    
   fAndBSpec = pow(max(dot(normal, halfway), 0.0), terrainFandB.shininess) * 
               light0.specCols * terrainFandB.specRefl;
	
   //colorsExport = max(dot(normal, lightDirection), 0.0f) * (light0.difCols * terrainFandB.difRefl);
   colorsExport = vec4(vec3(min(fAndBEmit + fAndBGlobAmb + fAndBAmb + 
                           fAndBDif + fAndBSpec, vec4(1.0))), 1.0);  
}