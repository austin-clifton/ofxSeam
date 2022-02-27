#version 410

uniform uvec2 resolution = uvec2(1920, 1080);

// The total number of shapes to draw on the screen in each dimension
uniform uvec2 totalShapes = uvec2(50, 50);

uniform float time = 0.f;

uniform float lineForceFactor = 8.f;
uniform float shapeSize = 0.0025;

out vec4 col;

void main() {
    vec2 aspectRatio = vec2(resolution) / max(resolution.x, resolution.y);
    vec2 fragCoord = vec2( gl_FragCoord.x / resolution.x, gl_FragCoord.y / resolution.y );

    uvec2 shapesDiv = totalShapes + 1;
    float spaceBetweenShapes = 1.f / shapesDiv.y;
    float maxForce = spaceBetweenShapes * lineForceFactor;

    // find the closest shape to this pixel
    vec2 closestCenter = round(fragCoord * shapesDiv) / shapesDiv;

    // sample this many shapes around the closest shape
    const int samples = 10;
    col = vec4(vec3(0.0), 1.0);
    // calculate the center of each shape near the closest shape with an offset of [-samples, samples]
    for (int yOff = -samples; yOff <= samples; yOff++) {
        // the center of the shape we are sampling, BEFORE offsets
        vec2 center = closestCenter + vec2(0.f, yOff * spaceBetweenShapes);

        // define a line which pushes things around
        float lineY = 0.5 + sin(center.x * 10.f) * 0.5;

        // are we above or below the line, and what's the exact distance?
        // we want both the sign and the distance here, to determine push force
        float distToLine = center.y - lineY;

        // calculate the offset for this center point
        float pushForceY = maxForce 
            * sign(distToLine) 
            * (1.0 - smoothstep(0.0, spaceBetweenShapes * lineForceFactor * 2.f, abs(distToLine) ) );

        // move the shape's center
        center.y = center.y + pushForceY * (sin(time * 0.1f));
        
        // calculate frag coord distance from shape center
        float dist = distance(fragCoord * aspectRatio, center * aspectRatio);
        // smooth it out
        dist = 1.0 - smoothstep(shapeSize * 0.5, shapeSize, dist);

        col.rgb += dist;
    }
}