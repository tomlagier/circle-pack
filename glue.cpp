
#include <emscripten.h>

extern "C" {

// Not using size_t for array indices as the values used by the javascript code are signed.
void array_bounds_check(const int array_size, const int array_idx) {
  if (array_idx < 0 || array_idx >= array_size) {
    EM_ASM_INT({
      throw 'Array index ' + $0 + ' out of bounds: [0,' + $1 + ')';
    }, array_idx, array_size);
  }
}

// Node

Node* EMSCRIPTEN_KEEPALIVE emscripten_bind_Node_Node_0() {
  return new Node();
}

Node* EMSCRIPTEN_KEEPALIVE emscripten_bind_Node_getChildren_0(Node* self) {
  return self->getChildren();
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Node_setChildren_1(Node* self, Node** arg0) {
  self->setChildren(arg0);
}

Node* EMSCRIPTEN_KEEPALIVE emscripten_bind_Node_eachBefore_1(Node* self, BeforeCallback arg0) {
  return self->eachBefore(arg0);
}

void EMSCRIPTEN_KEEPALIVE emscripten_bind_Node___destroy___0(Node* self) {
  delete self;
}

// VoidPtr

void EMSCRIPTEN_KEEPALIVE emscripten_bind_VoidPtr___destroy___0(void** self) {
  delete self;
}

}

