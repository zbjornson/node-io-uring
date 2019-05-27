#include "nan.h"
#include "v8.h"
#include <node_buffer.h>
#include <stdint.h>
#include "liburing/src/liburing.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <iostream>

static io_uring ring;
static int efd;
static uv_poll_t poller;
static uv_prepare_t preparer;
static unsigned pending = 0;

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

void OnSignal(uv_poll_t* handle, int status, int events) {
  // Reset the eventfd
  char nothing[8];
  int rv = read(efd, &nothing, 8);
  if (rv < 0) { /* impossible? */ }

  while (true) { // Drain the SQ
    io_uring_cqe* cqe;
    // Per source, this cannot return an error. (That's good because we have no
    // particular callback to invoke with an error.)
    io_uring_peek_cqe(&ring, &cqe);

    if (!cqe) return;

    io_uring_cqe_seen(&ring, cqe);

    pending--;
    if (!pending)
      uv_poll_stop(&poller);

    Request* req = static_cast<Request*>(io_uring_cqe_get_data(cqe));

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

void DoSubmit(uv_prepare_t* handle) {
  uv_prepare_stop(handle);
  int ret = io_uring_submit(&ring);
  if (ret < 0) {
    fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
  }
  if (!uv_is_active((uv_handle_t*)&poller))
    uv_poll_start(&poller, UV_READABLE, OnSignal);
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
  // TODO if (!sqe) {} // this returns NULL if buffer is full
  io_uring_prep_readv(sqe, fd, iov, 1, pos);
  io_uring_sqe_set_data(sqe, req);

  if (!uv_is_active((uv_handle_t*)&preparer))
    uv_prepare_start(&preparer, DoSubmit);

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

  if (!uv_is_active((uv_handle_t*)&preparer))
    uv_prepare_start(&preparer, DoSubmit);

  pending++;
}

NAN_MODULE_INIT(Init) {
  // TODO fixed limit here
  int ret = io_uring_queue_init(32, &ring, 0);
  if (ret < 0) {
    fprintf(stderr, "queue_init: %s\n", strerror(-ret));
  }

  efd = eventfd(0, 0);
  ret = io_uring_register(ring.ring_fd, IORING_REGISTER_EVENTFD, &efd, 1);
  if (ret < 0) {
    perror("REGISTER_EVENTFD");
  }

  uv_poll_init(Nan::GetCurrentEventLoop(), &poller, efd);
  // uv_poll_init(Nan::GetCurrentEventLoop(), &poller, ring.ring_fd);
  uv_prepare_init(Nan::GetCurrentEventLoop(), &preparer);

  NAN_EXPORT(target, read);
  NAN_EXPORT(target, writeBuffer);
}

NODE_MODULE(iou, Init);
