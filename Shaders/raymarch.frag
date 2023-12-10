#version 460

in vec2 uv;

layout(location = 0) out vec4 fragColor;

void swap(inout float a, inout float b) {
   float c = a;
   a = b;
   b = c;
}

const float PI = 3.141592;

uniform vec3 uAABBMin;
uniform vec3 uAABBMax;

uniform mat4 uInvP;
uniform mat4 uInvV;
uniform vec3 uCamPos;
uniform float uCloudScale;
uniform vec3 uCloudOffset;
uniform float uDensityMultiplier;
uniform float uDensityThreshold;
uniform float uLightAbsorption;

uniform sampler3D uNoiseTex1;
uniform sampler3D uNoiseTex2;
uniform sampler2D uBlueNoiseTex;
uniform sampler2D uDepthTexture;
uniform sampler2D uSceneTexture;

uniform vec3 uLightDirection;
uniform vec4 uLightColor;
uniform vec4 uLayerContribution;
uniform float uPhaseG;
uniform int uSugarPowder;

const int MAX_RAYMARCH_STEP = 32;
const int MAX_LIGHTMARCH_STEP = 6;


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

float GetHeightFraction(vec3 p) 
{
  return (p.y + uAABBMin.y) / (uAABBMax.y - uAABBMin.y);
}

float SampleDepth(vec2 uv) {
   return texture(uDepthTexture, uv).r;
}

float LinearizeDepth(float d, float zNear, float zFar) {
   return zNear * zFar / (zFar + d * (zNear - zFar));
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

   float heightFraction = GetHeightFraction(p);
   baseDensity = clamp(baseDensity, 0.0, 1.0);
   return max(baseDensity - coverage, 0.0f) * uDensityMultiplier;
}

vec2 RaySphereIntersection( in vec3 ro, in vec3 rd, in vec3 ce, float ra )
{
    vec3 oc = ro - ce;
    float b = dot( oc, rd );
    float c = dot( oc, oc ) - ra*ra;
    float h = b*b - c;
    if( h<0.0 ) return vec2(-1.0); // no intersection
    h = sqrt( h );
    return vec2( -b-h, -b+h );
}
/*
bool RayBoxIntersection(vec3 aabbMin, vec3 aabbMax, vec3 r0, vec3 rd, out vec2 t) 
{
   vec3 invRayDir = 1.0f / rd;
   vec3 t0 = (aabbMin - r0) * invRayDir;
   vec3 t1 = (aabbMax - r0) * invRayDir;

   vec3 tmin = min(t0, t1);
   vec3 tmax = max(t0, t1);

   float dstA = max(max(tmin.x, tmin.y), tmin.z);
   float dstB = min(min(tmax.x, tmax.y), tmax.z);

   t = vec2(dstA, dstB);

   if(dstB < dstA || dstB < 0.0f) return false;
   return true;
}
*/

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


float sugarPowder(float opticalDepth) {
  return 1.0f - exp(-opticalDepth * 2.0f);
}

float lightMarch(vec3 r0, vec3 rd) {
  vec2 t = RaySphereIntersection(r0, rd, vec3(0.0f), 4000.0f);
  float stepSize = ceil(t.y) / float(MAX_LIGHTMARCH_STEP);

  float opticalDepth = 0.0f;
  for(int i = 0; i < MAX_LIGHTMARCH_STEP; ++i) {
     r0 += rd * stepSize;
     opticalDepth += max(sampleDensity(r0, uDensityThreshold), 0.0f);
  }

  return max(exp(-opticalDepth * uLightAbsorption * stepSize), 0.1);
}

void main() {

   vec3 r0 = uCamPos;
   float linearDepth = LinearizeDepth(SampleDepth(uv * 0.5f + 0.5f), 0.5f, 4000.0f);

   vec3 rd = GetRayDir(uv);

   vec3 color = texture(uSceneTexture, uv * 0.5f	+ 0.5f).rgb;
   vec2 t = vec2(0.0f);

   vec2 t0 = RaySphereIntersection(r0, rd, vec3(0.0f), 1500.0f);
   vec2 t1 = RaySphereIntersection(r0, rd, vec3(0.0f), 4000.0f);

   float dstToBox = t0.y;
   if(dstToBox < linearDepth) {
   float dstInsideBox =	ceil(t1.y - t0.y);
   float stepSize =	dstInsideBox / float(MAX_RAYMARCH_STEP);

   float noiseOffset = texture(uBlueNoiseTex, uv).r;
   vec3	p =	r0 + (t.x +	noiseOffset) * rd;

   float transmittance = 1.0f;
   float totalEnergy = 0.0f;

   float cosTheta = dot(uLightDirection, normalize(-rd));
   float tau = stepSize	* uLightAbsorption;

   for(int i = 0; i < MAX_RAYMARCH_STEP; ++i) {
      float density = sampleDensity(p,	uDensityThreshold);
	  if(density >	0.0f) {
    	  float	lightTransmittance = lightMarch(p, uLightDirection);

		  float	inscattProb	= 1.0f;
		  if(uSugarPowder == 1)
    		  inscattProb =	sugarPowder(density	* stepSize);

		  totalEnergy += lightTransmittance	* henyeyGreenstein(cosTheta, uPhaseG) *	inscattProb	* transmittance;
		  transmittance	*= exp(-density	* tau);
	  }
   	  if(transmittance < 0.001f) break;
        p += rd * stepSize;
  }
  vec3 cloudColor	= totalEnergy *	uLightColor.xyz	* uLightColor.w;
  color	= transmittance * color + cloudColor;
	}
  color /=(1.0 + color);
  color	= pow(color, vec3(0.4545));
  fragColor	= vec4(color, 1.0f);
}