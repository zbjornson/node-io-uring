#include "nan.h"
#include "v8.h"
#include <node_buffer.h>
#include <stdint.h>
#include "liburing/src/liburing.h"

static io_uring ring;
// idle seems to be the only one that pumps the event loop; prepare and check do
// not. https://github.com/libuv/libuv/issues/2022
// http://docs.libuv.org/en/v1.x/guide/utilities.html#idler-pattern
// http://docs.libuv.org/en/v1.x/guide/utilities.html#check-prepare-watchers (no docs)
// http://docs.libuv.org/en/v1.x/idle.html
static uv_idle_t check;
static int pending = 0;

// TODO does this need to be an AsyncResource? 
class Request : public Nan::AsyncResource {
  public:
  Request(v8::Local<v8::Function> cb, v8::Local<v8::Object> _output) : AsyncResource("iouring") {
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

void DoCheck(uv_idle_t* handle) {
  while (true) { // get all available completions
    io_uring_cqe* cqe;
    // Per source, this cannot return an error. (That's good because we have no
    // particular callback to invoke with an error.)
    io_uring_get_completion(&ring, &cqe);

    if (!cqe) return;

    pending--;
    if (!pending) {
      int ret = uv_idle_stop(&check);

    }

    Request* req = (Request*)(void*)(cqe->user_data);

    if (cqe->res < 0) {
      Nan::HandleScope scope;
      v8::Local<v8::Value> argv[1] = { Nan::ErrnoException(-cqe->res) };
      req->callback.Call(1, argv, req);
    } else {
      Nan::HandleScope scope;
      v8::Local<v8::Value> argv[3] =
          { Nan::Null(), Nan::New(cqe->res), Nan::New(req->output) };
      req->callback.Call(3, argv, req);
    }

    delete req;
  }
}

/**
 * Matches node_file.cc Read, with checks omitted for brevity.
 * 0 fd       int32
 * 1 buffer   Buffer   destination
 * 2 offset   int64    offset into buffer
 * 3 length   int32    num bytes to read
 * 4 position int64    position in file, [TODO -1 for current position - this should update file posn]
 * 5 callback Function (err, bytesRead, buffer)
 */
NAN_METHOD(read) {
  int32_t fd = Nan::To<int32_t>(info[0]).FromJust();
  v8::Local<v8::Object> buffer_obj = info[1].As<v8::Object>();
  char* buffer_data = node::Buffer::Data(buffer_obj);
  const size_t off = info[2].As<v8::Integer>()->Value();
  const size_t len = static_cast<size_t>(info[3].As<v8::Int32>()->Value());
  const int64_t pos = info[4].As<v8::Integer>()->Value();
  v8::Local<v8::Function> cb = Nan::To<v8::Function>(info[5]).ToLocalChecked();

  Request* req = new Request(cb, buffer_obj);

  iovec* iov = new iovec(); // TODO is this deleted by the kernel?
  iov->iov_base = buffer_data + off;
  iov->iov_len = len;
  io_uring_sqe* sqe = io_uring_get_sqe(&ring);
  io_uring_prep_readv(sqe, fd, iov, 1, pos);
  io_uring_sqe_set_data(sqe, req);

  int ret = io_uring_submit(&ring);
  if (ret < 0) {
    // TODO this needs to be in the next tick
    v8::Local<v8::Value> argv[1] = { Nan::ErrnoException(-ret) };
    Nan::Call(cb, Nan::GetCurrentContext()->Global(), 1, argv);
    return;
  }

  if (!pending) {
    uv_idle_start(&check, DoCheck);
  }
  pending++;
}

/**
 * Matches node_file.cc WriteBuffer, with checks omitted for brevity.
 * 0 fd       int32
 * 1 buffer   Buffer   source
 * 2 offset   int64    offset into buffer
 * 3 length   int32    num bytes to write
 * 4 position int64    position in file [TODO null for current position]
 * 5 callback Function (err, bytesWritten, buffer)
 */
NAN_METHOD(writeBuffer) {
  int32_t fd = Nan::To<int32_t>(info[0]).FromJust();
  v8::Local<v8::Object> buffer_obj = info[1].As<v8::Object>();
  char* buffer_data = node::Buffer::Data(buffer_obj);
  const size_t off = info[2].As<v8::Integer>()->Value();
  const size_t len = static_cast<size_t>(info[3].As<v8::Int32>()->Value());
  const int64_t pos = info[4].As<v8::Integer>()->Value();
  v8::Local<v8::Function> cb = Nan::To<v8::Function>(info[5]).ToLocalChecked();

  Request* req = new Request(cb, buffer_obj);

  iovec* iov = new iovec(); // TODO is this deleted by the kernel?
  iov->iov_base = buffer_data + off;
  iov->iov_len = len;
  io_uring_sqe* sqe = io_uring_get_sqe(&ring);
  io_uring_prep_writev(sqe, fd, iov, 1, pos);
  io_uring_sqe_set_data(sqe, req);

  int ret = io_uring_submit(&ring);
  if (ret < 0) {
    // TODO this needs to be in the next tick
    v8::Local<v8::Value> argv[1] = { Nan::ErrnoException(-ret) };
    Nan::Call(cb, Nan::GetCurrentContext()->Global(), 1, argv);
    return;
  }

  if (!pending) {
    uv_idle_start(&check, DoCheck);
  }
  pending++;
}

NAN_MODULE_INIT(Init) {
  // TODO fixed limit here
  int ret = io_uring_queue_init(32, &ring, 0);
  if (ret < 0) {
    fprintf(stderr, "queue_init: %s\n", strerror(-ret));
  }

  ret = uv_idle_init(Nan::GetCurrentEventLoop(), &check);
  if (ret) {
    fprintf(stderr, "uv_idle_init: %d\n", ret);
  }

  NAN_EXPORT(target, read);
  NAN_EXPORT(target, writeBuffer);
}

NODE_MODULE(iou, Init);


