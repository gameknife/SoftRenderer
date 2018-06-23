const math = require('./gl-matrix.js');

var aabb = {};

aabb.create = function () {

    var out = {};

    out.lbb = math.vec3.fromValues(9999999,9999999,9999999);
    out.rtf = math.vec3.fromValues(-9999999,-9999999,-9999999);

    return out;
}

aabb.addPoint = function ( out, _vertex ) {
    out.lbb = math.vec3.min(out.lbb, out.lbb, _vertex);
    out.rtf = math.vec3.max(out.rtf, out.rtf, _vertex);
    return out;
}

aabb.mergeAABB = function ( out, _aabb ) {
    aabb.addPoint( out, _aabb.lbb );
    aabb.addPoint( out, _aabb.rtf );

    return out;
}

aabb.mergeOBB = function ( out, _aabb, obj2worldMatrix ) {
    // extract to 8 point
    let lbb = math.vec3.clone(_aabb.lbb);
    let rtf = math.vec3.clone(_aabb.rtf);
    let ltb = math.vec3.fromValues( lbb[0], rtf[1], lbb[2] );
    let ltf = math.vec3.fromValues( lbb[0], rtf[1], rtf[2] );
    let lbf = math.vec3.fromValues( lbb[0], lbb[1], rtf[2] );

    let rbb = math.vec3.fromValues( rtf[0], lbb[1], lbb[2] );
    let rbf = math.vec3.fromValues( rtf[0], lbb[1], rtf[2] );
    let rtb = math.vec3.fromValues( rtf[0], rtf[1], lbb[2] );

    math.vec3.transformMat4(lbb, lbb, obj2worldMatrix);
    math.vec3.transformMat4(rtf, rtf, obj2worldMatrix);
    math.vec3.transformMat4(ltb, ltb, obj2worldMatrix);
    math.vec3.transformMat4(ltf, ltf, obj2worldMatrix);
    math.vec3.transformMat4(lbf, lbf, obj2worldMatrix);
    math.vec3.transformMat4(rbb, rbb, obj2worldMatrix);
    math.vec3.transformMat4(rbf, rbf, obj2worldMatrix);
    math.vec3.transformMat4(rtb, rtb, obj2worldMatrix);

    aabb.addPoint( out, lbb );
    aabb.addPoint( out, rtf );
    aabb.addPoint( out, ltb );
    aabb.addPoint( out, ltf );
    aabb.addPoint( out, lbf );
    aabb.addPoint( out, rbb );
    aabb.addPoint( out, rbf );
    aabb.addPoint( out, rtb );

    return out;
}

aabb.center = function (out, src) {
    math.vec3.add(out, src.lbb, src.rtf);
    math.vec3.scale(out, out, 0.5);
    return out;
}

aabb.radius = function (src) {
    let distance = math.vec3.sub( math.vec3.create(), src.lbb, src.rtf);
    return math.vec3.length(distance) * 0.5;
}


exports.aabb = aabb;