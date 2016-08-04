using "../MultiRate.pipeline";

shader Ocean
{
    using Header;
    
    inline vec3 view = normalize(cameraPos - pos);
    // ray 
    inline vec3 ori = cameraPos;
    inline vec3 dir = -view;
    inline int iterGeom : lq = 0;
    inline int iterGeom : hq = 3;
    inline int iterDetail :lq = 2;
    inline int iterDetail : hq = 5;
   
    // tracing
    vec3 p
    {
        int NUM_STEPS = 8;
        vec3 rs;
        float tm = 0.0;
        float tx = 2000.0;    
        float hx = map(ori + dir * tx, time, iterGeom);
        if(hx > 0.0) 
            rs = ori + dir * tx;
        else
        {
            float hm = map(ori + dir * tm, time, iterGeom);    
            float tmid = 0.0;
            for(int i = 0:1:NUM_STEPS) {
                tmid = mix(tm,tx, hm/(hm-hx));                   
                rs = ori + dir * tmid;                   
                float hmid = map(rs, time, iterGeom);
                if(hmid < 0.0) {
                    tx = tmid;
                    hx = hmid;
                } else {
                    tm = tmid;
                    hm = hmid;
                }
            }
        }
        return rs;
    }

    vec3 dist = p - ori;
    float EPSILON_NRM	= 0.00005;
    [TextureResolution:"2048"]    
    vec3 n = getNormal(p, dot(dist,dist) * EPSILON_NRM, time, iterDetail);
    vec3 light = normalize(vec3(0.0,1.0,0.8)); 
             
    // color
    vec3 color = mix(
        getSkyColor(dir),
        getSeaColor(p,n,light,dir,dist),
    	pow(smoothstep(0.0,-0.05,dir.y),0.3));
      
    vec3 Normal = n;
    [TextureResolution:"1024"]
    vec4 waterColor = vec4(pow(color,vec3(0.75)), 1.0);
    vec4 terrainProjPos = viewProjectionMatrix * vec4(p, 1.0);
    [DepthOutput]
    out @fs float depthOut = (terrainProjPos.z/terrainProjPos.w)*0.5 + 0.5;
    float opacity = 1.0;
    vec4 outputColor = waterColor;//vec4(length(p-cameraPos)*0.011, p.y*0.7,0.0,1.0);//vec4(n.xzy*0.5 + 0.5, 1.0);//waterColor;
    
}

float hash(vec2 p) {
	float h = dot(p,vec2(127.1,311.7));	
    return fract(sin(h)*43758.5453123);
}
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);	
    vec2 u = f*f*(vec2(3.0)-f*2.0);
    return -1.0+2.0*mix( mix( hash( i + vec2(0.0,0.0) ), 
                     hash( i + vec2(1.0,0.0) ), u.x),
                mix( hash( i + vec2(0.0,1.0) ), 
                     hash( i + vec2(1.0,1.0) ), u.x), u.y);
}

// lighting
float diffuse(vec3 n,vec3 l,float p) {
    return pow(dot(n,l) * 0.4 + 0.6,p);
}
float specular(vec3 n,vec3 l,vec3 e,float s) {    
    float nrm = (s + 8.0) / (3.1415 * 8.0);
    return pow(max(dot(reflect(e,n),l),0.0),s) * nrm;
}

// sky
vec3 getSkyColor(vec3 e) {
    e.y = max(e.y,0.0);
    vec3 ret;
    ret.x = pow(1.0-e.y,2.0);
    ret.y = 1.0-e.y;
    ret.z = 0.6+(1.0-e.y)*0.4;
    return ret;
}

// sea
float sea_octave(vec2 uv, float choppy) {
    uv += vec2(noise(uv));        
    vec2 wv = vec2(1.0)-abs(sin(uv));
    vec2 swv = abs(cos(uv));    
    wv = mix(wv,swv,wv);
    return pow(1.0-pow(wv.x * wv.y,0.65),choppy);
}

float map(vec3 p, float seaTime, int iterGeom) {
    float SEA_FREQ = 0.16;
    float SEA_HEIGHT = 0.6;
    float SEA_CHOPPY = 4.0;
    int ITER_GEOMETRY = iterGeom; 
    
    float freq = SEA_FREQ;
    float amp = SEA_HEIGHT;
    float choppy = SEA_CHOPPY;
    vec2 uv = p.xz; uv.x *= 0.75;
    vec2 octave_m1 = vec2(1.6,1.2);
    vec2 octave_m2 = vec2(-1.2,1.6);
    float d, h = 0.0;    
    for(int i = 0:1:ITER_GEOMETRY) {        
    	d = sea_octave((uv+vec2(seaTime))*freq,choppy);
    	d += sea_octave((uv-vec2(seaTime))*freq,choppy);
        h += d * amp;  
        uv = vec2(dot(octave_m1, uv), dot(octave_m2, uv));      
    	freq *= 1.9; amp *= 0.22;
        choppy = mix(choppy,1.0,0.2);
    }
    return p.y - h;
}

float map_detailed(vec3 p, float seaTime, int iterDetail) {
    float SEA_FREQ = 0.16;
    float SEA_HEIGHT = 0.6;
    float SEA_CHOPPY = 4.0;
    int ITER_FRAGMENT = iterDetail;
    
    float freq = SEA_FREQ;
    float amp = SEA_HEIGHT;
    float choppy = SEA_CHOPPY;
    vec2 uv = p.xz; uv.x *= 0.75;
    float d, h = 0.0;    
    vec2 octave_m1 = vec2(1.6,1.2);
    vec2 octave_m2 = vec2(-1.2,1.6);
    for(int i = 0:1:ITER_FRAGMENT) {        
    	d = sea_octave((uv+vec2(seaTime))*freq,choppy);
    	d += sea_octave((uv-vec2(seaTime))*freq,choppy);
        h += d * amp;        
        uv = vec2(dot(octave_m1, uv), dot(octave_m2, uv));    
        freq *= 1.9; amp *= 0.22;
        choppy = mix(choppy,1.0,0.2);
    }
    return p.y - h;
}

vec3 getSeaColor(vec3 p, vec3 n, vec3 l, vec3 eye, vec3 dist) {  
    vec3 SEA_WATER_COLOR = vec3(0.8,0.9,0.6);
    vec3 SEA_BASE = vec3(0.1,0.19,0.22);
    float SEA_HEIGHT = 0.6;
    
    float fresnel = 1.0 - max(dot(n,-eye),0.0);
    fresnel = pow(fresnel,3.0) * 0.65;
        
    vec3 reflected = getSkyColor(reflect(eye,n));    
    vec3 refracted = SEA_BASE + SEA_WATER_COLOR * diffuse(n,l,80.0) * 0.12; 
    
    vec3 color = mix(refracted,reflected,fresnel);
    
    float atten = max(1.0 - dot(dist,dist) * 0.001, 0.0);
    color += SEA_WATER_COLOR * (p.y - SEA_HEIGHT) * 0.18 * atten;
    
    color += vec3(specular(n,l,eye,60.0));
    
    return color;
}

// tracing
vec3 getNormal(vec3 p, float eps, float seaTime, int iterDetail) {
    vec3 n;
    n.y = map_detailed(p, seaTime, iterDetail);    
    n.x = map_detailed(vec3(p.x+eps,p.y,p.z), seaTime, iterDetail) - n.y;
    n.z = map_detailed(vec3(p.x,p.y,p.z+eps), seaTime, iterDetail) - n.y;
    n.y = eps;
    return normalize(n);
}