# Check for thread local storage support.
# Required when building with DB STL support.
# Partly based on: http://autoconf-archive.cryp.to/ax_tls.html
# By Alan Woodland <ajw05@aber.ac.uk>  

AC_DEFUN([AX_TLS], [
  AC_MSG_CHECKING(for thread local storage (TLS) class)
  AC_CACHE_VAL(ac_cv_tls, [
    ax_tls_keywords="__thread __declspec(thread)"
    ac_cv_tls=none
    for ax_tls_keyword in $ax_tls_keywords; do
        AC_TRY_COMPILE([#include <stdlib.h>
            static void foo(void) {
            	static ] $ax_tls_keyword [ int bar;
            	exit(1);
            }], [],
            [ac_cv_tls=$ax_tls_keyword ; break])
    done
    if test "$ac_cv_tls" = "none" ; then
        AC_TRY_COMPILE(
          [#include <stdlib.h>
           #include <pthread.h>

           static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
           static pthread_key_t key;

           static void init_once(void) {
		pthread_key_create(&key, NULL);
           }
           static void *get_tls() {
                return (void *)pthread_getspecific(&key);
           }
           static void set_tls(void *p) {
                pthread_setspecific(&key, p);
           }], [],
           [ac_cv_tls=pthread])
    fi
  ])

  if test "$ac_cv_tls" != "none" ; then
        if test "$ac_cv_tls" = "pthread" ; then
		AC_DEFINE(HAVE_PTHREAD_TLS)
		AH_TEMPLATE(HAVE_PTHREAD_TLS,
		    [Define to 1 if using pthread_get/set_specific for TLS.])
        fi

	AC_DEFINE(HAVE_TLS)
	AH_TEMPLATE(HAVE_TLS,
	    [Define to 1 if compiler supports Thread Local Storage.])
  fi
  AC_MSG_RESULT($ac_cv_tls)
])

