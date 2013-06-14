/**
 * @file haka/compiler.h
 * @brief Compiler specific utilities
 * @author Pierre-Sylvain Desse
 *
 * The file contains some compiler specific utilities.
 */

#ifndef _HAKA_COMPILER_H
#define _HAKA_COMPILER_H

#define INLINE static inline

#define INIT __attribute__((constructor))
#define FINI __attribute__((destructor))

#define MIN(a, b)     ((a) < (b) ? (a) : (b))

#define UNUSED __attribute__((unused))


#endif /* _HAKA_COMPILER_H */
