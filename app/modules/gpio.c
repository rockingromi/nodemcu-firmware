// Module for interfacing with GPIO

//#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "lrotable.h"

#include "c_types.h"
#include "c_string.h"

#define PULLUP PLATFORM_GPIO_PULLUP
#define FLOAT PLATFORM_GPIO_FLOAT
#define OUTPUT PLATFORM_GPIO_OUTPUT
#define INPUT PLATFORM_GPIO_INPUT
#define INTERRUPT PLATFORM_GPIO_INT
#define HIGH PLATFORM_GPIO_HIGH
#define LOW PLATFORM_GPIO_LOW


#ifdef GPIO_INTERRUPT_ENABLE
static int gpio_cb_ref[GPIO_MAX_INDEX+1];
static lua_State* gL = NULL;

void lua_gpio_unref(unsigned pin){
  if(gpio_cb_ref[pin] != LUA_NOREF){
    if(gL!=NULL)
      luaL_unref(gL, LUA_REGISTRYINDEX, gpio_cb_ref[pin]);
  }
  gpio_cb_ref[pin] = LUA_NOREF;
}

void gpio_intr_callback( unsigned pin, unsigned level )
{
  NODE_DBG("pin:%d, level:%d \n", pin, level);
  if(gpio_cb_ref[pin] == LUA_NOREF)
    return;
  if(!gL)
    return;
  lua_rawgeti(gL, LUA_REGISTRYINDEX, gpio_cb_ref[pin]);
  lua_pushinteger(gL, level);
  lua_call(gL, 1, 0);
}

// Lua: trig( pin, type, function )
static int lgpio_trig( lua_State* L )
{
  unsigned type;
  unsigned pin;
  size_t sl;
  
  pin = luaL_checkinteger( L, 1 );
  /*MOD_CHECK_ID( gpio, pin );
  if(pin==0)
    return luaL_error( L, "no interrupt for D0" );*/

  const char *str = luaL_checklstring( L, 2, &sl );
  if (str == NULL)
    return luaL_error( L, "wrong arg type" );

  if(sl == 2 && c_strcmp(str, "up") == 0){
    type = GPIO_PIN_INTR_POSEDGE;
  }else if(sl == 4 && c_strcmp(str, "down") == 0){
    type = GPIO_PIN_INTR_NEGEDGE;
  }else if(sl == 4 && c_strcmp(str, "both") == 0){
    type = GPIO_PIN_INTR_ANYEGDE;
  }else if(sl == 3 && c_strcmp(str, "low") == 0){
    type = GPIO_PIN_INTR_LOLEVEL;
  }else if(sl == 4 && c_strcmp(str, "high") == 0){
    type = GPIO_PIN_INTR_HILEVEL;
  }else{
    type = GPIO_PIN_INTR_DISABLE;
  }

  // luaL_checkanyfunction(L, 3);
  if (lua_type(L, 3) == LUA_TFUNCTION || lua_type(L, 3) == LUA_TLIGHTFUNCTION){
    lua_pushvalue(L, 3);  // copy argument (func) to the top of stack
    if(gpio_cb_ref[pin] != LUA_NOREF)
      luaL_unref(L, LUA_REGISTRYINDEX, gpio_cb_ref[pin]);
    gpio_cb_ref[pin] = luaL_ref(L, LUA_REGISTRYINDEX);
  }

  int success = platform_gpio_intr_init(pin, type);
  CHECK_GPIO_SUCCESS(pin, success);
  return 0;  
}
#endif

// Lua: mode( pin, mode, pullup )
static int lgpio_mode( lua_State* L )
{
  unsigned mode, pullup = FLOAT;
  unsigned pin;

  pin = luaL_checkinteger( L, 1 );
  //MOD_CHECK_ID( gpio, pin );
  mode = luaL_checkinteger( L, 2 );
  if ( mode!=OUTPUT && mode!=INPUT && mode!=INTERRUPT)
    return luaL_error( L, "wrong arg type" );
  if(pin==16 && mode==INTERRUPT)
    return luaL_error( L, "no interrupt for 16" );
  if(lua_isnumber(L, 3))
    pullup = lua_tointeger( L, 3 );
  if(pullup!=FLOAT)
    pullup = PULLUP;
#ifdef GPIO_INTERRUPT_ENABLE
  gL = L;   // save to local gL, for callback function
  if (mode!=INTERRUPT){     // disable interrupt
    if(gpio_cb_ref[pin] != LUA_NOREF){
      luaL_unref(L, LUA_REGISTRYINDEX, gpio_cb_ref[pin]);
    }
    gpio_cb_ref[pin] = LUA_NOREF;
  }
#endif
  int r = platform_gpio_mode( pin, mode, pullup );
  CHECK_GPIO_SUCCESS(pin, r);
  /*if( r<0 )
    return luaL_error( L, "wrong pin num." );*/
  return 0;  
}

// Lua: read( pin )
static int lgpio_read( lua_State* L )
{
  unsigned pin;
  
  pin = luaL_checkinteger( L, 1 );
  //MOD_CHECK_ID( gpio, pin );

  signed level = platform_gpio_read( pin );
  CHECK_GPIO_SUCCESS(pin, level);
  lua_pushinteger( L, level );
  return 1; 
}

