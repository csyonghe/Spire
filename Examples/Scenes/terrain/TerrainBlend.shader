using "../DemoEngine.pipeline";

// layered material shader

// the terrain shader blends several layers of materials, each layer is expressed as a shader module

module GrassMaterial
{
    @perInstanceUniform sampler2D grass_a_map;
    @perInstanceUniform sampler2D grass_b_map;
    @perInstanceUniform sampler2D darkGrassMap;
    
    require vec2 uv;
    require float grassBlendFactor;
    require vec3 vert_pos;
    
    float darkgrassPattern = texture(darkGrassMap, uv* 0.2).x;
    vec3 darkGrassColor = vec3(0.52, 0.7, 0.25)*0.45;
    vec3 grass_b_albedo = texture(grass_b_map, uv).xyz;
    float grass_blend_b = grassBlendFactor;//index.y;
    float grassMacroMask = texture(grass_b_map, vert_pos.xz/vec2(90000.0)).r;
    float grass_a_b_mixFactor = clamp(grassMacroMask * 0.2 + grass_blend_b, 0.0, 1.0);
    vec3 grass_a_albedo = mix(darkGrassColor, texture(grass_a_map, uv).xyz, darkgrassPattern*0.6 + 0.4);
    public vec3 Albedo = mix(grass_b_albedo, grass_a_albedo, grass_a_b_mixFactor);
    
    @perInstanceUniform sampler2D grass_a_Nmap;
    @perInstanceUniform sampler2D grass_b_Nmap;
    vec3 grass_a_n = texture(grass_a_Nmap, uv).xyz;
    vec3 grass_b_n = texture(grass_b_Nmap, uv).xyz;
    public vec3 Normal = mix(grass_b_n, grass_a_n, grass_a_b_mixFactor);
    
    public float Roughness = 0.7;
    public float Specular = 0.4;
    public float Metallic = 0.1;
}

module SoilMaterial
{
    @perInstanceUniform sampler2D ground_gravel_map;
    @perInstanceUniform sampler2D ground_gravel_Nmap;
    
    require float soilTiling;
    require vec3 vert_pos;
    
    public vec3 Albedo = texture(ground_gravel_map, vert_pos.xz*vec2(soilTiling)).xyz;
    public vec3 Normal = texture(ground_gravel_Nmap, vert_pos.xz*vec2(soilTiling)).xyz;
    public float Roughness = 0.4;
    public float Specular = 0.4;
    public float Metallic = 0.3;
}

module RockMaterial
{
    @perInstanceUniform sampler2D rock_a_map;
    @perInstanceUniform sampler2D rock_a_Nmap;
    require vec2 uv;  
    public vec3 Abledo = texture(rock_a_map, uv).xyz;
    public vec3 Normal = texture(rock_a_Nmap, uv).xyz;
    public float Roughness = 0.8;
    public float Specular = 0.4;
    public float Metallic = 0.2;
}

module SnowMaterial
{
    @perInstanceUniform sampler2D snow_map;
    @perInstanceUniform sampler2D snow_Nmap;
    require vec2 uv;
    
    public vec3 Albedo = texture(snow_map, uv).xyz + vec3(0.2);
    public vec3 Normal = texture(snow_Nmap, uv).xyz;
    public float Roughness = 0.9;
    public float Specular = 0.1;
    public float Metallic = 0.1;
}

