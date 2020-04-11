import * as addon from '../node_natives';
import * as path from 'path';

// This file is required by the index.html file and will
// be executed in the renderer process for that window.
// All of the Node.js APIs are available in this process.
window.onload = () => {
  
    
  // 初始化canvas
  let canvas = bid('canvas') as HTMLCanvasElement;
  canvas.width = 854;
  canvas.height = 480;
  let context = canvas.getContext('2d');

console.log(`starting...`);
  // 设置资源路径
  let rootPath = path.dirname(__dirname);
  addon.setRootPath(rootPath);
  console.log("root: " + rootPath);
  
  // 初始化渲染器
  addon.initrender(canvas.width,canvas.height);

  bid('canvas-title').textContent = `CPU | ${canvas.width} x ${canvas.height} AA | 100%`;


  //Logger.init(bid('console-log-container'));

  //addon.setLogCallback( function(msg) {
    //Logger.info(msg);
  //} );



  // 创建backbuffer
  let bufferData = Buffer.alloc( canvas.width*canvas.height*4);
  let imageData = context.createImageData(canvas.width, canvas.height);



  // 取得profile数据显示器
  let profiledata = bid('profiledata');

  // meshfilter容器逻辑
  let holder = bid('mesh-holder');
  holder.ondrop = function ( ev ) {
    ev.preventDefault();
    let filetoken = ev.dataTransfer.getData("restoken");

    // 拖拽到meshfilter时，向渲染引擎发送设置模型的消息
    let event = "set_model";
    let ret = addon.sendEvent(event, filetoken);
    console.info(`${event} - ${filetoken}`);
  }
  holder.ondragover = function (ev) {
    ev.preventDefault();
  }

//   let resPanel = new EditorResourceManager();

//   // 刷新project面板
//   resPanel.init(bid('res-container'), path.join(__dirname, '../'));
//   resPanel.rescan_resources();
//   resPanel.reconstruct_filetree();
//   resPanel.refresh();

  // 默认打开sponza
  let event = "set_model";
  let ret = addon.sendEvent(event, 'media/sponza.obj');

  // 创建逻辑循环
  render_loop();
  function render_loop()
  {
      // 渲染引擎更新
      addon.rendertobuffer(bufferData);
      // 更新到html
      imageData.data.set(bufferData);
      context.putImageData(imageData, 0, 0);


      // 取得渲染引擎profile数据
      let data = addon.getprofiledata();
      // 设置profile数据
      profiledata.innerHTML=data;

      // 申请下一次循环
      requestAnimationFrame(render_loop);
  }

};

function bid(id){return document.getElementById(id);}