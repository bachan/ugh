#ifndef __AUX_CONFIG_H__
#define __AUX_CONFIG_H__

#if __GNUC__ >= 3
#define AUX_FORMAT(n,f,e) __attribute__ ((format (n, f, e)))
#else
#define AUX_FORMAT(n,f,e) /* void */
#endif /*  */

#endif /* __AUX_CONFIG_H__ */
