#include "nan.h"
#include "v8.h"
#include <node_buffer.h>
#include <stdint.h>
#include "liburing.h"

static io_uring ring;
static uv_check_t check;
static int pending = 0;

class Request {
  public:
  Request(v8::Local<v8::Function> cb, v8::Local<v8::Object> _output) {
    callback.Reset(cb);
    output.Reset(_output);
  }
  ~Request() {
    callback.Reset();
    output.Reset();
  }
  Nan::Callback callback;
  Nan::Persistent<v8::Object> output;
};

void DoCheck(uv_check_t* handle) {
  io_uring_cqe* cqe;
  int ret = io_uring_get_completion(&ring, &cqe);
  if (ret < 0) {
    // What should we do here? We're not in one particular request's context.
    fprintf(stderr, "io_uring_get_completion: %s\n", strerror(-ret));
  }

  if (!cqe) return;

  pending--;
  if (!pending) {
    uv_check_stop(&check);
  }

  Request* req = (Request*)(void*)(cqe->user_data);

  if (cqe->res < 0) {
    Nan::HandleScope scope;
    v8::Local<v8::Value> argv[1] = { Nan::ErrnoException(-cqe->res) };
    req->callback.Call(1, argv);
  } else {
    Nan::HandleScope scope;
    v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::New(req->output) };
    req->callback.Call(2, argv);
  }

  delete req;
}

NAN_METHOD(readFile) {
  int32_t fd = Nan::To<int32_t>(info[0]).FromJust();
  uint32_t len = Nan::To<uint32_t>(info[1]).FromJust();
  v8::Local<v8::Function> cb = Nan::To<v8::Function>(info[2]).ToLocalChecked();
  v8::Local<v8::Object> output = node::Buffer::New(v8::Isolate::GetCurrent(), len).ToLocalChecked();

  Request* req = new Request(cb, output);

  iovec* iov = new iovec(); // TODO is this deleted by the kernel?
  iov->iov_base = node::Buffer::Data(output);
  iov->iov_len = len;
  io_uring_sqe* sqe = io_uring_get_sqe(&ring);
  io_uring_prep_readv(sqe, fd, iov, 1, /*off*/0);
  io_uring_sqe_set_data(sqe, req);

  int ret = io_uring_submit(&ring);
  if (ret < 0) {
    Nan::ThrowError(Nan::ErrnoException(-ret, "io_uring_submit"));
    return;
  }

  if (!pending) {
    uv_check_start(&check, DoCheck);
  }
  pending++;
}

NAN_MODULE_INIT(Init) {
  // TODO fixed limit here
  int ret = io_uring_queue_init(32, &ring, 0);
  if (ret < 0) {
    fprintf(stderr, "queue_init: %s\n", strerror(-ret));
  }

  ret = uv_check_init(Nan::GetCurrentEventLoop(), &check);
  if (ret) {
    fprintf(stderr, "uv_check_init: %d\n", ret);
  }

  NAN_EXPORT(target, readFile);
}

NODE_MODULE(iou, Init);
