
float ShadowDirectionalLight(vec3 fragPos, vec3 normal, mat4 lightSpaceMatrix)
{
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; // perform perspective divide

    // keep the shadow at 0.0 when outside the farPlane region of the light's frustum.
    if(projCoords.z > 1.0)
        return 0.0;

    projCoords = projCoords * 0.5 + 0.5; // transform to [0,1] range
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowDirMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;

    // calculate bias
    vec3 lightDir = normalize(-dirLight.direction.xyz);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF 3x3 kernel
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowDirMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowDirMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }

    shadow /= 9.0;  
    return shadow;
}


float ShadowPointLight(vec3 fragPos, vec3 lightPos, int lightIndex)
{
    vec3 fragToLight = fragPos - lightPos;
    float closestDepth = texture(shadowPointMaps, vec4(fragToLight, float(lightIndex))).r;
    closestDepth *= farPlane;
    float currentDepth = length(fragToLight);

    float bias = 0.05; // we use a much larger bias since depth is now in [near_plane, far_plane] range
    float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;
    // TODO PCF kernel
    // display closestDepth as debug (to visualize depth cubemap)
    // FragColor = vec4(vec3(closestDepth / far_plane), 1.0);    
        
    return shadow;
}


float ShadowSpotLight(vec3 fragPos, vec3 normal, mat4 lightSpaceMatrix, int lightIndex)
{
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; // perform perspective divide

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        return 0.0;

    projCoords = projCoords * 0.5 + 0.5; // transform to [0,1] range

    float currentDepth = projCoords.z; // get depth of current fragment from light's perspective

    // calculate bias (based on depth map resolution and slope)
    vec3 lightDir = normalize(-spotLights.lights[lightIndex].direction.xyz);
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF 3x3 kernel
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowSpotMap, 0).xy);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowSpotMap, vec3(projCoords.xy + vec2(x, y) * texelSize, float(lightIndex))).r;
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }

    shadow /= 9.0;  
    return shadow;
}
