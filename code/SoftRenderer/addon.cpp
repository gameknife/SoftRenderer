#include <nan.h>

#include "stdafx.h"
#include "pathutil.h"
#include "SoftRenderApp.h"
#include "SrModelViewerApp.h"
#include "SrProfiler.h"

namespace demo {

using namespace Nan;
using namespace v8;

static int s_counter = 0;

// function to change buffer from js
NAN_METHOD(rotate) {
    char* buffer = (char*) node::Buffer::Data(info[0]->ToObject());
    unsigned int size = info[1]->Uint32Value();
    unsigned int rot = info[2]->Uint32Value();
   
    for(unsigned int i = 0; i < size; i++ ) {
        buffer[i] += rot;
    }

    s_counter++;
}

// function simply return a string from c++
NAN_METHOD(helloworld) {
    info.GetReturnValue().Set(Nan::New("world").ToLocalChecked());
    s_counter++;
}

NAN_METHOD(getprofiledata) {
	char* str = gEnv->profiler->getProfileData();
	info.GetReturnValue().Set(Nan::New(str).ToLocalChecked());
}

// function to simply return a static value from c++
NAN_METHOD(getcounter) {
    info.GetReturnValue().Set(Nan::New(s_counter));
}

SoftRenderApp* g_app;

NAN_METHOD(initOfflineSystem) {
	g_app = new SoftRenderApp();
	g_app->RegisterTask(new SrModelViewerApp);
	g_app->Init();
}

NAN_METHOD(renderToBuffer) {

	g_app->Update();

	const uint8* data = gEnv->renderer->getBuffer();
	int length = gEnv->renderer->getBufferLength();

	char* buffer = (char*)node::Buffer::Data(info[0]->ToObject());

	memcpy(buffer, data, length);
}

NAN_METHOD(shutDown){
	g_app->Destroy();
}

NAN_METHOD(setRootPath) 
{
	const char* str = *(String::Utf8Value(info[0]->ToString()));
	srSetRootPath(str);
}

NAN_METHOD(sendEvent)
{
    if(info.Length() != 2)
    {
        return;
    }

    Nan::Utf8String utf8_value0(info[0]);
    Nan::Utf8String utf8_value1(info[1]);

    const char* event = *utf8_value0;
    const char* param = *utf8_value1;

    g_app->SendEvent(event, param);
}

Nan::Callback* cbGlobal;
void LogToElectron(const char* line)
{
    const unsigned argc = 1;
    v8::Local<v8::Value> argv[argc] = { Nan::New(line).ToLocalChecked() };
    cbGlobal->Call(argc, argv);
}

NAN_METHOD(setLogCallback)
{
    cbGlobal = new Nan::Callback(info[0].As<v8::Function>());
    gEnv->logger->SetExternalLogCallback(LogToElectron);

    const unsigned argc = 1;
    v8::Local<v8::Value> argv[argc] = { Nan::New("callback setup to C++").ToLocalChecked() };
    cbGlobal->Call(argc, argv);
}

// export funcs
NAN_MODULE_INIT(Init) {    
   Nan::Set(target, New<String>("rotate").ToLocalChecked(),
        GetFunction(New<FunctionTemplate>(rotate)).ToLocalChecked());
   Nan::Set(target, New<String>("hello").ToLocalChecked(),
        GetFunction(New<FunctionTemplate>(helloworld)).ToLocalChecked());
   Nan::Set(target, New<String>("getcounter").ToLocalChecked(),
        GetFunction(New<FunctionTemplate>(getcounter)).ToLocalChecked());

   Nan::Set(target, New<String>("setRootPath").ToLocalChecked(),
	   GetFunction(New<FunctionTemplate>(setRootPath)).ToLocalChecked());
   Nan::Set(target, New<String>("initrender").ToLocalChecked(),
	   GetFunction(New<FunctionTemplate>(initOfflineSystem)).ToLocalChecked());
   Nan::Set(target, New<String>("rendertobuffer").ToLocalChecked(),
	   GetFunction(New<FunctionTemplate>(renderToBuffer)).ToLocalChecked());
   Nan::Set(target, New<String>("shutdownrender").ToLocalChecked(),
	   GetFunction(New<FunctionTemplate>(shutDown)).ToLocalChecked());
   Nan::Set(target, New<String>("getprofiledata").ToLocalChecked(),
	   GetFunction(New<FunctionTemplate>(getprofiledata)).ToLocalChecked());
   Nan::Set(target, New<String>("sendEvent").ToLocalChecked(),
	   GetFunction(New<FunctionTemplate>(sendEvent)).ToLocalChecked());
   Nan::Set(target, New<String>("setLogCallback").ToLocalChecked(),
	   GetFunction(New<FunctionTemplate>(setLogCallback)).ToLocalChecked());
}

NODE_MODULE(addon, Init)

}

