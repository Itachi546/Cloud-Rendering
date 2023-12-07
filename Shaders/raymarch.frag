#version 460

in vec2 uv;

layout(location = 0) out vec4 fragColor;

void swap(inout float a, inout float b) {
   float c = a;
   a = b;
   b = c;
}

const float PI = 3.141592;

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
uniform vec4 uLayerContribution;
uniform float uPhaseG = 0.5f;

const int MAX_RAYMARCH_STEP = 32;
const int MAX_LIGHTMARCH_STEP = 8;


float remap(in float val, in float inMin, in float inMax, in float outMin, in float outMax) {
    return (val - inMin)/(inMax - inMin) * (outMax - outMin) + outMin;
}

// clamps the input value to (inMin, inMax) and performs a remap
float clampRemap(in float val, in float inMin, in float inMax, in float outMin, in float outMax) {
    float clVal = clamp(val, inMin, inMax);
    return (clVal - inMin)/(inMax - inMin) * (outMax - outMin) + outMin;
}

float saturate(in float val) {
    return clamp(val, 0, 1);
}

float sampleDensity(vec3 p, float coverage) {
   p = p * 0.2 * uCloudScale + uCloudOffset;
   vec4 cloudBase = texture(uNoiseTex1, p);
   float lowFreqNoise = cloudBase.r * uLayerContribution.x;
   float highFreqNoise = dot(cloudBase.gba, uLayerContribution.yzw);
   float baseDensity = clampRemap(lowFreqNoise, highFreqNoise - 1, 1.0, 0.0, 1.0);

   vec3 cloudDetail = texture(uNoiseTex2, p).rgb;
   float detailNoise = dot(cloudDetail, vec3(0.5, 0.7, 1.0));
   baseDensity = clampRemap(baseDensity, detailNoise - 1.0f, 1.0f, 0.0f, 1.0f);

   baseDensity = clamp(baseDensity, 0.0, 1.0);
   return max(baseDensity - coverage, 0.0f) * uDensityMultiplier;
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

float henyeyGreenstein(float cosAngle, float eccentricity) {
    float eccentricity2 = eccentricity*eccentricity;
    return ((1.0 - eccentricity2) / pow(1.0 + eccentricity2 - 2.0*eccentricity*cosAngle, 3.0/2.0)) / (4*PI);
}


float lightMarch(vec3 r0, vec3 rd) {
  vec2 t = vec2(0.0f);
  if(!RayBoxIntersection(r0, rd, uAABBSize, t)) return 0.0f; 

  float stepSize = t.y / float(MAX_LIGHTMARCH_STEP);

  float opticalDepth = 0.0f;
  for(int i = 0; i < MAX_LIGHTMARCH_STEP; ++i) {
     r0 += rd * stepSize;
     opticalDepth += max(sampleDensity(r0, uDensityThreshold) * stepSize, 0.0f);
  }

  return exp(-opticalDepth);
}

void main() {
   vec3 r0 = uCamPos;
   vec3 rd = GetRayDir(uv);

   float h = uv.y * 0.5 + 0.5;
   vec3 backgroundColor = mix(vec3(0.5f, 0.7f, 1.0f), vec3(1.0, 0.8, 0.3), 1.0 - h); 
   vec3 color = backgroundColor;
   vec2 t = vec2(0.0f);

   if(RayBoxIntersection(r0, rd, uAABBSize, t)) {

      float dstInsideBox = t.y - t.x;
      float stepSize = dstInsideBox / float(MAX_RAYMARCH_STEP);
      vec3 p = r0 + t.x * rd;

      float transmittance = 1.0f;
      float totalEnergy = 0.0f;

      float	cosTheta = dot(uLightDirection,	normalize(-rd));
	  for(int i	= 0; i < MAX_RAYMARCH_STEP;	++i) {
		  p	+= rd *	stepSize;

          float density = sampleDensity(p, uDensityThreshold);
          if(density > 0.0f) {
    		  float	lightTransmittance = lightMarch(p, uLightDirection);
			  totalEnergy += lightTransmittance	* henyeyGreenstein(cosTheta, uPhaseG) *	transmittance *	density	* stepSize;
			  transmittance	*= exp(-density	* stepSize);
		  }
		  if(transmittance < 0.001f) break;
	  }
      vec3 cloudColor = totalEnergy * uLightColor.xyz * uLightColor.w;
      color = transmittance * backgroundColor + cloudColor;
   }
   color /=(1.0	+ color);
   color = pow(color, vec3(0.4545));

   fragColor = vec4(color, 1.0f);
}