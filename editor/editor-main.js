let addon = require('../node_natives/build/Release/addon.node');

window.onload = function(){

  // console.log('it works');

  // let buffer = Buffer.from("ABC");

  // console.log('func call time: ' + addon.getcounter()); // call c++ method
  // addon.rotate(buffer, buffer.length, 32);
  // console.log(buffer.toString('ascii'));
  // console.log('func call time: ' + addon.getcounter()); // call c++ method
  // console.log(addon.hello()); // call c++ method
  // console.log('func call time: ' + addon.getcounter()); // call c++ method
  // console.log(addon.hello()); // call c++ method
  // console.log('func call time: ' + addon.getcounter()); // call c++ method

  // gl init
  let canvas = bid('canvas');
  //let main3dcanvas = document.createElement('canvas');

  canvas.width = 854;
  canvas.height = 480;
  let context = canvas.getContext('2d');


  addon.initrender();

  let bufferData = new Buffer(854*480*4);
  let imageData = context.createImageData(854, 480);
  // create render loop
  render_loop();
  function render_loop()
  {
      addon.rendertobuffer(bufferData);
      imageData.data.set(bufferData);

      context.putImageData(imageData, 0, 0);

      requestAnimationFrame(render_loop);
  }

  //data[0] = 255;
    // Copy the pixels to a 2D canvas
    //var imageData = context.createImageData(854, 480);
    //imageData.data.set(data);

    //context.putImageData(imageData, 0, 0);

  //addon.shutdownrender();
};

function bid(id){return document.getElementById(id);}

