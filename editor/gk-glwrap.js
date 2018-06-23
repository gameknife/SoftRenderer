'use strict';
const math = require('./util/gl-matrix.js');

var glw = glw || {};
module.exports = glw;

// const
glw.VERSION = '0.1';
glw.PI2  = 6.28318530717958647692528676655900576;
glw.PI   = 3.14159265358979323846264338327950288;
glw.PIH  = 1.57079632679489661923132169163975144;
glw.PIH2 = 0.78539816339744830961566084581987572;

glw.VERTEX_LAYOUT_P = 0;
glw.VERTEX_LAYOUT_N = 1;
glw.VERTEX_LAYOUT_T0 = 2;
glw.VERTEX_LAYOUT_T1 = 3;
glw.VERTEX_LAYOUT_C = 4;

// global member
glw.ready = false;
glw.canvas = null;
glw.uicanvas = null;
glw.gl = null;
glw.canvas2d = null;
glw.curr_program = null;
glw.nullTexture = null;
glw.glext_depth_texture = false;

// public interface

// initialize webgl
glw.initGL = function(canvasId, canvasUI, options){
    this.ready = false;
    this.canvas = canvasId;
    this.uicanvas = canvasUI;
    this.gl = null;

    var opt = options || {};
    if(this.canvas == null){return false;}
    this.gl = this.canvas.getContext('webgl', opt)
           || this.canvas.getContext('experimental-webgl', opt);

    //create ui canvas
    this.canvas2d = canvasUI.getContext('2d');

    if(this.gl != null){
        this.ready = true;
    }

    console.info('gl context created.');
    console.info(this.gl.canvas);

    let availableExtensions = this.gl.getSupportedExtensions();
    for (let i = 0; i < availableExtensions.length; i++) {
        if (availableExtensions[i].indexOf('texture') >= 0
            && availableExtensions[i].indexOf('compressed') >= 0) {
            // show in console
            console.log(availableExtensions[i]);
        }
    }


    let checkerData = new Uint8Array(16);
    checkerData[0] = 0;
    checkerData[1] = 0;
    checkerData[2] = 0;
    checkerData[3] = 0;

    glw.nullTexture = glw._create_procedural_texture(2,2,checkerData);
    let ext = this.gl.getExtension("WEBGL_depth_texture");
    glw.glext_depth_texture = (ext !== undefined);

    return this.ready;
};

// glclear
glw.clear = function(color, depth, stencil){
    var gl = this.gl;
    var flg = gl.COLOR_BUFFER_BIT;
    gl.clearColor(color[0], color[1], color[2], color[3]);
    if(depth != null){
        gl.clearDepth(depth);
        flg = flg | gl.DEPTH_BUFFER_BIT;
    }
    if(stencil != null){
        gl.clearStencil(stencil);
        flg = flg | gl.STENCIL_BUFFER_BIT;
    }
    gl.clear(flg);
};

// glviewport
glw.viewport = function(x, y, width, height){
    var X = x || 0;
    var Y = y || 0;
    var w = width  || gkCore.canvas.width;
    var h = height || gkCore.canvas.height;
    this.gl.viewport(X, Y, w, h);
};

glw.set_uniform1f = function(name, fvalue)
{
    if(this.curr_program != null) {
        var loc = this.gl.getUniformLocation(this.curr_program, name);
        if (loc != -1) {
            this.gl.uniform1f(loc, fvalue);
        }
    }
};

glw.set_uniform3f = function (name, fvalue)
{
    //console.info(fvalue);
    if(this.curr_program != null)
    {
        var loc = this.gl.getUniformLocation(this.curr_program, name);
        if (loc != -1) {
            this.gl.uniform3f(loc, fvalue[0], fvalue[1], fvalue[2]);
        }
    }
};

glw.set_uniform4f = function (name, fvalue)
{
    //console.info(fvalue);
    if(this.curr_program != null)
    {
        var loc = this.gl.getUniformLocation(this.curr_program, name);
        if (loc != -1) {
            this.gl.uniform4f(loc, fvalue[0], fvalue[1], fvalue[2], fvalue[3]);
        }
    }
};

