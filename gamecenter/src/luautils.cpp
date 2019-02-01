/*******************************************************************************
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *******************************************************************************/

#include "luautils.h"
void registerCallback(lua_State* L, int index, LuaCallbackInfo* cbk)
{
    if(cbk->m_Callback != LUA_NOREF)
    {
        dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Callback);
        dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Self);
    }

    cbk->m_L = dmScript::GetMainThread(L);
    luaL_checktype(L, index, LUA_TFUNCTION);

    lua_pushvalue(L, index);
    cbk->m_Callback = dmScript::Ref(L, LUA_REGISTRYINDEX);

    dmScript::GetInstance(L);
    cbk->m_Self = dmScript::Ref(L, LUA_REGISTRYINDEX);
}

void invokeErrorCallback(CallbackInfo *cbkInfo)
{
    if(cbkInfo->m_Cbk->m_Callback == LUA_NOREF)
    {
        return;
    }

    lua_State* L = cbkInfo->m_Cbk->m_L;
    DM_LUA_STACK_CHECK(L, 0);

    lua_rawgeti(L, LUA_REGISTRYINDEX, cbkInfo->m_Cbk->m_Callback);

    // Setup self (the script instance)
    lua_rawgeti(L, LUA_REGISTRYINDEX, cbkInfo->m_Cbk->m_Self);
    lua_pushvalue(L, -1);
    dmScript::SetInstance(L);

    lua_newtable(L);

    if(cbkInfo->m_Error) {
	    lua_newtable(L);
	    lua_pushnumber(L, cbkInfo->m_Error->m_code);
	    lua_setfield(L, -2, "code");
	    lua_pushstring(L, cbkInfo->m_Error->m_description);
	    lua_setfield(L, -2, "message");

	    lua_setfield(L, -2, "error");
    }

    if(cbkInfo->m_Error) {
    	delete cbkInfo->m_Error;
    	cbkInfo->m_Error = 0;
    }

    if(cbkInfo->m_playerID){
      lua_pushstring(L, cbkInfo->m_playerID);
	    lua_setfield(L, -2, "playerID");
    }
    if(cbkInfo->m_alias){
      lua_pushstring(L, cbkInfo->m_alias);
	    lua_setfield(L, -2, "alias");
    }

    int number_of_arguments = 2; // instance + 1
    int ret = lua_pcall(L, number_of_arguments, 0, 0);
    if(ret != 0) {
        dmLogError("Error running callback: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

void invokeAchievementCallback(CallbackInfo *cbkInfo)
{
    if(cbkInfo->m_Cbk->m_Callback == LUA_NOREF)
    {
        return;
    }

    lua_State* L = cbkInfo->m_Cbk->m_L;
    DM_LUA_STACK_CHECK(L, 0);
    lua_rawgeti(L, LUA_REGISTRYINDEX, cbkInfo->m_Cbk->m_Callback);

    // Setup self (the script instance)
    lua_rawgeti(L, LUA_REGISTRYINDEX, cbkInfo->m_Cbk->m_Self);
    lua_pushvalue(L, -1);
    dmScript::SetInstance(L);

    lua_newtable(L);

    if(cbkInfo->m_achievements.Size() > 0) {
	    lua_newtable(L);
	    int count = 0;
	    for(uint32_t i = 0; i != cbkInfo->m_achievements.Size(); ++i) {
	    	SAchievement* sAchievement = &cbkInfo->m_achievements[i];
	    	lua_newtable(L);
			lua_pushstring(L, sAchievement->m_identifier);
			lua_setfield(L, -2, "identifier");
			lua_pushnumber(L, sAchievement->m_percentComplete);
			lua_setfield(L, -2, "percentComplete");
			lua_rawseti(L, -2, count + 1);
			count++;
	    	cbkInfo->m_achievements.EraseSwap(i--);
	    }
    	lua_setfield(L, -2, "achievements");
    }

    int number_of_arguments = 2; // instance + 1
    int ret = lua_pcall(L, number_of_arguments, 0, 0);
    if(ret != 0) {
        dmLogError("Error running callback: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

void unregisterCallback(LuaCallbackInfo* cbk)
{
    if(cbk->m_Callback != LUA_NOREF)
    {
        dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Callback);
        dmScript::Unref(cbk->m_L, LUA_REGISTRYINDEX, cbk->m_Self);
        cbk->m_Callback = LUA_NOREF;
    }
}

/** Gets a number (or a default value) as a integer from a table
*/
int checkTableNumber(lua_State* L, int index, const char* name, int default_value)
{
    DM_LUA_STACK_CHECK(L, 0);

    int result = -1;
    lua_pushstring(L, name);
    lua_gettable(L, index);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return default_value;
    }
    else if (lua_isnumber(L, -1)) {
        result = lua_tointeger(L, -1);
    } else {
        return DM_LUA_ERROR("Wrong type for table attribute '%s'. Expected number, got %s", name, luaL_typename(L, -1));
    }
    lua_pop(L, 1);
    return result;
}

/** Gets a number (or a default value) as a double from a table
*/
double checkTableNumber(lua_State* L, int index, const char* name, double default_value)
{
    DM_LUA_STACK_CHECK(L, 0);

    double result = -1.0;
    lua_pushstring(L, name);
    lua_gettable(L, index);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return default_value;
    }
    else if (lua_isnumber(L, -1)) {
        result = lua_tonumber(L, -1);
    } else {
        return DM_LUA_ERROR("Wrong type for table attribute '%s'. Expected number, got %s", name, luaL_typename(L, -1));
    }
    lua_pop(L, 1);
    return result;
}

/** Gets a string from a table
*/
const char* toTableString(lua_State* L, int index, const char* name)
{
    DM_LUA_STACK_CHECK(L, 0);

    lua_pushstring(L, name);
    lua_gettable(L, index);
    const char* result = lua_tostring(L, -1);
    lua_pop(L, 1);
    return result;
}

static void stackDump (lua_State *L) {
                int i;
                int top = lua_gettop(L);  /* depth of the stack */
                for (i = 1; i <= top; i++) {  /* repeat for each level */
                  int t = lua_type(L, i);
                  switch (t) {
                    case LUA_TSTRING: {  /* strings */
                      printf("'%s'", lua_tostring(L, i));
                      break;
                    }
                    case LUA_TBOOLEAN: {  /* Booleans */
                      printf(lua_toboolean(L, i) ? "true" : "false");
break; }
                    case LUA_TNUMBER: {  /* numbers */
                      printf("%g", lua_tonumber(L, i));
                      break;
                    }
                    default: {  /* other values */
                      printf("%s", lua_typename(L, t));
break; }
}
                  printf("  ");  /* put a separator */
                }
                printf("\n");  /* end the listing */
              }
