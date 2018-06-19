#include <nan.h>

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

// function to simply return a static value from c++
NAN_METHOD(getcounter) {
    info.GetReturnValue().Set(Nan::New(s_counter));
}

// export funcs
NAN_MODULE_INIT(Init) {
   Nan::Set(target, New<String>("rotate").ToLocalChecked(),
        GetFunction(New<FunctionTemplate>(rotate)).ToLocalChecked());
   Nan::Set(target, New<String>("hello").ToLocalChecked(),
        GetFunction(New<FunctionTemplate>(helloworld)).ToLocalChecked());
   Nan::Set(target, New<String>("getcounter").ToLocalChecked(),
        GetFunction(New<FunctionTemplate>(getcounter)).ToLocalChecked());
}

NODE_MODULE(addon, Init)

}