glw.set_uniform4x4fv = function(name, fvalue)
{
    if(this.curr_program != null) {
        var loc = this.gl.getUniformLocation(this.curr_program, name);
        if (loc != -1) {
            this.gl.uniformMatrix4fv(loc, false, fvalue);
        }
    }
};


glw.bind_texture = function( texobj, unit )
{
    let targetTO = texobj;
    if(texobj === null) {
        targetTO = glw.nullTexture;
    }

    if(this.curr_program != null)
    {
        var loc = this.gl.getUniformLocation(this.curr_program, '_MainTex');
        if (loc != -1) {
            this.gl.uniform1i(loc, 0);
        }
        loc = this.gl.getUniformLocation(this.curr_program, '_AlphaTex');
        if (loc != -1) {
            this.gl.uniform1i(loc, 1);
        }
        loc = this.gl.getUniformLocation(this.curr_program, '_GlobalNormalMap');
        if (loc != -1) {
            this.gl.uniform1i(loc, 5);
        }
        loc = this.gl.getUniformLocation(this.curr_program, '_GlobalDepthMap');
        if (loc != -1) {
            this.gl.uniform1i(loc, 6);
        }
        loc = this.gl.getUniformLocation(this.curr_program, '_GlobalShadowMap');
        if (loc != -1) {
            this.gl.uniform1i(loc, 7);
        }


    }

    {
        this.gl.activeTexture(33984 + unit);
        this.gl.bindTexture(this.gl.TEXTURE_2D, targetTO);
    }
};

glw.enableAlphaBlend = function () {
    this.gl.enable(this.gl.BLEND);
    //this.gl.depthMask(false);
    this.gl.blendFunc(this.gl.SRC_ALPHA, this.gl.ONE_MINUS_SRC_ALPHA);
}

glw.disableAlphaBlend = function () {
    this.gl.disable(this.gl.BLEND);
    //this.gl.depthMask(true);
}
/// mesh obj
glw.meshObject = function(webglContext){
    this.gl = webglContext;
};

// construct
glw.createMeshObject = function ( res, vertexLayout ) {

    var newObj = new meshObject(this.gl);

    // res comes single one
    let vertexStream = res.vbo;
    let indexStream_list = res.ibo.trilist;
    let indexStream_strip = res.ibo.tristrip;

    newObj.vboForReadback = vertexStream;
    newObj.trilistForReadback = indexStream_list;
    newObj.tristripForReadback = indexStream_strip;

    newObj.vbo = glw._create_vbo(vertexStream);
    if(indexStream_list)
    {
        newObj.trilist = glw._create_ibo(indexStream_list);
        newObj.indexSize_list = indexStream_list.length;
    }
    else
    {
        newObj.indexSize_list = 0;
    }
    if(indexStream_strip)
    {
        newObj.tristrip = glw._create_ibo(indexStream_strip);
        newObj.indexSize_strip = indexStream_strip.length;
    }
    else
    {
        newObj.indexSize_strip = 0;
    }

    var size = 0;
    if(vertexLayout.indexOf(glw.VERTEX_LAYOUT_P) !== -1 )
    {
        size += 3 * 4;
    }

    if(vertexLayout.indexOf(glw.VERTEX_LAYOUT_N) !== -1 )
    {
        size += 3 * 4;
    }

    if(vertexLayout.indexOf(glw.VERTEX_LAYOUT_T0) !== -1 )
    {
        size += 2 * 4;
    }
    newObj.vertexSize = size;
    newObj.vertexLayout = vertexLayout;

    // calc bbox
    // for( let i=0, len = vertexStream.length / size; i < len; ++i)
    // {
    //     let vertexPos = math.vec3.fromValues( vertexStream[i * size + 0], vertexStream[i * size + 1], vertexStream[i * size + 2]);
    //     math.vec3.min( newObj.lbb, newObj.lbb, vertexPos );
    //     math.vec3.max( newObj.rtf, newObj.rtf, vertexPos );
    // }

    return newObj;
};

class meshObject {

    constructor( glcontext ) {
        this.gl = glcontext;
        this.vbo = null;
        this.tristrip = null;
        this.trilist = null;
        this.vertexSize = null;
        this.vertexLayout = null;
        this.indexSize_list = 0;
        this.indexSize_strip = 0;
        this.lbb = math.vec3.fromValues(999999,999999,999999);
        this.rtf = math.vec3.fromValues(-999999,-999999,-999999);

        this.vboForReadback = null;
        this.trilistForReadback = null;
        this.tristripForReadback = null;
    }