shader TerrainBlend
{
    using Header;
    @perInstanceUniform vec2 uv0;
    @perInstanceUniform vec2 uv_size;
	@perInstanceUniform sampler2D indexMap;
    @perInstanceUniform sampler2D ground_pebble_map;
    @perInstanceUniform sampler2D ground_pebble_Nmap;
    inline float groundRockTiling = 1.0/228.0;
    vec2 uv = vert_uv * uv_size + uv0;
    vec4 index = texture(indexMap, uv);
    vec2 tiledUV = vert_pos.xz / 180.0;

    using grass = GrassMaterial(tiledUV, index.y);
    using rock = RockMaterial(tiledUV);
    using soil = SoilMaterial(1.0/142.0);
    using snow = SnowMaterial(tiledUV);

    inline float ground_soild_mixFactor = index.z;
    inline float snow_mixFactor = max(0.0, 1.0-index.x-index.y-index.z);

    inline float rockGrass_mixFactor = clamp((vert_normal.y-0.65)*10.0, 0.0, 1.0);
    
    vec3 ground_rock_albedo = texture(ground_pebble_map, vert_pos.xz * groundRockTiling).xyz;
    float ground_rock_mixFactor
    {
        float rockHeightmap = (ground_rock_albedo.x + ground_rock_albedo.y + ground_rock_albedo.z) * 0.33;
        return clamp(100.0*(1.0 - rockHeightmap),0.0, 1.0);
    }

    vec3 baseColor
    { 
        vec3 rockGrassAlbedo = mix(rock.Abledo, grass.Albedo, rockGrass_mixFactor);
        vec3 terrainBlend = mix(rockGrassAlbedo, soil.Albedo, ground_soild_mixFactor);
        vec3 terrainSnowBlendAlbedo = mix(terrainBlend, snow.Albedo, snow_mixFactor);
        return mix(ground_rock_albedo, terrainSnowBlendAlbedo, ground_rock_mixFactor);
	}
    [TextureResolution: "128"]
    [Storage:"RGB8"]
    @precomputeTex vec3 Albedo = baseColor*1.16;
    @precomputeUniform vec3 Albedo = baseColor * 1.15;
    @lqfs vec3 Albedo = baseColor*1.16;
    vec3 Albedo = baseColor;
 
    [TextureResolution: "128"]
    [Storage:"RGB8"]
	vec3 Normal
    {
        vec3 ground_rock_n = texture(ground_pebble_Nmap, vert_pos.xz * groundRockTiling).xyz;
        vec3 rockGrassN = mix(rock.Normal, grass.Normal, rockGrass_mixFactor);
        vec3 terrainBlendN = mix(rockGrassN, soil.Normal, ground_soild_mixFactor);
        vec3 terrainSnowBlendN = mix(terrainBlendN, snow.Normal, snow_mixFactor);
        return mix(ground_rock_n, terrainSnowBlendN, ground_rock_mixFactor);
    }
	
    [Storage:"R8"]
    float Roughness
    {
        float rockGrassR = mix(rock.Roughness, grass.Roughness, rockGrass_mixFactor);
        float terrainBlendR = mix(rockGrassR, soil.Roughness, ground_soild_mixFactor);
        float terrainSnowBlendR = mix(terrainBlendR, snow.Roughness, snow_mixFactor);
        float ground_rock_val = 0.3;
        return mix(ground_rock_val, terrainSnowBlendR, ground_rock_mixFactor);
    }
    
    [Storage:"R8"]
    float Specular
    {
        float ground_rock_val = 1.0;
        float rockGrassR = mix(rock.Specular, grass.Specular, rockGrass_mixFactor);
        float terrainBlendR = mix(rockGrassR, soil.Specular, ground_soild_mixFactor);
        float terrainSnowBlendR = mix(terrainBlendR, snow.Specular, snow_mixFactor);
        return mix(ground_rock_val, terrainSnowBlendR, ground_rock_mixFactor);
    }
    
    [Storage:"R8"]
    float Metallic
    {
        float ground_rock_val = 0.2;
        float rockGrassR = mix(rock.Metallic, grass.Metallic, rockGrass_mixFactor);
        float terrainBlendR = mix(rockGrassR, soil.Metallic, ground_soild_mixFactor);
        float terrainSnowBlendR = mix(terrainBlendR, snow.Metallic, snow_mixFactor);
        return mix(ground_rock_val, terrainSnowBlendR, ground_rock_mixFactor);
    }
    
    @lqfs vec3 Normal = vec3(0.0, 0.0, 1.0);
    @lqfs float Roughness = 0.7;
    @lqfs float Metallic = 0.3;
    @lqfs float Specular = 0.4;
    using Footer;
}