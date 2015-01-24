// Module for interfacing with the I2C interface

//#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "lrotable.h"

// Lua: speed = i2c.setup( id, sda, scl, speed )
static int i2c_setup( lua_State *L )
{
  unsigned id = luaL_checkinteger( L, 1 );
  unsigned sda = luaL_checkinteger( L, 2 );
  unsigned scl = luaL_checkinteger( L, 3 );

  MOD_CHECK_ID( i2c, id );
  MOD_CHECK_ID( gpio, sda );
  MOD_CHECK_ID( gpio, scl );

  if(scl==16 || sda==16)
    return luaL_error( L, "no i2c for GPIO16" );

  s32 speed = ( s32 )luaL_checkinteger( L, 4 );
  if (speed <= 0)
    return luaL_error( L, "wrong arg range" );
  lua_pushinteger( L, platform_i2c_setup( id, sda, scl, (u32)speed ) );
  return 1;
}

static int i2c_scan_find_dev(i2c_id, dev_addr)
{
  platform_i2c_send_start( i2c_id );
  int good = platform_i2c_send_address( i2c_id, (u16)dev_addr, 
    PLATFORM_I2C_DIRECTION_TRANSMITTER );
  platform_i2c_send_stop( i2c_id );
  return good;
}

// Lua: i2c.scan( id )
// if you just pass bus id, it scans GPIOs 0,2,4,5,12,13,14
// if you want to scan other GPIOs, pass them to the function
static int i2c_scan( lua_State *L )
{
  unsigned id = luaL_optinteger(L, 1, 0);
  unsigned sda = luaL_optinteger(L, 2, -1);
  unsigned scl = luaL_optinteger(L, 3, -1);
  char temp[128];

  MOD_CHECK_ID( i2c, id );

  int sdas[] = {0,2,4,5,12,13,14};
  int scls[] = {0,2,4,5,12,13,14};
  int N = sizeof(sdas)/sizeof(sdas[0]);
  if(sda >=0 && sda <= 15 && scl >= 0 && scl <= 15){
    sdas[0] = scls[1] = sda;
    scls[0] = sdas[1] = scl;
    N = 2;
  }

  int found = 0;
  int found_sda, found_scl, found_addr, ok;
  int speed = PLATFORM_I2C_SPEED_SLOW;
  c_sprintf(temp, "scanning on i2c bus %d\n", id);
  c_puts(temp);
  int sdai,scli, addr;
  for(sdai = 0; sdai < N; sdai++){
    WRITE_PERI_REG(0x60000914, 0x73); //reset watchdog
    for(scli = 0; scli < N; scli++){
      
      sda = sdas[sdai];
      scl = scls[scli];
      if(sda == scl){
        continue;
      }

      ok = platform_i2c_setup( id, sda, scl, (u32)speed );
      if(!ok){
        c_puts("error setting up i2c\n");
        lua_pushnil(L); lua_pushnil(L); lua_pushnil(L);
        return 3;
      }

      int found_n = 0;
      for(addr = 0; addr < 128; addr++){
        if(i2c_scan_find_dev(id, addr)){
          WRITE_PERI_REG(0x60000914, 0x73); //reset watchdog
          found = 1;
          found_addr = addr;
          found_scl = scl;
          found_sda = sda;
          c_sprintf(temp, 
            "found device at 0x%x/%d, SDA at GPIO%d, SCL at GPIO%d\n", 
            addr, addr, sda, scl);
          c_puts(temp);
          found_n++;
          if(N != 2 && found_n > 3){
            c_sprintf(temp, 
              "skipping GPIO%d , maybe SDA/GPIO%d is pulled low?\n", 
              sda, sda);
            c_puts(temp);
            scli = N;
            break;
          }
        }
      }

    }
  }

  if(!found){
    c_puts( "no i2c devices found!\n");
    lua_pushnil(L); lua_pushnil(L); lua_pushnil(L);
  }else{
    lua_pushinteger(L, found_sda);
    lua_pushinteger(L, found_scl);
    lua_pushinteger(L, found_addr);
  }

  return 3;
}

// Lua: i2c.start( id )
static int i2c_start( lua_State *L )
{
  unsigned id = luaL_checkinteger( L, 1 );

  MOD_CHECK_ID( i2c, id );
  platform_i2c_send_start( id );
  return 0;
}

// Lua: i2c.stop( id )
static int i2c_stop( lua_State *L )
{
  unsigned id = luaL_checkinteger( L, 1 );

  MOD_CHECK_ID( i2c, id );
  platform_i2c_send_stop( id );
  return 0;
}

// Lua: status = i2c.address( id, address, direction )
static int i2c_address( lua_State *L )
{
  unsigned id = luaL_checkinteger( L, 1 );
  int address = luaL_checkinteger( L, 2 );
  int direction = luaL_checkinteger( L, 3 );

  MOD_CHECK_ID( i2c, id );
  if ( address < 0 || address > 127 )
    return luaL_error( L, "wrong arg range" );
  lua_pushboolean( L, platform_i2c_send_address( id, (u16)address, direction ) );
  return 1;
}

