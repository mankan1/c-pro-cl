#ifndef PROM_LOCK_H
#define PROM_LOCK_H
#include <pthread.h>

#if USE_SPINLOCK == 0

typedef pthread_mutex_t prom_lock_t;
#define prom_lock_init(lock) pthread_mutex_init(lock, NULL)
#define prom_lock_lock(lock) pthread_mutex_lock(lock)
#define prom_lock_unlock(lock) pthread_mutex_unlock(lock)
#define prom_lock_destroy(lock) pthread_mutex_destroy(lock)

#else

typedef pthread_spinlock_t prom_lock_t;
#define prom_lock_init(lock) pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE)
#define prom_lock_lock(lock) pthread_spin_lock(lock)
#define prom_lock_unlock(lock) pthread_spin_unlock(lock)
#define prom_lock_destroy(lock) pthread_spin_destroy(lock)
#endif

#endif  // PROM_LOCK_H
