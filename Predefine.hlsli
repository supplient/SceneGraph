#ifndef PREDEFINE_H
#define PREDEFINE_H

/* Note
    This file is only for test and checklist purpose.
    All predefines here should be set when compiling.
*/

#ifndef MAX_DIR_LIGHT_NUM
    #define MAX_DIR_LIGHT_NUM 5
#endif
#ifndef MAX_POINT_LIGHT_NUM
    #define MAX_POINT_LIGHT_NUM 5
#endif
#ifndef MAX_SPOT_LIGHT_NUM
    #define MAX_SPOT_LIGHT_NUM 5
#endif
#ifndef MAX_RECT_LIGHT_NUM
    #define MAX_RECT_LIGHT_NUM 10
#endif

#ifndef TEXTURE_NUM
    #define TEXTURE_NUM 3
#endif
#ifndef SPOT_SHADOW_TEX_NUM
    #define SPOT_SHADOW_TEX_NUM 1
#endif
#ifndef DIR_SHADOW_TEX_NUM
    #define DIR_SHADOW_TEX_NUM 1
#endif
#ifndef POINT_SHADOW_TEX_NUM
    #define POINT_SHADOW_TEX_NUM 1
#endif

// Switchs
#ifndef RECT_LIGHT_ON
    #define RECT_LIGHT_ON 0
#endif
#ifndef SHADING_MODEL
    #define SHADING_MODEL 0
// 0: Simplest Diffuse
// 1: Phong Model
// 2: LTC
#endif
#ifndef HBAO_DEBUG
    #define HBAO_DEBUG 0
#endif
#ifndef HAS_SHADOW
    #define HAS_SHADOW 0
#endif
// Swichs End

// #define MULTIPLE_SAMPLE 4

#endif// PREDEFINE_H