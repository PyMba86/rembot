#pragma once

#include <unistd.h>

#define CGSleep usleep

#define M_iPI  0.31830988618
#define M_iPI2 0.63661977236
#define M_PI2  1.57079632679
#define M_PI4  0.78539816339
#define M_2PI  6.28318530718

#define CG_EXPORT

/*! \brief Standard floating point number.
 *
 * Use -DDOUBLE switch during compilation to specify that this
 * type is 64-bit floating point. By default we assume 32-bit
 * floating point numbers.
 */
#ifdef SINGLE
typedef float  CG_Float;
#elif defined DOUBLE
typedef double CG_Float;
#else
typedef double CG_Float;
#endif

typedef char CG_I8;
typedef unsigned char CG_UI8;

typedef short CG_I16;
typedef unsigned short CG_UI16;

typedef int   CG_I32;
typedef unsigned int   CG_UI32;

typedef long long CG_I64;

// Units
#define _MM_ (1e-3)
#define _M_ (1e0)
#define _KM_ (1e3)

#include <cstddef>
#include <exception>

namespace CG {
/*! \brief An Exception class.
 *
 * Used for propagating unexpected errors. An information
 * string with each Exception can also be provided.
 */
    class CG_EXPORT Exception : public std::exception {
    public:
        /*! \brief The Constructor. */
        Exception(const char *info, ...);

        /*! \brief The Destructor. */
        ~Exception() throw();

        char _info[1024]; //!< Information about the Exception
    };
}