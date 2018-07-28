let addon = require('../node_natives/build/Release/addon.node');
let path = require('path');
const resPanel = require('./respanel.js');

window.onload = function(){

  let canvas = bid('canvas');
  canvas.width = 854;
  canvas.height = 480;
  let context = canvas.getContext('2d');

  let rootPath = path.dirname(__dirname);

  console.log( addon.hello() );

  addon.setRootPath(rootPath);
 
  console.log(rootPath);

  //return;
  addon.initrender();

  let bufferData = new Buffer(854*480*4);
  let imageData = context.createImageData(854, 480);

  let profiledata = bid('profiledata');

  // temporary feature
  let holder = bid('mesh-holder');

  holder.ondrop = function ( ev ) {
    ev.preventDefault();
    let filetoken = ev.dataTransfer.getData("restoken");
    //console.info(filetoken);
    let event = "set_model";
    let ret = addon.sendEvent(event, filetoken);
    //console.info(ret);
  }

  holder.ondragover = function (ev) {
    ev.preventDefault();
}

  // respanel refresh
  resPanel.init(bid('res-container'), path.join(__dirname, '../'));
  resPanel.rescan_resources();
  resPanel.reconstruct_filetree();
  resPanel.refresh();

  // create render loop
  render_loop();
  function render_loop()
  {
      addon.rendertobuffer(bufferData);
      let data = addon.getprofiledata();
      
      profiledata.innerHTML=data;

      imageData.data.set(bufferData);

      context.putImageData(imageData, 0, 0);

      requestAnimationFrame(render_loop);
  }

  //addon.shutdownrender();
};

function bid(id){return document.getElementById(id);}