    bind() {
        var offset = 0;

        if(this.vertexLayout.indexOf(glw.VERTEX_LAYOUT_P) !== -1 )
        {
            var location = this.gl.getAttribLocation(glw.curr_program, "position");
            if(location !== -1)
            {
                this.gl.enableVertexAttribArray(location);
                this.gl.vertexAttribPointer(location,3,this.gl.FLOAT,false,this.vertexSize,offset);
            }
            offset += 3 * 4;
        }

        if(this.vertexLayout.indexOf(glw.VERTEX_LAYOUT_N) !== -1 )
        {
            var location = this.gl.getAttribLocation(glw.curr_program, "normal");
            if(location !== -1) {
                this.gl.enableVertexAttribArray(location);
                this.gl.vertexAttribPointer(location, 3, this.gl.FLOAT, false, this.vertexSize, offset);
            }
            offset += 3 * 4;
        }

        if(this.vertexLayout.indexOf(glw.VERTEX_LAYOUT_T0) !== -1 )
        {
            var location = this.gl.getAttribLocation(glw.curr_program, "texcoord");
            if(location !== -1) {
                this.gl.enableVertexAttribArray(location);
                this.gl.vertexAttribPointer(location,2,this.gl.FLOAT,false,this.vertexSize,offset);
            }
            offset += 2 * 4;
        }


    }

    draw() {

        this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.vbo);
        this.bind();

        if(this.indexSize_list > 0)
        {
            this.gl.bindBuffer(this.gl.ELEMENT_ARRAY_BUFFER, this.trilist);
            glw._draw_elements(this.gl.TRIANGLES, this.indexSize_list);
        }
        if(this.indexSize_strip > 0)
        {
            this.gl.bindBuffer(this.gl.ELEMENT_ARRAY_BUFFER, this.tristrip);
            glw._draw_elements(this.gl.TRIANGLE_STRIP, this.indexSize_strip);
        }

    }

}
// mesh obj

/// program obj
glw.createProgramObject = function (vsSource, psSource) {

    var newObj = new programObject(this.gl);
    newObj.glprogram = newObj.create(vsSource, psSource);
    if(newObj.glprogram === null)
    {
        return null;
    }
    // analyze uniforms from vs & fs source
    newObj.loc[0] = this.gl.getAttribLocation(newObj.glprogram, "position");
    newObj.loc[1] = this.gl.getAttribLocation(newObj.glprogram, "normal");
    newObj.loc[2] = this.gl.getAttribLocation(newObj.glprogram, "texcoord");

    return newObj;
};

class programObject
{
    constructor(glContext) {
        this.gl = glContext;
        this.glprogram = null;
        this.loc = {};
    }

    create(vsSource, fsSource) {
        var prg = null;
        var vs = glw._create_shader_from_source(vsSource, this.gl.VERTEX_SHADER);
        var fs = glw._create_shader_from_source(fsSource, this.gl.FRAGMENT_SHADER);
        prg = glw._create_program(vs, fs);
        return prg;
    };

    set_uniform( uniform_name, uniform_value )
    {

    };

    use()
    {
        glw._use_program( this.glprogram );
    };
}
// program obj



/// internal interface

// create shader
glw._create_shader_from_source = function(source, type){
    var shader, msg;
    switch(type){
        case this.gl.VERTEX_SHADER:
            shader = this.gl.createShader(this.gl.VERTEX_SHADER);
            break;
        case this.gl.FRAGMENT_SHADER:
            shader = this.gl.createShader(this.gl.FRAGMENT_SHADER);
            break;
        default :
            return;
    }
    this.gl.shaderSource(shader, source);
    this.gl.compileShader(shader);
    if(this.gl.getShaderParameter(shader, this.gl.COMPILE_STATUS)){
        return shader;
    }else{
        msg = this.gl.getShaderInfoLog(shader);
        // directly print to console
        logger.error(msg);
        console.warn('compile failed of shader: ' + msg);
    }
};

