let addon = require('../node_natives/build/Release/addon.node');
let path = require('path');

window.onload = function(){

  let canvas = bid('canvas');
  canvas.width = 854;
  canvas.height = 480;
  let context = canvas.getContext('2d');

  let rootPath = path.dirname(__dirname);
  addon.setRootPath(rootPath);
 
  console.log(rootPath);

  //return;
  addon.initrender();

  let bufferData = new Buffer(854*480*4);
  let imageData = context.createImageData(854, 480);

  let profiledata = bid('profiledata');

  // create render loop
  render_loop();
  function render_loop()
  {
      addon.rendertobuffer(bufferData);
      let data = addon.getprofiledata();
      //console.log(data);
      profiledata.innerHTML=data;

      imageData.data.set(bufferData);

      context.putImageData(imageData, 0, 0);

      requestAnimationFrame(render_loop);
  }

  //addon.shutdownrender();
};

function bid(id){return document.getElementById(id);}

