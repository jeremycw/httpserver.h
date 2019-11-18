#ifndef FIBER_H
#define FIBER_H

#include <ev.h>

#define FIBER_TIMEOUT 1

typedef void* fiber_t;

#define _spawn_fiber(_1, _2, _3, NAME, ...) NAME

#define fiber_spawn(...) _spawn_fiber(__VA_ARGS__, \
    spawn_fiber2, spawn_fiber1, _err)(__VA_ARGS__)

#define spawn_fiber_main(coroutine, param) \
  ctx->state = 0; \
  ctx->arg = param; \
  ev_init(&ctx->timer, coroutine##_timeout); \
  ctx->timer.data = ctx; \
  ev_io_init(&ctx->io, coroutine##_callback, -1, EV_READ); \
  await_t await = call_##coroutine(&ctx->state, ctx->arg); \
  fiber_schedule_or_finish(coroutine)

#define spawn_fiber1(coroutine, param) \
  coroutine##_ctx_t* ctx = malloc(sizeof(coroutine##_ctx_t)); \
  spawn_fiber_main(coroutine, param)

#define spawn_fiber2(coroutine, param, fiber_handle) \
  coroutine##_ctx_t* ctx = malloc(sizeof(coroutine##_ctx_t)); \
  fiber_handle = (void*)ctx; \
  spawn_fiber_main(coroutine, param)

#define fiber_resume(coroutine, handle) \
  coroutine##_ctx_t* ctx = (coroutine##_ctx_t*)handle; \
  await_t await = call_##coroutine(&ctx->state, ctx->arg); \
  fiber_schedule_or_finish(coroutine)

#define fiber_schedule_or_finish(coroutine) \
  ev_io_stop(fiber_scheduler, &ctx->io); \
  if (ctx->state == -1) { \
    await.fd = -1; \
    ev_timer_stop(fiber_scheduler, &ctx->timer); \
    free(ctx); \
  } else { \
    if (await.timeout > 0.f) { \
      ctx->timer.repeat = await.timeout; \
      ev_timer_again(fiber_scheduler, &ctx->timer); \
    } else if (await.timeout < 0.f) { \
      ev_timer_stop(fiber_scheduler, &ctx->timer); \
    } \
    ctx->io.data = ctx; \
    ev_io_init(&ctx->io, coroutine##_callback, await.fd, await.type); \
    ev_io_start(fiber_scheduler, &ctx->io); \
  }

#define fiber_decl(name, type) \
  typedef struct { \
    int state; \
    type arg; \
    ev_timer timer; \
    ev_io io; \
  } name##_ctx_t; \
  void name##_callback(EV_P_ ev_io *w, int revents); \
  void name##_timeout(EV_P_ ev_timer *w, int revents); \
  await_t call_##name(int* state, type arg);

#define fiber_defn(name, type) \
  void name##_callback(EV_P_ ev_io *w, int revents) { \
    name##_ctx_t* ctx = (name##_ctx_t*)w->data; \
    await_t await = call_##name(&ctx->state, ctx->arg); \
    fiber_schedule_or_finish(name) \
  } \
  void name##_timeout(EV_P_ ev_timer *w, int revents) { \
    name##_ctx_t* ctx = (name##_ctx_t*)w->data; \
    fibererror = FIBER_TIMEOUT; \
    await_t await = call_##name(&ctx->state, ctx->arg); \
    fiber_schedule_or_finish(name) \
  }

#define fiber_scheduler_init() fiber_scheduler = EV_DEFAULT

#define fiber_scheduler_run() ev_run(fiber_scheduler, 0)

typedef struct {
  int fd;
  int type;
  float timeout;
} await_t;

void schedule_fiber(await_t await, ev_io* io, void* ctx, void(*cb)(struct ev_loop*, ev_io*, int));
await_t fiber_await(int fd, int type, float timeout);
await_t fiber_pause();

extern struct ev_loop* fiber_scheduler;
extern int fibererror;

#endif

#ifdef FIBER_IMPL
#ifndef FIBER_IMPL_ONCE
#define FIBER_IMPL_ONCE

int fibererror = 0;
struct ev_loop* fiber_scheduler;

await_t fiber_pause() {
  return (await_t) {
    .fd = -1,
    .type = 0,
    .timeout = -1.f
  };
}

await_t fiber_await(int fd, int type, float timeout) {
  return (await_t) {
    .fd = fd,
    .type = type,
    .timeout = timeout
  };
}

#endif
#endif
