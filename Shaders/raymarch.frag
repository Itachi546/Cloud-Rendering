#version 460

in vec2 uv;

layout(location = 0) out vec4 fragColor;

void swap(inout float a, inout float b) {
   float c = a;
   a = b;
   b = c;
}

uniform vec3 uAABBMin;
uniform vec3 uAABBMax;

uniform mat4 uInvP;
uniform mat4 uInvV;

uniform vec3 uCamPos;

uniform sampler3D uNoiseTex1;
uniform sampler3D uNoiseTex2;

bool RayBoxIntersection(vec3 r0, vec3 rd, vec3 min, vec3 max, out vec2 t)
{
   vec3 invRd = 1.0f /	rd;
   vec3	t0 = (min -	r0)	* invRd;
   vec3	t1 = (max -	r0)	* invRd;

   float tMin =	t0.x;
   float tMax =	t1.x;
   if (tMin	> tMax)	swap(tMin, tMax);
   
   if (t0.y	> t1.y)	swap(t0.y, t1.y);

   if (t0.y	> tMax || tMin > t1.y) return false;
   tMin	= max(t0.y,	tMin);
   tMax	= min(t1.y,	tMax);

   if (t0.z	> t1.z)	swap(t0.z, t1.z);

   if (t0.z	> tMax || tMin > t1.z) return false;
   tMin	= max(t0.z,	tMin);
   tMax	= min(t1.z,	tMax);

   t.x = tMin;
   t.y = tMax;
   
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

void main() {
   vec3 r0 = uCamPos;
   vec3 rd = GetRayDir(uv);

   vec3 color = vec3(0.0f); 
   vec2 t = vec2(0.0f);
   if(RayBoxIntersection(r0, rd, uAABBMin, uAABBMax, t)) {
      float dist = t.y - t.x;
      float t = exp(-0.9 * dist);
      color = t * color + vec3(1.0f) * (1.0f - t);
   }
   color *= texture(uNoiseTex1, vec3(uv * 0.5f + 0.5f, 0.1f)).g;
   fragColor = vec4(color, 1.0f);
}