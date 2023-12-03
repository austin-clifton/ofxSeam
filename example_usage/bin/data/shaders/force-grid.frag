#version 410

uniform uvec2 resolution = uvec2(1920, 1080);

// The total number of shapes to draw on the screen in each dimension
uniform uvec2 totalShapes = uvec2(100, 100);

uniform float time = 0.f;

uniform float lineForceFactor = 5.f;
uniform float shapeSize = 0.001;

uniform float noiseFactor = 0.1f;

out vec4 col;

// simplex noise from:
// https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
//	Simplex 3D Noise 
//	by Ian McEwan, Ashima Arts
//
vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}
vec4 taylorInvSqrt(vec4 r){return 1.79284291400159 - 0.85373472095314 * r;}

float snoise(vec3 v){ 
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

// First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

// Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  //  x0 = x0 - 0. + 0.0 * C 
  vec3 x1 = x0 - i1 + 1.0 * C.xxx;
  vec3 x2 = x0 - i2 + 2.0 * C.xxx;
  vec3 x3 = x0 - 1. + 3.0 * C.xxx;

// Permutations
  i = mod(i, 289.0 ); 
  vec4 p = permute( permute( permute( 
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

// Gradients
// ( N*N points uniformly over a square, mapped onto an octahedron.)
  float n_ = 1.0/7.0; // N=7
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z *ns.z);  //  mod(p,N*N)

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

//Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

// Mix final noise value
  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), 
                                dot(p2,x2), dot(p3,x3) ) );
}


void main() {
    vec2 aspectRatio = vec2(resolution) / max(resolution.x, resolution.y);
    vec2 fragCoord = vec2( gl_FragCoord.x / resolution.x, gl_FragCoord.y / resolution.y );

    uvec2 shapesDiv = totalShapes + 1;
    vec2 spaceBetweenShapes = 1.f / shapesDiv;
    vec2 maxForce = spaceBetweenShapes * lineForceFactor;

    // find the closest shape to this pixel
    vec2 closestCenter = round(fragCoord * shapesDiv) / shapesDiv;

    // sample this many shapes around the closest shape
    const int samples = 5;
    col = vec4(vec3(0.0), 1.0);

    // TEMP
    vec2 forcesCenter = vec2(0.5);
    float r = 0.25;
    float h = 0.15;

    const int totalPoints = 4;

    vec2 forcePoints[totalPoints];
    for (int i = 0; i < totalPoints; i++) {
        float ti = time + i * 8.34;
        vec2 radialOffset = vec2(cos(ti), sin(ti));
        float noiseOffset = snoise(vec3(r * radialOffset.x, time, r * radialOffset.y)) * noiseFactor;
        forcePoints[i] = forcesCenter + (r + noiseOffset) * radialOffset;

        // float noiseOffset = snoise(vec3(forceOffset.x, time, forceOffset.y)) * noiseFactor;;
        // vec2 forceOffset = (r + noiseOffset) * vec2(cos(ti), sin(ti));
        // forcePoints[i] = forcesCenter + forceOffset;
    }
    
    for (int xOff = -samples; xOff <= samples; xOff++) {
        float centerX = closestCenter.x + xOff * spaceBetweenShapes.x;

        for (int yOff = -samples; yOff <= samples; yOff++) {
            vec2 ogCenter = vec2(centerX, closestCenter.y + yOff * spaceBetweenShapes.y);

            for (int i = 0; i < totalPoints; i++) {
                vec2 center = ogCenter;

                vec2 forcePoint = forcePoints[i];

                // calculate distance from center point to forcePoint
                float dist = distance(ogCenter, forcePoint);

                // also calculate the normalized direction from center point to force point
                vec2 dirToForce = normalize(ogCenter - forcePoint);

                // calculate the force which the forcePoint applies to this center point
                float distFactor = 1.0 - smoothstep(
                    0.f,
                    length(spaceBetweenShapes) * lineForceFactor * 2.f,
                    dist
                );
                distFactor = max(0.3, distFactor);
                vec2 force = maxForce * dirToForce * distFactor;

                center += force;

                // calculate the final distance from frag coord to the center of the shape
                float distToCoord = distance(fragCoord * aspectRatio, center * aspectRatio);
                // smooth it out
                distToCoord = 1.0 - smoothstep(shapeSize * 0.5, shapeSize, distToCoord);

                col.rgb += distToCoord;
            }
        }
    }

    /*
    // calculate the center of each shape near the closest shape with an offset of [-samples, samples]
    for (int yOff = -samples; yOff <= samples; yOff++) {
        // the center of the shape we are sampling, BEFORE offsets
        vec2 center = closestCenter + vec2(0.f, yOff * spaceBetweenShapes);

        // define a line which pushes things around
        float lineY = fragCoord.x;

        // are we above or below the line, and what's the exact distance?
        // we want both the sign and the distance here, to determine push force
        float distToLine = center.y - lineY;

        // calculate the offset for this center point
        float pushForceY = maxForce 
            * sign(distToLine) 
            * (1.0 - smoothstep(
                0.f, 
                spaceBetweenShapes * lineForceFactor * 2.f, 
                abs(distToLine) 
            ) 
        );

        // move the shape's center
        center.y = center.y + pushForceY * (0.5 * sin(time * 0.1f) + 0.5);

        // as the shape gets closer to the line, its size approaches zero
        float actualShapeSize = shapeSize * smoothstep(0.f, spaceBetweenShapes, abs(distToLine));
        
        // calculate frag coord distance from shape center
        float dist = distance(fragCoord * aspectRatio, center * aspectRatio);
        // smooth it out
        dist = 1.0 - smoothstep(actualShapeSize * 0.5, actualShapeSize, dist);

        col.rgb += dist;
    }
    */
}