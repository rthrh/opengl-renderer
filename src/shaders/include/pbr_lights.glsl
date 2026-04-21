

float AttenuationClamped(float dist, float range) {
    float attenuation = clamp(1.0 - (dist / range), 0.0, 1.0);
    attenuation = attenuation * attenuation;
    return attenuation;
}

float AttenuationInvSquare(float dist, float range) {
    float attenuation = 1.0 / (dist * dist);
    return attenuation;
}


// TODO pack parameters
vec3 CalcDirectionalLight(vec3 N, vec3 V,
                          vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3  lightColor = dirLight.colorAndIntensity.rgb;
    float intensity = dirLight.colorAndIntensity.a;
    vec3  L = normalize(-dirLight.direction.xyz); // toward light

    vec3 radiance = lightColor * intensity;

    return CalcPBR(N, V, L, albedo, metallic, roughness, F0, radiance);
}

vec3 CalcPointLight(PointLight light, vec3 N, vec3 V, vec3 fragPos,
                    vec3 albedo, float metallic, float roughness, vec3 F0) {

    vec3 position = light.positionAndRange.xyz;
    float range = light.positionAndRange.w;
    float distance = length(position - fragPos);
    if (distance > range) {
        return vec3(0.0);
    }

    vec3 lightColor = light.colorAndIntensity.rgb;
    float intensity  = light.colorAndIntensity.a;

    vec3 L = normalize(position - fragPos);

    float attenuation = AttenuationClamped(distance, range);

    vec3 radiance = lightColor * intensity * attenuation;

    return CalcPBR(N, V, L, albedo, metallic, roughness, F0, radiance);
}

vec3 CalcSpotLight(SpotLight light, vec3 N, vec3 V, vec3 fragPos,
                   vec3 albedo, float metallic, float roughness, vec3 F0) {

    vec3 position = light.position.xyz;
    float range = light.range;
    float distance = length(position - fragPos);
    if (distance > range) {
        return vec3(0.0);
    }

    vec3 lightColor = light.colorAndIntensity.rgb;
    float intensity = light.colorAndIntensity.a;

    vec3 L = normalize(position - fragPos);

    float attenuation = AttenuationClamped(distance, range);

    // TODO add early exit if frag pos is outside of outer cone
    float theta = dot(L, normalize(-light.direction.xyz));
    float epsilon = max(light.innerCone - light.outerCone, 0.0001);
    float spotFactor = clamp((theta - light.outerCone) / epsilon, 0.0, 1.0);

    vec3 radiance = lightColor * intensity * attenuation * spotFactor;

    return CalcPBR(N, V, L, albedo, metallic, roughness, F0, radiance);
}