// Lua: wrote = i2c.write( id, data1, [data2], ..., [datan] )
// data can be either a string, a table or an 8-bit number
static int i2c_write( lua_State *L )
{
  unsigned id = luaL_checkinteger( L, 1 );
  const char *pdata;
  size_t datalen, i;
  int numdata;
  u32 wrote = 0;
  unsigned argn;

  MOD_CHECK_ID( i2c, id );
  if( lua_gettop( L ) < 2 )
    return luaL_error( L, "wrong arg type" );
  for( argn = 2; argn <= lua_gettop( L ); argn ++ )
  {
    // lua_isnumber() would silently convert a string of digits to an integer
    // whereas here strings are handled separately.
    if( lua_type( L, argn ) == LUA_TNUMBER )
    {
      numdata = ( int )luaL_checkinteger( L, argn );
      if( numdata < 0 || numdata > 255 )
        return luaL_error( L, "wrong arg range" );
      if( platform_i2c_send_byte( id, numdata ) != 1 )
        break;
      wrote ++;
    }
    else if( lua_istable( L, argn ) )
    {
      datalen = lua_objlen( L, argn );
      for( i = 0; i < datalen; i ++ )
      {
        lua_rawgeti( L, argn, i + 1 );
        numdata = ( int )luaL_checkinteger( L, -1 );
        lua_pop( L, 1 );
        if( numdata < 0 || numdata > 255 )
          return luaL_error( L, "wrong arg range" );
        if( platform_i2c_send_byte( id, numdata ) == 0 )
          break;
      }
      wrote += i;
      if( i < datalen )
        break;
    }
    else
    {
      pdata = luaL_checklstring( L, argn, &datalen );
      for( i = 0; i < datalen; i ++ )
        if( platform_i2c_send_byte( id, pdata[ i ] ) == 0 )
          break;
      wrote += i;
      if( i < datalen )
        break;
    }
  }
  lua_pushinteger( L, wrote );
  return 1;
}

// Lua: read = i2c.read( id, size )
static int i2c_read( lua_State *L )
{
  unsigned id = luaL_checkinteger( L, 1 );
  u32 size = ( u32 )luaL_checkinteger( L, 2 ), i;
  luaL_Buffer b;
  int data;

  MOD_CHECK_ID( i2c, id );
  if( size == 0 )
    return 0;
  luaL_buffinit( L, &b );
  for( i = 0; i < size; i ++ )
    if( ( data = platform_i2c_recv_byte( id, i < size - 1 ) ) == -1 )
      break;
    else
      luaL_addchar( &b, ( char )data );
  luaL_pushresult( &b );
  return 1;
}

// Module function map
#define MIN_OPT_LEVEL   2
#include "lrodefs.h"
const LUA_REG_TYPE i2c_map[] = 
{
  { LSTRKEY( "setup" ),  LFUNCVAL( i2c_setup ) },
  { LSTRKEY( "start" ), LFUNCVAL( i2c_start ) },
  { LSTRKEY( "stop" ), LFUNCVAL( i2c_stop ) },
  { LSTRKEY( "address" ), LFUNCVAL( i2c_address ) },
  { LSTRKEY( "write" ), LFUNCVAL( i2c_write ) },
  { LSTRKEY( "read" ), LFUNCVAL( i2c_read ) },
  { LSTRKEY( "scan" ), LFUNCVAL( i2c_scan ) },
#if LUA_OPTIMIZE_MEMORY > 0
  // { LSTRKEY( "FAST" ), LNUMVAL( PLATFORM_I2C_SPEED_FAST ) },
  { LSTRKEY( "SLOW" ), LNUMVAL( PLATFORM_I2C_SPEED_SLOW ) },
  { LSTRKEY( "TRANSMITTER" ), LNUMVAL( PLATFORM_I2C_DIRECTION_TRANSMITTER ) },
  { LSTRKEY( "RECEIVER" ), LNUMVAL( PLATFORM_I2C_DIRECTION_RECEIVER ) },
#endif
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_i2c( lua_State *L )
{
#if LUA_OPTIMIZE_MEMORY > 0
  return 0;
#else // #if LUA_OPTIMIZE_MEMORY > 0
  luaL_register( L, AUXLIB_I2C, i2c_map );
  
  // Add the stop bits and parity constants (for i2c.setup)
  // MOD_REG_NUMBER( L, "FAST", PLATFORM_I2C_SPEED_FAST );
  MOD_REG_NUMBER( L, "SLOW", PLATFORM_I2C_SPEED_SLOW ); 
  MOD_REG_NUMBER( L, "TRANSMITTER", PLATFORM_I2C_DIRECTION_TRANSMITTER );
  MOD_REG_NUMBER( L, "RECEIVER", PLATFORM_I2C_DIRECTION_RECEIVER );
  
  return 1;
#endif // #if LUA_OPTIMIZE_MEMORY > 0
}

