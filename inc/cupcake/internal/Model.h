
#ifndef CUPCAKE_MODEL_H
#define CUPCAKE_MODEL_H

// This file basically defaults CUPCAKE_THREADED as appropriate

#ifndef CUPCAKE_THREADED
#ifndef _WIN32
#define CUPCAKE_THREADED
#endif
#endif

#endif // CUPCAKE_MODEL_H
