#version 460

in vec2 uv;

layout(location = 0) out vec4 fragColor;

void swap(inout float a, inout float b) {
   float c = a;
   a = b;
   b = c;
}

#define M_PI  3.141592

uniform vec3 uAABBSize;

uniform mat4 uInvP;
uniform mat4 uInvV;
uniform vec3 uCamPos;
uniform float uCloudScale;
uniform vec3 uCloudOffset;
uniform float uDensityMultiplier;
uniform float uDensityThreshold;

uniform sampler3D uNoiseTex1;
uniform sampler3D uNoiseTex2;
uniform vec3 uLightDirection;
uniform vec4 uLightColor;

const int MAX_RAYMARCH_STEP = 32;
const int MAX_LIGHTMARCH_STEP = 16;
const float PHASE_G = 0.3f;

float getDensity(vec3 p) {
   vec4 noiseTex1 = texture(uNoiseTex1, p * 0.1 * uCloudScale + uCloudOffset).rgba;
   float density = noiseTex1.r * noiseTex1.g;
   density = density - noiseTex1.b + noiseTex1.a;
   return max(density - uDensityThreshold, 0.0f) * uDensityMultiplier;
}

// Hash by David_Hoskins
#define UI0 1597334673U
#define UI1 3812015801U
#define UI2 uvec2(UI0, UI1)
#define UI3 uvec3(UI0, UI1, 2798796415U)
#define UIF (1.0 / float(0xffffffffU))

vec2 hash22(vec2 p)
{
	uvec2 q = uvec2(ivec2(p)) * UI2;
	q = (q.x ^ q.y) * UI2;
	return -1. + 2. * vec2(q) * UIF;
}

bool RayBoxIntersection( in vec3 ro, in vec3 rd, vec3 boxSize, out vec2 t) 
{
    vec3 m = 1.0/rd; // can precompute if traversing a set of aligned boxes
    vec3 n = m*ro;   // can precompute if traversing a set of aligned boxes
    vec3 k = abs(m)*boxSize;
    vec3 t1 = -n - k;
    vec3 t2 = -n + k;
    float tN = max( max( t1.x, t1.y ), t1.z );
    float tF = min( min( t2.x, t2.y ), t2.z );
    if( tN>tF || tF<0.0) return false;
    t = vec2( tN, tF );
    return true;
}

vec3 GetRayDir(vec2 ndcCoord) {
  vec4 ndc = vec4(ndcCoord, -1.0f, 1.0f);
  vec4	viewCoord =	uInvP * ndc;
  viewCoord.z =	-1.0f;
  viewCoord.w =	0.0f;
  
  vec4 worldCoord = uInvV * viewCoord;
  return normalize(worldCoord.xyz);
}

float phase(const float g, const float cosTheta)
{
    float denom = 1 + g * g - 2 * g * cosTheta;
    return 1 / (4 * M_PI) * (1 - g * g) / (denom * sqrt(denom));
}

float lightMarch(vec3 r0, vec3 rd) {
  vec2 t = vec2(0.0f);
  if(!RayBoxIntersection(r0, rd, uAABBSize, t)) return 0.0f; 

  float stepSize = t.y / float(MAX_LIGHTMARCH_STEP);

  float opticalDepth = 0.0f;
  for(int i = 0; i < MAX_LIGHTMARCH_STEP; ++i) {
     opticalDepth += max(getDensity(r0) * stepSize, 0.0f);
     r0 += rd * stepSize;
  }

  return exp(-opticalDepth);
}

void main() {
   vec3 r0 = uCamPos;
   vec3 rd = GetRayDir(uv);

   vec3 backgroundColor = vec3(0.5f, 0.7f, 1.0f); 
   vec3 color = backgroundColor;
   vec2 t = vec2(0.0f);

   if(RayBoxIntersection(r0, rd, uAABBSize, t)) {

      float dstInsideBox = t.y - t.x;
      float stepSize = dstInsideBox / float(MAX_RAYMARCH_STEP);
      vec3 p = r0 + t.x * rd;

      float transmittance = 1.0f;
      float totalEnergy = 0.0f;

      float cosTheta = dot(uLightDirection, rd);
	  for(int i	= 0; i < MAX_RAYMARCH_STEP;	++i) {
          float density = getDensity(p);

          float lightTransmittance = lightMarch(p, uLightDirection);
          totalEnergy += lightTransmittance * phase(PHASE_G, cosTheta) * transmittance * density * stepSize;

          transmittance *= exp(-density * stepSize);

          if(transmittance < 0.001f) break;

		  p	+= rd *	stepSize;
	  }
      vec3 cloudColor = totalEnergy * uLightColor.xyz * uLightColor.w;
      color = transmittance * backgroundColor + cloudColor;
   }
   color /=(1.0 + color);
   color = pow(color, vec3(0.4545));

   fragColor = vec4(color, 1.0f);
}