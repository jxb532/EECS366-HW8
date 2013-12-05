   
uniform vec3 AmbientContribution,DiffuseContribution,SpecularContribution;
uniform float exponent;
uniform int mapMode;

attribute vec3 rm_Binormal;
attribute vec3 rm_Tangent;

varying vec3 Texcoord, fvNormal, fvLight, fvView, fvHalfway;

/*
===============================================================================
   Phong Shading: Vertex Program
===============================================================================
*/

void main(void)
{
   gl_Position = ftransform();
   Texcoord = gl_MultiTexCoord0.xy;
   
   // Transform vertex position to view space
   
   vec4 fvObjectPosition = gl_ModelViewMatrix * gl_Vertex;
   
   // Compute normal, light and view vectors in view space
   vec3 fvLightDirection    = normalize(gl_LightSource[0].position.xyz - fvObjectPosition.xyz);
   vec3 fvViewDirection     = normalize(-fvObjectPosition.xyz);
   
   fvNormal   = normalize(gl_NormalMatrix * gl_Normal);

   
   // Compute the halfway vector if the halfway approximation is used   
   
   fvHalfway  = normalize(fvLightDirection + fvViewDirection );
   
   fvView = fvViewDirection;
   fvLight = fvLightDirection;
   
   if (mapMode == 11) { //placeholder for now
	   fvView.x  = dot( fvTangent, fvViewDirection );
	   fvView.y  = dot( fvBinormal, fvViewDirection );
	   fvView.z  = dot( fvNormal, fvViewDirection );
	   
	   fvLight.x  = dot( fvTangent, fvLightDirection.xyz );
	   fvLight.y  = dot( fvBinormal, fvLightDirection.xyz );
	   fvLight.z  = dot( fvNormal, fvLightDirection.xyz );
   }

}