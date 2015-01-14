// Module for interfacing with PWM

//#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "lrotable.h"

#include "c_types.h"

// Lua: realfrequency = setup( gpio, frequency, duty )
static int lpwm_setup( lua_State* L )
{
  s32 freq;	  // signed, to error check for negative values
  unsigned duty;
  unsigned gpio;
  
  gpio = luaL_checkinteger( L, 1 );
  if(gpio==16)
    return luaL_error( L, "no pwm for GPIO16" );
  MOD_CHECK_gpio( pwm, gpio );
  freq = luaL_checkinteger( L, 2 );
  if ( freq <= 0 )
    return luaL_error( L, "wrong arg range" );
  duty = luaL_checkinteger( L, 3 );
  if ( duty > NORMAL_PWM_DEPTH )
    // Negative values will turn out > 100, so will also fail.
    return luaL_error( L, "wrong arg range" );
  freq = platform_pwm_setup( gpio, (u32)freq, duty );
  if(freq==0)
    return luaL_error( L, "too many pwms." );
  lua_pushinteger( L, freq );
  return 1;  
}

// Lua: close( gpio )
static int lpwm_close( lua_State* L )
{
  unsigned gpio;
  
  gpio = luaL_checkinteger( L, 1 );
  MOD_CHECK_gpio( pwm, gpio );
  platform_pwm_close( gpio );
  return 0;  
}

// Lua: start( gpio )
static int lpwm_start( lua_State* L )
{
  unsigned gpio;
  gpio = luaL_checkinteger( L, 1 );
  MOD_CHECK_gpio( pwm, gpio );
  platform_pwm_start( gpio );
  return 0;  
}

// Lua: stop( gpio )
static int lpwm_stop( lua_State* L )
{
  unsigned gpio;
  
  gpio = luaL_checkinteger( L, 1 );
  MOD_CHECK_gpio( pwm, gpio );
  platform_pwm_stop( gpio );
  return 0;  
}

// Lua: realclock = setclock( gpio, clock )
static int lpwm_setclock( lua_State* L )
{
  unsigned gpio;
  s32 clk;	// signed to error-check for negative values
  
  gpio = luaL_checkinteger( L, 1 );
  MOD_CHECK_gpio( pwm, gpio );
  clk = luaL_checkinteger( L, 2 );
  if ( clk <= 0 )
    return luaL_error( L, "wrong arg range" );
  clk = platform_pwm_set_clock( gpio, (u32)clk );
  lua_pushinteger( L, clk );
  return 1;
}

// Lua: clock = getclock( gpio )
static int lpwm_getclock( lua_State* L )
{
  unsigned gpio;
  u32 clk;
  
  gpio = luaL_checkinteger( L, 1 );
  MOD_CHECK_gpio( pwm, gpio );
  clk = platform_pwm_get_clock( gpio );
  lua_pushinteger( L, clk );
  return 1;
}

// Lua: realduty = setduty( gpio, duty )
static int lpwm_setduty( lua_State* L )
{
  unsigned gpio;
  s32 duty;  // signed to error-check for negative values
  
  gpio = luaL_checkinteger( L, 1 );
  MOD_CHECK_gpio( pwm, gpio );
  duty = luaL_checkinteger( L, 2 );
  if ( duty > NORMAL_PWM_DEPTH )
    return luaL_error( L, "wrong arg range" );
  duty = platform_pwm_set_duty( gpio, (u32)duty );
  lua_pushinteger( L, duty );
  return 1;
}

// Lua: duty = getduty( gpio )
static int lpwm_getduty( lua_State* L )
{
  unsigned gpio;
  u32 duty;
  
  gpio = luaL_checkinteger( L, 1 );
  MOD_CHECK_gpio( pwm, gpio );
  duty = platform_pwm_get_duty( gpio );
  lua_pushinteger( L, duty );
  return 1;
}

// Module function map
#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
const LUA_REG_TYPE pwm_map[] = 
{
  { LSTRKEY( "setup" ), LFUNCVAL( lpwm_setup ) },
  { LSTRKEY( "close" ), LFUNCVAL( lpwm_close ) },
  { LSTRKEY( "start" ), LFUNCVAL( lpwm_start ) },
  { LSTRKEY( "stop" ), LFUNCVAL( lpwm_stop ) },
  { LSTRKEY( "setclock" ), LFUNCVAL( lpwm_setclock ) },
  { LSTRKEY( "getclock" ), LFUNCVAL( lpwm_getclock ) },
  { LSTRKEY( "setduty" ), LFUNCVAL( lpwm_setduty ) },
  { LSTRKEY( "getduty" ), LFUNCVAL( lpwm_getduty ) },
#if LUA_OPTIMIZE_MEMORY > 0

#endif
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_pwm( lua_State *L )
{
#if LUA_OPTIMIZE_MEMORY > 0
  return 0;
#else // #if LUA_OPTIMIZE_MEMORY > 0
  luaL_register( L, AUXLIB_PWM, pwm_map );
  return 1;
#endif // #if LUA_OPTIMIZE_MEMORY > 0  
}