static int crazy_gpio_toggle(lua_State* L){
  unsigned pin;
  
  uint32_t gpio_no = luaL_checkinteger( L, 1 );

  int bit_value = 0x0;
  uint32_t set_mask_0 = bit_value<<gpio_no;
  uint32_t clear_mask_0 = ((~bit_value)&0x01)<<gpio_no;
  uint32_t enable_mask_0 = 1<<gpio_no;
  uint32_t disable_mask_0 = 0;

  bit_value = 0x1;
  uint32_t set_mask_1 = bit_value<<gpio_no;
  uint32_t clear_mask_1 = ((~bit_value)&0x01)<<gpio_no;
  uint32_t enable_mask_1 = 1<<gpio_no;
  uint32_t disable_mask_1 = 0;

  uint32_t i;
  for(i = 10*1000*1000; i >= 0; i--){
    gpio_output_set(set_mask_0, clear_mask_0, enable_mask_0, disable_mask_0);
    gpio_output_set(set_mask_1, clear_mask_1, enable_mask_1, disable_mask_1);
    gpio_output_set(set_mask_0, clear_mask_0, enable_mask_0, disable_mask_0);
    gpio_output_set(set_mask_1, clear_mask_1, enable_mask_1, disable_mask_1);
    gpio_output_set(set_mask_0, clear_mask_0, enable_mask_0, disable_mask_0);
    gpio_output_set(set_mask_1, clear_mask_1, enable_mask_1, disable_mask_1);
    gpio_output_set(set_mask_0, clear_mask_0, enable_mask_0, disable_mask_0);
    gpio_output_set(set_mask_1, clear_mask_1, enable_mask_1, disable_mask_1);
    gpio_output_set(set_mask_0, clear_mask_0, enable_mask_0, disable_mask_0);
    gpio_output_set(set_mask_1, clear_mask_1, enable_mask_1, disable_mask_1);
    gpio_output_set(set_mask_0, clear_mask_0, enable_mask_0, disable_mask_0);
    gpio_output_set(set_mask_1, clear_mask_1, enable_mask_1, disable_mask_1);
    gpio_output_set(set_mask_0, clear_mask_0, enable_mask_0, disable_mask_0);
    gpio_output_set(set_mask_1, clear_mask_1, enable_mask_1, disable_mask_1);
    gpio_output_set(set_mask_0, clear_mask_0, enable_mask_0, disable_mask_0);
    gpio_output_set(set_mask_1, clear_mask_1, enable_mask_1, disable_mask_1);
  }

  return 0;
}

// Lua: write( pin, level )
static int lgpio_write( lua_State* L )
{
  unsigned level;
  unsigned pin;
  
  pin = luaL_checkinteger( L, 1 );
  //MOD_CHECK_ID( gpio, pin );
  level = luaL_checkinteger( L, 2 );
  level = level & 0x1; //remove conditional branching
  //if ( level!=HIGH && level!=LOW )
  //  return luaL_error( L, "wrong arg type" );
  int success = platform_gpio_write(pin, level);
  //CHECK_GPIO_SUCCESS(pin, success);
  return 0;  
}

// Module function map
#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
const LUA_REG_TYPE gpio_map[] = 
{
  { LSTRKEY( "mode" ), LFUNCVAL( lgpio_mode ) },
  { LSTRKEY( "crazy_toggle" ), LFUNCVAL( crazy_gpio_toggle ) },
  { LSTRKEY( "read" ), LFUNCVAL( lgpio_read ) },
  { LSTRKEY( "write" ), LFUNCVAL( lgpio_write ) },
#ifdef GPIO_INTERRUPT_ENABLE
  { LSTRKEY( "trig" ), LFUNCVAL( lgpio_trig ) },
#endif
#if LUA_OPTIMIZE_MEMORY > 0
#ifdef GPIO_INTERRUPT_ENABLE
  { LSTRKEY( "INT" ), LNUMVAL( INTERRUPT ) },
#endif
  { LSTRKEY( "OUTPUT" ), LNUMVAL( OUTPUT ) },
  { LSTRKEY( "INPUT" ), LNUMVAL( INPUT ) },
  { LSTRKEY( "HIGH" ), LNUMVAL( HIGH ) },
  { LSTRKEY( "LOW" ), LNUMVAL( LOW ) },
  { LSTRKEY( "FLOAT" ), LNUMVAL( FLOAT ) },
  { LSTRKEY( "PULLUP" ), LNUMVAL( PULLUP ) },
#endif
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_gpio( lua_State *L )
{
#ifdef GPIO_INTERRUPT_ENABLE
  int i;
  for(i=0;i<GPIO_PIN_NUM;i++){
    gpio_cb_ref[i] = LUA_NOREF;
  }
  platform_gpio_init(gpio_intr_callback);
#endif

#if LUA_OPTIMIZE_MEMORY > 0
  return 0;
#else // #if LUA_OPTIMIZE_MEMORY > 0
  luaL_register( L, AUXLIB_GPIO, gpio_map );
  // Add constants
#ifdef GPIO_INTERRUPT_ENABLE
  MOD_REG_NUMBER( L, "INT", INTERRUPT );
#endif
  MOD_REG_NUMBER( L, "OUTPUT", OUTPUT );
  MOD_REG_NUMBER( L, "INPUT", INPUT );
  MOD_REG_NUMBER( L, "FLOAT", FLOAT );
  MOD_REG_NUMBER( L, "PULLUP", PULLUP );
  return 1;
#endif // #if LUA_OPTIMIZE_MEMORY > 0  
}
