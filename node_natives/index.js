"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
var addon = require('./build/Release/addon.node');

exports.setRootPath = function(rootPath) {
    addon.setRootPath(rootPath);
};
exports.initrender = function( w, h){
    addon.initrender(w,h);
};
exports.sendEvent = function(event, msg){
    addon.sendEvent(event,msg);
};
exports.rendertobuffer = function(buffer){
    addon.rendertobuffer(buffer);
};
exports.getprofiledata = function(){
    return addon.getprofiledata();
};
