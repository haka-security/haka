
#ifndef _HAKA_COMPILER_H
#define _HAKA_COMPILER_H

#define INLINE static inline

#define INIT __attribute__((constructor))
#define FINI __attribute__((destructor))

#define MIN(a, b)     ((a) < (b) ? (a) : (b))

#define UNUSED __attribute__((unused))

#define STATIC_ASSERT(COND, MSG) typedef char static_assertion_##MSG[(COND)?1:-1]

#endif /* _HAKA_COMPILER_H */
