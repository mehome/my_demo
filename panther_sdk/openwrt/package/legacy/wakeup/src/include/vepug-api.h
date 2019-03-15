#ifndef __VEPUG_API_H__
#define __VEPUG_API_H__


#ifdef __cplusplus
extern "C" {      
#endif            


/************************************************************************/
/*                                                                      */
/*     Alango $$VERSION_STRING                                          */
/*                                                                      */
/************************************************************************/

#define PROFILE_VERSION 55 // Profile version as it seen...
#define NUM_MEM_REGIONS 1 // Number of memory regions to be used for proper package work
#define VERSION_STRING "Alango Voice Enchancement Package v0.1" // To be used for package identification
#define MAX_MICROPHONES 16 // Max number of Microphones
#define MAX_BEAMS 16 // Max number of beams


typedef short           pitem_t;             // 16 bits
typedef unsigned short  uitem_t;             // 16 bits
typedef short           pcm_t;                   // input PCM data. Normally 16 bits


/************************************************************************/
/*  MEMORY regions                                                      */
/************************************************************************/
typedef struct mem_reg_s                                                  
{                                                                         
    void        *mem;           /* memory provided                      */
    int         size;           /* overall size of the region in bytes. */
} mem_reg_t;                                                              
                                                                          
/************************************************************************/
/*  ERROR definitions and codes                                         */
/************************************************************************/
typedef struct err_s {                                                    
    short err;                                    /* error's id         */
    short pid;                                    /* error profile's ID */
    short memb;                                   /* member number      */
} err_t;                                                                  

#undef ERR_NO_ERROR                                                                                        
#undef ERR_OUT_OF_RANGE                                                                                    
#undef ERR_BLK_NOT_IMPLEMENTED                                                                             
#undef ERR_NULL_POINTER                                                                                    
#undef ERR_OBJ_CORRUPTED                                                                                   
#undef ERR_DEMO_EXPIRED                                                                                    
#undef ERR_INVALID_VALUE                                                                                   
#undef ERR_INVALID_COMB                                                                                    
#undef ERR_INVALID_CRC                                                                                     
#undef ERR_NOT_ENOUGH_MEMORY                                                                               
#undef ERR_BUILD_NOT_CORRECT                                                                               
#undef ERR_STEREO_EC_DISABLED                                                                              
#undef ERR_ES_NLD_DISABLED                                                                                 
#undef ERR_UNKNOWN                                                                                         
                                                                                                           
#define ERR_NO_ERROR                0        /* no errors at all */                                        
#define ERR_OUT_OF_RANGE            1        /* parameter is out of range */                               
#define ERR_BLK_NOT_IMPLEMENTED     2        /* block is not implemented */                                
#define ERR_NULL_POINTER            3        /* wrong parameter */                                         
#define ERR_OBJ_CORRUPTED           4        /* object override happened in vcp_process */                 
#define ERR_DEMO_EXPIRED            5        /* Demo interval ended */                                     
#define ERR_INVALID_VALUE           6        /* just invalid */                                            
#define ERR_INVALID_COMB            7        /* invalid value combination */                               
#define ERR_INVALID_CRC             8        /* invalid CRC */                                             
#define ERR_NOT_ENOUGH_MEMORY       9        /* not enough memory */                                       
#define ERR_BUILD_NOT_CORRECT       10        /* Build and profile are not matched Lite and not Lite */    
#define ERR_STEREO_EC_DISABLED      11        /* Stereo EC is not allowed in current build */              
#define ERR_ES_NLD_DISABLED         12        /* NLD is not allowed in current build */                    
                                                                                                           
#define ERR_UNKNOWN                 -1                                                                     

#if defined(__TEXT_ERRORS_PRINT) /* shall you need text error print please define this in caller's routine */
    /* check 'main()' for details */       
    static char *__text_error[] = {        
        "no error",                        
        "parameter is out of range",       
        "block is not implemented",        
        "parameter is a null pointer",     
        "object has been corrupted",       
        "demo license has expired",        
        "invalid parameter's value",       
        "invalid parameters combination",  
        "invalid CRC",                     
        "not enough memory allocated",     
        "build and profile do not match",  
        "stereo EC is not allowed",        
        "nld is not allowed"               
    };                                                                                                     
#endif

/************************************************************************/
/****        API definition                                          ****/
/****        depending on the package configuration                  ****/
/****        some routines may not exist in the package              ****/
/************************************************************************/

/************************************************************************/
/* BASIC ROUTINES:                                                      */
/************************************************************************/

/************************************************************************ 
FUNCTION:
    vepug_init() -- initializes VCP object with given profile and memory area(s). 
ARGS:
    p       - pointer to actual profile.            
    reg     - pointer to memory region structure(s).
RETURNS: integer values, corresponding to the following errors:
    ERR_NO_ERROR,     
    ERR_INVALID_CRC,  
    ERR_INVALID_VALUE 
    For precise error detection use vcp_init_debug()
OTHER:
    Memory region(s) must be allocated prior vepug_init() call,
    size field of 'reg' must be filled with allocated size.
    Once inited, neither 'reg' not its fields can be altered.
    Profile 'p' can be destroyed after exit.
************************************************************************/
extern err_t vepug_init(void *p, mem_reg_t *reg);

/************************************************************************
FUNCTION:                                                                
    vepug_process() -- does main VCP processing
ARGS:                                    
    in/out  - pointers to in/out streams            
    reg     - pointer to memory region structure(s).
RETURNS:
    ERR_OBJ_CORRUPTED,
    ERR_DEMO_EXPIRED, 
    ERR_UNKNOWN,      
    ERR_NO_ERROR      
OTHER:
    input buffers must be filled with data with size defined in  
    general profile field 'frlen'. Output sizes will be the same.
    Input buffers must not be modified during call.              
************************************************************************/                      
/* ADD YOURS HERE !!!!!!!!!! */
/************************************************************************/
/* SEVICE ROUTINES AND DEFINITIONS                                      */
/* SEVICE LIBRARY REQUIRED                                              */
/************************************************************************/

/************************************************************************
FUNCTION:                                          
   *_debug() -- same as above with some extra debug
                functionality and paranoid checks. 
ARGS:                                              
   in/out  - pointers to in/out streams            
   reg     - pointer to memory region structure(s).
RETURNS:                                           
   all errors possible                             
************************************************************************/                            
extern err_t vepug_process_multibeam(mem_reg_t *reg, pcm_t *txin, pcm_t *rxin, pcm_t *tmp);
extern err_t vepug_process(mem_reg_t *reg, pcm_t *txin, pcm_t *rxin, pcm_t *tmp);
/* ADD YOURS HERE !!!!!!!!!! */
/************************************************************************
FUNCTION:                                         
   vepug_get_mem_size() -- computes required memory.
ARGS:                                          
   p - pointer to actual profile.             
   reg     - pointer to memory region structure(s).
   mem     - memory allocated with size returned by vepug_get_hook_size()      
RETURNS:
   Error code
OTHER:
   upon return memory region(s) will have 'size'
   field reflecting memory required.            
***********************************************************************/ 
extern err_t vepug_process_beammux(mem_reg_t *reg, pcm_t *txin, pcm_t *rxin, pcm_t *tmp);
extern int vepug_get_beammux_chan(mem_reg_t *reg);
extern int vepug_get_beammux_chan_real(mem_reg_t *reg);
extern err_t vepug_get_mem_size(void *p, mem_reg_t *reg, void *mem);
extern err_t vepug_bli_get(mem_reg_t *reg, pcm_t *bli_score, pcm_t *bli_inst, pcm_t *bli_bkg, pcm_t *bli_peak);


/************************************************************************ 
FUNCTION:
   vepug_get_hook_size()  -- returns the size of memory required
           for low stack use in vepug_get_mem_size().
RETURNS:
   size required
***********************************************************************/
extern err_t vepug_bli_bkg_freeze(mem_reg_t *reg, pcm_t itime);
extern int vepug_get_hook_size(void);

/******** THE END ********************************************/
#ifdef __cplusplus
}
#endif


#endif // __VEPUG_API_H__