// create program
glw._create_program = function(vs, fs){
    var program = this.gl.createProgram();
    this.gl.attachShader(program, vs);
    this.gl.attachShader(program, fs);
    this.gl.linkProgram(program);
    if(this.gl.getProgramParameter(program, this.gl.LINK_STATUS)){
        this.gl.useProgram(program);
        return program;
    }else{
        var msg = this.gl.getProgramInfoLog(program);
        logger.error(msg);
        console.warn('link program failed: ' + msg);
        return null;
    }
};

// use program
glw._use_program = function(prg){
    this.gl.useProgram(prg);
    this.curr_program = prg;
};


// gen & bind vbo data
// TODO: managed later
glw._create_vbo = function(data){
    if(data == null){return;}
    var vbo = this.gl.createBuffer();
    this.gl.bindBuffer(this.gl.ARRAY_BUFFER, vbo);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, new Float32Array(data), this.gl.STATIC_DRAW);
    this.gl.bindBuffer(this.gl.ARRAY_BUFFER, null);
    return vbo;
};

// gen & bind ibo data
// TODO: managed later
glw._create_ibo = function(data){

    if(data == null){return;}

    var ibo = this.gl.createBuffer();

    this.gl.bindBuffer(this.gl.ELEMENT_ARRAY_BUFFER, ibo);
    this.gl.bufferData(this.gl.ELEMENT_ARRAY_BUFFER, new Int16Array(data), this.gl.STATIC_DRAW);
    this.gl.bindBuffer(this.gl.ELEMENT_ARRAY_BUFFER, null);

    return ibo;
};

// gen & bind texture
glw._create_procedural_texture = function(width, height, data){

    let gl = this.gl;
    let fTexture = gl.createTexture();

    gl.bindTexture(gl.TEXTURE_2D, fTexture);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, data);

    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);

    return fTexture;
};


glw._create_bind_texture = function(data){

    let gl = this.gl;
    let buffer = gl.createTexture();

    gl.bindTexture(gl.TEXTURE_2D, buffer);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, data);

    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
    gl.bindTexture(gl.TEXTURE_2D, null);

    return buffer;
};

glw._create_bind_rawtexture = function(width, height, rawdata){

    let gl = this.gl;
    let buffer = gl.createTexture();

    gl.bindTexture(gl.TEXTURE_2D, buffer);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, rawdata);

    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
    gl.bindTexture(gl.TEXTURE_2D, null);

    return buffer;
};

// fbo
glw._create_framebuffer = function(width, height){

    // check & prepare
    if(width == null || height == null){return;}
    var gl = this.gl;

    // create fbo
    var frameBuffer = gl.createFramebuffer();
    gl.bindFramebuffer(gl.FRAMEBUFFER, frameBuffer);

    let depthComponent = null;

    if(glw.glext_depth_texture)
    {
        gkCore.logger.info("[glwarp] bind depth texture");
        var fTexture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, fTexture);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.DEPTH_COMPONENT, width, height, 0, gl.DEPTH_COMPONENT, gl.UNSIGNED_INT, null);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.TEXTURE_2D, fTexture, 0);

        depthComponent = fTexture;
    }
    else
    {
        gkCore.logger.info("[glwarp] bind depth rbuffer");
        // depth by renderbuffer, compitable with all
        var depthRenderBuffer = gl.createRenderbuffer();
        gl.bindRenderbuffer(gl.RENDERBUFFER, depthRenderBuffer);
        gl.renderbufferStorage(gl.RENDERBUFFER, gl.DEPTH_COMPONENT16, width, height);
        gl.framebufferRenderbuffer(gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.RENDERBUFFER, depthRenderBuffer);

        depthComponent = depthRenderBuffer;
    }





    // color by render textrue
    var fTexture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, fTexture);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, fTexture, 0);

    // clear state
    gl.bindTexture(gl.TEXTURE_2D, null);
    gl.bindRenderbuffer(gl.RENDERBUFFER, null);
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);

    return {framebuffer: frameBuffer, depthRenderbuffer: depthComponent, rendertexture: fTexture};
};

// dp
glw._draw_arrays = function(primitive, vertexCount){
    this.gl.drawArrays(primitive, 0, vertexCount);
};

// dip
glw._draw_elements = function(primitive, indexLength){
    this.gl.drawElements(primitive, indexLength, this.gl.UNSIGNED_SHORT, 0);
};
